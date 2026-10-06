// SEGURAR OK NUMA LISTA DE TITULOS = O MENU DO CARTAZ (dono, 06/10/2026: "nao
// tem o menu contextual quando abre uma lista"). Cobre o caminho real do app:
//
//   1. o protocolo do gesto (ctxhold_*), sem tela: toque, segurar, repeticao,
//      outra tecla cancela, soltar sem armar nao e clique, celula que nao
//      aceita o menu segue o caminho de antes;
//   2. "Ver tudo" de um catalogo (vertudo.c, grade vinda de um addon falso
//      local): toque abre a pagina, segurar abre o menu no limiar com o dedo
//      ainda no botao, Voltar fecha e o FOCO fica no mesmo cartao;
//   3. pagina de colecao (vertudo_colecao): o mesmo;
//   4. resultados da Busca (busca.c incluido: o painel de resultados e estatico).
//
// Capturas PNG (menu aberto sobre cada lista) na pasta pedida.
//
//   bash tests/ctxlista.sh [pasta-das-capturas]
#include "../src/busca.c"
#include "ajustes.h"
#include "catalogo.h"
#include "colecoes.h"
#include "ctxlista.h"
#include "ctxmenu.h"
#include "dados.h"
#include "descoberta.h"
#include "vertudo.h"
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <unistd.h>

static int t_falhas;
static void confere(const char *o_que, int ok) {
  printf("  %-72s %s\n", o_que, ok ? "ok" : "FALHOU");
  if (!ok) t_falhas++;
}

static SDL_Event tecla(Uint32 tipo, SDL_Keycode k, int repete) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = tipo;
  e.key.keysym.sym = k;
  e.key.repeat = (Uint8)repete;
  e.key.state = tipo == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
  return e;
}

// ---------------------------------------------------------------- 1. protocolo
static void protocolo(void) {
  CtxHold h;
  SDL_Event d = tecla(SDL_KEYDOWN, SDLK_RETURN, 0), u = tecla(SDL_KEYUP, SDLK_RETURN, 0);
  SDL_Event rep = tecla(SDL_KEYDOWN, SDLK_RETURN, 1), seta = tecla(SDL_KEYDOWN, SDLK_RIGHT, 0);
  printf("\nprotocolo do gesto:\n");
  memset(&h, 0, sizeof h);
  confere("KEYUP sem armar nao e clique", ctxhold_evento(&h, &u, 1) == CTXH_NADA);
  confere("KEYDOWN numa celula que aceita arma e consome", ctxhold_evento(&h, &d, 1) == CTXH_CONSUMIDO && h.armado);
  confere("soltar logo e TOQUE (e desarma)", ctxhold_evento(&h, &u, 1) == CTXH_TOQUE && !h.armado);
  confere("celula que nao aceita: KEYDOWN segue o caminho de antes", ctxhold_evento(&h, &d, 0) == CTXH_NADA && !h.armado);
  ctxhold_evento(&h, &d, 1);
  confere("a repeticao do controle nao rearma nem vira toque", ctxhold_evento(&h, &rep, 1) == CTXH_CONSUMIDO);
  ctxhold_evento(&h, &seta, 1);
  confere("outra tecla cancela o gesto", !h.armado && ctxhold_evento(&h, &u, 1) == CTXH_NADA);
  ctxhold_evento(&h, &d, 1);
  confere("antes do limiar o passo nao dispara", ctxhold_passo(&h, h.desde + NV_HOLD_MS - 10, 0) == 0);
  confere("no limiar dispara UMA vez", ctxhold_passo(&h, h.desde + NV_HOLD_MS, 0) == 1 &&
                                        ctxhold_passo(&h, h.desde + NV_HOLD_MS + 50, 0) == 0);
  confere("menu que nao abriu: soltar depois do limiar ainda e toque", ctxhold_evento(&h, &u, 1) == CTXH_TOQUE);
  confere("progresso vai de 0 a 1", ctxhold_progresso(&h, 0) == 0.0f);
}

// -------------------------------------------------------------------- tela/GL
static const char *t_saida;
static int t_tela;   // 0 vertudo, 1 busca
static void quadro(void) {
  SDL_Event e;
  Uint32 agora = SDL_GetTicks();
  while (SDL_PollEvent(&e)) {
    // O roteador de app.c: o menu e modal e vem antes de qualquer tela.
    if (ctx_aberto()) ctx_evento(&e);
    else if (t_tela == 0) vertudo_evento(&e);
    else busca_evento(&e);
  }
  tex_bombear(3);
  gfx_novo_quadro();
  tex_novo_quadro();
  gfx_sem_recorte();
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  txt_novo_quadro();
  if (t_tela == 0) { vertudo_atualizar(1.0f / 60.0f, agora); vertudo_desenhar(agora); }
  else { busca_atualizar(1.0f / 60.0f, agora); busca_desenhar(agora); }
  ctx_atualizar(1.0f / 60.0f, agora);
  ctx_desenhar(agora);
}
static void quadros(SDL_Window *w, int n) {
  int i;
  for (i = 0; i < n; i++) { quadro(); SDL_GL_SwapWindow(w); }
}
static void durante(SDL_Window *w, Uint32 ms) {
  Uint32 t0 = SDL_GetTicks();
  while (SDL_GetTicks() - t0 < ms) { quadro(); SDL_GL_SwapWindow(w); SDL_Delay(8); }
}
static void empurrar(Uint32 tipo, SDL_Keycode k) {
  SDL_Event e = tecla(tipo, k, 0);
  SDL_PushEvent(&e);
}
static void toque(SDL_Window *w, SDL_Keycode k) {
  empurrar(SDL_KEYDOWN, k); quadros(w, 2);
  empurrar(SDL_KEYUP, k);   quadros(w, 2);
}
static void captura(const char *nome) {
  char caminho[700];
  unsigned char *pix = malloc(1920 * 1080 * 4);
  SDL_Surface *s;
  int y;
  assert(pix);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  assert(s);
  for (y = 0; y < 1080; y++)
    memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  snprintf(caminho, sizeof caminho, "%s/%s", t_saida, nome);
  assert(IMG_SavePNG(s, caminho) == 0);
  SDL_FreeSurface(s);
  free(pix);
  printf("  captura: %s\n", caminho);
}
static void capturaAssentada(SDL_Window *w, const char *nome, int n) {
  int i;
  for (i = 0; i < n; i++) {
    quadro();
    if (i == n - 1) captura(nome);
    SDL_GL_SwapWindow(w);
  }
}

// O gesto inteiro numa lista: segura, confere a dica, o menu no limiar, o OK
// ainda afundado nao escolhe, e Voltar devolve o foco ao mesmo cartao.
static void segurarEConferir(SDL_Window *w, const char *rotulo, const char *png,
                             int (*aberta)(void), int (*foco)(void)) {
  int f0 = foco();
  char msg[160];
  empurrar(SDL_KEYDOWN, SDLK_RETURN);
  durante(w, NV_HOLD_MS / 2);
  snprintf(msg, sizeof msg, "%s: no meio do gesto o menu ainda nao abriu", rotulo);
  confere(msg, !ctx_aberto());
  durante(w, NV_HOLD_MS / 2 + 150);
  snprintf(msg, sizeof msg, "%s: no limiar, com o dedo no botao, o menu abre", rotulo);
  confere(msg, ctx_aberto());
  { SDL_Event rep = tecla(SDL_KEYDOWN, SDLK_RETURN, 1); SDL_PushEvent(&rep); }
  quadros(w, 3);
  snprintf(msg, sizeof msg, "%s: a repeticao automatica do OK nao escolhe opcao", rotulo);
  confere(msg, ctx_aberto());
  empurrar(SDL_KEYUP, SDLK_RETURN);
  capturaAssentada(w, png, 40);
  snprintf(msg, sizeof msg, "%s: soltar depois do limiar nao abre a pagina", rotulo);
  confere(msg, ctx_aberto());
  toque(w, SDLK_ESCAPE);
  quadros(w, 4);
  snprintf(msg, sizeof msg, "%s: Voltar fecha so o menu, a lista segue aberta", rotulo);
  confere(msg, !ctx_aberto() && aberta());
  snprintf(msg, sizeof msg, "%s: o foco volta ao MESMO cartao", rotulo);
  confere(msg, foco() == f0);
}

static int vtAberta(void) { return vertudo_aberta(); }
static int vtFoco(void) { return vertudo_foco(); }
static int buAberta(void) { return painel == 1; }
static int buFoco(void) { return focoRes.fileira * 1000 + focoRes.coluna; }

static ColFolder pasta;
static ColSource pastaFontes[COL_SOURCE_MAX];

static void esperarGrade(SDL_Window *w, int minimo) {
  int i;
  for (i = 0; i < 400 && desc_vertudo_n() < minimo; i++) { quadros(w, 1); SDL_Delay(10); }
}

#define NCAT 60
static CatItem itens[NCAT];
static CatFileira t_fil;

int main(int argc, char **argv) {
  const char *dir = getenv("NUVIO_DADOS");
  const char *base = argc > 2 ? argv[2] : "http://127.0.0.1:8771";
  SDL_Window *w;
  SDL_GLContext gl;
  GLuint fbo, fboTex;
  int i, idx;
  t_saida = argc > 1 ? argv[1] : "/tmp";
  if (!dir || !dir[0]) { printf("NUVIO_DADOS ausente; recusando\n"); return 2; }
  dados_iniciar(dir);
  if (strcmp(dados_dir(), dir)) { printf("dados_dir() != NUVIO_DADOS; recusando\n"); return 2; }
  ajustes_dir(dir);

  protocolo();

  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  w = SDL_CreateWindow("Nuvio: segurar OK em listas", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(w);
  gl = SDL_GL_CreateContext(w);
  assert(gl);
  glGenTextures(1, &fboTex);
  glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(192);
  gfx_icones_dir("deploy/app/art");
  gfx_snap_iniciar(1920, 1080);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // Catalogo da home (a Busca varre ele; "Ver tudo" acrescenta o que abrir).
  memset(itens, 0, sizeof itens);
  snprintf(t_fil.chave, sizeof t_fil.chave, "teste_movie_top");
  snprintf(t_fil.titulo, sizeof t_fil.titulo, "Em alta");
  snprintf(t_fil.tipo, sizeof t_fil.tipo, "movie");
  t_fil.ini = 0; t_fil.n = NCAT;
  for (i = 0; i < NCAT; i++) {
    CatItem *c = &itens[i];
    snprintf(c->imdb, sizeof c->imdb, "tt%07d", 4000000 + i);
    snprintf(c->tipo, sizeof c->tipo, "movie");
    snprintf(c->titulo, sizeof c->titulo, "Titulo de teste %d", i);
    snprintf(c->poster, sizeof c->poster, "deploy/app/art/%02d.jpg", i % 40);
    snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", (i + 7) % 40);
  }
  cat_definir_tudo(itens, NCAT, &t_fil, 1);

  // ---------------------------------------------------------- 2. Ver tudo
  printf("\nVer tudo (catalogo):\n");
  t_tela = 0;
  vertudo_abrir(base, "movie", "top", "Em alta");
  esperarGrade(w, 12);
  confere("a grade do addon falso chegou", desc_vertudo_n() >= 12);
  durante(w, 600);
  toque(w, SDLK_RIGHT); toque(w, SDLK_DOWN);   // um cartao do meio da grade
  // toque curto
  empurrar(SDL_KEYDOWN, SDLK_RETURN); quadros(w, 2);
  confere("KEYDOWN sozinho nao abre nada", !ctx_aberto() && vertudo_pediu_abrir() < 0);
  empurrar(SDL_KEYUP, SDLK_RETURN); quadros(w, 2);
  idx = vertudo_pediu_abrir();
  confere("toque curto abre a pagina do titulo (na soltura)", idx >= 0 && !ctx_aberto());
  segurarEConferir(w, "catalogo", "lista-catalogo-menu.png", vtAberta, vtFoco);
  confere("depois do menu a soltura nao vira clique", vertudo_pediu_abrir() < 0);
  // "Ver detalhes" (primeira opcao) abre a pagina do titulo.
  empurrar(SDL_KEYDOWN, SDLK_RETURN); durante(w, NV_HOLD_MS + 150); empurrar(SDL_KEYUP, SDLK_RETURN); quadros(w, 4);
  toque(w, SDLK_RETURN); quadros(w, 3);
  idx = ctx_pediu_detalhes();
  confere("\"Ver detalhes\" entrega o titulo da grade a quem abre a pagina", idx >= 0 && !ctx_aberto());
  vertudo_fechar_seco();
  quadros(w, 3);

  // ------------------------------------------------------------ 3. Colecao
  printf("\nPagina de colecao:\n");
  memset(&pasta, 0, sizeof pasta);
  pasta.sources = pastaFontes;
  snprintf(pasta.title, sizeof pasta.title, "Cinema de teste");
  snprintf(pasta.group, sizeof pasta.group, "Themes");
  snprintf(pastaFontes[0].title, sizeof pastaFontes[0].title, "Filmes");
  snprintf(pastaFontes[0].type, sizeof pastaFontes[0].type, "movie");
  snprintf(pastaFontes[0].base, sizeof pastaFontes[0].base, "%s", base);
  snprintf(pastaFontes[0].catId, sizeof pastaFontes[0].catId, "top");
  pasta.nSources = 1;
  vertudo_colecao(&pasta);
  esperarGrade(w, 12);
  confere("a colecao carregou", desc_vertudo_n() >= 12);
  durante(w, 600);
  toque(w, SDLK_DOWN);                         // das abas para a grade
  toque(w, SDLK_RIGHT);
  segurarEConferir(w, "colecao", "lista-colecao-menu.png", vtAberta, vtFoco);
  vertudo_fechar_seco();
  quadros(w, 3);

  // -------------------------------------------------------------- 4. Busca
  printf("\nResultados da Busca:\n");
  t_tela = 1;
  busca_iniciar();
  quadros(w, 10);
  painel = 1;                                   // o foco nos resultados (Populares)
  quadros(w, 30);
  toque(w, SDLK_RIGHT);
  confere("ha um resultado em foco", temItemFoco);
  segurarEConferir(w, "busca", "lista-busca-menu.png", buAberta, buFoco);
  confere("a Busca nao pediu o titulo", busca_pediu_abrir(NULL) == 0);
  empurrar(SDL_KEYDOWN, SDLK_RETURN); quadros(w, 2);
  empurrar(SDL_KEYUP, SDLK_RETURN); quadros(w, 2);
  confere("toque curto num resultado ainda abre o titulo", busca_pediu_abrir(&idx) && idx >= 0 && !ctx_aberto());

  printf("\n%s\n", t_falhas ? "FALHOU" : "PASSOU");
  return t_falhas ? 1 : 0;
}
