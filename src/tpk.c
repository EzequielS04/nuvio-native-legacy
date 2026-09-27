// Ponte do alvo .tpk da Samsung. Ver o porque em tpk.h.
//
// O mesmo protocolo serve aos dois hosts: no Tizen 6+ quem chama
// nv_tpk_quadro e o render do GLWindow (NUI); no 4/5 e o OnUpdate da
// TVGLApplication da Samsung (Tizen.NET.TV), o molde do JuvoPlayer.OpenGL.
// Nos dois o contexto esta corrente quando a chamada chega, e quem troca os
// buffers e o framework, depois que ela volta.
//
// Dois fios dividem UM contexto EGL, o do GLWindow do host .NET:
//   - o fio de desenho do NUI chama nv_tpk_quadro() com o contexto corrente,
//     solta o contexto, passa a vez ao app e espera ele devolver;
//   - o fio do app roda o main() de sempre; quando o laco chega no
//     SDL_GL_SwapWindow (tpk_gl_trocar) ele solta o contexto e devolve a vez.
// Quem troca os buffers e o GLWindow, depois que nv_tpk_quadro retorna. Um
// contexto EGL so pode estar corrente num fio por vez; a vez (`vez`) garante
// isso sem que nenhum dos lados precise saber o que o outro faz.
#ifdef NV_TPK
#include <SDL2/SDL.h>
#include "tpk_egl.h"
#include <GLES2/gl2.h>
#include <time.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv);

TpkEgl tpkEgl;
int tpk_egl_carregar(void) {
  static const char *const nomes[] = { "libEGL.so.1", "libEGL.so", "libEGL.so.1.4", NULL };
  void *h = NULL;
  int i;
  if (tpkEgl.MakeCurrent) return 0;
  for (i = 0; nomes[i] && !h; i++) h = dlopen(nomes[i], RTLD_NOW | RTLD_GLOBAL);
  if (!h) { printf("[tpk] libEGL nao encontrada: %s\n", dlerror()); fflush(stdout); return -1; }
#define PEGA(c, n) *(void **)&tpkEgl.c = dlsym(h, n)
  PEGA(GetCurrentDisplay, "eglGetCurrentDisplay"); PEGA(GetCurrentSurface, "eglGetCurrentSurface");
  PEGA(GetCurrentContext, "eglGetCurrentContext"); PEGA(GetError, "eglGetError");
  PEGA(QueryContext, "eglQueryContext"); PEGA(ChooseConfig, "eglChooseConfig");
  PEGA(GetConfigAttrib, "eglGetConfigAttrib"); PEGA(GetDisplay, "eglGetDisplay");
  PEGA(Initialize, "eglInitialize"); PEGA(BindAPI, "eglBindAPI");
  PEGA(CreateWindowSurface, "eglCreateWindowSurface"); PEGA(CreateContext, "eglCreateContext");
  PEGA(SwapInterval, "eglSwapInterval"); PEGA(SwapBuffers, "eglSwapBuffers");
  PEGA(MakeCurrent, "eglMakeCurrent");
#undef PEGA
  return tpkEgl.MakeCurrent && tpkEgl.GetCurrentContext ? 0 : -1;
}

// A VEZ. Quem tem a vez pode ter o contexto corrente; o outro nao toca em GL.
//   vez == VEZ_NUI: o fio do framework (render do GLWindow, ou o OnUpdate da
//                   TVGLApplication) segura o contexto;
//   vez == VEZ_APP: o fio do app esta desenhando um quadro.
// `pendente` = o app entregou um quadro que ainda nao foi trocado na tela.
// `appEsperando` = o app esta parado em tpk_gl_criar/tpk_gl_trocar esperando
// a vez; so entao vale a pena entregar o contexto.
enum { VEZ_NUI = 0, VEZ_APP = 1 };
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  mudou = PTHREAD_COND_INITIALIZER;
static int vez = VEZ_NUI, iniciado, terminou, pendente, appEsperando, nuiCorrente = 1;
static EGLDisplay dpy = EGL_NO_DISPLAY;
static EGLSurface sup = EGL_NO_SURFACE;
static EGLContext ctx = EGL_NO_CONTEXT;
static int telaW = 1920, telaH = 1080;
static char dirArte[512];
static char erro[512];
// Configuracao do host (nv_tpk_config): quanto o framework espera o app num
// quadro (ms; <0 = ate ele terminar) e se desliga o vsync do eglSwapBuffers.
static int esperaMs = 50, swapZero;
static unsigned nQuadros, nPulados;

// Fio do app: devolve a vez ao framework e espera ela voltar.
static void appEsperaVez(int entregou) {
  pthread_mutex_lock(&trava);
  if (entregou) { vez = VEZ_NUI; pendente = 1; pthread_cond_broadcast(&mudou); }
  appEsperando = 1;
  pthread_cond_broadcast(&mudou);
  while (vez != VEZ_APP) pthread_cond_wait(&mudou, &trava);
  appEsperando = 0;
  pthread_mutex_unlock(&trava);
}

static void *fioApp(void *arg) {
  char *argv[] = { "nuvio", dirArte, NULL };
  int r;
  (void)arg;
  r = main(2, argv);
  printf("[tpk] main devolveu %d\n", r);
  fflush(stdout);
  pthread_mutex_lock(&trava);
  if (vez == VEZ_APP) eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  terminou = 1;
  vez = VEZ_NUI;
  pthread_cond_broadcast(&mudou);
  pthread_mutex_unlock(&trava);
  return NULL;
}

// Chamada uma vez pelo host, antes do primeiro quadro. Os caminhos vem do
// Tizen.Applications (res/ e data/ do pacote): no .tpk nao ha HOME util nem
// pasta ao lado do executavel.
__attribute__((visibility("default")))
int nv_tpk_iniciar(const char *arte, const char *dados, int w, int h) {
  pthread_t t;
  pthread_attr_t at;
  char log[600];
  if (iniciado) return 0;
  iniciado = 1;
  if (tpk_egl_carregar() != 0) { snprintf(erro, sizeof erro, "libEGL nao encontrada na TV"); return -1; }
  if (w > 0 && h > 0) { telaW = w; telaH = h; }
  snprintf(dirArte, sizeof dirArte, "%s", arte);
  snprintf(log, sizeof log, "%s/nuvio.log", dados);
  { char ant[600];
    snprintf(ant, sizeof ant, "%s/nuvio-anterior.log", dados);
    setenv("NUVIO_LOG_ANTERIOR", ant, 1); }
  setenv("NUVIO_DADOS", dados, 1);
  setenv("NUVIO_LOG", log, 1);
  setenv("HOME", dados, 1);
  setenv("SDL_VIDEODRIVER", "dummy", 1);
  pthread_attr_init(&at);
  // O main() e o laco usam pilha como um processo normal (buffers de 4-8 KB
  // aos montes); o padrao de fio do Tizen e 8 MB mas nao e garantido.
  pthread_attr_setstacksize(&at, 8 << 20);
  if (pthread_create(&t, &at, fioApp, NULL) != 0) return -1;
  pthread_detach(t);
  return 0;
}

// 1 depois que o main() do app devolveu: o host fecha.
__attribute__((visibility("default")))
int nv_tpk_terminou(void) { return terminou; }

// Motivo de o app nao ter subido, para o host por na tela ("" = nenhum).
__attribute__((visibility("default")))
const char *nv_tpk_erro(void) { return erro; }

// Host, antes do primeiro quadro. esperaMs: quanto cada quadro do framework
// espera o app (<0 = sem limite, para o GLWindow da API8, que troca os
// buffers mesmo quando o quadro e pulado). swapZero: eglSwapInterval(0) —
// no Tizen 9 o swap com intervalo 1 espera o vblank do TDM e o GLWindow ja
// dorme ate 60 Hz sozinho (dali-adaptor gl-window-render-thread.cpp).
__attribute__((visibility("default")))
void nv_tpk_config(int espera, int zero) { esperaMs = espera; swapZero = zero; }

// Fio do framework, contexto dele corrente (ou nao, se o app ficou com a vez
// no quadro anterior). Devolve 1 = troque os buffers, 0 = pule este quadro
// (nao houve desenho novo), -1 = o app acabou, feche.
//
// Nunca espera mais que esperaMs: no Tizen 6.0 este callback roda no fio
// PRINCIPAL do NUI (AddIdle, gl-window-impl.cpp da 6.0), e segurar aqui
// seguraria teclas, relogio e eventos do player. Se o app nao terminou o
// quadro a tempo, o contexto fica com ele e o quadro seguinte do framework
// pula ate ele devolver.
__attribute__((visibility("default")))
int nv_tpk_quadro(void) {
  int r;
  if (ctx == EGL_NO_CONTEXT) {
    dpy = eglGetCurrentDisplay();
    sup = eglGetCurrentSurface(EGL_DRAW);
    ctx = eglGetCurrentContext();
    if (swapZero && tpkEgl.SwapInterval) tpkEgl.SwapInterval(dpy, 0);
    printf("[tpk] contexto do framework: dpy=%p sup=%p ctx=%p espera=%d swap0=%d\n",
           dpy, sup, ctx, esperaMs, swapZero);
    fflush(stdout);
  }
  pthread_mutex_lock(&trava);
  if (vez == VEZ_APP) {                 // o app ainda desenha o quadro anterior
    r = terminou ? -1 : 0;
    nPulados++;
    pthread_mutex_unlock(&trava);
    return r;
  }
  if (!nuiCorrente) { eglMakeCurrent(dpy, sup, sup, ctx); nuiCorrente = 1; }
  if (pendente) { pendente = 0; pthread_mutex_unlock(&trava); nQuadros++; return 1; }
  if (terminou) { pthread_mutex_unlock(&trava); return -1; }
  if (!appEsperando) {
    // Arranque: o app ainda nao chegou ao primeiro quadro. No GLWindow da API8
    // (que troca de qualquer jeito) limpa para preto em vez de mostrar lixo.
    pthread_mutex_unlock(&trava);
    if (esperaMs < 0) { glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT); return 1; }
    return 0;
  }
  eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  nuiCorrente = 0;
  vez = VEZ_APP;
  pthread_cond_broadcast(&mudou);
  if (esperaMs < 0) {
    while (vez == VEZ_APP && !terminou) pthread_cond_wait(&mudou, &trava);
  } else {
    struct timespec ate;
    clock_gettime(CLOCK_REALTIME, &ate);
    ate.tv_nsec += (long)esperaMs * 1000000L;
    ate.tv_sec += ate.tv_nsec / 1000000000L;
    ate.tv_nsec %= 1000000000L;
    while (vez == VEZ_APP && !terminou)
      if (pthread_cond_timedwait(&mudou, &trava, &ate) != 0) break;
  }
  if (vez == VEZ_APP) {                  // passou do prazo: o app segue com o contexto
    r = terminou ? -1 : 0;
    nPulados++;
    pthread_mutex_unlock(&trava);
    return r;
  }
  eglMakeCurrent(dpy, sup, sup, ctx);
  nuiCorrente = 1;
  r = pendente ? 1 : (terminou ? -1 : 0);
  pendente = 0;
  pthread_mutex_unlock(&trava);
  if (r == 1 && (++nQuadros % 600) == 0) {
    printf("[tpk] quadros=%u pulados=%u\n", nQuadros, nPulados);
    fflush(stdout);
  }
  return r;
}

void *tpk_gl_criar(void) {
  appEsperaVez(0);
  if (!eglMakeCurrent(dpy, sup, sup, ctx)) {
    printf("[tpk] eglMakeCurrent no fio do app falhou: 0x%x\n", eglGetError());
    snprintf(erro, sizeof erro, "eglMakeCurrent no fio do app falhou: 0x%x", eglGetError());
    return NULL;
  }
  return (void *)1;
}

void tpk_gl_trocar(void) {
  eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  appEsperaVez(1);
  eglMakeCurrent(dpy, sup, sup, ctx);
}

void tpk_tamanho(int *w, int *h) {
  if (w) *w = telaW;
  if (h) *h = telaH;
}

int tpk_gl_atributo(SDL_GLattr a, int *v) {
  EGLint val = -1, id = 0;
  EGLConfig cfg;
  EGLint n = 0, pede[] = { EGL_CONFIG_ID, 0, EGL_NONE };
  EGLint qual;
  switch (a) {
    case SDL_GL_RED_SIZE:   qual = EGL_RED_SIZE; break;
    case SDL_GL_GREEN_SIZE: qual = EGL_GREEN_SIZE; break;
    case SDL_GL_BLUE_SIZE:  qual = EGL_BLUE_SIZE; break;
    case SDL_GL_ALPHA_SIZE: qual = EGL_ALPHA_SIZE; break;
    default: if (v) *v = 0; return -1;
  }
  eglQueryContext(dpy, ctx, EGL_CONFIG_ID, &id);
  pede[1] = id;
  if (eglChooseConfig(dpy, pede, &cfg, 1, &n) && n == 1)
    eglGetConfigAttrib(dpy, cfg, qual, &val);
  if (v) *v = val;
  return 0;
}

// Tecla do controle, pelo nome que o NUI da (Key.KeyPressedName). Vem do fio
// principal do .NET; SDL_PushEvent e seguro de qualquer fio.
__attribute__((visibility("default")))
void nv_tpk_tecla(const char *nome, int apertou) {
  // Mesma traducao do .wgt (tools/tizen-shell.html), para os dois pacotes da
  // Samsung se comportarem igual. Nomes: developer.samsung.com/smarttv/develop/
  // tizen-net-tv/guides/user-interaction.html
  //  - toda tecla de play/pause vira SDLK_PAUSE (player.c alterna e poe o foco
  //    no Play; o app nao tem "so pausar"), inclusive XF86PlayBack, que e o
  //    play/pause do Smart Remote 2021+;
  //  - Stop volta; retroceder/avancar viram as setas (seek do player);
  //  - CH+ e a azul viram "s" (Salvos), vermelha/verde abrem o painel de log (F9).
  static const struct { const char *n; SDL_Keycode k; } T[] = {
    { "Up", SDLK_UP }, { "Down", SDLK_DOWN }, { "Left", SDLK_LEFT }, { "Right", SDLK_RIGHT },
    { "Return", SDLK_RETURN }, { "KP_Enter", SDLK_RETURN }, { "Select", SDLK_RETURN },
    { "XF86Back", SDLK_AC_BACK }, { "Escape", SDLK_AC_BACK }, { "BackSpace", SDLK_BACKSPACE },
    { "XF86PlayBack", SDLK_PAUSE }, { "XF86AudioPlay", SDLK_PAUSE }, { "XF86AudioPause", SDLK_PAUSE },
    { "XF86AudioPlayPause", SDLK_PAUSE }, { "XF86AudioStop", SDLK_AC_BACK },
    { "XF86AudioRewind", SDLK_LEFT }, { "XF86AudioForward", SDLK_RIGHT },
    { "XF86AudioNext", SDLK_RIGHT }, { "XF86AudioPrev", SDLK_LEFT },
    { "XF86NextChapter", SDLK_RIGHT }, { "XF86PreviousChapter", SDLK_LEFT },
    { "XF86RaiseChannel", SDLK_s }, { "XF86Blue", SDLK_s },
    { "XF86Red", SDLK_F9 }, { "XF86Green", SDLK_F9 },
    { "Minus", SDLK_MINUS },
  };
  SDL_Event e;
  SDL_Keycode k = SDLK_UNKNOWN;
  size_t i;
  if (!nome) return;
  for (i = 0; i < sizeof T / sizeof T[0]; i++)
    if (!strcmp(nome, T[i].n)) { k = T[i].k; break; }
  if (k == SDLK_UNKNOWN && nome[0] >= '0' && nome[0] <= '9' && !nome[1]) k = (SDL_Keycode)nome[0];
  if (k == SDLK_UNKNOWN) {
    printf("[tecla] tpk sem mapa: %s\n", nome);
    fflush(stdout);
    return;
  }
  SDL_zero(e);
  e.type = apertou ? SDL_KEYDOWN : SDL_KEYUP;
  e.key.state = apertou ? SDL_PRESSED : SDL_RELEASED;
  e.key.keysym.sym = k;
  e.key.keysym.scancode = SDL_GetScancodeFromKey(k);
  SDL_PushEvent(&e);
}
#endif
