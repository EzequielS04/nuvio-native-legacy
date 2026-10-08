#include "capmkv.h"
#include "mkvass.h"
#include "rede.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CAP_JANELA (320L * 1024)
#define CAP_TENTATIVAS 3
#define CAP_CACHE 4

int capmkv_espera_inicial_ms = 4000;
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static unsigned ger;                       // reproducao corrente
static double nomeado, ultimo;             // regra da LG: nome > ultimo no ultimo quarto

typedef struct { char url[512]; int n; MkvCap caps[MKV_MAX_CAPS]; } Entrada;
static Entrada cache[CAP_CACHE];
static int cacheProx;

static int urlMkvPossivel(const char *u) {
  // MP4/M4V nao tem Chapters do Matroska: nem vale a descida.
  const char *q = strpbrk(u, "?#");
  size_t n = q ? (size_t)(q - u) : strlen(u);
  return !(n > 4 && (!strncasecmp(u + n - 4, ".mp4", 4) || !strncasecmp(u + n - 4, ".m4v", 4)));
}

static int cacheLer(const char *url, MkvCap *caps, int max) {
  int i, n = -1;
  pthread_mutex_lock(&trava);
  for (i = 0; i < CAP_CACHE; i++)
    if (cache[i].url[0] && !strcmp(cache[i].url, url)) {
      n = cache[i].n < max ? cache[i].n : max;
      if (n > 0) memcpy(caps, cache[i].caps, (size_t)n * sizeof *caps);
      break;
    }
  pthread_mutex_unlock(&trava);
  return n;
}
static void cacheGuardar(const char *url, const MkvCap *caps, int n) {
  Entrada *e;
  if (strlen(url) >= sizeof cache[0].url) return;
  pthread_mutex_lock(&trava);
  e = &cache[cacheProx++ % CAP_CACHE];
  snprintf(e->url, sizeof e->url, "%s", url);
  e->n = n;
  if (n > 0) memcpy(e->caps, caps, (size_t)n * sizeof *caps);
  pthread_mutex_unlock(&trava);
}

static void dormir(int ms) {
  struct timespec t = { ms / 1000, (long)(ms % 1000) * 1000000L };
  nanosleep(&t, NULL);
}
static int vale(unsigned g) { int v; pthread_mutex_lock(&trava); v = g == ger; pthread_mutex_unlock(&trava); return v; }

int capmkv_trechos(const MkvCap *caps, int n, IntroTrecho *out, int max) {
  int k = 0;
  double ini = 0, fim = 0, cred, previa;
  if (!caps || n < 1 || !out || max < 1) return 0;
  if (mkv_intro_nomeada(caps, n, &ini, &fim) && k < max) {
    out[k].inicio = ini; out[k].fim = fim; out[k].tipo = INTRO_ABERTURA; k++;
  }
  cred = mkv_creditos_nomeados(caps, n);
  previa = mkv_previa_nomeada(caps, n);
  if (cred > 1.0 && k < max) {
    out[k].inicio = cred;
    out[k].fim = previa > cred ? previa : 0.0;      // 0 = ate o fim
    out[k].tipo = INTRO_CREDITOS; k++;
  }
  // A previa final nao e credito: so marca onde o cartao do proximo pode subir.
  if (previa > 1.0 && k < max) {
    out[k].inicio = previa; out[k].fim = 0.0; out[k].tipo = INTRO_PREVIA; k++;
  }
  return k;
}

static void alimentarIntro(const MkvCap *caps, int n) {
  IntroTrecho v[4];
  int k = capmkv_trechos(caps, n, v, 4);
  if (k > 0) intro_definir_capitulos(v, k);
}

void capmkv_aplicar(const MkvCap *caps, int n) {
  pthread_mutex_lock(&trava);
  nomeado = n > 0 ? mkv_creditos_nomeados(caps, n) : 0.0;
  ultimo = n > 0 ? mkv_creditos_ultimo(caps, n) : 0.0;
  pthread_mutex_unlock(&trava);
  if (n > 0) {
    printf("[mkv] %d capitulos; creditos nomeados em %.0fs, ultimo em %.0fs\n", n, nomeado, ultimo);
    fflush(stdout);
    alimentarIntro(caps, n);
  }
}

double capmkv_creditos(double dur) {
  double a, b;
  pthread_mutex_lock(&trava); a = nomeado; b = ultimo; pthread_mutex_unlock(&trava);
  if (a > 1.0) return a;
  if (b > 1.0 && dur > 1.0 && b > dur * 0.75) return b;
  return 0.0;
}

// Uma leitura: cabecalho da pre-busca (sem rede) ou 320 KB; Chapters fora da
// janela vai pelo SeekHead. `*recusa` = 1 quando o servidor recusou (recuar).
static int lerUma(const char *url, MkvCap *caps, int max, int *recusa) {
  unsigned char *cab = NULL;
  long cabN = 0;
  int n = 0, st = 0, proprio = 0;
  *recusa = 0;
  if (!mkvass_cabecalho(url, &cab, &cabN) || !cab) {
    cab = (unsigned char *)rede_baixar_trecho_st(url, 20, 0, CAP_JANELA - 1, &cabN, &st, NULL, NULL, 0);
    proprio = 1;
    if (!cab) { *recusa = 1; return -1; }
  }
  st = -1;
  n = mkv_capitulos_alem(url, cab, cabN, caps, max, &st);
  free(cab);
  // st == -1: nao houve pedido extra, o resultado (inclusive 0) e definitivo.
  // Pedido extra sem 2xx: transporte/CDN recusou — recuar e tentar de novo.
  if (!n && st != -1 && (st < 200 || st >= 300)) { *recusa = 1; return -1; }
  (void)proprio;
  return n;
}

typedef struct { char url[512]; unsigned g; } Pedido;

static int lerComRecuo(const char *url, MkvCap *caps, int max, unsigned g, int esperaMs) {
  static const int RECUO_MS[CAP_TENTATIVAS] = { 0, 3000, 10000 };
  int t, n;
  for (t = 0; t < CAP_TENTATIVAS; t++) {
    int recusa = 0, ms = RECUO_MS[t] + (t == 0 ? esperaMs : 0);
    while (ms > 0) { int p = ms > 250 ? 250 : ms; dormir(p); ms -= p; if (g && !vale(g)) return -2; }
    n = lerUma(url, caps, max, &recusa);
    if (n >= 0) return n;                    // inclusive "arquivo sem capitulos"
    if (g && !vale(g)) return -2;
  }
  return -1;
}

static void *fio(void *arg) {
  Pedido *p = arg;
  MkvCap caps[MKV_MAX_CAPS];
  int n = lerComRecuo(p->url, caps, MKV_MAX_CAPS, p->g, capmkv_espera_inicial_ms);
  if (n >= 0 && vale(p->g)) {
    cacheGuardar(p->url, caps, n);
    printf("[mkv] capitulos (android/tpk): %d\n", n);
    fflush(stdout);
    if (n > 0) capmkv_aplicar(caps, n);
  }
  free(p);
  return NULL;
}

void capmkv_zerar(void) {
  pthread_mutex_lock(&trava);
  ger++; nomeado = ultimo = 0.0;
  pthread_mutex_unlock(&trava);
  intro_definir_capitulos(NULL, 0);
}

void capmkv_iniciar(const char *url) {
  Pedido *p;
  pthread_t t;
  MkvCap caps[MKV_MAX_CAPS];
  int n;
  capmkv_zerar();
  if (!url || !url[0] || !urlMkvPossivel(url) || strlen(url) >= sizeof p->url) return;
  n = cacheLer(url, caps, MKV_MAX_CAPS);
  if (n >= 0) { if (n > 0) capmkv_aplicar(caps, n); return; }
  p = malloc(sizeof *p);
  if (!p) return;
  snprintf(p->url, sizeof p->url, "%s", url);
  pthread_mutex_lock(&trava); p->g = ger; pthread_mutex_unlock(&trava);
  if (pthread_create(&t, NULL, fio, p) == 0) pthread_detach(t); else free(p);
}

int capmkv_ler_agora(const char *url, MkvCap *caps, int max, int esperaMs) {
  int n = lerComRecuo(url, caps, max, 0, esperaMs);
  return n;
}
