// Regression: unknown logo language and fullscreen carousel edge navigation.
#include "../src/detail.c"
#include <assert.h>
int main(void) {
  CatItem c = {0};
  const char *u = "https://image.tmdb.org/t/p/w500/image.png";
  assert(!mostrarNomeLogo(&c, u, 1, "pt-BR"));
  assert(!mostrarNomeLogo(&c, u, 0, "pt-BR"));
  assert(mostrarNomeLogo(&c, NULL, 0, "pt-BR"));
  snprintf(c.logoIdiomaUrl, sizeof c.logoIdiomaUrl, "%s", u);
  snprintf(c.logoIdioma, sizeof c.logoIdioma, "pt");
  assert(!mostrarNomeLogo(&c, u, 1, "pt-BR"));
  snprintf(c.logoIdioma, sizeof c.logoIdioma, "en");
  assert(mostrarNomeLogo(&c, u, 1, "pt-BR"));
  assert(mostrarNomeLogo(&c, "https://image.tmdb.org/t/p/w300/image.png", 1, "pt-BR"));
  assert(!mostrarNomeLogo(&c, "https://other.example/image.png", 1, "pt-BR"));
  assert(!mostrarNomeLogo(&c, "https://image.tmdb.org/t/p/w500/other.png", 1, "pt-BR"));
  snprintf(c.logoIdioma, sizeof c.logoIdioma, "und");
  assert(!mostrarNomeLogo(&c, u, 1, "pt-BR"));
  snprintf(c.tipo, sizeof c.tipo, "movie");
  cat_definir_tudo(&c, 1, NULL, 0); idx = 0;
  carro = 1; carCheia = 1; carN = 3; carPos = carAplicado = 1;
  nivel = 0; saindo = 0;
  SDL_Event e = {0}; e.type = SDL_KEYDOWN;
  botao = nBotoes() - 1; e.key.keysym.sym = SDLK_RIGHT;
  detail_evento(&e); assert(carPos == 1 && !saindo);
  botao = 0; e.key.keysym.sym = SDLK_LEFT;
  detail_evento(&e); assert(carPos == 1 && !saindo);
  carPasso(1); assert(carPos == 1);
  carCheia = 0;
  botao = nBotoes() - 1; e.key.keysym.sym = SDLK_RIGHT;
  detail_evento(&e); assert(carPos == 2);
  carCheia = 1; carPos = 1; maisAcoes = 0; botao = 0;
  assert(nBotoes() == 2);
  e.key.keysym.sym = SDLK_RIGHT; detail_evento(&e);
  assert(maisAcoes && acaoEm(botao) == ACAO_MAIS && nBotoes() > 2);
  e.key.keysym.sym = SDLK_RIGHT; detail_evento(&e);
  assert(acaoEm(botao) == ACAO_LISTA);
  e.key.keysym.sym = SDLK_LEFT; detail_evento(&e); detail_evento(&e);
  assert(!maisAcoes && botao == 0 && nBotoes() == 2);
  e.key.keysym.sym = SDLK_RIGHT; detail_evento(&e);
  e.key.keysym.sym = SDLK_ESCAPE; detail_evento(&e);
  assert(!maisAcoes && !saindo && botao == 0);
  puts("PASS: title/navigation and focus-only action group");
  return 0;
}
