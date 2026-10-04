// Regression: unknown logo language and fullscreen carousel edge navigation.
#define desc_pedir_titulo_tmdb capturar_rota_tmdb
#define extras_relacionado_imdb relacionado_tmdb_fixture
#include "../src/detail.c"
#undef desc_pedir_titulo_tmdb
#undef extras_relacionado_imdb
#include <assert.h>
static long rotaId;
static char rotaTipo[16];
void capturar_rota_tmdb(long id, const char *tipo) {
  rotaId = id;
  snprintf(rotaTipo, sizeof rotaTipo, "%s", tipo);
}
const char *relacionado_tmdb_fixture(int i) {
  return i == 0 ? "tmdb:42" : "";
}
int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  // Detail geometry is the published1.7.4 layout; global Glass remains.
  assert(NV_DETP_G_TEMP == 1080 && NV_DETP_TEMP_Y == 1160);
  assert(NV_DETP_G_EP == 1194 && NV_DETP_EP_Y == 1286);
  assert(NV_DETP_EP_W == 640 && NV_DETP_EP_H == 414);
  assert(CAR_TEXTO_SOBE == 64);
  assert(ORDEM_FILME[SEC_ELENCO] == SEC_ELENCO);
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
  assert(mostrarNomeLogo(&c, u, 0, "pt-BR")); // Known foreign art does not delay the name.
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
  assert(maisAcoes && acaoEm(botao) == ACAO_LISTA && nBotoes() > 2);
  assert(acaoEm(0) == ACAO_PRIMARIO);
  pedMarcar = 0;
  SDL_Delay(2);
  e.key.keysym.sym = SDLK_RETURN; e.type = SDL_KEYDOWN; detail_evento(&e);
  e.type = SDL_KEYUP; detail_evento(&e); assert(pedMarcar);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = SDLK_RIGHT; detail_evento(&e);
  assert(acaoEm(botao) == ACAO_ASSISTIDO);
  e.key.keysym.sym = SDLK_LEFT; detail_evento(&e); detail_evento(&e);
  assert(!maisAcoes && botao == 0 && nBotoes() == 2);
  e.key.keysym.sym = SDLK_RIGHT; detail_evento(&e);
  e.key.keysym.sym = SDLK_ESCAPE; detail_evento(&e);
  assert(!maisAcoes && !saindo && botao == 0);
  carro=0; maisAcoes=0; nivel=0; botao=0;
  assert(ajustes_home_layout()==HOME_LAYOUT_MODERNA);
  assert(acoesAgrupadas() && nBotoes()==2);
  e.type=SDL_KEYDOWN; e.key.keysym.sym=SDLK_RIGHT;
  detail_evento(&e); assert(maisAcoes && acaoEm(botao)==ACAO_LISTA);
  e.key.keysym.sym=SDLK_ESCAPE; detail_evento(&e);
  assert(!maisAcoes && !saindo);
  carro=0; pessoaAberta=1; saindo=0;
  e.type=SDL_KEYDOWN; e.key.keysym.sym=SDLK_ESCAPE;
  detail_evento(&e); assert(!pessoaAberta && !saindo);
  pessoaAberta=1; e.key.keysym.sym=SDLK_AC_BACK;
  detail_evento(&e); assert(!pessoaAberta && !saindo);
  pessoaAberta=1; e.key.keysym.sym=SDLK_BACKSPACE;
  detail_evento(&e); assert(!pessoaAberta && !saindo);
  // Dedicated recommendations are focusable for both media kinds. Route
  // opaque TMDB ids using the current media kind, without performing I/O.
  for (int serie = 0; serie < 2; serie++) {
    snprintf(c.tipo, sizeof c.tipo, "%s", serie ? "series" : "movie");
    cat_definir_tudo(&c, 1, NULL, 0); idx = 0;
    carro = 0; nivel = 1; foco.fileira = SEC_RELACIONADOS; foco.coluna = 0;
    rotaId = 0; rotaTipo[0] = 0;
    e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN;
    detail_evento(&e);
    e.type = SDL_KEYUP; detail_evento(&e);
    assert(rotaId == 42);
    assert(!strcmp(rotaTipo, serie ? "tv" : "movie"));
  }
  puts("PASS: title/navigation and focus-only action group");
  return 0;
}
