#include "legauto.h"
#include "legsync.h"
#include "linguas.h"
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#define LA_TOK 24
#define LA_TOKLEN 16

// Palavras minusculas de 2+ letras/digitos ("1080p", "web", "dl", "yify").
// `s` ate o fim ou ate '?'; para o URL, so depois da ultima '/'.
static int palavras(const char *s, char t[LA_TOK][LA_TOKLEN], int ehUrl) {
  int n = 0, k = 0;
  const char *p;
  char cur[LA_TOKLEN];
  if (!s) return 0;
  if (ehUrl) {
    const char *q = strchr(s, '?'), *b = s, *x;
    size_t len = q ? (size_t)(q - s) : strlen(s);
    for (x = s; x < s + len; x++) if (*x == '/') b = x + 1;
    s = b;
    // o limite do '?' vale: copia ate ele
    for (p = s; p < s + len && *p; p++) {
      unsigned char c = (unsigned char)*p;
      if (isalnum(c)) { if (k < LA_TOKLEN - 1) cur[k++] = (char)tolower(c); }
      else { if (k >= 2 && n < LA_TOK) { cur[k] = 0; memcpy(t[n++], cur, (size_t)k + 1); } k = 0; }
    }
    if (k >= 2 && n < LA_TOK) { cur[k] = 0; memcpy(t[n++], cur, (size_t)k + 1); }
    return n;
  }
  for (p = s;; p++) {
    unsigned char c = (unsigned char)*p;
    if (c && isalnum(c)) { if (k < LA_TOKLEN - 1) cur[k++] = (char)tolower(c); }
    else {
      if (k >= 2 && n < LA_TOK) { cur[k] = 0; memcpy(t[n++], cur, (size_t)k + 1); }
      k = 0;
      if (!c) break;
    }
  }
  return n;
}

static int generica(const char *w) {
  static const char *const G[] = { "srt", "sub", "ass", "vtt", "mkv", "mp4", "avi", "www", "com", "en", "pt", "br", "por", "eng", "pob" };
  size_t i;
  for (i = 0; i < sizeof G / sizeof *G; i++) if (!strcmp(w, G[i])) return 1;
  return 0;
}

int legauto_afinidade_release(const char *arquivo, const char *midia) {
  char a[LA_TOK][LA_TOKLEN], m[LA_TOK][LA_TOKLEN];
  int na = palavras(arquivo, a, 0), nm = palavras(midia, m, strchr(midia ? midia : "", '/') != NULL), i, j, r = 0;
  for (i = 0; i < na; i++) {
    if (generica(a[i])) continue;
    for (j = 0; j < nm; j++) if (!strcmp(a[i], m[j])) { r++; break; }
  }
  return r;
}

int legauto_escolher(const Legenda *v, int n, const char *pref, const char *midia,
                     const uint64_t *excl, int nExcl, uint64_t lembrada) {
  int i, k, melhor = -1;
  long ms = -1;
  for (i = 0; i < n; i++) {
    long s;
    int af, jaFoi = 0;
    uint64_t h;
    if (!v[i].url[0]) continue;
    af = ling_afinidade(v[i].idioma, pref);
    if (af <= 0 || !v[i].idioma[0]) continue;
    h = legsync_hash_url(v[i].url);
    for (k = 0; k < nExcl; k++) if (excl[k] == h) jaFoi = 1;
    if (jaFoi) continue;
    s = (long)af * 1000 + (long)legauto_afinidade_release(v[i].arquivo, midia) * 10 + (v[i].arquivo[0] ? 1 : 0);
    if (lembrada && h == lembrada) s += 100000;
    if (s > ms) { ms = s; melhor = i; }
  }
  return melhor;
}

static struct { char chave[96]; uint64_t hash; unsigned ordem; } mem[4];
static unsigned ordemMem;

static void chave(const char *t, const char *l, char *d, size_t n) {
  char f[8];
  ling_normalizar(l, f, sizeof f);
  // So a familia ("pob", "pt-BR" e "pt" sao a mesma chave): a pessoa pede "pt",
  // a legenda aplicada diz "pob".
  snprintf(d, n, "%s|%.2s", t ? t : "", ling_selo(f));
}

void legauto_lembrar(const char *titulo, const char *idioma, uint64_t hash) {
  char k[96];
  int i, v = 0;
  if (!titulo || !*titulo || !hash) return;
  chave(titulo, idioma, k, sizeof k);
  for (i = 0; i < 4; i++) if (!strcmp(mem[i].chave, k)) { v = i; goto grava; }
  for (i = 1; i < 4; i++) if (mem[i].ordem < mem[v].ordem) v = i;
grava:
  snprintf(mem[v].chave, sizeof mem[v].chave, "%s", k);
  mem[v].hash = hash; mem[v].ordem = ++ordemMem;
}

uint64_t legauto_lembrada(const char *titulo, const char *idioma) {
  char k[96];
  int i;
  if (!titulo || !*titulo) return 0;
  chave(titulo, idioma, k, sizeof k);
  for (i = 0; i < 4; i++) if (mem[i].chave[0] && !strcmp(mem[i].chave, k)) return mem[i].hash;
  return 0;
}
