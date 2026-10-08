// PONTEIRO NAS TELAS (relato do dono: "LG pointer is not working in all
// components and page"). tests/ponteiro.c prova o modulo sozinho; este prova
// que as TELAS registram alvos e que passar por cima foca e clicar ativa —
// com eventos SDL de mouse de verdade passando por ponteiro_evento, como o
// main.c faz no aparelho.
//
// Telas cobertas aqui (as que tem fixture sem rede): Biblioteca e Ajustes
// (indice e lista da categoria). Explorar e "Ver tudo" dependem de mapa/TMDB e
// do descoberta em rede; ficam fora (ver o relato do agente).
//
//   bash tests/ponteiro_telas.sh [pasta-das-capturas]
//
// Com uma pasta, salva PNGs dos estados com o cursor em cima (NUVIO_SHOT_FONTE
// escolhe a fonte; 3 = Montserrat, a da TV do dono).
#include "biblioteca.h"
#include "ajustes.h"
#include "ponteiro.h"
#include "catalogo.h"
#include "dados.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "extras.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *janela;
static const char *pasta;
static int falhas;
#define CONFERE(c, msg) do { if (c) printf("ok   %s\n", msg); \
  else { printf("FALHA: %s (linha %d)\n", msg, __LINE__); falhas++; } } while (0)

static Uint32 relogio = 100000;
static Uint32 agora(void) { return relogio; }

enum { TELA_BIB, TELA_AJ };
static int tela;
static void entregar(const SDL_Event *e) {
  if (tela == TELA_BIB) biblioteca_evento(e); else ajustes_evento(e);
}
static void quadros(int n) {
  for (int i = 0; i < n; i++) {
    relogio += 16;
    SDL_PumpEvents();
    txt_novo_quadro(); tex_novo_quadro(); tex_bombear(6);
    if (tela == TELA_BIB) biblioteca_atualizar(1.0f / 60.0f, SDL_GetTicks());
    else ajustes_atualizar(1.0f / 60.0f, SDL_GetTicks());
    glClearColor(0.025f, 0.025f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ponteiro_quadro(relogio);
    if (tela == TELA_BIB) biblioteca_desenhar(SDL_GetTicks()); else ajustes_desenhar(SDL_GetTicks());
    ponteiro_desenhar();
    SDL_GL_SwapWindow(janela);
  }
}
static void captura(const char *nome) {
  char cam[800];
  unsigned char *pix;
  SDL_Surface *s;
  if (!pasta) return;
  quadros(1);
  // O quadro de cima ja foi trocado; redesenha sem trocar para ler.
  glClear(GL_COLOR_BUFFER_BIT);
  ponteiro_quadro(relogio);
  if (tela == TELA_BIB) biblioteca_desenhar(SDL_GetTicks()); else ajustes_desenhar(SDL_GetTicks());
  ponteiro_desenhar();
  pix = malloc(1920 * 1080 * 4);
  assert(pix);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  for (int y = 0; y < 1080; y++)
    memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  snprintf(cam, sizeof cam, "%s/%s", pasta, nome);
  IMG_SavePNG(s, cam);
  SDL_FreeSurface(s);
  free(pix);
  printf("captura: %s\n", cam);
}

static void mover(float x, float y) {
  SDL_Event e; SDL_zero(e);
  e.type = SDL_MOUSEMOTION; e.motion.x = (int)x; e.motion.y = (int)y;
  relogio += 400;   // depois do freio do conteudo que anda (PONT_ASSENTA_MS)
  ponteiro_evento(&e, entregar);
}
static void clicar(float x, float y) {
  SDL_Event e; SDL_zero(e);
  e.type = SDL_MOUSEBUTTONDOWN; e.button.button = SDL_BUTTON_LEFT;
  e.button.x = (int)x; e.button.y = (int)y;
  relogio += 400;
  ponteiro_evento(&e, entregar);
  e.type = SDL_MOUSEBUTTONUP;
  relogio += 60;
  ponteiro_evento(&e, entregar);
}
static void okTecla(void) {
  SDL_Event e; SDL_zero(e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN; relogio += 400;
  if (!ponteiro_evento(&e, entregar)) entregar(&e);
  e.type = SDL_KEYUP;
  if (!ponteiro_evento(&e, entregar)) entregar(&e);
}
// O centro do alvo i da lista que o hit-test le.
static int alvos(const PonteiroAlvo **v) { return ponteiro_teste_lista(v); }
static float cx(const PonteiroAlvo *a) { return a->x + a->w * 0.5f; }
static float cy(const PonteiroAlvo *a) { return a->y + a->h * 0.5f; }

static const char *NOMES[] = {
  "CODA", "Aftersun", "Dune: Part Two", "The Brutalist", "Poor Things", "The Substance",
  "Anatomy of a Fall", "Isle of Dogs", "Civil War", "Bacurau", "Cidade de Deus", "Noir",
};
#define NN (int)(sizeof NOMES / sizeof *NOMES)
static void povoar(void) {
  static CatItem v[NN];
  for (int i = 0; i < NN; i++) {
    CatItem *it = &v[i];
    memset(it, 0, sizeof *it);
    snprintf(it->titulo, sizeof it->titulo, "%s", NOMES[i]);
    snprintf(it->imdb, sizeof it->imdb, "tt900%02d", i);
    snprintf(it->tipo, sizeof it->tipo, "%s", i % 3 ? "movie" : "series");
    snprintf(it->genero, sizeof it->genero, "Movie · Drama");
    snprintf(it->poster, sizeof it->poster, "deploy/app/art/%02d.jpg", i);
    it->naLista = 1;
    it->naColecao = 1;
  }
  cat_definir(v, NN);
}

static void testeBiblioteca(void) {
  const PonteiroAlvo *v;
  int n, a = -1, b = -1, idxA = -1, idxB = -1, idx = -1;
  printf("\n== Biblioteca\n");
  tela = TELA_BIB;
  povoar();
  biblioteca_iniciar();
  mover(5, 5);           // o cursor aparece
  quadros(70);
  n = alvos(&v);
  CONFERE(n > 0, "biblioteca registra alvos");
  // Duas celulas da grade: alvos abaixo da faixa de filtros, em colunas
  // diferentes.
  for (int i = 0; i < n; i++) {
    if (v[i].y < 330.0f) continue;
    if (a < 0) a = i;
    else if (b < 0 && v[i].y == v[a].y && v[i].x > v[a].x + 50.0f) b = i;
  }
  CONFERE(a >= 0 && b >= 0, "biblioteca: celulas da grade sao alvos");
  if (a < 0 || b < 0) return;
  { float ax = cx(&v[a]), ay = cy(&v[a]), bx = cx(&v[b]), by = cy(&v[b]);
    clicar(ax, ay); quadros(2);
    CONFERE(biblioteca_pediu_abrir(&idxA) && idxA >= 0, "biblioteca: clique na celula A abre o titulo");
    clicar(bx, by); quadros(2);
    CONFERE(biblioteca_pediu_abrir(&idxB) && idxB >= 0 && idxB != idxA,
            "biblioteca: clique na celula B abre OUTRO titulo");
    // Hover sozinho poe o foco: o OK do controle depois abre a celula de baixo
    // do cursor.
    mover(ax, ay); quadros(2);
    captura("biblioteca-hover-celula.png");
    okTecla(); quadros(2);
    CONFERE(biblioteca_pediu_abrir(&idx) && idx == idxA, "biblioteca: passar por cima foca (OK abre a celula A)"); }
  // A faixa de cima (abas/seletores) tambem e alvo.
  n = alvos(&v);
  { int topo = 0;
    for (int i = 0; i < n; i++) if (v[i].y < 330.0f) topo++;
    CONFERE(topo >= 3, "biblioteca: abas e seletores sao alvos"); }
  biblioteca_encerrar();
}

static void testeAjustes(void) {
  const PonteiroAlvo *v;
  int n, k = -1, antes, depois;
  printf("\n== Ajustes\n");
  tela = TELA_AJ;
  ajustes_iniciar();
  mover(5, 5);
  quadros(90);
  n = alvos(&v);
  CONFERE(n > 0, "ajustes (indice) registra alvos");
  CONFERE(ajustes_foco_no_indice(), "ajustes abre no indice");
  // A categoria mais baixa do indice (os cartoes moram embaixo das pilulas).
  for (int i = 0; i < n; i++) if (k < 0 || v[i].y > v[k].y) k = i;
  if (k < 0) return;
  { float x = cx(&v[k]), y = cy(&v[k]);
    mover(x, y); quadros(4);
    captura("ajustes-hover-categoria.png");
    clicar(x, y); quadros(90); }
  CONFERE(!ajustes_foco_no_indice(), "ajustes: clique na categoria entra nela");
  n = alvos(&v);
  CONFERE(n > 1, "ajustes (lista da categoria) registra alvos");
  antes = ajustes_opcao_em_foco();
  // A linha mais baixa da lista: nao e a que abriu com o foco.
  k = -1;
  for (int i = 0; i < n; i++) if (k < 0 || v[i].y > v[k].y) k = i;
  if (k < 0) return;
  mover(cx(&v[k]), cy(&v[k])); quadros(20);
  depois = ajustes_opcao_em_foco();
  CONFERE(depois != antes && !ajustes_foco_no_indice(), "ajustes: passar por cima da linha foca ela");
  captura("ajustes-hover-linha.png");
}

int main(int argc, char **argv) {
  const char *quero = getenv("NUVIO_DADOS");
  SDL_GLContext gl;
  pasta = argc > 1 ? argv[1] : NULL;
  if (!quero || !*quero) { printf("RECUSADO: rode por tests/ponteiro_telas.sh\n"); return 2; }
  dados_iniciar(quero);
  if (strcmp(dados_dir(), quero)) { printf("RECUSADO: dados_dir() nao e a pasta do teste\n"); return 2; }
  { const char *f = getenv("NUVIO_SHOT_FONTE");
    char cam[700]; FILE *fa;
    snprintf(cam, sizeof cam, "%s/ajustes.txt", dados_dir());
    fa = fopen(cam, "w");
    if (fa) { fprintf(fa, "idioma 0\n"); if (f && *f) fprintf(fa, "fonteInterface %d\n", atoi(f)); fclose(fa); } }
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Nuvio: ponteiro nas telas", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(janela);
  gl = SDL_GL_CreateContext(janela);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  extras_carregar("deploy/app/art");
  ajustes_iniciar();
  ajustes_dir(dados_dir());
  // Relogio e janela injetados: o hit-test converte 1920x1080 -> logico 1:1 e
  // os freios de tempo andam pelo relogio do teste.
  ponteiro_teste_relogio(agora);
  ponteiro_teste_janela(1920, 1080);
  ponteiro_teste_toque(0);

  testeBiblioteca();
  testeAjustes();

  printf("\n%s: %d falha(s)\n", falhas ? "FALHOU" : "PASSOU", falhas);
  return falhas ? 1 : 0;
}
