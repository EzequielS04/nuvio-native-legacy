// FUNDO PELO CAMINHO DA LUZ IMERSIVA (gfx_luz_canal, fundo.c): o "Frost" e a
// "Arte borrada" de tela cheia saem de um quadro pequeno de 320x180 assado pelo
// MESMO codigo da luz imersiva (ambCriarAlvos/ambAssarEm) so quando a chave
// muda, e vao a tela pela passada do ambPintar (GFX_SNAP opaco). Na C9
// (Mali-G71) o assado proprio do 1bcd6ebe saiu escuro e as capturas do Mac
// continuavam iguais. Este teste:
//   (1) le de volta o PROPRIO quadro pequeno e confere que a pintura chegou la;
//   (2) compara a tela assada com o desenho direto de sempre (Frost e Borrada)
//       e confere que a conferencia de uma vez do fundo.c (pixel da tela contra
//       a conta do CPU) passa;
//   (3) assa com a mistura desligada por fora: nao escurece;
//   (4) com a Dinamica imersiva ligada (outra paleta na luz da cena) a Borrada
//       assa UMA vez em varios quadros (nao disputa o quadro da imersiva) e sai
//       igual;
//   (5) arte sem paleta -> paleta chega: o assado sai na hora certa;
//   (6) o despejo de uma vez (fundo.c) gravou a tela e o quadro pequeno dos dois
//       fundos, sem pedido nenhum, e nao grava de novo.
// Roda no GL 2.1 do Mac e, com NV_GLES_NO_MAC, no GLES2 do ANGLE (o dialeto e
// as regras de FBO da TV): bash tests/fundo_assado.sh [gles]
#include "gfx.h"
#include "fundo.h"
#include "corviva.h"
#include "ajustes.h"
#include "layout.h"
#include "text.h"
#include "tex_cache.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef NV_GLES_NO_MAC
#include <EGL/egl.h>
#endif

static GLuint fbo, fboTex;
static const int LW = 1920, LH = 1080;

static void contexto(void) {
#ifdef NV_GLES_NO_MAC
  EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  EGLint maj, min, n;
  EGLConfig c;
  static const EGLint ca[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
                               EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                               EGL_NONE };
  static const EGLint sa[] = { EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };
  static const EGLint xa[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
  EGLSurface s; EGLContext x;
  assert(d != EGL_NO_DISPLAY && eglInitialize(d, &maj, &min));
  assert(eglChooseConfig(d, ca, &c, 1, &n) && n == 1);
  s = eglCreatePbufferSurface(d, c, sa); assert(s != EGL_NO_SURFACE);
  x = eglCreateContext(d, c, EGL_NO_CONTEXT, xa); assert(x != EGL_NO_CONTEXT);
  assert(eglMakeCurrent(d, s, s, x));
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *w = SDL_CreateWindow("fundo", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  assert(SDL_GL_CreateContext(w));
#endif
  printf("GL: %s | %s\n", (const char *)glGetString(GL_VERSION), (const char *)glGetString(GL_RENDERER));
}

// Media RGB de um retangulo da tela (y de cima), em niveis 0..255.
static void media(GLuint alvo, int w, int h, int x0, int y0, int rw, int rh, double o[3]) {
  unsigned char *px = malloc((size_t)rw * (size_t)rh * 4);
  long s[3] = { 0, 0, 0 };
  int i;
  glFinish();
  glBindFramebuffer(GL_FRAMEBUFFER, alvo);
  glReadPixels(x0, h - y0 - rh, rw, rh, GL_RGBA, GL_UNSIGNED_BYTE, px);
  for (i = 0; i < rw * rh; i++) { s[0] += px[i * 4]; s[1] += px[i * 4 + 1]; s[2] += px[i * 4 + 2]; }
  for (i = 0; i < 3; i++) o[i] = (double)s[i] / (rw * rh);
  free(px);
  (void)w;
}
static double lum(const double c[3]) { return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]; }

static void quadro(void) {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
  gfx_novo_quadro();
  glDisable(GL_SCISSOR_TEST);
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

// Uma grade de 6x4 amostras de 40x40 da tela; devolve a maior diferenca media
// por canal entre a e b e guarda as medias.
#define NA 24
static void grade(double m[NA][3]) {
  int i;
  for (i = 0; i < NA; i++) media(fbo, LW, LH, 80 + (i % 6) * 320, 60 + (i / 6) * 260, 40, 40, m[i]);
}
static double dif(double a[NA][3], double b[NA][3]) {
  double d = 0; int i, k;
  for (i = 0; i < NA; i++) for (k = 0; k < 3; k++) if (fabs(a[i][k] - b[i][k]) > d) d = fabs(a[i][k] - b[i][k]);
  return d;
}
static double lumMax(double a[NA][3]) {
  double l = 0; int i;
  for (i = 0; i < NA; i++) if (lum(a[i]) > l) l = lum(a[i]);
  return l;
}

// Desenha o fundo `modo` duas vezes: assado e direto. Devolve a diferenca.
// O assado vai primeiro: e o primeiro desenho opaco que o despejo de uma vez
// grava, como no app.
static double comparar(int modo, const char *chave, int misturaDesligada, const char *nome) {
  double a[NA][3], b[NA][3], d;
  int antes = gfx_n_fundo_assados;
  GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
  quadro();
  // O estado que um quadro de verdade pode deixar: alguem desenhou antes com a
  // mistura desligada (o furo do video, gfx_ambiente opaco) — glDisable direto,
  // como o gpun_quadro_fim e o player fazem.
  if (misturaDesligada) glDisable(GL_BLEND);
  fundo_desenhar_modo(modo, tela, 0.0f, chave, 1.0f);
  if (misturaDesligada) glEnable(GL_BLEND);
  grade(a);
  gfx_fundo_assado_desligado = 1;
  quadro(); fundo_desenhar_modo(modo, tela, 0.0f, chave, 1.0f); grade(b);
  gfx_fundo_assado_desligado = 0;
  d = dif(a, b);
  printf("%s: assado %d vez(es), diferenca max %.1f niveis, luz max assado %.1f / direto %.1f\n", nome,
         gfx_n_fundo_assados - antes, d, lumMax(a), lumMax(b));
  return d;
}

static int pintouCor;
static void pintarTeste(void *ctx) {
  (void)ctx;
  pintouCor++;
  // meia tela vermelha por cima do clear: precisa da mistura e do viewport certos
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W * 0.5f, NV_TELA_H }, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f);
  gfx_cor((GfxRect){ NV_TELA_W * 0.5f, 0, NV_TELA_W * 0.5f, NV_TELA_H }, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
}

int main(void) {
  CorvivaPaleta p;
  double m[3], d;
  int falhas = 0;
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  contexto();
  glGenTextures(1, &fboTex); glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  { char c[700]; FILE *f; snprintf(c, sizeof c, "%s/ajustes.txt", getenv("NUVIO_DADOS"));
    f = fopen(c, "w"); assert(f); fprintf(f, "idioma 0\n"); fclose(f);
    ajustes_dir(getenv("NUVIO_DADOS")); }
  ajustes_iniciar();

  /* (1) O QUADRO PEQUENO EM SI: a pintura chega nele (nao so o clear). */
  { float k[1] = { 7.0f };
    GLuint t;
    unsigned char esq[3], dir[3];
    quadro();
    t = gfx_luz_canal(1, k, 1, pintarTeste, NULL);
    assert(t && pintouCor == 1);
    assert(gfx_luz_canal_px(t, 0.25f, 0.5f, esq) && gfx_luz_canal_px(t, 0.75f, 0.5f, dir));
    printf("quadro pequeno: esquerda %d,%d,%d  direita %d,%d,%d\n", esq[0], esq[1], esq[2], dir[0], dir[1], dir[2]);
    // esquerda: fundo + 50% vermelho; direita: verde opaco
    if (!(esq[0] > 120 && esq[0] < 140 && dir[1] > 250 && dir[0] < 3)) {
      printf("FALHA: o quadro pequeno nao recebeu a pintura\n"); falhas++; }
    // mesma chave: nao repinta
    quadro(); assert(gfx_luz_canal(1, k, 1, pintarTeste, NULL) == t && pintouCor == 1);
  }

  /* (2) FROST: assado = direto (dither e ampliacao: alguns niveis), e a
   * conferencia do fundo.c passa. */
  d = comparar(FUNDO_FROST, NULL, 0, "frost");
  if (d > 4.0) { printf("FALHA: frost assado difere do direto\n"); falhas++; }
  printf("conferencia frost: %d\n", fundo_conferencia(FUNDO_FROST));
  if (fundo_conferencia(FUNDO_FROST) != 1) { printf("FALHA: conferencia do frost\n"); falhas++; }

  /* (2b) ARTE BORRADA, com a paleta de uma arte. */
  memset(&p, 0, sizeof p);
  p.ok = 1;
  { const float r[4][3] = { { .80f, .25f, .20f }, { .20f, .45f, .85f }, { .85f, .70f, .20f }, { .30f, .75f, .40f } };
    memcpy(p.regiao, r, sizeof r); }
  corviva_anotar("teste://arte-a", &p);
  assert(corviva_paleta("teste://arte-a", &p) && p.ok);
  d = comparar(FUNDO_BORRADA, "teste://arte-a", 0, "borrada");
  if (d > 4.0) { printf("FALHA: borrada assada difere da direta\n"); falhas++; }
  printf("conferencia borrada: %d\n", fundo_conferencia(FUNDO_BORRADA));
  if (fundo_conferencia(FUNDO_BORRADA) != 1) { printf("FALHA: conferencia da borrada\n"); falhas++; }

  /* (3) O MESMO COM A MISTURA DESLIGADA POR FORA quando o assado roda (no
   * 1bcd6ebe isto saia QUASE PRETO: o veu substituia a luz). */
  p.regiao[0][0] = .70f; corviva_anotar("teste://arte-b", &p);
  d = comparar(FUNDO_BORRADA, "teste://arte-b", 1, "borrada, mistura desligada antes");
  if (d > 4.0) { printf("FALHA: borrada assada com a mistura desligada difere\n"); falhas++; }

  /* (4) COM A DINAMICA IMERSIVA LIGADA: a luz da cena tem OUTRA paleta e e
   * assada por gfx_ambiente_preparar todo quadro (main.c). Antes a Borrada
   * usava o mesmo quadro e as chaves se revezavam (dois assados por quadro).
   * Agora: um assado da Borrada em seis quadros, e a tela igual a sem imersiva. */
  { GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
    double a1[NA][3], b1[NA][3];
    float ambAnt[4][3], forcaAnt = nv_ambiente_forca;
    int antes, i, j;
    quadro(); fundo_desenhar_modo(FUNDO_BORRADA, tela, 0.0f, "teste://arte-b", 1.0f); grade(b1);
    memcpy(ambAnt, nv_ambiente_viva, sizeof ambAnt);
    for (i = 0; i < 4; i++) for (j = 0; j < 3; j++) nv_ambiente_viva[i][j] = 0.1f + 0.2f * (float)i;
    nv_ambiente_forca = 1.0f;
    antes = gfx_n_fundo_assados;
    for (i = 0; i < 6; i++) {
      glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
      gfx_novo_quadro();
      nv_tempo_viva = 0.2f * (float)i;   // a respiracao: a luz da cena reassa
      gfx_ambiente_preparar();
      glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      gfx_ambiente(1.0f);
      fundo_desenhar_modo(FUNDO_BORRADA, tela, 0.0f, "teste://arte-b", 1.0f);
      gfx_ambiente_descarregar();
    }
    grade(a1);
    d = dif(a1, b1);
    printf("borrada com a imersiva: %d assado(s) em 6 quadros, diferenca max %.1f niveis\n",
           gfx_n_fundo_assados - antes, d);
    if (gfx_n_fundo_assados - antes > 0) { printf("FALHA: a borrada reassou com a imersiva\n"); falhas++; }
    if (d > 4.0) { printf("FALHA: borrada com a imersiva difere\n"); falhas++; }
    memcpy(nv_ambiente_viva, ambAnt, sizeof ambAnt);
    nv_ambiente_forca = forcaAnt; nv_tempo_viva = 0.0f;
  }

  /* (5) ARTE SEM PALETA AINDA -> PALETA CHEGA: sem paleta nada e assado (o
   * caminho da arte decide); com ela, o proximo quadro assa a cor certa. */
  { GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
    int antes = gfx_n_fundo_assados;
    quadro(); fundo_desenhar_modo(FUNDO_BORRADA, tela, 0.0f, "teste://arte-c", 1.0f);
    assert(gfx_n_fundo_assados == antes);
    p.regiao[0][0] = .95f; p.regiao[1][2] = .30f; corviva_anotar("teste://arte-c", &p);
    quadro(); fundo_desenhar_modo(FUNDO_BORRADA, tela, 0.0f, "teste://arte-c", 1.0f);
    assert(gfx_n_fundo_assados == antes + 1);
    media(fbo, LW, LH, 20, 500, 60, 60, m);
    printf("borrada com a paleta que chegou: borda esquerda %.0f,%.0f,%.0f\n", m[0], m[1], m[2]);
    if (!(m[0] > m[2] + 15)) { printf("FALHA: a luz da esquerda (vermelha) nao apareceu\n"); falhas++; } }

  /* (6) DESPEJO DE UMA VEZ: os desenhos acima ja gravaram, sem pedido, a tela
   * e o quadro pequeno de cada fundo; apagados, mais 120 desenhos nao regravam. */
  { GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
    static const char *const nome[2] = { "frost", "borrada" };
    char b1[2][700], b2[2][700];
    struct stat st;
    int i, k;
    const char *dir = getenv("NUVIO_DUMP_FUNDO_DIR");
    assert(dir);
    for (k = 0; k < 2; k++) {
      long t1, t2;
      snprintf(b1[k], sizeof b1[k], "%s/nuvio-fundo-%s.bmp", dir, nome[k]);
      snprintf(b2[k], sizeof b2[k], "%s/nuvio-fundo-%s-assado.bmp", dir, nome[k]);
      t1 = stat(b1[k], &st) == 0 ? (long)st.st_size : -1L;
      t2 = stat(b2[k], &st) == 0 ? (long)st.st_size : -1L;
      printf("despejo %s: tela %ld bytes, quadro pequeno %ld bytes\n", nome[k], t1, t2);
      if (t1 != 54 + 960 * 540 * 4 || t2 != 54 + 320 * 180 * 4) { printf("FALHA: despejo %s\n", nome[k]); falhas++; }
      unlink(b1[k]); unlink(b2[k]);
    }
    for (i = 0; i < 60; i++) {
      quadro(); fundo_desenhar_modo(FUNDO_FROST, tela, 0.0f, NULL, 1.0f);
      quadro(); fundo_desenhar_modo(FUNDO_BORRADA, tela, 0.0f, "teste://arte-b", 1.0f);
    }
    for (k = 0; k < 2; k++)
      if (stat(b1[k], &st) == 0 || stat(b2[k], &st) == 0) { printf("FALHA: o despejo de %s repetiu\n", nome[k]); falhas++; }
  }

  if (falhas) { printf("fundo_assado: %d falha(s)\n", falhas); return 1; }
  printf("fundo_assado: ok\n");
  return 0;
}
