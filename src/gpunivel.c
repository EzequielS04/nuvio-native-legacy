// Nivel de GPU do .tpk. O porque, os niveis e a regra estao em gpunivel.h.
//
// SEM _Thread_local (a .so do Tizen 4/5 recusa TLS, tests/tpk40_tls.sh): tudo
// aqui roda no fio de desenho, e os estaticos sao so dele.
#include "gpunivel.h"
#include "gfx.h"
#include "dados.h"
#include "perfiltv.h"
#include "layout.h"
#include "gl_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef NV_TPK
#include <dlfcn.h>
#include "tpk_egl.h"
#endif

#define GPUN_NIVEL_AUTO_MAX 2   // efeitos minimos; o 3 (720p) so forcado, ver gpun_medir
#define GPUN_ARQ "gpu-nivel.txt"
// Regra do adaptativo (gpunivel.h). Os numeros:
//  - 45 fps: abaixo disso o registro 8825 (22-29) e o jank visivel; a Tizen 6
//    que roda a 60 fica longe dele, entao ela nunca desce.
//  - espera >= 6 ms e maior que a CPU: o quadro e decidido pelo que a GPU
//    devolve, e nao pelo que a CPU submete (no 8825: espera 62, CPU 5).
//  - janela de 4 s, aquecimento de 3 s na primeira entrada na home (a subida
//    das primeiras artes e trabalho de CPU/upload, nao do regime) e 2 s depois
//    de cada degrau; teto de 24 s de home medida por arranque.
#define GPUN_FPS_BOM     45.0
// Abaixo disto, JA com efeitos leves, a tela esta travada: tira mais efeitos
// (registro 9859: Mali-400, Tizen 4.0, 10-21 fps no 1).
#define GPUN_FPS_CRITICO 25.0
#define GPUN_ESPERA_MIN   6.0
#define GPUN_JANELA_MS 4000.0
#define GPUN_AQUECE_MS 3000.0
#define GPUN_ASSENTA_MS 2000.0
#define GPUN_TETO_MS  24000.0

static int nivel = 0, adaptativo = 0, decidido = 0;
static const char *origem = "padrao";
static char renderer[160] = "?", versaoGl[160] = "?", modelo[96] = "?", tizen[32] = "?";
static unsigned long chave;
static int telaW = 1920, telaH = 1080;
// Alvo interno do nivel 3 (720p).
static GLuint intFbo, intTex;
static int intW, intH, intFalhou, intLigado;
// Descarte (glInvalidateFramebuffer ou glDiscardFramebufferEXT).
typedef void (*PfnDescarte)(GLenum, GLsizei, const GLenum *);
static PfnDescarte descarte;
#ifdef NV_TPK
static const char *descarteNome = "nenhum";
#endif
static int profStencil;   // a janela tem profundidade/stencil para descartar
// Medida.
static double aquece = GPUN_AQUECE_MS, janMs, janEsp, janCpu, totalMs;
static int janN, estavaNaHome;

#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#define NV_GL_COLOR   0x1800   // GL_COLOR (ES3) == GL_COLOR_EXT
#define NV_GL_DEPTH   0x1801
#define NV_GL_STENCIL 0x1802

static unsigned long djb2(const char *s, unsigned long h) {
  while (s && *s) h = h * 33u + (unsigned char)*s++;
  return h;
}

static const char *glTxt(GLenum e) {
  const char *s = (const char *)glGetString(e);
  return s ? s : "?";
}

#ifdef NV_TPK
// Modelo e versao da Tizen pela API nativa (a mesma que o host .NET le em
// Tizen.System.Information), por dlopen: o link da .so nao ganha dependencia
// nova, e sem a biblioteca fica "?".
static void infoPlataforma(void) {
  static const char *const libs[] = { "libcapi-system-info.so.0", "libcapi-system-info.so", NULL };
  int (*get)(const char *, char **) = NULL;
  void *h = NULL;
  int i;
  for (i = 0; libs[i] && !h; i++) h = dlopen(libs[i], RTLD_NOW);
  if (h) *(void **)&get = dlsym(h, "system_info_get_platform_string");
  if (!get) return;
  { char *v = NULL;
    if (get("http://tizen.org/feature/platform.version", &v) == 0 && v) {
      snprintf(tizen, sizeof tizen, "%s", v); free(v); } }
  { char *v = NULL;
    if (get("http://tizen.org/system/model_name", &v) == 0 && v) {
      snprintf(modelo, sizeof modelo, "%s", v); free(v); } }
}

// "OpenGL ES 3.2 ..." -> 3. So um contexto que SE DIZ 3.x ganha as funcoes de 3.x.
static int versaoEs(const char *v) {
  int ma = 0, mi = 0;
  if (v && sscanf(v, "OpenGL ES %d.%d", &ma, &mi) >= 1) return ma;
  return 0;
}

// Extensao inteira na lista (sem casar prefixo de outra).
static int temExt(const char *lista, const char *nome) {
  size_t n = strlen(nome);
  const char *p = lista;
  while (p && (p = strstr(p, nome)) != NULL) {
    if ((p == lista || p[-1] == ' ') && (p[n] == ' ' || p[n] == 0)) return 1;
    p += n;
  }
  return 0;
}

// A lista em linhas de ~480 caracteres, sem o prefixo "GL_" (compacta).
static void logExtensoes(const char *ext) {
  char linha[560];
  int n = 0, total = 0;
  const char *p = ext;
  linha[0] = 0;
  while (p && *p) {
    const char *f;
    size_t t;
    while (*p == ' ') p++;
    if (!*p) break;
    f = strchr(p, ' ');
    t = f ? (size_t)(f - p) : strlen(p);
    if (t > 3 && !strncmp(p, "GL_", 3)) { p += 3; t -= 3; }
    if (n + (int)t + 1 > 480) { printf("[gl] ext: %s\n", linha); n = 0; linha[0] = 0; }
    if (t < sizeof linha - 2) { memcpy(linha + n, p, t); n += (int)t; linha[n++] = ' '; linha[n] = 0; }
    total++;
    p += t;
  }
  if (n) printf("[gl] ext: %s\n", linha);
  printf("[gl] %d extensoes\n", total);
}
#endif

static void gravar(void) {
  char buf[400];
  if (!adaptativo) return;
  snprintf(buf, sizeof buf, "versao=1\nchave=%lx\nnivel=%d\ngpu=%s\ngl=%s\nmodelo=%s\ntizen=%s\n",
           chave, nivel, renderer, versaoGl, modelo, tizen);
  if (!dados_gravar(GPUN_ARQ, buf)) printf("[gpu-nivel] nao gravou %s\n", GPUN_ARQ);
}

#if (defined(NV_TPK) || defined(NV_ANDROID)) && !defined(NV_TPK_NIVEL_FORCADO)
static void ler(void) {
  char *t = dados_ler(GPUN_ARQ);
  unsigned long c = 0;
  int v = 0, n = -1;
  const char *p;
  if (!t) { printf("[gpu-nivel] sem %s: comeca do 0\n", GPUN_ARQ); return; }
  if ((p = strstr(t, "versao=")) != NULL) v = atoi(p + 7);
  if ((p = strstr(t, "chave=")) != NULL) c = strtoul(p + 6, NULL, 16);
  if ((p = strstr(t, "nivel=")) != NULL) n = atoi(p + 6);
  free(t);
  if (v != 1 || n < 0 || n > 3) { printf("[gpu-nivel] %s invalido: comeca do 0\n", GPUN_ARQ); return; }
  if (c != chave) {
    printf("[gpu-nivel] %s de outra GPU/driver/firmware (chave %lx, agora %lx): mede de novo do 0\n",
           GPUN_ARQ, c, chave);
    return;
  }
  nivel = n > GPUN_NIVEL_AUTO_MAX ? GPUN_NIVEL_AUTO_MAX : n;
  origem = "salvo";
}
#endif

static void aplicar(int n, const char *porque) {
  if (n < 0) n = 0;
  if (n > 3) n = 3;
  // No arranque (porque vazio) so fala quando ha o que dizer: na LG e no .wgt
  // o nivel e sempre 0 e o log deles nao ganha linha nova.
  if (n != nivel || (!porque[0] && strcmp(origem, "padrao")))
    printf("[gpu-nivel] nivel %d -> %d (%s)\n", nivel, n, porque[0] ? porque : origem);
  nivel = n;
  gfx_definir_efeitos_leves(nivel >= 1);
  gfx_definir_efeitos_minimos(nivel >= 2);
  fflush(stdout);
}

void gpun_iniciar(int w, int h) {
  const char *ext = "";
  GLint db = 0, sb = 0;
  telaW = w > 0 ? w : 1920;
  telaH = h > 0 ? h : 1080;
  snprintf(renderer, sizeof renderer, "%s", glTxt(GL_RENDERER));
  snprintf(versaoGl, sizeof versaoGl, "%s", glTxt(GL_VERSION));
  ext = glTxt(GL_EXTENSIONS);
  glGetIntegerv(GL_DEPTH_BITS, &db);
  glGetIntegerv(GL_STENCIL_BITS, &sb);
#ifdef NV_TPK
  infoPlataforma();
  printf("[gl] GL_VERSION=%s\n", versaoGl);
  printf("[gl] GL_RENDERER=%s | GL_VENDOR=%s\n", renderer, glTxt(GL_VENDOR));
  printf("[gl] GL_SHADING_LANGUAGE_VERSION=%s\n", glTxt(GL_SHADING_LANGUAGE_VERSION));
  printf("[gl] janela: profundidade=%d stencil=%d | TV %s / Tizen %s\n", (int)db, (int)sb, modelo, tizen);
  logExtensoes(ext);
  // ES3 so se o contexto SE DIZ 3.x e o ponteiro existe; senao a extensao.
  if (tpkEgl.GetProcAddress) {
    if (versaoEs(versaoGl) >= 3) {
      *(void **)&descarte = tpkEgl.GetProcAddress("glInvalidateFramebuffer");
      if (descarte) descarteNome = "glInvalidateFramebuffer (ES3)";
    }
    if (!descarte && temExt(ext, "GL_EXT_discard_framebuffer")) {
      *(void **)&descarte = tpkEgl.GetProcAddress("glDiscardFramebufferEXT");
      if (descarte) descarteNome = "glDiscardFramebufferEXT";
    }
  }
  profStencil = db > 0 || sb > 0;
  printf("[gl] descarte de alvo: %s%s\n", descarteNome,
         descarte && profStencil ? " (+ profundidade/stencil da janela no fim do quadro)" : "");
#else
  (void)ext; (void)db; (void)sb;
#ifdef __APPLE__
  snprintf(modelo, sizeof modelo, "mac");
#elif defined(NV_LINUX_DESKTOP)
  snprintf(modelo, sizeof modelo, "linux-desktop");
#endif
#endif
  ptv_definir_gpu_fraca(ptv_gpu_fraca(renderer));
  chave = djb2(tizen, djb2(modelo, djb2(versaoGl, djb2(renderer, 5381))));

#if defined(NV_TPK_NIVEL_FORCADO)
  nivel = NV_TPK_NIVEL_FORCADO;
  origem = "forcado na build (NV_TPK_NIVEL_FORCADO)";
#elif defined(NV_TPK) || defined(NV_ANDROID)
  // ANDROID tambem mede: TV box e Google TV vao de Mali-G52 a GPUs bem mais
  // fortes. Na TCL Smart TV Pro o vidro + cor viva dava 29 fps sustentado.
  adaptativo = 1;
  origem = "adaptativo";
  ler();
#else
  { const char *e = getenv("NUVIO_GPU_NIVEL");
#if defined(__APPLE__) || defined(NV_LINUX_DESKTOP)
    if (e && *e) { nivel = atoi(e); origem = "NUVIO_GPU_NIVEL"; }
#else
    (void)e;
#endif
  }
#endif
  aplicar(nivel, "");
}

// Ajuste "Efeitos visuais" (so .tpk): 0 automatico, 1 completos, 2 leves.
// Completos/Leves fixam o nivel e desligam a medida; Automatico volta a medir
// a partir do que esta gravado (ou do 0). Build com NV_TPK_NIVEL_FORCADO
// ignora: o canario de teste manda.
static int forca720;
void gpun_forcar_720(void) {
  forca720 = 1; adaptativo = 0; decidido = 1;
  aplicar(3, "ajuste: interface 720p");
}

void gpun_preferencia(int p) {
  if (forca720) return;
#if (defined(NV_TPK) || defined(NV_ANDROID)) && !defined(NV_TPK_NIVEL_FORCADO)
  if (p == 1) { adaptativo = 0; aplicar(0, "ajuste: efeitos completos"); return; }
  if (p == 2) { adaptativo = 0; aplicar(1, "ajuste: efeitos leves"); return; }
  adaptativo = 1; decidido = 0; origem = "adaptativo"; nivel = 0;
  ler();
  aplicar(nivel, "ajuste: automatico");
#else
  (void)p;
#endif
}

int gpun_nivel(void) { return nivel; }
void gpun_definir_nivel(int n) { aplicar(n, "definido por gpun_definir_nivel"); }

void gpun_log_perfil(long memMB, int texMb, int fios, int heroi) {
#ifdef NV_TPK
  printf("[perfil] tpk mem=%ldMB gpu=\"%s\"%s tizen=%s modelo=%s -> tex=%dMB fios=%d heroi=%d"
         " escala=%s efeitos=%s nivel=%d (%s)\n",
         memMB, renderer, ptv_gpu_fraca_atual() ? " (fraca)" : "", tizen, modelo, texMb, fios, heroi,
         nivel >= 3 ? "1280x720->1920x1080" : "1920x1080",
         nivel >= 2 ? "minimos" : nivel >= 1 ? "leves" : "cheios", nivel, origem);
  fflush(stdout);
#else
  (void)memMB; (void)texMb; (void)fios; (void)heroi;
#endif
}

void gpun_descartar_cor(int padrao) {
  GLenum a = padrao ? NV_GL_COLOR : GL_COLOR_ATTACHMENT0;
  if (descarte) descarte(GL_FRAMEBUFFER, 1, &a);
}

static int intPreparar(void) {
  GLint ant = 0;
  GLenum st;
  if (intFbo) return 1;
  if (intFalhou) return 0;
  intW = (telaW * 2 + 1) / 3;
  intH = (telaH * 2 + 1) / 3;
  glGenTextures(1, &intTex);
  glBindTexture(GL_TEXTURE_2D, intTex);
  // RGBA: o alpha e o canal do furo do video (gfx_furo) e tem de chegar a janela.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, intW, intH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gfx_tex_esquecer(0);
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &ant);
  glGenFramebuffers(1, &intFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, intFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, intTex, 0);
  st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)ant);
  if (st != GL_FRAMEBUFFER_COMPLETE) {
    printf("[gpu-nivel] alvo interno %dx%d incompleto (0x%x): fica no nivel 1\n", intW, intH, (unsigned)st);
    glDeleteFramebuffers(1, &intFbo); glDeleteTextures(1, &intTex);
    intFbo = intTex = 0; intFalhou = 1;
    aplicar(1, "sem alvo interno");
    return 0;
  }
  printf("[gpu-nivel] alvo interno %dx%d RGBA -> janela %dx%d\n", intW, intH, telaW, telaH);
  fflush(stdout);
  return 1;
}

void gpun_quadro_inicio(void) {
  intLigado = 0;
  if (nivel < 3 || !intPreparar()) return;
  glBindFramebuffer(GL_FRAMEBUFFER, intFbo);
  glViewport(0, 0, intW, intH);
  gfx_tamanho_alvo(intW, intH);
  intLigado = 1;
  // O glClear de main.c vem logo depois e ja diz a GPU que nada do quadro
  // anterior precisa ser lido neste alvo.
}

void gpun_quadro_fim(void) {
  if (intLigado) {
    GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
    float asp = gfx_tex_aspect_atual, op = gfx_opacidade_grupo;
    intLigado = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, telaW, telaH);
    gfx_tamanho_alvo(telaW, telaH);
    glDisable(GL_SCISSOR_TEST);
    // A janela inteira vai ser coberta pela ampliacao, opaca: nada dela
    // precisa ser lido. Sem descarte, um clear diz o mesmo a GPU.
    if (descarte) gpun_descartar_cor(1);
    else { glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT); }
    glDisable(GL_BLEND);   // copia RGB E alpha exatos (o furo do video)
    gfx_tex_aspect_atual = 0.0f;
    gfx_opacidade_grupo = 1.0f;
    gfx_rect(tela, intTex, GFX_COPIA, 0, 0.0f, 1.0f, 0.0f, 1, 1, 1, 1);
    // Solta a textura do alvo: no quadro seguinte ela volta a ser o alvo de
    // desenho, e ficar ligada para leitura ao mesmo tempo e o laco de
    // realimentacao que o GLES deixa indefinido.
    glBindTexture(GL_TEXTURE_2D, 0);
    gfx_tex_esquecer(0);
    gfx_tex_aspect_atual = asp;
    gfx_opacidade_grupo = op;
    glEnable(GL_BLEND);
  }
  // Profundidade/stencil da janela nao sao lidos por ninguem depois do swap.
  if (descarte && profStencil) {
    static const GLenum ps[2] = { NV_GL_DEPTH, NV_GL_STENCIL };
    descarte(GL_FRAMEBUFFER, 2, ps);
  }
}

#ifdef NV_GPUN_TESTE
// tests/gpunivel.sh: a regra do adaptativo sem TV nem GL.
void gpun_teste_reiniciar(void) {
  nivel = 0; adaptativo = 1; decidido = 0; origem = "teste";
  aquece = GPUN_AQUECE_MS; janMs = janEsp = janCpu = totalMs = 0; janN = 0; estavaNaHome = 0;
}
int gpun_teste_decidido(void) { return decidido; }
#endif

static void decidir(const char *porque) {
  decidido = 1;
  printf("[gpu-nivel] decidido: fica no nivel %d (%s)\n", nivel, porque);
  fflush(stdout);
  gravar();
}

void gpun_medir(double dtms, double espera, double cpu, int naHome, int cheia) {
  double fps, e, c;
  if (!adaptativo || decidido) return;
  if (!naHome || !cheia || dtms > 1000.0) {
    // Fora da home (ou suspensao): a janela em curso nao vale; ao voltar,
    // aquece de novo antes de medir.
    if (estavaNaHome) { janN = 0; janMs = janEsp = janCpu = 0; if (aquece < GPUN_ASSENTA_MS) aquece = GPUN_ASSENTA_MS; }
    estavaNaHome = 0;
    return;
  }
  estavaNaHome = 1;
  if (aquece > 0) { aquece -= dtms; return; }
  janN++; janMs += dtms; janEsp += espera; janCpu += cpu; totalMs += dtms;
  if (janMs < GPUN_JANELA_MS) return;
  fps = janN * 1000.0 / janMs;
  e = janEsp / janN;
  c = janCpu / janN;
  printf("[gpu-nivel] janela na home: nivel=%d fps=%.1f espera=%.1fms cpu=%.1fms (%d quadros)\n",
         nivel, fps, e, c, janN);
  janN = 0; janMs = janEsp = janCpu = 0;
  if (fps >= GPUN_FPS_BOM) { decidir("fps bom"); return; }
  if (e >= GPUN_ESPERA_MIN && e > c) {
    // O adaptativo PARA no nivel 1 quando ele ja resolve. Teste nas duas
    // Tizen 5.0 (#180, 29/09): "efeitos" ficou liso e bonito; o 720p ficou
    // mais liso mas com o texto borrado demais — ninguem gostou, e por isso o
    // 720p (nivel 3) so existe forcado. Quando o 1 AINDA fica abaixo de
    // GPUN_FPS_CRITICO (a Mali-400 do registro 9859, UA40N5300, Tizen 4.0,
    // 10-21 fps com efeitos leves), desce ao 2: efeitos MINIMOS, em 1080p.
    if (nivel == 0 || (nivel == 1 && fps < GPUN_FPS_CRITICO)) {
      aplicar(nivel + 1, nivel == 0 ? "GPU presa: efeitos leves"
                                    : "GPU presa mesmo com efeitos leves: efeitos minimos");
      gravar();
      aquece = GPUN_ASSENTA_MS;
      if (totalMs >= GPUN_TETO_MS) decidir("teto de tempo de medida");
      return;
    }
    decidir("GPU presa, mas ja no ultimo nivel");
    return;
  }
  decidir("lento pela CPU ou pelo resto, nao pela GPU: menos pixel nao ajuda");
}
