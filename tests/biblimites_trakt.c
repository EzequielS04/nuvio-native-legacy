// Teto e paginacao das listas do Trakt (#6 Shield: biblioteca com 213 de 1600+).
// Sem rede: o Trakt falso pagina a watchlist por ?page=&limit= (cada pagina
// capada em 500, como o servidor pode fazer) e responde a colecao inteira de
// uma vez. Compilado duas vezes: com e sem -DNV_ANDROID.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../src/trakt.c"
#ifndef TRAKT_LISTA_MAX   /* codigo antigo (prova de que o teste falha antes) */
#ifdef NV_ANDROID
#define TRAKT_LISTA_MAX 3000
#else
#define TRAKT_LISTA_MAX 400
#endif
#define SEM_CRESC 1
#endif

static int falhas, nPedidos, nPaginasWl;
static int filmesWl = 1100, seriesWl = 500, filmesCol = 600;
static void verifica(int ok, const char *caso) {
  if (!ok) { fprintf(stderr, "FAIL: %s\n", caso); falhas++; }
}

unsigned long long cat_historico_geracao(void) { return 1; }
void cat_historico_definir_id(const char *i, const char *t, int v) { (void)i; (void)t; (void)v; }
int cat_historico_definir_se_geracao(const char *i, const char *t, int v, unsigned long long g) {
  (void)i; (void)t; (void)v; (void)g; return 1;
}

static char *itens(int inicio, int qtd, int serie) {
  size_t cap = (size_t)qtd * 120 + 8, u = 0;
  char *s = malloc(cap);
  int i;
  assert(s);
  s[u++] = '[';
  for (i = 0; i < qtd; i++)
    u += (size_t)snprintf(s + u, cap - u,
      "%s{\"listed_at\":\"x\",\"%s\":{\"title\":\"T%d\",\"ids\":{\"imdb\":\"tt%07d\"}}}",
      i ? "," : "", serie ? "show" : "movie", inicio + i, (serie ? 5000000 : 0) + inicio + i);
  snprintf(s + u, cap - u, "]");
  return s;
}
char *rede_baixar_com(const char *url, int segundos, const char *const *cab) {
  int serie = strstr(url, "/shows") != NULL, total, p = 1, lim = 0, ini, q;
  const char *pg = strchr(url, '?');
  (void)segundos; (void)cab;
  nPedidos++;
  if (strstr(url, "/sync/collection/")) return itens(0, serie ? 0 : filmesCol, serie);
  assert(strstr(url, "/sync/watchlist/"));
  total = serie ? seriesWl : filmesWl;
  // SEM page/limit o servidor falso entrega so 250 (um Trakt que trunca a
  // resposta sem paginar e exatamente o sintoma do Shield: 213 de 1600+).
  if (!pg) return itens(0, total < 250 ? total : 250, serie);
  assert(sscanf(pg, "?page=%d&limit=%d", &p, &lim) == 2 && p > 0 && lim > 0);
  if (lim > 500) lim = 500;
  nPaginasWl++;
  ini = (p - 1) * lim;
  q = ini < total ? total - ini : 0;
  if (q > lim) q = lim;
  return itens(ini, q, serie);
}

const char *i18n(const char *s) { return s; }

// Captura o que trakt_lista imprime em stdout.
static char logbuf[4096];
static int salvo = -1;
static char caminho[256];
static void captura(int liga) {
  fflush(stdout);
  if (liga) {
    int fd;
    snprintf(caminho, sizeof caminho, "%s/biblimites-log.%d", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp", (int)getpid());
    fd = open(caminho, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    salvo = dup(1);
    dup2(fd, 1); close(fd);
  } else {
    FILE *f;
    size_t n;
    dup2(salvo, 1); close(salvo);
    f = fopen(caminho, "r");
    n = f ? fread(logbuf, 1, sizeof logbuf - 1, f) : 0;
    logbuf[n] = 0;
    if (f) fclose(f);
    unlink(caminho);
  }
}

int main(void) {
  CatItem *v;
  int n, esperado, logpath = 0;
  (void)logpath;
  ligado = 1;
  snprintf(token, sizeof token, "t"); snprintf(cliente, sizeof cliente, "c");
#ifdef NV_ANDROID
  esperado = 1600;
#else
  esperado = 400;
#endif
  verifica(TRAKT_LISTA_MAX == (esperado > 400 ? 3000 : 400), "TRAKT_LISTA_MAX por plataforma");

  captura(1);
  v = malloc(sizeof(CatItem) * (size_t)TRAKT_LISTA_MAX);
  n = trakt_lista("watchlist", v, TRAKT_LISTA_MAX);
  captura(0);
  verifica(n == esperado, "watchlist: itens guardados == teto/plataforma");
#ifdef NV_ANDROID
  verifica(strstr(logbuf, "watchlist: 1600 de 1600 (paginas 6)") != NULL, "log N de M com paginas");
  verifica(!strstr(logbuf, "teto"), "sem aviso de teto quando cabe");
  { int i, rep = 0;
    for (i = 1; i < n; i++) if (!strcmp(v[i].imdb, v[i-1].imdb)) rep++;
    verifica(!rep && !strcmp(v[1099].tipo, "movie") && !strcmp(v[1100].tipo, "series"),
             "sem duplicata e filmes antes de series"); }
#else
  verifica(strstr(logbuf, "watchlist: 400 de 400") != NULL ||
           strstr(logbuf, "watchlist: 400 de ") != NULL, "log N de M");
  verifica(strstr(logbuf, "teto 400 da plataforma") != NULL, "log do teto no TV");
  verifica(nPaginasWl <= 3, "TV nao pede alem do teto");
#endif
  free(v);
#ifndef SEM_CRESC
  { CatItem *w = NULL;
    int m = trakt_lista_cresc("watchlist", &w, TRAKT_LISTA_MAX);
    verifica(m == esperado && w && !strcmp(w[0].imdb, "tt0000000"), "variante crescente igual a fixa");
    free(w); }
#endif

  nPedidos = 0;
  captura(1);
  v = malloc(sizeof(CatItem) * (size_t)TRAKT_LISTA_MAX);
  n = trakt_lista("collection", v, TRAKT_LISTA_MAX);
  captura(0);
  verifica(n == (filmesCol < TRAKT_LISTA_MAX ? filmesCol : TRAKT_LISTA_MAX), "colecao");
  verifica(nPedidos == 2, "colecao nao pagina: um pedido por tipo");
  verifica(v[0].naColecao && !v[0].naLista, "marca de colecao");
  free(v);

  if (!falhas)
#ifdef NV_ANDROID
    puts("biblimites trakt (android): PASS");
#else
    puts("biblimites trakt (tv): PASS");
#endif
  return falhas ? 1 : 0;
}
