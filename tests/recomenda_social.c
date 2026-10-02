// Redesenho do Social, lado cliente: nome nunca UUID, parse do feed e do perfil
// do amigo (com o JSON que o Worker emite de verdade — copiado de
// `wrangler dev` em 02/10/2026), o merge/dedupe com o Trakt, a fila de
// atividade (so com alcance >= 1, progresso funde) e o cache do feed em disco.
//
//   bash tests/recomenda_social.sh
//
// Mesma receita de tests/recomenda.c: inclui o .c e intercepta a rede por
// #define. So escreve em NUVIO_DADOS, e confere isso antes.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NV_REC_URL "http://127.0.0.1:8799"
#define rede_baixar_etag  teste_rede_etag
#define rede_postar_st    teste_rede_postar
#define rede_baixar_st    teste_rede_st
#define rede_baixar_com   teste_rede_com
#include "../src/recomenda.c"

static const char *respCorpo = "{}";
static int  respStatus = 200;
static char respEtag[96];
static char ultimaUrl[600], ultimoCorpo[2400], ultimoIfNone[200];
static int  nGet, nPost;

static char *dup(const char *s) { char *r = s ? malloc(strlen(s) + 1) : NULL; if (r) strcpy(r, s); return r; }
char *teste_rede_etag(const char *u, int seg, const char *const *cab, int *st, char *etag, unsigned te) {
  int k; (void)seg; nGet++;
  snprintf(ultimaUrl, sizeof ultimaUrl, "%s", u); ultimoIfNone[0] = 0;
  for (k = 0; cab && cab[k]; k++)
    if (!strncmp(cab[k], "If-None-Match:", 14)) snprintf(ultimoIfNone, sizeof ultimoIfNone, "%s", cab[k] + 15);
  if (st) *st = respStatus;
  if (etag && te) snprintf(etag, te, "%s", respEtag);
  return respStatus == 304 ? NULL : dup(respCorpo);
}
char *teste_rede_postar(const char *u, int seg, const char *const *cab, const char *c, int *st) {
  (void)seg; (void)cab; nPost++;
  snprintf(ultimaUrl, sizeof ultimaUrl, "%s", u);
  snprintf(ultimoCorpo, sizeof ultimoCorpo, "%s", c ? c : "");
  if (st) *st = respStatus;
  return dup(respCorpo);
}
char *teste_rede_st(const char *u, int seg, const char *const *cab, int *st) {
  (void)seg; (void)cab; nGet++;
  snprintf(ultimaUrl, sizeof ultimaUrl, "%s", u);
  if (st) *st = respStatus;
  return dup(respCorpo);
}
char *teste_rede_com(const char *u, int seg, const char *const *cab) {
  (void)seg; (void)cab; nGet++; snprintf(ultimaUrl, sizeof ultimaUrl, "%s", u); return dup(respCorpo);
}

static int falhas;
#define CONFERE(c, ...) do { if (!(c)) { falhas++; printf("FALHA: " __VA_ARGS__); printf("\n"); } } while (0)

// Saidas REAIS do Worker (wrangler dev --local, teste-social.sh), 02/10/2026.
static const char *FEED =
  "{\"cursor\":11,\"itens\":[{\"id\":11,\"de\":\"nuvio:aaa\",\"deNome\":\"Henrique\",\"deAvatar\":\"\",\"grau\":1,\"via\":\"\",\"ev\":\"reacao\",\"imdb\":\"tt0000003\",\"midia\":\"movie\",\"titulo\":\"\",\"poster\":\"\",\"temporada\":0,\"episodio\":0,\"pct\":0,\"reacao\":1,\"criado\":1790974381},"
  "{\"id\":10,\"de\":\"nuvio:aaa\",\"deNome\":\"Henrique\",\"deAvatar\":\"\",\"grau\":1,\"via\":\"\",\"ev\":\"inicio\",\"imdb\":\"tt0000007\",\"midia\":\"series\",\"titulo\":\"Serie \\\"X\\\"\",\"poster\":\"https://image.tmdb.org/t/p/w342/a.jpg\",\"temporada\":2,\"episodio\":5,\"pct\":3,\"reacao\":0,\"criado\":1790974381},"
  "{\"id\":9,\"de\":\"pub:gwvwd2j5rm\",\"deNome\":\"\",\"deAvatar\":\"\",\"grau\":2,\"via\":\"Gustavo\",\"ev\":\"fim\",\"imdb\":\"tt0000001\",\"midia\":\"movie\",\"titulo\":\"Filme Um\",\"poster\":\"\",\"temporada\":0,\"episodio\":0,\"pct\":100,\"reacao\":0,\"criado\":1790970000},"
  "{\"id\":8,\"de\":\"nuvio:x\",\"ev\":\"progresso\",\"imdb\":\"tt0000001\",\"criado\":1}]}";
static const char *AMIGO =
  "{\"id\":\"nuvio:aaa\",\"nome\":\"Henrique\",\"avatar\":\"\",\"grau\":1,\"via\":\"\",\"desde\":1790974380,\"origem\":\"codigo\",\"compartilha\":1,"
  "\"mes\":{\"mes\":\"2026-10\",\"seg\":2700,\"filmes\":1,\"series\":1},"
  "\"agora\":{\"imdb\":\"tt0000007\",\"midia\":\"series\",\"titulo\":\"Serie \\\"X\\\"\",\"poster\":\"https://image.tmdb.org/t/p/w342/a.jpg\",\"temporada\":2,\"episodio\":5,\"pct\":3,\"atualizado\":1790974381},"
  "\"gostou\":[{\"imdb\":\"tt0000003\",\"midia\":\"movie\",\"titulo\":\"\",\"poster\":\"\",\"criado\":1790974381}],"
  "\"recs\":[{\"id\":1,\"imdb\":\"tt0000002\",\"tipo\":\"movie\",\"titulo\":\"Filme Dois\",\"poster\":\"\",\"criado\":1790974381,\"estado\":\"reacao\",\"reacao\":1},"
  "{\"id\":2,\"imdb\":\"tt0000005\",\"tipo\":\"movie\",\"titulo\":\"Cinco\",\"poster\":\"\",\"criado\":1790974300,\"estado\":\"comecou\",\"reacao\":null}],"
  "\"gosto\":{\"total\":2,\"iguais\":1,\"pct\":50}}";
static const char *AMIGO_FECHADO =
  "{\"id\":\"nuvio:bbb\",\"nome\":\"Gustavo\",\"avatar\":\"\",\"grau\":1,\"via\":\"\",\"desde\":1,\"origem\":\"\",\"compartilha\":0,"
  "\"mes\":null,\"agora\":null,\"gostou\":[],\"recs\":[],\"gosto\":null}";

static RecEvento ev(int fonte, int acao, const char *pessoa, const char *imdb, long long quando) {
  RecEvento e;
  memset(&e, 0, sizeof e);
  e.fonte = fonte; e.acao = acao; e.grau = 1;
  snprintf(e.pessoa, sizeof e.pessoa, "%s", pessoa);
  snprintf(e.imdb, sizeof e.imdb, "%s", imdb);
  e.quando = quando;
  return e;
}

int main(void) {
  const char *dir = getenv("NUVIO_DADOS");
  char nome[64];
  const char *cab[4] = { "Authorization: Bearer x", "X-Nuvio-Auth: nuvio", NULL, NULL };
  if (!dir || !dir[0]) { printf("recomenda_social: sem NUVIO_DADOS; recusando\n"); return 2; }
  dados_iniciar(dir);
  if (strcmp(dados_dir(), dir)) { printf("recomenda_social: dados_dir errado; recusando\n"); return 2; }
  mtx = SDL_CreateMutex();

  // --- nome nunca UUID ---------------------------------------------------------
  rec_nome_exibicao(nome, sizeof nome, "", "nuvio:5269539e-1111-4222-8333-944455556666");
  CONFERE(!strncmp(nome, "Amigo #", 7), "sem nome vira 'Amigo #n' (veio \"%s\")", nome);
  CONFERE(!strstr(nome, "5269"), "e nunca o UUID (veio \"%s\")", nome);
  CONFERE(strlen(nome) <= 10, "e curto (veio \"%s\")", nome);
  { char outro[64];
    rec_nome_exibicao(outro, sizeof outro, "", "nuvio:5269539e-1111-4222-8333-944455556666");
    CONFERE(!strcmp(nome, outro), "e estavel para o mesmo id"); }
  rec_nome_exibicao(nome, sizeof nome, "", "trakt:pedrinho");
  CONFERE(!strcmp(nome, "pedrinho"), "trakt sem nome vira o slug (veio \"%s\")", nome);
  rec_nome_exibicao(nome, sizeof nome, "Júlia", "nuvio:x");
  CONFERE(!strcmp(nome, "Júlia"), "com nome, o nome");
  { RecItem it;
    const char *j = "{\"id\":3,\"de\":\"nuvio:5269539e-aaaa\",\"deNome\":\"\",\"imdb\":\"tt1\",\"titulo\":\"T\"}";
    lerItem(j, j + strlen(j), &it);
    CONFERE(!strncmp(it.deNome, "Amigo #", 7), "rec de quem nao tem nome: 'Amigo #n' (veio \"%s\")", it.deNome); }

  // --- parse do feed (JSON real) -------------------------------------------------
  { RecEvento f[8];
    int n = feedParse(FEED, f, 8);
    CONFERE(n == 3, "feed: 3 eventos validos, progresso fora (veio %d)", n);
    CONFERE(f[0].acao == REC_ACAO_REACAO && f[0].reacao == 1, "reacao lida");
    CONFERE(f[1].acao == REC_ACAO_INICIO && f[1].temporada == 2 && f[1].episodio == 5, "inicio com episodio");
    CONFERE(!strcmp(f[1].titulo, "Serie \"X\"") && !strcmp(f[1].midia, "series"), "titulo com aspas e midia");
    CONFERE(f[2].grau == 2 && !strcmp(f[2].via, "Gustavo"), "amigo de amigo com via");
    CONFERE(!strncmp(f[2].pessoaNome, "Amigo #", 7), "amigo de amigo sem nome nao mostra o handle (\"%s\")", f[2].pessoaNome); }

  // --- perfil do amigo (JSON real) ----------------------------------------------
  { static RecAmigo a;
    CONFERE(amigoParse(AMIGO, &a), "amigo: parse");
    CONFERE(!strcmp(a.nome, "Henrique") && a.grau == 1 && !strcmp(a.origem, "codigo"), "amigo: nome/grau/origem");
    CONFERE(a.compartilha && a.temMes && a.seg == 2700 && a.filmes == 1 && a.series == 1, "amigo: mes");
    CONFERE(a.temAgora && !strcmp(a.agora.imdb, "tt0000007") && a.agora.pct == 3, "amigo: agora");
    CONFERE(a.nGostou == 1 && !strcmp(a.gostou[0].imdb, "tt0000003"), "amigo: gostou");
    CONFERE(a.nRecs == 2 && a.recs[0].estado == REC_REC_REAGIU && a.recs[0].temReacao && a.recs[0].reacao == 1,
            "amigo: rec reagiu");
    CONFERE(a.recs[1].estado == REC_REC_COMECOU && !a.recs[1].temReacao, "amigo: rec comecou, sem reacao");
    CONFERE(a.temGosto && a.gostoPct == 50 && a.gostoTotal == 2, "amigo: gosto parecido");
    CONFERE(amigoParse(AMIGO_FECHADO, &a), "amigo fechado: parse");
    CONFERE(!a.compartilha && !a.temMes && !a.temAgora && !a.temGosto && !a.nGostou, "amigo fechado: nada de atividade"); }

  // --- merge / dedupe ---------------------------------------------------------------
  { RecEvento d[6], s[6];
    int n;
    d[0] = ev(REC_FONTE_NUVIO, REC_ACAO_FIM, "trakt:ana", "tt1", 10000);
    d[0].reacao = 0; snprintf(d[0].titulo, sizeof d[0].titulo, "Nosso");
    s[0] = ev(REC_FONTE_TRAKT, REC_ACAO_FIM, "trakt:ana", "tt1", 10000 + 1800);   // mesmo fato, 30 min
    snprintf(s[0].poster, sizeof s[0].poster, "https://trakt/p.jpg");
    s[1] = ev(REC_FONTE_TRAKT, REC_ACAO_FIM, "trakt:ana", "tt1", 10000 + 7200);   // 2 h depois: outro
    s[2] = ev(REC_FONTE_TRAKT, REC_ACAO_INICIO, "trakt:ana", "tt1", 10000);       // outra acao
    s[3] = ev(REC_FONTE_TRAKT, REC_ACAO_FIM, "trakt:bia", "tt1", 10000);          // outra pessoa
    s[4] = ev(REC_FONTE_TRAKT, REC_ACAO_FIM, "trakt:ana", "tt1", 0);              // sem hora: funde
    n = rec_eventos_unir(d, 1, s, 5, 6);
    CONFERE(n == 4, "dedupe: 4 eventos distintos (veio %d)", n);
    { int i, nosso = -1;
      for (i = 0; i < n; i++) if (d[i].fonte == REC_FONTE_NUVIO) nosso = i;
      CONFERE(nosso >= 0 && !strcmp(d[nosso].titulo, "Nosso"), "na fusao fica o do nosso servidor");
      CONFERE(nosso >= 0 && !strcmp(d[nosso].poster, "https://trakt/p.jpg"), "completado com a capa do Trakt");
      CONFERE(nosso >= 0 && d[nosso].quando == 11800, "com a hora mais nova (veio %lld)", nosso >= 0 ? d[nosso].quando : 0);
      for (i = 1; i < n; i++) CONFERE(d[i - 1].quando >= d[i].quando, "ordem decrescente em %d", i); }
    // sem hora vai para o fim
    d[0] = ev(REC_FONTE_TRAKT, REC_ACAO_FIM, "trakt:c", "tt9", 0);
    s[0] = ev(REC_FONTE_NUVIO, REC_ACAO_SALVO, "nuvio:z", "tt8", 50);
    n = rec_eventos_unir(d, 1, s, 1, 6);
    CONFERE(n == 2 && d[0].quando == 50 && d[1].quando == 0, "sem hora vai para o fim");
    // teto: cheio, entra so o mais novo, sai o mais velho
    d[0] = ev(REC_FONTE_NUVIO, REC_ACAO_FIM, "p", "tt1", 100);
    d[1] = ev(REC_FONTE_NUVIO, REC_ACAO_FIM, "p", "tt2", 200);
    s[0] = ev(REC_FONTE_TRAKT, REC_ACAO_FIM, "q", "tt3", 300);
    s[1] = ev(REC_FONTE_TRAKT, REC_ACAO_FIM, "q", "tt4", 50);
    n = rec_eventos_unir(d, 2, s, 2, 2);
    CONFERE(n == 2 && d[0].quando == 300 && d[1].quando == 200, "teto respeitado, mais novos ficam"); }

  // --- Trakt -> RecEvento ----------------------------------------------------------
  { CatItem ci; RecEvento e;
    memset(&ci, 0, sizeof ci);
    snprintf(ci.imdb, sizeof ci.imdb, "tt0111161");
    snprintf(ci.tipo, sizeof ci.tipo, "movie");
    snprintf(ci.socialSlug, sizeof ci.socialSlug, "pedrinho");
    snprintf(ci.socialNome, sizeof ci.socialNome, "Pedro");
    snprintf(ci.socialAcao, sizeof ci.socialAcao, "assistindo agora");
    CONFERE(rec_evento_de_trakt(&ci, 0, &e) && e.fonte == REC_FONTE_TRAKT && e.acao == REC_ACAO_INICIO &&
            !strcmp(e.pessoa, "trakt:pedrinho"), "trakt: assistindo agora = inicio, pessoa trakt:<slug>");
    snprintf(ci.socialSlug, sizeof ci.socialSlug, "nuvio:aaa");
    snprintf(ci.socialAcao, sizeof ci.socialAcao, "assistiu");
    CONFERE(rec_evento_de_trakt(&ci, 5, &e) && e.fonte == REC_FONTE_NUVIO && e.acao == REC_ACAO_FIM &&
            !strcmp(e.pessoa, "nuvio:aaa"), "item Nuvio da fileira antiga mantem o id do servico");
    ci.imdb[0] = 0;
    CONFERE(!rec_evento_de_trakt(&ci, 0, &e), "sem imdb nao serve"); }

  // --- fila de atividade ----------------------------------------------------------
  { RecAtiv a;
    memset(&a, 0, sizeof a);
    snprintf(a.ev, sizeof a.ev, "progresso");
    snprintf(a.imdb, sizeof a.imdb, "tt0000001");
    snprintf(a.midia, sizeof a.midia, "movie");
    a.seg = 60; a.pct = 10;
    CONFERE(recomenda_alcance() == REC_ALCANCE_NAO_PERGUNTADO, "alcance nasce nao perguntado");
    CONFERE(!recomenda_atividade(&a), "sem resposta: nada entra na fila");
    SDL_LockMutex(mtx); alcance = 1; alcancePendente = -2; SDL_UnlockMutex(mtx);
    CONFERE(recomenda_atividade(&a), "nivel 1: entra");
    a.seg = 30; a.pct = 15;
    recomenda_atividade(&a);
    CONFERE(nAtivN == 1 && ativN[0].seg == 90 && ativN[0].pct == 15, "progresso funde (seg %d pct %d n %d)",
            ativN[0].seg, ativN[0].pct, nAtivN);
    snprintf(a.ev, sizeof a.ev, "hackear");
    CONFERE(!recomenda_atividade(&a), "ev invalido recusado");
    snprintf(a.ev, sizeof a.ev, "reacao"); a.reacao = -1; a.rec = 42;
    snprintf(a.titulo, sizeof a.titulo, "Tab\there \"aspas\"");
    recomenda_atividade(&a);
    nPost = 0; respStatus = 200; respCorpo = "{\"ok\":1}";
    enviarAtivNova(cab);
    CONFERE(nPost == 2 && nAtivN == 0, "fila esvazia no ciclo (%d posts)", nPost);
    CONFERE(strstr(ultimoCorpo, "\"ev\":\"reacao\"") && strstr(ultimoCorpo, "\"reacao\":-1") &&
            strstr(ultimoCorpo, "\"rec\":42") && strstr(ultimoCorpo, "\\\"aspas\\\"") &&
            !strchr(ultimoCorpo, '\t'), "corpo do contrato (%s)", ultimoCorpo);
    CONFERE(strstr(ultimaUrl, "/v1/atividade") != NULL, "rota /v1/atividade");
    recomenda_atividade(&a);
    recomenda_responder_alcance(0);
    CONFERE(nAtivN == 0 && recomenda_alcance() == 0, "nivel 0 limpa a fila");
    nPost = 0;
    enviarAlcance(cab);
    CONFERE(nPost == 1 && strstr(ultimoCorpo, "\"nivel\":0"), "nivel 0 vai ao servidor");
    { char *b = dados_ler(REC_ARQ_ALCANCE);
      CONFERE(b && atoi(b) == 0, "resposta gravada no aparelho"); free(b); } }

  // --- reconciliacao com /v1/eu -------------------------------------------------------
  SDL_LockMutex(mtx);
  alcance = REC_ALCANCE_NAO_PERGUNTADO; alcancePendente = -2;
  socNovoRegistrado("Amigo #12", "", 2);
  CONFERE(alcance == 2 && alcancePendente == -2, "nao perguntado adota o do servidor");
  CONFERE(!strcmp(meuNome, "Amigo #12"), "meu nome vem do servidor");
  alcance = 1;
  socNovoRegistrado("X", "", 0);
  CONFERE(alcancePendente == 1, "resposta daqui vence e e reenviada");
  SDL_UnlockMutex(mtx);

  // --- feed: rede, 304 e disco ---------------------------------------------------------
  respCorpo = FEED; respStatus = 200; snprintf(respEtag, sizeof respEtag, "\"f-11-3\"");
  lerFeedNovo(cab, 1);
  CONFERE(recomenda_feed_n() == 3, "feed lido da rede (%d)", recomenda_feed_n());
  CONFERE(strstr(ultimaUrl, "/v1/feed") != NULL, "rota /v1/feed");
  respStatus = 304;
  lerFeedNovo(cab, 1);
  CONFERE(!strcmp(ultimoIfNone, "\"f-11-3\""), "manda o etag (%s)", ultimoIfNone);
  CONFERE(recomenda_feed_n() == 3, "304 mantem a lista");
  SDL_LockMutex(mtx); nFeedN = 0; etagFeed[0] = 0; socNovoCarregar(); SDL_UnlockMutex(mtx);
  CONFERE(recomenda_feed_n() == 3 && !strcmp(etagFeed, "\"f-11-3\""), "feed volta do disco no arranque");
  { RecEvento u[10]; CatItem t[2]; long long q[2] = { 1790974381 + 600, 0 };
    int n;
    memset(t, 0, sizeof t);
    snprintf(t[0].imdb, sizeof t[0].imdb, "tt0000007");
    snprintf(t[0].tipo, sizeof t[0].tipo, "series");
    snprintf(t[0].socialSlug, sizeof t[0].socialSlug, "nuvio:aaa");   // mesmo fato que o feed
    snprintf(t[0].socialAcao, sizeof t[0].socialAcao, "assistindo agora");
    snprintf(t[1].imdb, sizeof t[1].imdb, "tt0000099");
    snprintf(t[1].socialSlug, sizeof t[1].socialSlug, "pedrinho");
    snprintf(t[1].socialAcao, sizeof t[1].socialAcao, "assistiu");
    n = recomenda_feed_unido(u, 10, t, q, 2);
    CONFERE(n == 4, "feed unido: 3 nossos + 1 Trakt novo (veio %d)", n);
    CONFERE(u[n - 1].fonte == REC_FONTE_TRAKT && u[n - 1].quando == 0, "o do Trakt sem hora no fim"); }

  // --- perfil do amigo: rede e cache ---------------------------------------------------
  respCorpo = AMIGO; respStatus = 200;
  CONFERE(!recomenda_amigo_pedir("nuvio:a a"), "id com espaco recusado");
  CONFERE(recomenda_amigo_pedir("nuvio:aaa"), "pedido aceito");
  lerAmigo(cab);
  { static RecAmigo a;
    CONFERE(strstr(ultimaUrl, "/v1/amigo?id=nuvio:aaa") != NULL, "url do amigo (%s)", ultimaUrl);
    CONFERE(recomenda_amigo_estado() == REC_SOC_OK && recomenda_amigo(&a) && a.gostoPct == 50, "amigo lido");
    SDL_LockMutex(mtx); temAmigo = 0; amigoPedido[0] = 0; SDL_UnlockMutex(mtx);
    respStatus = 500;
    recomenda_amigo_pedir("nuvio:aaa");
    CONFERE(recomenda_amigo(&a) && !strcmp(a.nome, "Henrique"), "cache em disco aparece antes da rede");
    lerAmigo(cab);
    CONFERE(recomenda_amigo_estado() == REC_SOC_FALHA && recomenda_amigo(&a), "falha de rede mantem o cache"); }

  // --- esquecer ---------------------------------------------------------------------------
  recomenda_esquecer();
  CONFERE(recomenda_feed_n() == 0 && recomenda_alcance() == REC_ALCANCE_NAO_PERGUNTADO, "esquecer zera");
  { char *b = dados_ler(REC_ARQ_FEED); CONFERE(!b, "esquecer apaga o feed do disco"); free(b); }

  printf(falhas ? "recomenda_social: %d falhas\n" : "recomenda_social: ok\n", falhas);
  return falhas ? 1 : 0;
}
