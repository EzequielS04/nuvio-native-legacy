#include "scrobble.h"
#include "rede.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// De trakt.h, sem puxar o SDL para dentro do teste (assinaturas identicas).
int trakt_ativo(void);
int trakt_cabecalhos(const char **cab, char *aut, size_t nAut, char *chave, size_t nChave);

#define SCR_LIMIAR_STOP 80.0   // a partir daqui o Trakt conta como assistido
#define SCR_ESTAVEL_MS  1200u
#define SCR_FILA        8

typedef struct { char acao[8]; char id[64]; double pct; } Tarefa;

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  vazia = PTHREAD_COND_INITIALIZER;
static Tarefa fila[SCR_FILA];
static int    nFila, fioVivo;

// Estado do titulo em cena (so o fio do desenho mexe: sem trava).
static char     idAtual[64];
static int      enviado;      // 0 nada enviado, 1 start no ar, 2 pause no ar
static int      candidato;    // estado desejado ainda nao confirmado (1/2), 0 nenhum
static unsigned candidatoDesde;

static double limitar(double pct) { return pct < 0 ? 0 : pct > 100 ? 100 : pct; }

// "tt123", "tt123:2:5", "tmdb:m603", "tmdb:t1399:2:5". Devolve 0 para ids que o
// Trakt nao resolve ("kitsu:", "mal:" ...): melhor nao mandar do que mandar um
// 404 que o log antigo chamava de "ok".
int scrobble_corpo(const char *id, double pct, char *dst, size_t n) {
  char base[40]; int t = 0, e = 0, k;
  const char *dp;
  if (!id || !*id || !dst || !n) return 0;
  dp = strchr(id, ':');
  if (!strncmp(id, "tmdb:", 5)) {
    const char *p = id + 5; char tp = *p; long num;
    if ((tp != 'm' && tp != 't') || !p[1]) return 0;
    num = atol(p + 1);
    if (num <= 0) return 0;
    dp = strchr(p, ':');
    if (dp) sscanf(dp + 1, "%d:%d", &t, &e);
    if (tp == 't' && !(t > 0 && e > 0)) return 0;
    if (tp == 't')
      k = snprintf(dst, n, "{\"show\":{\"ids\":{\"tmdb\":%ld}},"
                   "\"episode\":{\"season\":%d,\"number\":%d},\"progress\":%.2f}",
                   num, t, e, limitar(pct));
    else
      k = snprintf(dst, n, "{\"movie\":{\"ids\":{\"tmdb\":%ld}},\"progress\":%.2f}",
                   num, limitar(pct));
    return k > 0 && (size_t)k < n;
  }
  if (id[0] != 't' || id[1] != 't') return 0;
  { size_t l = dp ? (size_t)(dp - id) : strlen(id);
    if (l < 3 || l >= sizeof base) return 0;
    memcpy(base, id, l); base[l] = 0; }
  if (dp) sscanf(dp + 1, "%d:%d", &t, &e);
  if (t > 0 && e > 0)
    k = snprintf(dst, n, "{\"show\":{\"ids\":{\"imdb\":\"%s\"}},"
                 "\"episode\":{\"season\":%d,\"number\":%d},\"progress\":%.2f}",
                 base, t, e, limitar(pct));
  else
    k = snprintf(dst, n, "{\"movie\":{\"ids\":{\"imdb\":\"%s\"}},\"progress\":%.2f}",
                 base, limitar(pct));
  return k > 0 && (size_t)k < n;
}

static int sucesso(int st) { return (st >= 200 && st < 300) || st == 409; }

static int enviar(const Tarefa *t, int *status) {
  const char *cab[4];
  char aut[200], chave[140], corpo[400], url[64], *r;
  *status = 0;
  if (!trakt_cabecalhos(cab, aut, sizeof aut, chave, sizeof chave)) return -1;
  if (!scrobble_corpo(t->id, t->pct, corpo, sizeof corpo)) return -2;
  snprintf(url, sizeof url, "https://api.trakt.tv/scrobble/%s", t->acao);
  r = rede_postar_st(url, 20, cab, corpo, status);
  free(r);
  return 0;
}

static void *trabalhar(void *u) {
  (void)u;
  for (;;) {
    Tarefa t; int st, rc, tentativa;
    pthread_mutex_lock(&trava);
    if (nFila == 0) { fioVivo = 0; pthread_cond_broadcast(&vazia); pthread_mutex_unlock(&trava); return NULL; }
    t = fila[0]; memmove(fila, fila + 1, (size_t)(--nFila) * sizeof fila[0]);
    pthread_mutex_unlock(&trava);
    for (tentativa = 0; ; tentativa++) {
      char rot[24] = "";
      int tt = 0, ee = 0;
      const char *dp = strchr(t.id, ':');
      rc = enviar(&t, &st);
      if (dp && !strncmp(t.id, "tmdb:", 5)) dp = strchr(dp + 1, ':');
      if (dp) sscanf(dp + 1, "%d:%d", &tt, &ee);
      if (tt > 0 && ee > 0) snprintf(rot, sizeof rot, " S%dE%d", tt, ee);
      { char nome[40]; size_t l = dp ? (size_t)(dp - t.id) : strlen(t.id);
        if (l >= sizeof nome) l = sizeof nome - 1;
        memcpy(nome, t.id, l); nome[l] = 0;
        if (rc == -1)      printf("[trakt] scrobble %s %s%s ignorado: Trakt nao vinculado\n", t.acao, nome, rot);
        else if (rc == -2) printf("[trakt] scrobble %s %s%s ignorado: id sem equivalente no Trakt\n", t.acao, nome, rot);
        else               printf("[trakt] scrobble %s %s%s %.1f%% -> %d%s\n", t.acao, nome, rot, t.pct,
                                  st, st == 409 ? " (ja registrado)" : sucesso(st) ? "" : " FALHOU");
        fflush(stdout); }
      // Um /stop perdido e o episodio que nao entra no historico: uma segunda
      // tentativa depois de uma pausa cobre 401 (token sendo renovado), 429 e
      // 5xx. start/pause perdidos se corrigem sozinhos no proximo evento.
      if (rc == 0 && !sucesso(st) && tentativa == 0 && !strcmp(t.acao, "stop") &&
          (st == 0 || st == 401 || st == 429 || st >= 500)) { sleep(3); continue; }
      break;
    }
  }
}

static void enfileirar(const char *acao, const char *id, double pct) {
  pthread_mutex_lock(&trava);
  if (nFila == SCR_FILA) {   // cheia: o mais velho perdeu o sentido
    memmove(fila, fila + 1, (SCR_FILA - 1) * sizeof fila[0]); nFila--;
  }
  snprintf(fila[nFila].acao, sizeof fila[nFila].acao, "%s", acao);
  snprintf(fila[nFila].id, sizeof fila[nFila].id, "%s", id);
  fila[nFila].pct = limitar(pct);
  nFila++;
  if (!fioVivo) {
    pthread_t th;
    fioVivo = 1;
    if (pthread_create(&th, NULL, trabalhar, NULL) != 0) fioVivo = 0;
    else pthread_detach(th);
  }
  pthread_mutex_unlock(&trava);
}

void scrobble_zerar(void) { idAtual[0] = 0; enviado = 0; candidato = 0; }

void scrobble_passo(const char *id, double pos, double dur, int tocando,
                    int pronto, unsigned ms) {
  int alvo;
  if (!id || !*id || dur < 120.0 || !pronto || !trakt_ativo()) return;
  if (strcmp(idAtual, id)) { scrobble_zerar(); snprintf(idAtual, sizeof idAtual, "%s", id); }
  alvo = tocando ? 1 : 2;
  // Pausado antes de qualquer start nao diz nada ao Trakt.
  if (alvo == enviado || (alvo == 2 && enviado == 0)) { candidato = 0; return; }
  if (candidato != alvo) { candidato = alvo; candidatoDesde = ms; return; }
  if (ms - candidatoDesde < SCR_ESTAVEL_MS) return;
  enfileirar(alvo == 1 ? "start" : "pause", id, 100.0 * pos / dur);
  enviado = alvo; candidato = 0;
}

void scrobble_sair(const char *id, double pos, double dur) {
  double pct;
  if (!id || !*id || dur <= 1.0 || !trakt_ativo()) return;
  pct = limitar(100.0 * pos / dur);
  enfileirar(pct >= SCR_LIMIAR_STOP ? "stop" : "pause", id, pct);
  scrobble_zerar();
}

void scrobble_drenar(void) {
  pthread_mutex_lock(&trava);
  while (nFila > 0 || fioVivo) pthread_cond_wait(&vazia, &trava);
  pthread_mutex_unlock(&trava);
}
