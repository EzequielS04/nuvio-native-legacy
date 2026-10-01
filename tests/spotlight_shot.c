// CAPTURAS DO SPOTLIGHT (spotlight.h), sem rede e sem interacao.
//
// O que cada foto prova:
//   -entrada     a caixa no meio da animacao de entrada;
//   -vazio       campo vazio: pesquisas recentes do perfil, "Limpar" e "Em
//                alta" (os primeiros da primeira fileira de addon), por cima de
//                uma tela de posteres escurecida;
//   -digitado    "the" digitado: melhor resultado em destaque e Titulos, a
//                lista mudando a cada letra;
//   -foco        o foco na lista (direita a partir da ultima coluna): a linha
//                do melhor resultado acesa na cor do tema e o "OK Abrir";
//   -foco2       duas linhas abaixo;
//   -pessoa      a pessoa (elenco com id do TMDB) em foco, sem foto;
//   -colecao     uma pasta de colecao achada pelo nome;
//   -nada        um termo sem resultado: o aviso, nao uma lista vazia;
//   -vidro       a mesma busca com a Interface de vidro ligada;
//   -pessoa-tmdb "keanu": pessoas do /search/person do TMDB (resposta de
//                tests/fixtures, sem rede) com "Conhecido por";
//   -canal       "brasil": dois canais da Live TV de um guia carregado de um
//                addon falso local (tests/spotlight_canais_servidor.py);
//   -pessoa-tmdb-vivo  so com NUVIO_TMDB_DIR (pasta com tmdb.txt): o mesmo
//                pedido no TMDB de verdade, com as fotos de perfil.
// E confere, sem olho: digitar filtra, OK no titulo pede SPOT_TITULO com o
// indice certo e fecha, OK na pessoa pede SPOT_PESSOA com o tmdb, Voltar
// fecha, a busca feita entra nas recentes; a pessoa do TMDB so e pedida
// quando o texto para (um pedido para "keanu" inteiro, nenhum por letra), a
// URL leva o idioma, e OK nela pede SPOT_PESSOA com o titulo conhecido.
#include "spotlight.h"
#include "buscasrec.h"
#include "dados.h"
#include "ajustes.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "catalogo.h"
#include "colecoes.h"
#include "spotpessoa.h"
#include "guia.h"
#include "addons.h"
#include "descoberta.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Tipos de linha de spotlight.c (enum interno L_*).
enum { T_AVISO = 1, T_TOPO = 2, T_PESSOA = 4, T_COLECAO = 5, T_RECENTE = 9 };

static SDL_Window *janela;
static char ultimaUrl[800];

// O TMDB de mentira: guarda a URL e devolve a resposta gravada.
static char *tmdbFalso(const char *url) {
  FILE *f = fopen("tests/fixtures/tmdb_search_person.json", "rb");
  char *b;
  long n;
  snprintf(ultimaUrl, sizeof ultimaUrl, "%s", url);
  if (!f) return NULL;
  fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
  b = malloc((size_t)n + 1);
  if (b && fread(b, 1, (size_t)n, f) != (size_t)n) { free(b); b = NULL; }
  if (b) b[n] = 0;
  fclose(f);
  return b;
}
static char dirArte[600];

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  spot_evento(&e);
  e.type = SDL_KEYUP;
  spot_evento(&e);
}
static void digitar(const char *s) { for (; *s; s++) tecla(*s == ' ' ? SDLK_SPACE : (SDL_Keycode)*s); }
static void paraLista(void) { int i; for (i = 0; i < 6; i++) tecla(SDLK_RIGHT); }

// O "fundo": fileiras de posteres do pacote, como uma home.
static void fundo(void) {
  int r, c;
  char cam[700];
  gfx_cor((GfxRect){ 0, 0, 1920, 1080 }, 0, 0.07f, 0.07f, 0.08f, 1.0f);
  for (r = 0; r < 3; r++)
    for (c = 0; c < 8; c++) {
      GfxRect p = { 104.0f + c * 232.0f, 120.0f + r * 330.0f, 208.0f, 312.0f };
      GLuint t;
      snprintf(cam, sizeof cam, "%s/poster/%02d.jpg", dirArte, (r * 8 + c) % 40);
      t = tex_obter_larg(cam, p.w);
      if (t) { gfx_tex_aspect_atual = tex_aspecto(cam);
               gfx_rect(p, t, GFX_CARD, 0, 0, 0, 0.06f, 0, 0, 0, 1.0f);
               gfx_tex_aspect_atual = 0; }
      else gfx_cor(p, 0.06f, 0.2f, 0.2f, 0.22f, 1.0f);
    }
}

static void quadro(void) {
  Uint32 agora = SDL_GetTicks();
  SDL_PumpEvents();
  txt_novo_quadro();
  tex_novo_quadro();
  tex_bombear(16);
  gfx_novo_quadro();
  spot_atualizar(1.0f / 60.0f, agora);
  glClearColor(0.051f, 0.051f, 0.051f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  fundo();
  spot_desenhar(agora, 0);
  SDL_GL_SwapWindow(janela);
}

static void capturaEm(const char *saida, const char *nome, Uint32 ms) {
  unsigned char *pix = malloc(1920 * 1080 * 4);
  char arq[700];
  SDL_Surface *s;
  int y;
  Uint32 t0 = SDL_GetTicks();
  assert(pix);
  while (SDL_GetTicks() - t0 < ms) quadro();
  quadro();
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  assert(s);
  for (y = 0; y < 1080; y++)
    memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  snprintf(arq, sizeof arq, "%s-%s.bmp", saida, nome);
  assert(SDL_SaveBMP(s, arq) == 0);
  SDL_FreeSurface(s);
  free(pix);
  printf("captura: %s\n", arq);
}
static void captura(const char *saida, const char *nome) { capturaEm(saida, nome, 900); }

static int achar(int tipo, const char *texto) {
  int i;
  for (i = 0; i < spot_n_linhas(); i++)
    if (spot_linha_tipo(i) == tipo && (!texto || strstr(spot_linha_texto(i), texto))) return i;
  return -1;
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nuvio-spotlight";
  const char *dir = getenv("NUVIO_DADOS");
  SDL_GLContext gl;
  SpotPedido p;
  char cwd[400];
  if (!dir || !*dir) return 2;
  assert(getcwd(cwd, sizeof cwd));
  snprintf(dirArte, sizeof dirArte, "%s/deploy/app/art", cwd);
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Nuvio: Spotlight", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(janela);
  gl = SDL_GL_CreateContext(janela);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(160);
  gfx_icones_dir("deploy/app/art");
  dados_iniciar(dir);
  assert(!strcmp(dados_dir(), dir));
  ajustes_iniciar();

  // Catalogo do pacote em duas fileiras "de addon" (base preenchida: e o que
  // faz a primeira virar "Em alta").
  cat_carregar("deploy/app/art");
  { static CatFileira fl[2];
    int n = cat_n();
    snprintf(fl[0].chave, sizeof fl[0].chave, "shot.a");
    snprintf(fl[0].titulo, sizeof fl[0].titulo, "Top 10 de hoje - Filme");
    snprintf(fl[0].tipo, sizeof fl[0].tipo, "movie");
    snprintf(fl[0].base, sizeof fl[0].base, "https://addon.exemplo");
    snprintf(fl[0].catId, sizeof fl[0].catId, "top");
    fl[0].ini = 0; fl[0].n = n / 2;
    fl[1] = fl[0];
    snprintf(fl[1].chave, sizeof fl[1].chave, "shot.b");
    snprintf(fl[1].titulo, sizeof fl[1].titulo, "Lançamentos - Série");
    snprintf(fl[1].catId, sizeof fl[1].catId, "novos");
    fl[1].ini = n / 2; fl[1].n = n - n / 2;
    cat_republicar_fileiras(fl, 2); }
  // Elenco com id do TMDB num titulo (o pacote nao traz id): e o que a
  // pessoa precisa para abrir a filmografia.
  { int i;
    for (i = 0; i < cat_n(); i++) {
      const CatItem *ci = cat_item(i);
      if (ci && strstr(ci->titulo, "Prestige")) {
        static CatItem novo;
        novo = *ci;
        novo.nElenco = 2;
        snprintf(novo.elenco[0].nome, sizeof novo.elenco[0].nome, "Hugh Jackman");
        novo.elenco[0].tmdb = 6968;
        snprintf(novo.elenco[1].nome, sizeof novo.elenco[1].nome, "Christian Bale");
        novo.elenco[1].tmdb = 3894;
        novo.nota = 85;
        cat_atualizar_item(i, &novo);
      }
    } }
  { static ColFolder f[1];
    memset(f, 0, sizeof f);
    snprintf(f[0].id, sizeof f[0].id, "shot-col");
    snprintf(f[0].title, sizeof f[0].title, "The Dark Knight Trilogy");
    snprintf(f[0].group, sizeof f[0].group, "Sagas");
    snprintf(f[0].cover, sizeof f[0].cover, "%s/02.jpg", dirArte);
    f[0].extra = 1;
    col_extra_definir(f, 1); }

  { static const char *termos[] = { "duna", "breaking bad", "interestelar", "the office" };
    int i; for (i = 0; i < 4; i++) buscasrec_registrar(termos[i]); }

  spot_abrir(0);
  capturaEm(saida, "entrada", 60);
  captura(saida, "vazio");
  assert(achar(T_RECENTE, "the office") >= 0);

  digitar("the");
  assert(!strcmp(spot_consulta(), "the"));
  captura(saida, "digitado");
  assert(achar(T_TOPO, NULL) >= 0);
  assert(achar(T_PESSOA, NULL) < 0);   // "the" nao comeca nome nenhum
  paraLista();
  assert(spot_linha_focada() == achar(T_TOPO, NULL));
  captura(saida, "foco");
  tecla(SDLK_DOWN); tecla(SDLK_DOWN);
  captura(saida, "foco2");

  // Pessoa: "chri" acha Christian Bale no elenco do Prestige.
  tecla(SDLK_ESCAPE);
  assert(!spot_aberto());
  spot_abrir(0);
  digitar("chri");
  { int alvo = achar(T_PESSOA, "Christian Bale"), k;
    assert(alvo >= 0);
    paraLista();
    for (k = 0; k < 20 && spot_linha_focada() != alvo; k++) tecla(SDLK_DOWN);
    assert(spot_linha_focada() == alvo); }
  captura(saida, "pessoa");
  tecla(SDLK_RETURN);
  assert(spot_pediu(&p));
  assert(p.tipo == SPOT_PESSOA && p.tmdb == 3894);
  assert(!spot_aberto());
  assert(!strcmp(buscasrec_termo(0), "chri"));

  // Titulo: OK no melhor resultado pede o indice dele.
  spot_abrir(0);
  digitar("prestige");
  paraLista();
  tecla(SDLK_RETURN);
  assert(spot_pediu(&p));
  assert(p.tipo == SPOT_TITULO && cat_item(p.indice) &&
         strstr(cat_item(p.indice)->titulo, "Prestige"));

  // Colecao.
  spot_abrir(0);
  digitar("dark kn");
  assert(achar(T_COLECAO, "Dark Knight") >= 0);
  captura(saida, "colecao");

  // Nada encontrado.
  tecla(SDLK_ESCAPE);
  spot_abrir(0);
  digitar("zzqx");
  assert(achar(T_AVISO, NULL) >= 0);
  captura(saida, "nada");

  // Texto de fora (o caminho do ditado): espacos nas pontas saem.
  spot_texto_externo("  the  ");
  assert(!strcmp(spot_consulta(), "the"));

  // Vidro.
  tecla(SDLK_ESCAPE);
  ajustes_definir_vidro(1);
  assert(ajustes_vidro());
  spot_abrir(0);
  digitar("the");
  paraLista();
  tecla(SDLK_DOWN);
  captura(saida, "vidro");
  ajustes_definir_vidro(0);
  tecla(SDLK_ESCAPE);

  // PESSOA DO TMDB, com debounce. Digitar "keanu" de uma vez: nada sai antes
  // de SPP_ESPERA_MS; depois sai UM pedido, com o termo inteiro e o idioma.
  { SpotPessoa lidas[SPP_MAX];
    char *json = tmdbFalso("");
    int n0, alvo, k;
    assert(json);
    // a leitura pura: quem nao tem titulo conhecido fica de fora, e o "name"
    // de dentro de known_for nao vira o nome da pessoa
    assert(spotpessoa_extrair(json, lidas, SPP_MAX) == 2);
    assert(!strcmp(lidas[0].nome, "Keanu Reeves") && lidas[0].tmdb == 6384);
    assert(lidas[0].tituloTmdb == 603 && !strcmp(lidas[0].tituloTipo, "movie"));
    assert(strstr(lidas[0].conhecido, "Matrix") && strstr(lidas[0].conhecido, "Constantine"));
    assert(!lidas[0].foto[0]);
    assert(lidas[1].tituloTmdb == 1399 && !strcmp(lidas[1].tituloTipo, "tv"));
    free(json);
    spotpessoa_teste(tmdbFalso);
    n0 = spotpessoa_disparos();
    spot_abrir(0);
    digitar("keanu");
    quadro(); quadro();
    assert(spotpessoa_disparos() == n0);          // ainda dentro da espera
    assert(achar(T_PESSOA, "Keanu") < 0);
    { Uint32 t0 = SDL_GetTicks(); while (SDL_GetTicks() - t0 < 900) quadro(); }
    assert(spotpessoa_disparos() == n0 + 1);      // um pedido so
    assert(strstr(ultimaUrl, "/search/person?") && strstr(ultimaUrl, "query=keanu"));
    { char lg[40]; snprintf(lg, sizeof lg, "language=%s", desc_tmdb_idioma());
      assert(strstr(ultimaUrl, lg)); }
    alvo = achar(T_PESSOA, "Keanu Reeves");
    assert(alvo >= 0);
    assert(achar(T_PESSOA, "Sem Titulo") < 0);
    paraLista();
    for (k = 0; k < 30 && spot_linha_focada() != alvo; k++) tecla(SDLK_DOWN);
    assert(spot_linha_focada() == alvo);
    captura(saida, "pessoa-tmdb");
    // apagar e redigitar a ultima letra antes da espera: "kean" nunca sai, e
    // "keanu" vem do cache
    tecla(SDLK_BACKSPACE); digitar("u");
    { Uint32 t0 = SDL_GetTicks(); while (SDL_GetTicks() - t0 < 600) quadro(); }
    assert(spotpessoa_disparos() == n0 + 1);
    alvo = achar(T_PESSOA, "Keanu Reeves");
    paraLista();
    for (k = 0; k < 30 && spot_linha_focada() != alvo; k++) tecla(SDLK_DOWN);
    tecla(SDLK_RETURN);
    assert(spot_pediu(&p));
    assert(p.tipo == SPOT_PESSOA && p.indice < 0 && p.tmdb == 6384);
    assert(p.tituloTmdb == 603 && !strcmp(p.tituloTipo, "movie"));
    spotpessoa_teste(NULL); }

  // Canais: o guia carregado do addon falso (o .sh sobe o servidor).
  if (getenv("SPOT_CANAIS_BASE") && *getenv("SPOT_CANAIS_BASE")) {
    const char *b = getenv("SPOT_CANAIS_BASE");
    static CatFileira fl[3];
    char man[700];
    int idx[4], k;
    Uint32 t0;
    snprintf(man, sizeof man, "%s/manifest.json", b);
    addons_adicionar("Canais Teste", man);
    for (k = 0; k < cat_n_fileiras() && k < 2; k++) fl[k] = *cat_fileira(k);
    memset(&fl[2], 0, sizeof fl[2]);
    snprintf(fl[2].chave, sizeof fl[2].chave, "shot.canais");
    snprintf(fl[2].titulo, sizeof fl[2].titulo, "Canais");
    snprintf(fl[2].tipo, sizeof fl[2].tipo, "tv");
    snprintf(fl[2].base, sizeof fl[2].base, "%s", b);
    snprintf(fl[2].catId, sizeof fl[2].catId, "canais");
    cat_republicar_fileiras(fl, 3);
    guia_carregar();
    t0 = SDL_GetTicks();
    while (SDL_GetTicks() - t0 < 15000 && guia_buscar_canais("brasil", idx, 4) < 2) {
      guia_atualizar(0.016f, SDL_GetTicks());
      SDL_Delay(16);
    }
    assert(guia_buscar_canais("brasil", idx, 4) == 2);
    spot_abrir(0);
    digitar("brasil");
    { int alvo = achar(6 /* L_CANAL */, "ESPN Brasil");
      assert(alvo >= 0);
      paraLista();
      for (k = 0; k < 30 && spot_linha_focada() != alvo; k++) tecla(SDLK_DOWN);
      assert(spot_linha_focada() == alvo); }
    captura(saida, "canal");
    tecla(SDLK_RETURN);
    assert(spot_pediu(&p) && p.tipo == SPOT_CANAL && !strcmp(p.id, "teste:espn"));
  }

  // O TMDB de verdade, so quando pedido (NUVIO_TMDB_DIR com tmdb.txt).
  if (getenv("NUVIO_TMDB_DIR") && *getenv("NUVIO_TMDB_DIR")) {
    Uint32 t0;
    desc_tmdb(getenv("NUVIO_TMDB_DIR"));
    { char cd[700]; snprintf(cd, sizeof cd, "%s/img", dir); mkdir(cd, 0755); tex_cache_dir(cd); }
    spot_abrir(0);
    digitar("pedro pascal");
    t0 = SDL_GetTicks();
    while (SDL_GetTicks() - t0 < 10000 && spotpessoa_n("pedro pascal") < 0) quadro();
    quadro();   // a remontagem e no spot_atualizar seguinte
    printf("tmdb vivo: %d pessoas\n", spotpessoa_n("pedro pascal"));
    assert(spotpessoa_n("pedro pascal") > 0);
    assert(achar(T_PESSOA, "Pedro Pascal") >= 0);
    capturaEm(saida, "pessoa-tmdb-vivo", 3500);
  }
  puts("PASS: Spotlight filtra, abre titulo/pessoa, guarda a busca e fecha.");
  return 0;
}
