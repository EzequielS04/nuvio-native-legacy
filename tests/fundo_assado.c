// FUNDO ASSADO (gfx_fundo_assado, fundo.c): o "Frost" e a "Arte borrada" de
// tela cheia saem de um quadro pequeno de 320x180 pintado so quando a chave
// muda. Na C9 (Mali-G71) os dois sairam QUASE PRETOS depois do 1bcd6ebe, e as
// capturas do Mac continuavam iguais. Este teste:
//   (1) le de volta o PROPRIO quadro pequeno (glReadPixels num FBO dele) e
//       confere que a pintura chegou la, nao so o clear;
//   (2) compara a tela assada com o desenho direto de sempre (Frost e Borrada);
//   (3) assa a Borrada com o estado de GL que um quadro de verdade deixa (a
//       mistura desligada por quem desenhou antes) e confere que nao escurece;
//   (4) arte sem paleta -> paleta chega: o assado sai na hora certa.
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
static double comparar(int modo, const char *chave, int misturaDesligada, const char *nome) {
  double a[NA][3], b[NA][3], d;
  int antes = gfx_n_fundo_assados;
  GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
  gfx_fundo_assado_desligado = 1;
  quadro(); fundo_desenhar_modo(modo, tela, 0.0f, chave, 1.0f); grade(b);
  gfx_fundo_assado_desligado = 0;
  quadro();
  // O estado que um quadro de verdade pode deixar: alguem desenhou antes com a
  // mistura desligada (o furo do video, gfx_ambiente opaco) — glDisable direto,
  // como o gpun_quadro_fim e o player fazem.
  if (misturaDesligada) glDisable(GL_BLEND);
  fundo_desenhar_modo(modo, tela, 0.0f, chave, 1.0f);
  if (misturaDesligada) glEnable(GL_BLEND);
  grade(a);
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
    GLuint t, f2;
    double esq[3], dir[3];
    quadro();
    t = gfx_fundo_assado(2, k, 1, pintarTeste, NULL);
    assert(t && pintouCor == 1);
    // a conferencia da criacao (padrao assado e lido pelo GFX_FOSCO) passou
    printf("conferencia da criacao: %d\n", gfx_fundo_assado_conferencia);
    if (gfx_fundo_assado_conferencia != 1) { printf("FALHA: conferencia\n"); falhas++; }
    glGenFramebuffers(1, &f2); glBindFramebuffer(GL_FRAMEBUFFER, f2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    media(f2, 320, 180, 20, 20, 100, 140, esq);
    media(f2, 320, 180, 200, 20, 100, 140, dir);
    glDeleteFramebuffers(1, &f2); gfx_tex_esquecer(0);
    printf("quadro pequeno: esquerda %.0f,%.0f,%.0f  direita %.0f,%.0f,%.0f\n",
           esq[0], esq[1], esq[2], dir[0], dir[1], dir[2]);
    // esquerda: fundo + 50% vermelho; direita: verde opaco (dither: +-1)
    if (!(esq[0] > 120 && esq[0] < 140 && dir[1] > 250 && dir[0] < 3)) {
      printf("FALHA: o quadro pequeno nao recebeu a pintura\n"); falhas++; }
    // mesma chave: nao repinta
    quadro(); assert(gfx_fundo_assado(2, k, 1, pintarTeste, NULL) == t && pintouCor == 1);
  }

  /* (2) FROST: assado = direto (dither e ampliacao: alguns niveis). */
  d = comparar(FUNDO_FROST, NULL, 0, "frost");
  if (d > 4.0) { printf("FALHA: frost assado difere do direto\n"); falhas++; }

  /* (3) ARTE BORRADA, com a paleta de uma arte. */
  memset(&p, 0, sizeof p);
  p.ok = 1;
  { const float r[4][3] = { { .80f, .25f, .20f }, { .20f, .45f, .85f }, { .85f, .70f, .20f }, { .30f, .75f, .40f } };
    memcpy(p.regiao, r, sizeof r); }
  corviva_anotar("teste://arte-a", &p);
  assert(corviva_paleta("teste://arte-a", &p) && p.ok);
  d = comparar(FUNDO_BORRADA, "teste://arte-a", 0, "borrada");
  if (d > 4.0) { printf("FALHA: borrada assada difere da direta\n"); falhas++; }

  /* (4) O MESMO COM A MISTURA DESLIGADA POR FORA quando o assado roda. Antes
   * da correcao isto saia QUASE PRETO (luz max 6,9 contra 54,3): o veu de 28%
   * substituia a luz. O assado agora poe o proprio estado (mistura, funcao,
   * viewport, uTela, unidade 0 solta) e nao depende do que o quadro deixou. */
  p.regiao[0][0] = .70f; corviva_anotar("teste://arte-b", &p);
  d = comparar(FUNDO_BORRADA, "teste://arte-b", 1, "borrada, mistura desligada antes");
  if (d > 4.0) { printf("FALHA: borrada assada com a mistura desligada difere\n"); falhas++; }

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

  if (falhas) { printf("fundo_assado: %d falha(s)\n", falhas); return 1; }
  printf("fundo_assado: ok\n");
  return 0;
}
