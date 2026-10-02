// #216: dedo real, sem SDL host, video ou dispositivo. Eventos SDL normalizados.
#define main ponteiro_roteiro_mouse
#include "ponteiro.c"
#undef main
#include <math.h>

static void dedo(Uint32 tipo, Sint64 id, float x, float y) {
  SDL_Event e; SDL_zero(e);
  e.type = tipo; e.tfinger.touchId = 3; e.tfinger.fingerId = id;
  e.tfinger.x = x; e.tfinger.y = y;
  CONFERE(ponteiro_evento(&e, entregar) == 1, "evento de dedo consumido");
}
static void tocar(float x, float y) {
  dedo(SDL_FINGERDOWN, 1, x / 1920.0f, y / 1080.0f);
  dedo(SDL_FINGERUP, 1, x / 1920.0f, y / 1080.0f);
}
static void preparar(void (*alvos)(void)) {
  ponteiro_iniciar(); ponteiro_teste_toque(1);
  quadro(alvos); zerar(); nFocar = nAtivar = 0;
}
static void canto(void) { ponteiro_alvo(1900, 1060, 20, 20, focar, NULL, 1, 1); }
int main(void) {
  ponteiro_teste_relogio(agora);
  // Janela 960x540 e drawable diferente nao mudam coords normalizadas SDL.
  ponteiro_teste_janela(960, 540);
  preparar(home);
  dedo(SDL_FINGERDOWN, 1, 150.0f / 1920.0f, 150.0f / 1080.0f);
  CONFERE(nFocar == 0 && nEntregues == 0, "DOWN nao foca nem abre");
  dedo(SDL_FINGERUP, 1, 150.0f / 1920.0f, 150.0f / 1080.0f);
  CONFERE(nFocar == 1 && focoA == 0 && focoB == 0 && nEntregues == 2,
          "UP confirma toque, foca e envia um OK");
  CONFERE(entregues[0].type == SDL_KEYDOWN && entregues[1].type == SDL_KEYUP &&
          entregues[0].key.keysym.sym == SDLK_RETURN,
          "um par RETURN completo");
  CONFERE(fabsf(ponteiro_x() - 150.0f) < 0.01f && fabsf(ponteiro_y() - 150.0f) < 0.01f,
          "coords normalizadas viram viewport logico, independente DPI");
  // SDL tambem envia mouse derivado do mesmo tap: todos sao consumidos.
  for (int i = 0; i < 4; i++) {
    SDL_Event e; SDL_zero(e);
    e.type = i == 0 ? SDL_MOUSEMOTION : i == 1 ? SDL_MOUSEBUTTONDOWN :
             i == 2 ? SDL_MOUSEBUTTONUP : SDL_MOUSEWHEEL;
    if (i == 0) e.motion.which = SDL_TOUCH_MOUSEID;
    else if (i == 3) { e.wheel.which = SDL_TOUCH_MOUSEID; e.wheel.y = 1; }
    else { e.button.which = SDL_TOUCH_MOUSEID; e.button.button = SDL_BUTTON_LEFT; }
    CONFERE(ponteiro_evento(&e, entregar) == 1, "mouse emulado de toque consumido");
  }
  CONFERE(nEntregues == 2 && nFocar == 1, "mouse emulado nao duplica acao");

  preparar(home);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  dedo(SDL_FINGERMOTION, 1, .30f, .14f);
  dedo(SDL_FINGERMOTION, 1, .08f, .14f); // retornar nao transforma swipe emtap
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues && !nFocar, "arrasto cancela mesmo retornando ao inicio");
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  dedo(SDL_FINGERDOWN, 2, .09f, .14f);
  dedo(SDL_FINGERUP, 2, .09f, .14f);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues && !nFocar, "multifinger cancela o gesto inteiro");
  tocar(150, 150);
  CONFERE(nEntregues == 2, "proximo gesto simples funciona apos multifinger");
  preparar(home);
  dedo(SDL_FINGERDOWN, 1, NAN, .14f);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  dedo(SDL_FINGERUP, 1, .08f, INFINITY);
  dedo(SDL_FINGERUP, 99, .08f, .14f);
  CONFERE(!nEntregues, "NaN infinito e UP sem DOWN nao ativam");
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  SDL_Event e; SDL_zero(e); e.type = SDL_WINDOWEVENT; e.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
  ponteiro_evento(&e, entregar);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues, "perda de foco cancela tap");
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  e.type = SDL_APP_WILLENTERBACKGROUND; ponteiro_evento(&e, entregar);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues, "background cancela tap");
  preparar(home);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  quadro(homeComFolha);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues && !nAtivar, "modal aberta entre DOWN e UP cancela alvo antigo");
  preparar(homeComFolha);
  tocar(1450, 500);
  CONFERE(!nEntregues && !nAtivar, "anteparo da folha absorve tap no vazio");
  tocar(1550, 150);
  CONFERE(nEntregues == 2 && focoA == 7 && focoB == 0, "alvo da camada superior recebe tap");
  zerar(); tocar(150, 500);
  CONFERE(nAtivar == 1 && ativA == 99 && !nEntregues, "acao propria so na soltura");
  preparar(canto);
  dedo(SDL_FINGERDOWN, 1, 1.01f, 1.01f); dedo(SDL_FINGERUP, 1, 1.01f, 1.01f);
  CONFERE(nEntregues == 2 && ponteiro_x() == 1919.0f && ponteiro_y() == 1079.0f,
          "bordas finitas limitadas ao viewport");
  printf("ponteiro toque: %s\n", falhas ? "FALHOU" : "PASS");
  return falhas ? 1 : 0;
}
