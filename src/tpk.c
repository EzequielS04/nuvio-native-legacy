// Ponte do alvo .tpk (Samsung Tizen 6+). Ver o porque em tpk.h.
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
#include <EGL/egl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv);

enum { VEZ_NUI = 0, VEZ_APP = 1 };
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  mudou = PTHREAD_COND_INITIALIZER;
static int vez = VEZ_NUI, iniciado, terminou;
static EGLDisplay dpy = EGL_NO_DISPLAY;
static EGLSurface sup = EGL_NO_SURFACE;
static EGLContext ctx = EGL_NO_CONTEXT;
static int telaW = 1920, telaH = 1080;
static char dirArte[512];

static void esperarVez(int quem) {
  pthread_mutex_lock(&trava);
  while (vez != quem && !(quem == VEZ_NUI && terminou)) pthread_cond_wait(&mudou, &trava);
  pthread_mutex_unlock(&trava);
}

static void passarVez(int quem) {
  pthread_mutex_lock(&trava);
  vez = quem;
  pthread_cond_broadcast(&mudou);
  pthread_mutex_unlock(&trava);
}

static void *fioApp(void *arg) {
  char *argv[] = { "nuvio", dirArte, NULL };
  int r;
  (void)arg;
  r = main(2, argv);
  printf("[tpk] main devolveu %d\n", r);
  fflush(stdout);
  eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  pthread_mutex_lock(&trava);
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
  if (w > 0 && h > 0) { telaW = w; telaH = h; }
  snprintf(dirArte, sizeof dirArte, "%s", arte);
  snprintf(log, sizeof log, "%s/nuvio.log", dados);
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

// Fio de desenho do NUI, contexto do GLWindow corrente. 0 = o app acabou e o
// host deve fechar.
__attribute__((visibility("default")))
int nv_tpk_quadro(void) {
  if (terminou) return 0;
  if (ctx == EGL_NO_CONTEXT) {
    dpy = eglGetCurrentDisplay();
    sup = eglGetCurrentSurface(EGL_DRAW);
    ctx = eglGetCurrentContext();
    printf("[tpk] contexto do GLWindow: dpy=%p sup=%p ctx=%p\n", dpy, sup, ctx);
    fflush(stdout);
  }
  eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  passarVez(VEZ_APP);
  esperarVez(VEZ_NUI);
  if (terminou) return 0;
  eglMakeCurrent(dpy, sup, sup, ctx);
  return 1;
}

void *tpk_gl_criar(void) {
  esperarVez(VEZ_APP);
  if (!eglMakeCurrent(dpy, sup, sup, ctx))
    printf("[tpk] eglMakeCurrent no fio do app falhou: 0x%x\n", eglGetError());
  return (void *)1;
}

void tpk_gl_trocar(void) {
  eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  passarVez(VEZ_NUI);
  esperarVez(VEZ_APP);
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
  static const struct { const char *n; SDL_Keycode k; } T[] = {
    { "Up", SDLK_UP }, { "Down", SDLK_DOWN }, { "Left", SDLK_LEFT }, { "Right", SDLK_RIGHT },
    { "Return", SDLK_RETURN }, { "KP_Enter", SDLK_RETURN }, { "Select", SDLK_RETURN },
    { "XF86Back", SDLK_AC_BACK }, { "Escape", SDLK_AC_BACK }, { "BackSpace", SDLK_BACKSPACE },
    { "XF86AudioPlay", SDLK_AUDIOPLAY }, { "XF86AudioPause", SDLK_AUDIOPLAY },
    { "XF86AudioPlayPause", SDLK_AUDIOPLAY }, { "XF86AudioStop", SDLK_AUDIOSTOP },
    { "XF86AudioRewind", SDLK_AUDIOPREV }, { "XF86AudioForward", SDLK_AUDIONEXT },
    { "XF86AudioNext", SDLK_AUDIONEXT }, { "XF86AudioPrev", SDLK_AUDIOPREV },
    { "XF86ChannelUp", SDLK_PAGEUP }, { "XF86ChannelDown", SDLK_PAGEDOWN },
    { "XF86Red", SDLK_F1 }, { "XF86Green", SDLK_F2 }, { "XF86Yellow", SDLK_F3 }, { "XF86Blue", SDLK_F4 },
    { "XF86Info", SDLK_i }, { "XF86Menu", SDLK_MENU },
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
