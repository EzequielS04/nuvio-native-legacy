// URL DE CARTAZ COMPRIDA CHEGA INTEIRA ATE A TELA E VOLTA DO DISCO (#361).
//
// O relato (markcodes): cartaz de provedor como o pictorium passa de ~300
// caracteres e nao carrega. CatItem.poster ja tinha 1024 desde o #200; o corte
// estava nas COPIAS da URL pelo caminho. Este teste leva uma URL de ~600
// caracteres, com query string (o formato de provedor de cartaz com nota), por
// cada parada:
//
//   1. JSON do catalogo/meta do addon -> deMeta -> CatItem (poster, e o fundo
//      que nasce do poster quando o addon nao manda `background`);
//   2. fundo e logo compridos demais para os 512 do CatItem: vazios (e o
//      registro diz por que), nunca um prefixo cortado que vira 404;
//   3. pedido da textura: a URL que vai a rede, a chave e o nome no disco;
//   4. lista de Salvos local: gravar, largar, ler de novo;
//   5. biblioteca da conta (sync_pull_library) -> ContaLibItem;
//   6. jornal da conta (contapend, binario proprio: -DPUL_PEND): gravar,
//      reabrir e subir no sync_push_library.
//
// "Continuar assistindo" nao guarda arte (progresso.c guarda chave, tempo e
// tipo): o card sai do CatItem, coberto pelo passo 1.
//
//   bash tests/poster_url_longa.sh
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PUL_URL 600

// Formato de provedor de cartaz com nota: caminho com id, query com a
// configuracao e um token longo no fim. O reporter nao colou a URL; o
// tamanho e a forma sao os do relato (passa de 300, tem query).
static void montaUrl(char *d, size_t n, const char *tipo, size_t alvo) {
  static const char ABC[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  size_t k, i = 0;
  snprintf(d, n,
           "https://pictorium.example.dev/api/v2/%s/movie/tt0111161?lang=en-US"
           "&ratings=imdb,tmdb,trakt,letterboxd,rottentomatoes,metacritic"
           "&badge=bottom-right&style=modern&blur=0&quality=high&fallback=tmdb"
           "&cfg=", tipo);
  k = strlen(d);
  while (k < alvo && k + 1 < n) d[k++] = ABC[(i++ * 7 + 3) % 64];
  d[k] = 0;
}

static int falhas;
#define CONFERE(nome, cond) do { \
    if (cond) printf("ok    %s\n", nome); \
    else { printf("FALHA %s\n", nome); falhas++; } } while (0)

// Inteira ou ausente. Um PREFIXO da URL e o defeito: vira um 404 calado.
static int inteiraOuVazia(const char *campo, const char *url) {
  return !campo[0] || !strcmp(campo, url);
}

#ifdef PUL_PEND
// ============================================================ jornal da conta
#include "contapend.h"
#include "js.h"

int perfis_ativo(void) { return 1; }
int sessao_logada(void) { return 1; }
const char *sessao_usuario(void) { return "u-361"; }
const char *dados_cliente_id(void) { return "cliente-teste"; }
static char *disco;
char *dados_ler(const char *n) { (void)n; return disco ? strdup(disco) : NULL; }
int dados_gravar(const char *n, const char *c) { (void)n; free(disco); disco = strdup(c); return 1; }
int dados_apagar(const char *n) { (void)n; free(disco); disco = NULL; return 1; }
char *contacache_ler(const char *s, int p, const char *u, long *q) {
  (void)s; (void)p; (void)u; (void)q; return NULL; }
void cat_historico_definir_id(const char *i, const char *t, int v) { (void)i; (void)t; (void)v; }
static long long relogio(void) { return 1759000000000LL; }

static char subiu[4096];
char *sessao_rpc(const char *fn, const char *corpo, int *st) {
  *st = 200;
  if (!strcmp(fn, "sync_pull_library")) return strdup("[]");
  if (!strcmp(fn, "sync_push_library")) {
    const char *a = js_array(corpo, NULL, "p_items");
    subiu[0] = 0;
    if (a) js_texto(a, js_fim(a), "poster", subiu, sizeof subiu);
    return strdup("null");
  }
  return strdup("[]");
}

int main(void) {
  char url[PUL_URL + 1];
  montaUrl(url, sizeof url, "poster", PUL_URL);
  assert(strlen(url) == PUL_URL);
  contapend_relogio(relogio);
  contapend_sem_fio(1);
  assert(contapend_lista("tt0111161", "movie", "Um Sonho de Liberdade", url, 1) == 1);
  CONFERE("contapend: o arquivo do jornal guarda a URL inteira",
          disco && strstr(disco, url) != NULL);
  contapend_esquecer();                       // fecha o app: memoria some, disco fica
  assert(contapend_enviar() >= 1);
  CONFERE("contapend: depois de reabrir, o sync_push_library sobe a URL inteira",
          !strcmp(subiu, url));
  printf(falhas ? "FALHOU (%d)\n" : "PASSOU\n", falhas);
  return falhas ? 1 : 0;
}

#else
// ===================================================== descoberta + tex_cache
#include "../src/descoberta.c"
static char redePediu[2048];
static int redeVezes;
char *redeTeste(const char *u, int timeout, long *n);
#define rede_baixar_bin redeTeste
#include "../src/tex_cache.c"
#undef rede_baixar_bin
#include "salvos.h"
#include "contalib.h"
#include "dados.h"
#include <unistd.h>

char *redeTeste(const char *u, int timeout, long *n) {
  unsigned char *b = calloc(1, 1024);
  (void)timeout;
  snprintf(redePediu, sizeof redePediu, "%s", u);
  redeVezes++;
  b[0] = 0xff; b[1] = 0xd8;                   // assinatura de JPEG basta
  *n = 1024;
  return (char *)b;
}

static int deJson(const char *json, CatItem *ci) {
  return deMeta(json, json + strlen(json), "movie", ci);
}

int main(void) {
  char url[PUL_URL + 1], fundo[PUL_URL + 1], logo[PUL_URL + 1], enorme[1601];
  char json[6000], dir[400], sub[700];
  CatItem *ci = calloc(1, sizeof *ci);
  assert(ci);
  montaUrl(url, sizeof url, "poster", PUL_URL);
  montaUrl(fundo, sizeof fundo, "background", PUL_URL);
  montaUrl(logo, sizeof logo, "logo", PUL_URL);
  montaUrl(enorme, sizeof enorme, "poster", 1600);
  assert(strlen(url) == PUL_URL && strlen(enorme) == 1600);
  { const char *t = getenv("TMPDIR");
    snprintf(dir, sizeof dir, "%s/nuvio-poster-longa-XXXXXX", t && *t ? t : "/tmp");
    assert(mkdtemp(dir)); }

  // 1. Catalogo: so o poster (o addon de cartaz nao manda `background`).
  snprintf(json, sizeof json,
           "{\"id\":\"tt0111161\",\"type\":\"movie\",\"name\":\"Um Sonho de Liberdade\","
           "\"poster\":\"%s\"}", url);
  assert(deJson(json, ci));
  CONFERE("catalogo: CatItem.poster recebe a URL de 600 caracteres inteira",
          !strcmp(ci->poster, url));
  CONFERE("catalogo: o fundo que nasce do poster e a URL inteira ou nenhum (nunca cortada)",
          inteiraOuVazia(ci->backdrop, url) && inteiraOuVazia(ci->backdropCatalogo, url));

  // 2. Meta com fundo e logo que nao cabem nos 512 do CatItem.
  snprintf(json, sizeof json,
           "{\"id\":\"tt0111161\",\"type\":\"movie\",\"name\":\"Um Sonho de Liberdade\","
           "\"poster\":\"%s\",\"background\":\"%s\",\"logo\":\"%s\"}", url, fundo, logo);
  assert(deJson(json, ci));
  CONFERE("meta: poster inteiro com fundo e logo compridos ao lado",
          !strcmp(ci->poster, url));
  CONFERE("meta: fundo de 600 caracteres inteiro ou vazio, nunca um prefixo",
          inteiraOuVazia(ci->backdrop, fundo) || !strcmp(ci->backdrop, url));
  CONFERE("meta: logo de 600 caracteres inteiro ou vazio, nunca um prefixo",
          inteiraOuVazia(ci->logo, logo));
  // Poster maior que o proprio CatItem.poster (1024): sem corte calado.
  snprintf(json, sizeof json,
           "{\"id\":\"tt0111161\",\"type\":\"movie\",\"name\":\"Um Sonho de Liberdade\","
           "\"poster\":\"%s\"}", enorme);
  deJson(json, ci);
  CONFERE("catalogo: poster de 1600 caracteres nao vira prefixo de 1023",
          inteiraOuVazia(ci->poster, enorme) && inteiraOuVazia(ci->backdrop, enorme));

  // 3. Textura: o pedido guarda a URL, a rede recebe a URL e o disco usa hash.
  CONFERE("tex: o item de textura comporta a URL inteira",
          strlen(url) < sizeof itens[0].caminho);
  tex_cache_dir(dir);
  { char dst[600], outro[600], u2[PUL_URL + 1];
    const char *nome;
    nomeDeCache(url, dst, sizeof dst);
    nome = strrchr(dst, '/') ? strrchr(dst, '/') + 1 : dst;
    // FNV em unsigned long: 8 hex no 32 bits (TV), 16 no Mac, mais a extensao.
    CONFERE("tex: nome no disco e o hash, curto (nao a URL)", strlen(nome) <= 24);
    snprintf(u2, sizeof u2, "%s", url);
    u2[PUL_URL - 1] = u2[PUL_URL - 1] == 'A' ? 'B' : 'A';   // so o ultimo byte muda
    nomeDeCache(u2, outro, sizeof outro);
    CONFERE("tex: o hash cobre a URL inteira (ultimo byte muda o nome)", strcmp(dst, outro));
    assert(garantirLocal(url, dst, sizeof dst, NULL, NULL));
    CONFERE("tex: a rede recebe a URL inteira", redeVezes == 1 && !strcmp(redePediu, url));
    CONFERE("tex: o arquivo baixado fica sob o nome do hash", access(dst, F_OK) == 0);
    unlink(dst); }

  // 4. Salvos: gravar, largar a lista (troca de perfil) e ler de volta.
  snprintf(sub, sizeof sub, "%s/dados", dir);
  setenv("NUVIO_DADOS", sub, 1);
  dados_iniciar(dir);
  salvos_perfil(1);
  salvos_iniciar();
  snprintf(json, sizeof json,
           "{\"id\":\"tt0111161\",\"type\":\"movie\",\"name\":\"Um Sonho de Liberdade\","
           "\"poster\":\"%s\"}", url);
  assert(deJson(json, ci));
  snprintf(ci->imdb, sizeof ci->imdb, "tt0111161");
  snprintf(ci->tipo, sizeof ci->tipo, "movie");
  assert(salvos_definir(ci, 1) == 1);
  CONFERE("salvos: em memoria, a URL inteira", salvos_n() == 1 && !strcmp(salvos_item(0)->poster, url));
  salvos_perfil(2);
  salvos_perfil(1);
  CONFERE("salvos: lida de volta do disco, a URL inteira",
          salvos_n() == 1 && !strcmp(salvos_item(0)->poster, url));
  salvos_esquecer();

  // 5. Biblioteca da conta.
  snprintf(json, sizeof json,
           "[{\"content_id\":\"tt0111161\",\"content_type\":\"movie\",\"name\":\"Um Sonho de Liberdade\","
           "\"poster\":\"%s\",\"background\":\"%s\",\"added_at\":\"2026-10-01T10:00:00Z\"}]",
           url, fundo);
  CONFERE("conta: a linha da biblioteca e lida", contalib_ler_biblioteca(json) == 1);
  CONFERE("conta: ContaLibItem.poster recebe a URL inteira",
          contalib_n() == 1 && !strcmp(contalib_item(0)->poster, url));
  CONFERE("conta: fundo de 600 caracteres inteiro ou vazio, nunca um prefixo",
          contalib_n() == 1 && inteiraOuVazia(contalib_item(0)->backdrop, fundo));

  free(ci);
  printf(falhas ? "FALHOU (%d)\n" : "PASSOU\n", falhas);
  return falhas ? 1 : 0;
}
#endif
