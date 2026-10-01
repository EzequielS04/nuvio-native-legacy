// MEDIDA DE CUSTO DE DESENHO DA HOME E DO DETALHE, no Mac, por cenario.
//
// Existe pela queixa do testador da 1.6 ("the 1.6 polishes make the app
// slower", Samsung 2022) e pelos registros de campo: a 1.6 pinta mais telas
// cheias por quadro que a 1.5.4 (webOS 3,5 -> 4,9 telas; [gpu-modos] fill).
// Aqui a conta e feita SEM TV: preenchimento submetido por modo de desenho
// (gfx_fill_modo), desenhos, trocas de programa, quantos desenhos de tela
// cheia com mistura, e o tempo de CPU de desenhar. A GPU do Mac nao e a Mali,
// entao o glFinish sai so como referencia; o que vale comparar entre dois
// commits e o preenchimento e a contagem, que sao deterministicos.
//
//   bash tests/fluidez_perf.sh                 # todos os cenarios
//   NV_CENARIO=imersiva bash tests/fluidez_perf.sh
//   PERF_BMP=/tmp/x bash tests/fluidez_perf.sh # guarda /tmp/x-<cenario>.bmp
//
// Cenarios (home parada e navegando com a seta, e o detalhe aberto):
//   moderna   layout Moderna, destaque em tela cheia, tema Branco, sem vidro
//   imersiva  Moderna + "Dinamica imersiva" + vidro (a C9 do dono, 30/09)
//   padrao    layout Padrao
//   dinamica  layout Dinamica
#include "ajustes.h"
#include "catalogo.h"
#include "corviva.h"
#include "dados.h"
#include "detail.h"
#include "gfx.h"
#include "home.h"
#include "layout.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gl_compat.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NFIL   8
#define PORFIL 24
#define NCAT   (NFIL * PORFIL)

static double perFreq;
static double ms(Uint64 a, Uint64 b) { return (double)(b - a) * 1000.0 / perFreq; }
static const char *dirDados;

typedef struct {
  double upd, des, gpu, fill, fillVis;
  int rects, progs, binds, cheios, cheiosMist, txtRast;
  double modo[GFX_NMODOS];
} Quadro;

// Desenhos de tela cheia COM mistura: e a conta que a Mali paga a mais (uma
// leitura da tela por pixel). Medido pelo gfx_rect via gancho de contagem.
// -DNV_PERF_BASE: arvore antiga, sem o contador nem a luz pendente.
#ifdef NV_PERF_BASE
static int gfx_n_cheio_mistura = -1;
#define gfx_ambiente_descarregar() ((void)0)
#else
extern int gfx_n_cheio_mistura;
#endif

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  if (detail_aberto()) detail_evento(&e); else home_evento(&e);
  e.type = SDL_KEYUP;
  if (detail_aberto()) detail_evento(&e); else home_evento(&e);
}

static void guardar(const char *nome) {
  unsigned char *pix = malloc(1920 * 1080 * 4);
  SDL_Surface *sf = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  int y;
  assert(pix && sf);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  for (y = 0; y < 1080; y++)
    memcpy((char *)sf->pixels + y * sf->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  assert(SDL_SaveBMP(sf, nome) == 0);
  SDL_FreeSurface(sf);
  free(pix);
  printf("captura: %s\n", nome);
}

// O MESMO caminho de main.c, fase a fase.
static void quadro(SDL_Window *w, Quadro *q, Uint32 agora) {
  Uint64 t0, t1, t2, t3;
  float dt = 1.0f / 60.0f;
  int k;
  SDL_PumpEvents();
  tex_bombear(3);
  txt_rasterizadas = 0;
  t0 = SDL_GetPerformanceCounter();
  home_atualizar(dt, agora);
  detail_atualizar(dt, agora);
  corviva_quadro(dt, ajustes_cor_viva(), ajustes_cor_logo(), ajustes_animacoes_reduzidas());
  t1 = SDL_GetPerformanceCounter();
  gfx_novo_quadro();
  tex_novo_quadro();
  gfx_sem_recorte();
  gfx_ambiente_preparar();
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  gfx_ambiente(1.0f);
  txt_novo_quadro();
  if (!detail_cobre_tela()) home_desenhar(agora);
  detail_desenhar(agora);
  gfx_ambiente_descarregar();
  t2 = SDL_GetPerformanceCounter();
  glFinish();
  t3 = SDL_GetPerformanceCounter();
  if (q) {
    q->upd = ms(t0, t1); q->des = ms(t1, t2); q->gpu = ms(t2, t3);
    q->fill = gfx_fill; q->fillVis = gfx_fill_vis;
    q->rects = gfx_n_rect; q->progs = gfx_n_prog; q->binds = gfx_n_bind;
    q->cheios = gfx_n_cheio; q->cheiosMist = gfx_n_cheio_mistura;
    q->txtRast = txt_rasterizadas;
    for (k = 0; k < GFX_NMODOS; k++) q->modo[k] = gfx_fill_modo[k];
  }
  SDL_GL_SwapWindow(w);
}

static void povoar(void) {
  static CatItem itens[NCAT];
  static CatFileira fils[NFIL];
  int f, i;
  memset(itens, 0, sizeof itens);
  memset(fils, 0, sizeof fils);
  for (f = 0; f < NFIL; f++) {
    snprintf(fils[f].chave, sizeof fils[f].chave, "perf_%02d", f);
    snprintf(fils[f].titulo, sizeof fils[f].titulo, "Fileira de teste %d - Filme", f);
    snprintf(fils[f].tipo, sizeof fils[f].tipo, "movie");
    fils[f].ini = f * PORFIL;
    fils[f].n = PORFIL;
    for (i = 0; i < PORFIL; i++) {
      int k = f * PORFIL + i, t = (k * 13) % 900;
      CatItem *c = &itens[k];
      snprintf(c->imdb, sizeof c->imdb, "tt%07d", 1000000 + t);
      snprintf(c->tipo, sizeof c->tipo, "%s", t % 3 ? "movie" : "series");
      snprintf(c->titulo, sizeof c->titulo, "Titulo de teste numero %d", t);
      snprintf(c->meta, sizeof c->meta, "%d", 1990 + t % 35);
      snprintf(c->genero, sizeof c->genero, "Filme · Drama");
      snprintf(c->sinopse, sizeof c->sinopse,
               "Uma sinopse de teste longa o bastante para ocupar tres linhas do "
               "destaque e medir o texto que a home rasteriza e guarda por titulo %d.", t);
      snprintf(c->poster, sizeof c->poster, "deploy/app/art/%02d.jpg", t % 40);
      snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", (t + 7) % 40);
      snprintf(c->classificacao, sizeof c->classificacao, "%s", t % 2 ? "14" : "");
      c->nota = 50 + t % 45;
      if (f == 0 && i < 6) { c->progresso = 10 + i * 6; c->restanteMin = 20 + i; }
    }
  }
  cat_definir_tudo(itens, NCAT, fils, NFIL);
}

static void ajusta(const char *cenario) {
  char cam[700];
  FILE *a;
  int layout = 0, vidro = 0, tema = 0, cheio = 1;
  if (!strcmp(cenario, "imersiva")) { vidro = 1; tema = 15; }
  else if (!strcmp(cenario, "padrao")) layout = 1;
  else if (!strcmp(cenario, "dinamica")) layout = 2;
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dirDados);
  a = fopen(cam, "w");
  assert(a);
  // No arquivo, 0 = LIGADO nos ajustes de liga/desliga (V_LIGA).
  fprintf(a, "idioma 0\ntrailerHero 1\nhomeLayoutLocal %d\nvidroLocal %d\n"
             "modernLandscapePostersEnabled 1\nmodernHeroFullScreenBackdropEnabled %d\n"
             "selected_theme %d\ncardDepthEnabled 0\nposterLabelsEnabled 0\n",
          layout, vidro ? 0 : 1, cheio ? 0 : 1, tema);
  if (getenv("NV_AJ")) fprintf(a, "%s\n", getenv("NV_AJ"));
  fclose(a);
  ajustes_dir(dirDados);
}

static int cmpd(const void *a, const void *b) {
  double x = *(const double *)a, y = *(const double *)b;
  return x < y ? -1 : x > y;
}
static double p95(double *v, int n) { qsort(v, (size_t)n, sizeof *v, cmpd); return v[(int)(0.95 * (n - 1))]; }

static void relatar(const char *cen, const char *rotulo, Quadro *qs, int n) {
  double *v = malloc(sizeof(double) * (size_t)n);
  double su = 0, sd = 0, sg = 0, sf = 0, sfv = 0, sm[GFX_NMODOS];
  long sr = 0, sp = 0, sb = 0, sc = 0, scm = 0, st = 0;
  int i, k;
  memset(sm, 0, sizeof sm);
  for (i = 0; i < n; i++) {
    su += qs[i].upd; sd += qs[i].des; sg += qs[i].gpu; sf += qs[i].fill; sfv += qs[i].fillVis;
    sr += qs[i].rects; sp += qs[i].progs; sb += qs[i].binds; sc += qs[i].cheios; scm += qs[i].cheiosMist;
    st += qs[i].txtRast;
    for (k = 0; k < GFX_NMODOS; k++) sm[k] += qs[i].modo[k];
  }
  for (i = 0; i < n; i++) v[i] = qs[i].des;
  printf("[%s] %-10s upd=%.2f des=%.2f(p95 %.2f) gpu=%.2f ms | fill=%.2fx vis=%.2fx cheios=%.1f mist=%.1f"
         " | rects=%ld progs=%ld binds=%ld txt=%ld\n",
         cen, rotulo, su / n, sd / n, p95(v, n), sg / n, sf / n, sfv / n,
         (double)sc / n, (double)scm / n, sr / n, sp / n, sb / n, st);
  printf("[%s] %-10s fill por modo:", cen, rotulo);
  for (k = 0; k < GFX_NMODOS; k++) if (sm[k] / n >= 0.005) printf(" %d=%.2f", k, sm[k] / n);
  printf("\n");
  free(v);
}

static void cenario(SDL_Window *w, const char *cen) {
  static Quadro qs[300];
  int i, n = 300;
  Uint32 t = 1000;
  ajusta(cen);
  home_ir_topo();
  // Assenta: artes sobem, molas param.
  for (i = 0; i < 240; i++) { quadro(w, NULL, t); t += 16; }
  // Primeira fileira em foco (o destaque sai do foco), depois parado.
  tecla(SDLK_DOWN);
  for (i = 0; i < 120; i++) { quadro(w, NULL, t); t += 16; }
  for (i = 0; i < n; i++) { quadro(w, &qs[i], t); t += 16; }
  relatar(cen, "parada", qs, n);
  if (getenv("PERF_BMP")) { char b[800]; snprintf(b, sizeof b, "%s-%s-parada.bmp", getenv("PERF_BMP"), cen); guardar(b); }
  // CROSSFADE DO DESTAQUE, no meio: um passo a direita (a arte nova sobe e o
  // esvanecimento corre ate o fim), volta a esquerda (arte ja em cache, o
  // esvanecimento comeca na hora) e captura no 4o e no 8o quadro da volta.
  // E a mesma sequencia nas duas arvores; a captura compara a composicao em
  // camadas (gfx_hero_camadas) com as duas passadas misturadas.
  if (getenv("PERF_BMP")) {
    char b[800];
    tecla(SDLK_RIGHT);
    for (i = 0; i < 90; i++) { quadro(w, NULL, t); t += 16; }
    tecla(SDLK_LEFT);
    for (i = 0; i < 4; i++) { quadro(w, NULL, t); t += 16; }
    snprintf(b, sizeof b, "%s-%s-xfade4.bmp", getenv("PERF_BMP"), cen); guardar(b);
    for (i = 0; i < 4; i++) { quadro(w, NULL, t); t += 16; }
    snprintf(b, sizeof b, "%s-%s-xfade8.bmp", getenv("PERF_BMP"), cen); guardar(b);
    for (i = 0; i < 90; i++) { quadro(w, NULL, t); t += 16; }
  }
  for (i = 0; i < n; i++) {
    if (i % 12 == 0) tecla(i < n / 2 ? SDLK_RIGHT : SDLK_LEFT);
    quadro(w, &qs[i], t); t += 16;
  }
  relatar(cen, "navegando", qs, n);
  // Detalhe por cima.
  { HomeItem it;
    if (home_item_focado(&it)) {
      detail_abrir(&it);
      for (i = 0; i < 180; i++) { quadro(w, NULL, t); t += 16; }
      for (i = 0; i < n; i++) { quadro(w, &qs[i], t); t += 16; }
      relatar(cen, "detalhe", qs, n);
      if (getenv("PERF_BMP")) { char b[800]; snprintf(b, sizeof b, "%s-%s-detalhe.bmp", getenv("PERF_BMP"), cen); guardar(b); }
      tecla(SDLK_AC_BACK);
      for (i = 0; i < 120; i++) { quadro(w, NULL, t); t += 16; }
    } else printf("[%s] sem item focado: detalhe nao medido\n", cen);
  }
}

int main(int argc, char **argv) {
  static const char *const todos[] = { "moderna", "imersiva", "padrao", "dinamica" };
  const char *so = getenv("NV_CENARIO");
  SDL_Window *w;
  SDL_GLContext gl;
  int i;
  (void)argc; (void)argv;
  dados_iniciar("deploy/app/art");
  dirDados = dados_dir();
  if (!dirDados || !strstr(dirDados, "nuvio-fluidez-perf")) {
    fprintf(stderr, "recuse: NUVIO_DADOS tem de ser a pasta temporaria do teste\n");
    return 1;
  }
  perFreq = (double)SDL_GetPerformanceFrequency();
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  w = SDL_CreateWindow("Nuvio: fluidez (medida)", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1920, 1080,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(w);
  gl = SDL_GL_CreateContext(w);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  gfx_snap_iniciar((int)NV_TELA_W / 2, (int)NV_TELA_H / 2);
  gfx_borrao_iniciar(480, 270);
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(192);
  gfx_icones_dir("deploy/app/art");
  ajusta(so ? so : "moderna");
  assert(home_iniciar("deploy/app/art"));
  povoar();
  for (i = 0; i < 4; i++)
    if (!so || !strcmp(so, todos[i])) cenario(w, todos[i]);
  SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(w);
  SDL_Quit();
  return 0;
}
