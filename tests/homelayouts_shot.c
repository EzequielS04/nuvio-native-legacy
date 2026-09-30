// CAPTURA DA HOME NOS TRES LAYOUTS (Moderna, Padrao, Dinamica), SEM REDE.
//
// As artes sao as de deploy/app/art: fundo 16:9 (NN.jpg), cartaz 2:3
// (poster/NN.jpg) e logo (logo/NN.png). As fileiras imitam o que a descoberta
// publica de verdade: "Continuar assistindo", um catalogo de destaque, um "Em
// alta", generos e um "Top 10" — os SINAIS pelos quais o layout Dinamica
// escolhe a forma de cada fileira (home.c, dinTipoDaFileira).
//
// Uso (o .sh compila e converte para PNG):
//   bash tests/homelayouts_shot.sh /tmp/nv-home-layouts-shots [camadas]
//
// `camadas` = digitos dos layouts a capturar, "012" por padrao. Para cada layout
// roda duas passadas, com a Interface de vidro desligada e ligada, e em cada
// uma fotografa o repouso no destaque e o foco em CADA fileira. O nome do
// arquivo diz tudo: <prefixo>-L<layout>-g<vidro>-<n>-<fileira>.bmp.
//
// Ao final imprime, por captura, o preenchimento (gfx_fill, em telas cheias) e
// o numero de retangulos do quadro — o que da para medir no Mac do custo de
// desenho; o custo em ms de GPU so existe na TV.
#include "ajustes.h"
#include "artehero.h"
#include "catalogo.h"
#include "colecoes.h"
#include "corviva.h"
#include "ctxmenu.h"
#include "dados.h"
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

#define NA 40
static const char *NOMES[] = {
  "O Diabo Veste Vermelho", "A Ilha do Farol", "Mata Fechada", "Sofá no Deserto",
  "Preto e Branco", "Ensaio Seis", "Noite de Verão", "O Último Trem",
  "Cidade Cinza", "Rio Acima", "Sal e Luz", "A Casa do Lago",
  "Vento Norte", "Fronteira", "Depois da Chuva", "Cartas de Inverno",
  "Ouro Velho", "O Jardim", "Marés", "Sem Volta",
};
#define NN (int)(sizeof NOMES / sizeof *NOMES)

typedef struct { const char *chave, *titulo, *tipo; int n, catalogo; } Fil;
static const Fil FILS[] = {
  { "continue_watching", "Continuar assistindo", "movie",  6, 0 },
  { "pop_movie",  "Popular - Filme",         "movie",  10, 1 },
  { "trend_series", "Em alta - Série",       "series", 10, 1 },
  { "drama_movie", "Drama - Filme",          "movie",  10, 1 },
  { "top10_hoje",  "Top 10 · Filmes hoje",   "movie",  10, 1 },
  { "comedia_movie", "Comédia - Filme",      "movie",  10, 1 },
  { "ficcao_movie", "Ficção científica - Filme", "movie", 10, 1 },
};
#define NF (getenv("NV_COL") ? 3 : (int)(sizeof FILS / sizeof *FILS))

static SDL_Window *janela;
static const char *dirDados;
static double fillUlt, fillVisUlt, modoUlt[GFX_NMODOS]; static int rectUlt;

static void gravar(const char *bmp) {
  unsigned char *pix = malloc(1920 * 1080 * 4);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  int y;
  assert(pix && s);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  for (y = 0; y < 1080; y++)
    memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  assert(SDL_SaveBMP(s, bmp) == 0);
  SDL_FreeSurface(s);
  free(pix);
}

static void quadros(int n, const char *bmp) {
  int i;
  for (i = 0; i < n; i++) {
    Uint32 agora = SDL_GetTicks();
    SDL_PumpEvents();
    tex_bombear(8);
    home_atualizar(1.0f / 60.0f, agora);
    corviva_quadro(1.0f / 60.0f, ajustes_cor_viva(), ajustes_cor_logo(), ajustes_animacoes_reduzidas());
    txt_novo_quadro();
    tex_novo_quadro();
    gfx_novo_quadro();
    gfx_ambiente_preparar();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    gfx_ambiente(1.0f);
    home_desenhar(agora);
    ctx_atualizar(1.0f / 60.0f, agora);
    ctx_desenhar(agora);
    if (i == n - 1) { fillUlt = gfx_fill; fillVisUlt = gfx_fill_vis; rectUlt = gfx_n_rect;
                      memcpy(modoUlt, gfx_fill_modo, sizeof modoUlt); }
    if (bmp && i == n - 1) gravar(bmp);
    SDL_GL_SwapWindow(janela);
    SDL_Delay(8);
  }
}

// SEGURAR OK de verdade: KEYDOWN, quadros ate passar NV_HOLD_MS (o relogio e o
// SDL_GetTicks real), KEYUP. E o caminho da home que abre o menu do cartaz.
static void segurarOk(void) {
  SDL_Event e;
  Uint32 ini = SDL_GetTicks();
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN;
  home_evento(&e);
  while (SDL_GetTicks() - ini < NV_HOLD_MS + 150) quadros(1, NULL);
  e.type = SDL_KEYUP;
  if (ctx_aberto()) ctx_evento(&e); else home_evento(&e);
}
static void teclaCtx(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  ctx_evento(&e);
  e.type = SDL_KEYUP;
  ctx_evento(&e);
}

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  home_evento(&e);
  e.type = SDL_KEYUP;
  home_evento(&e);
}

// Ajustes pelo mesmo arquivo que a TV le. Idioma 0 = pt; animacoes NORMAIS
// (as molas assentam em ~90 quadros); trailer desligado (sem rede).
static void ajusta(int layout, int vidro) {
  char cam[700];
  FILE *a;
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dirDados);
  a = fopen(cam, "w");
  assert(a);
  fprintf(a, "idioma 0\ntrailerHero 1\nhomeLayoutLocal %d\nvidroLocal %d\n"
             "modernLandscapePostersEnabled 1\nselected_theme %d\n",
          layout, vidro ? 0 : 1, getenv("NV_TEMA") ? atoi(getenv("NV_TEMA")) : 0);
  if (getenv("NV_AJ")) fprintf(a, "%s\n", getenv("NV_AJ"));   // ex.: "heroSectionEnabled 1"
  fclose(a);
  ajustes_dir(dirDados);
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nv-home-layouts-shots/h";
  const char *camadas = argc > 2 ? argv[2] : "012";
  static CatItem itens[80];
  static CatFileira fils[8];
  SDL_GLContext gl;
  char bmp[800], cache[700];
  int i, k, total = 0, ini = 0;

  dados_iniciar("deploy/app/art");
  dirDados = dados_dir();
  if (!dirDados || !strstr(dirDados, "nuvio-homelayouts-shot")) {
    fprintf(stderr, "recuse: NUVIO_DADOS tem de ser a pasta temporaria do teste\n");
    return 1;
  }
  (void)argc;
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Nuvio: layouts da home", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 1920, 1080,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(janela);
  gl = SDL_GL_CreateContext(janela);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  // NV_EFEITOS=1 leves, 2 minimos (os niveis de gpunivel.h sem o 720p).
  if (getenv("NV_EFEITOS")) {
    int e = atoi(getenv("NV_EFEITOS"));
    gfx_definir_efeitos_leves(e >= 1);
    gfx_definir_efeitos_minimos(e >= 2);
  }
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(192);
  gfx_borrao_iniciar(480, 270);
  artehero_definir_falhou(tex_falhou);
  snprintf(cache, sizeof cache, "%s/cache", dirDados);
  tex_cache_dir(cache);
  gfx_icones_dir("deploy/app/art");
  ajusta(0, 0);
  assert(home_iniciar("deploy/app/art"));

  memset(itens, 0, sizeof itens);
  memset(fils, 0, sizeof fils);
  for (k = 0; k < NF; k++) {
    CatFileira *f = &fils[k];
    snprintf(f->chave, sizeof f->chave, "%s", FILS[k].chave);
    snprintf(f->titulo, sizeof f->titulo, "%s", FILS[k].titulo);
    snprintf(f->tipo, sizeof f->tipo, "%s", FILS[k].tipo);
    if (FILS[k].catalogo) {
      snprintf(f->base, sizeof f->base, "https://addon.invalid/x");
      snprintf(f->catId, sizeof f->catId, "%s", FILS[k].chave);
    }
    f->ini = ini; f->n = FILS[k].n;
    for (i = 0; i < FILS[k].n; i++) {
      CatItem *c = &itens[ini + i];
      int a = (ini + i) % NA;
      snprintf(c->imdb, sizeof c->imdb, "tt90%05d", ini + i);
      snprintf(c->tipo, sizeof c->tipo, "%s", FILS[k].tipo);
      snprintf(c->titulo, sizeof c->titulo, "%s", NOMES[(ini + i) % NN]);
      snprintf(c->genero, sizeof c->genero, "%s · Drama", strcmp(FILS[k].tipo, "series") ? "Filme" : "Série");
      snprintf(c->meta, sizeof c->meta, "%d · 2 h 04 min", 2018 + (ini + i) % 8);
      snprintf(c->classificacao, sizeof c->classificacao, "%s", (i % 3) ? "14" : "16");
      snprintf(c->sinopse, sizeof c->sinopse,
               "Sinopse de enchimento, comprida o bastante para ocupar as linhas "
               "que o destaque reserva para ela, como num titulo de verdade.");
      snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", a);
      snprintf(c->backdropCatalogo, sizeof c->backdropCatalogo, "%s", c->backdrop);
      snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%02d.jpg", a);
      if (a < 10) snprintf(c->logo, sizeof c->logo, "deploy/app/art/logo/%02d.png", a);
      c->nota = 68 + (ini + i) % 25;
      if (k == 0) { c->progresso = 20 + i * 12; c->restanteMin = 90 - i * 10; }
    }
    ini += FILS[k].n;
    total = ini;
  }
  if (getenv("NV_COL")) {   // NV_COL=1: um grupo de colecao no fim, com as quatro cadeias de arte
    // NV_COL_FORMA=POSTER|LANDSCAPE|SQUARE: o tileShape das quatro pastas
    // (ausente = sem o campo, que o web le como quadrado).
    char fonte[400];
    char js[4600];
    snprintf(fonte, sizeof fonte, "%s%s%s\"sources\":[{\"addonBaseUrl\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]",
             getenv("NV_COL_FORMA") ? "\"tileShape\":\"" : "",
             getenv("NV_COL_FORMA") ? getenv("NV_COL_FORMA") : "",
             getenv("NV_COL_FORMA") ? "\"," : "");
    snprintf(js, sizeof js,
      "{\"collections\":[{\"id\":\"cs\",\"title\":\"Streaming\",\"backdropImageUrl\":\"deploy/app/art/07.jpg\",\"folders\":["
      "{\"id\":\"a\",\"title\":\"Com hero e capa\",\"heroBackdropUrl\":\"deploy/app/art/03.jpg\",\"coverImageUrl\":\"deploy/app/art/poster/12.jpg\",%s},"
      "{\"id\":\"b\",\"title\":\"So capa\",\"coverImageUrl\":\"deploy/app/art/05.jpg\",%s},"
      "{\"id\":\"c\",\"title\":\"So fundo da colecao\",%s},"
      "{\"id\":\"d\",\"title\":\"Com hero sem capa\",\"heroBackdropUrl\":\"deploy/app/art/09.jpg\",%s}]}]}",
      fonte, fonte, fonte, fonte);
    assert(col_definir_json(js) == 4);
  }
  cat_definir_tudo(itens, total, fils, NF);
  quadros(60, NULL);

  { const char *L;
    for (L = camadas; *L; L++) {
      int layout = *L - '0', vidro;
      for (vidro = 0; vidro < 2; vidro++) {
        int r;
        ajusta(layout, vidro);
        assert(ajustes_home_layout() == layout);
        for (r = 0; r < 14; r++) tecla(SDLK_UP);
        quadros(120, NULL);
        snprintf(bmp, sizeof bmp, "%s-L%d-g%d-0-destaque.bmp", saida, layout, vidro);
        quadros(1, bmp);
        printf("[shot] L%d g%d destaque: fill=%.2f vis=%.2f rects=%d\n", layout, vidro, fillUlt, fillVisUlt, rectUlt);
        if (getenv("NV_FILL_MODOS")) {   // quem preenche: modo=telas cheias
          int m; printf("[shot]   modos:");
          for (m = 0; m < GFX_NMODOS; m++) if (modoUlt[m] > 0.05) printf(" %d=%.2f", m, modoUlt[m]);
          printf("\n"); }
        for (r = 0; r < 9; r++) {
          tecla(SDLK_DOWN);
          quadros(110, NULL);
          snprintf(bmp, sizeof bmp, "%s-L%d-g%d-%d-fileira.bmp", saida, layout, vidro, r + 1);
          quadros(1, bmp);
          printf("[shot] L%d g%d fileira %d: fill=%.2f vis=%.2f rects=%d\n", layout, vidro, r + 1, fillUlt, fillVisUlt, rectUlt);
          if (getenv("NV_FILL_MODOS")) {
            int m; printf("[shot]   modos:");
            for (m = 0; m < GFX_NMODOS; m++) if (modoUlt[m] > 0.02) printf(" %d=%.2f", m, modoUlt[m]);
            printf("\n"); }
          if (r == 2) {   // um passo para o lado: rolagem horizontal + foco
            tecla(SDLK_RIGHT); tecla(SDLK_RIGHT);
            quadros(110, NULL);
            snprintf(bmp, sizeof bmp, "%s-L%d-g%d-%d-fileira-lado.bmp", saida, layout, vidro, r + 1);
            quadros(1, bmp);
          }
        }
        /* MEIO DA ROLAGEM: 12 quadros depois de subir uma fileira. */
        tecla(SDLK_UP);
        quadros(10, NULL);
        snprintf(bmp, sizeof bmp, "%s-L%d-g%d-x-meio-da-rolagem.bmp", saida, layout, vidro);
        quadros(1, bmp);
      }
    } }

  // NV_CTX=<n>: menu do cartaz SEGURANDO OK na fileira n (contada do destaque
  // para baixo), a pagina de estilos, a escolha e a home depois dela.
  if (getenv("NV_CTX")) {
    int r, alvo = atoi(getenv("NV_CTX"));
    ajusta(camadas[0] - '0', 0);
    for (r = 0; r < 14; r++) tecla(SDLK_UP);
    quadros(60, NULL);
    for (r = 0; r < alvo; r++) { tecla(SDLK_DOWN); quadros(40, NULL); }
    quadros(80, NULL);
    snprintf(bmp, sizeof bmp, "%s-ctx-0-antes.bmp", saida); quadros(1, bmp);
    segurarOk();
    quadros(60, NULL);
    snprintf(bmp, sizeof bmp, "%s-ctx-1-menu.bmp", saida); quadros(1, bmp);
    printf("[shot] ctx aberto=%d\n", ctx_aberto());
    // Ja na pagina de estilos (colecao) o OK escolhe; no titulo, desce ate
    // "Estilo da fileira" (a ultima) e entra.
    if (!getenv("NV_CTX_COL")) {
      for (r = 0; r < 8; r++) teclaCtx(SDLK_DOWN);
      teclaCtx(SDLK_RETURN);
      quadros(40, NULL);
      snprintf(bmp, sizeof bmp, "%s-ctx-2-estilos.bmp", saida); quadros(1, bmp);
    }
    // Uma forma abaixo da atual e OK.
    teclaCtx(SDLK_DOWN);
    quadros(20, NULL);
    snprintf(bmp, sizeof bmp, "%s-ctx-3-escolha.bmp", saida); quadros(1, bmp);
    teclaCtx(SDLK_RETURN);
    quadros(120, NULL);
    printf("[shot] ctx depois da escolha aberto=%d\n", ctx_aberto());
    snprintf(bmp, sizeof bmp, "%s-ctx-4-depois.bmp", saida); quadros(1, bmp);
  }

  tex_encerrar();
  txt_encerrar();
  SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(janela);
  SDL_Quit();
  return 0;
}
