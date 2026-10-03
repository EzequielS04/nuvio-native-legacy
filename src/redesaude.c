// Saude da rede — ver redesaude.h.
#include "redesaude.h"
#include <pthread.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static RedeSaude global;

int rede_saude_passo(RedeSaude *s, int ok, unsigned host, unsigned agora) {
  int i;
  if (!host) return 0;
  if (ok) {
    s->falhas = 0; s->nHosts = 0;
    if (s->offline) {
      s->offline = 0; s->voltou = agora; s->voltouOk = 1; s->seq++;
      return 1;
    }
    return 0;
  }
  if (s->offline) return 0;
  if (!s->falhas) s->primeira = agora;
  s->falhas++;
  for (i = 0; i < s->nHosts; i++) if (s->hosts[i] == host) break;
  if (i == s->nHosts && s->nHosts < 4) s->hosts[s->nHosts++] = host;
  if (s->falhas >= REDE_SAUDE_FALHAS && s->nHosts >= REDE_SAUDE_HOSTS &&
      agora - s->primeira >= REDE_SAUDE_JANELA &&
      !(s->voltouOk && agora - s->voltou < REDE_SAUDE_CARENCIA)) {
    s->offline = 1; s->seq++;
    return 1;
  }
  return 0;
}

// FNV-1a do host da URL ("https://a.b/c" -> "a.b"); 0 para endereco local.
static unsigned hostHash(const char *url) {
  const char *p = url ? strstr(url, "://") : NULL, *f;
  unsigned h = 2166136261u;
  size_t n;
  if (!p) return 0;
  p += 3;
  f = p + strcspn(p, "/:?#");
  n = (size_t)(f - p);
  if (!n) return 0;
  if ((n >= 4 && !strncmp(p, "127.", 4)) || (n == 9 && !strncmp(p, "localhost", 9)) ||
      (n >= 3 && !strncmp(p, "10.", 3)) || (n >= 8 && !strncmp(p, "192.168.", 8)))
    return 0;
  for (; p < f; p++) { h ^= (unsigned char)*p; h *= 16777619u; }
  return h ? h : 1u;
}

static unsigned agoraMs(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (unsigned)((unsigned long long)t.tv_sec * 1000ull + (unsigned long long)t.tv_nsec / 1000000ull);
}

void rede_saude_nota(int codigo, const char *url) {
  int ok;
  unsigned h;
  switch (codigo) {
    case 0: ok = 1; break;
    case 6: case 7: case 28: case 35: case 52: case 55: case 56: ok = 0; break;
    default: return;   // cancelado (42), teto (23), uso errado: nao diz nada da rede
  }
  h = hostHash(url);
  if (!h) return;
  pthread_mutex_lock(&trava);
  rede_saude_passo(&global, ok, h, agoraMs());
  pthread_mutex_unlock(&trava);
}

int rede_saude_offline(void) {
  int o;
  pthread_mutex_lock(&trava);
  o = global.offline;
  pthread_mutex_unlock(&trava);
  return o;
}

unsigned rede_saude_seq(void) {
  unsigned s;
  pthread_mutex_lock(&trava);
  s = global.seq;
  pthread_mutex_unlock(&trava);
  return s;
}
