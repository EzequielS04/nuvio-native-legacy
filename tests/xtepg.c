// Grade curta do Xtream (src/xtepg.c, #158): pedido canal a canal em fio
// proprio, resultado entrando na tela so em xtepg_passo, e as consultas
// agora/proximo/faixa com a mesma semantica de epg.h. O xtream_epg_curto e
// DUBLE: conta os pedidos e responde o que o teste mandar.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "../src/xtepg.h"
#include "../src/xtream.h"

static pthread_mutex_t t = PTHREAD_MUTEX_INITIALIZER;
static int pedidos, falhar, vazio;
static time_t base;
int xtream_e_id(const char *id) { return id && !strncmp(id, "xtream:", 7); }
int xtream_epg_curto(const char *id, XtreamProg *out, int cap, int *status) {
  int n = 0;
  pthread_mutex_lock(&t); pedidos++; pthread_mutex_unlock(&t);
  if (falhar) { if (status) *status = 429; return -1; }
  if (status) *status = 200;
  if (vazio) return 0;
  assert(cap >= 3);
  out[0].ini = base - 600;  out[0].fim = base + 600;  snprintf(out[0].titulo, sizeof out[0].titulo, "Agora %s", id + 7); n++;
  out[1].ini = base + 600;  out[1].fim = base + 1800; snprintf(out[1].titulo, sizeof out[1].titulo, "Depois"); n++;
  out[2].ini = base + 1800; out[2].fim = base + 3600; snprintf(out[2].titulo, sizeof out[2].titulo, "Mais tarde"); n++;
  return n;
}

static void esperar(const char *id) {
  int k;
  for (k = 0; k < 400 && !xtepg_tem(id); k++) {
    struct timespec ts = { 0, 5000000 };
    xtepg_passo();
    nanosleep(&ts, NULL);
  }
}

int main(void) {
  EpgProg p, f[8];
  int n;
  base = time(NULL);

  xtepg_querer("stalker:1");
  xtepg_querer("tt123");
  assert(pedidos == 0);
  puts("ok  so canal Xtream e pedido");

  assert(!xtepg_agora("xtream:10", base, &p));
  xtepg_querer("xtream:10");
  xtepg_querer("xtream:10");                 // em voo: nao repete
  esperar("xtream:10");
  assert(xtepg_tem("xtream:10"));
  assert(pedidos == 1);
  assert(xtepg_agora("xtream:10", base, &p) && !strcmp(p.titulo, "Agora 10"));
  assert(xtepg_proximo("xtream:10", base, 0, &p) && !strcmp(p.titulo, "Depois"));
  assert(xtepg_proximo("xtream:10", base, 1, &p) && !strcmp(p.titulo, "Mais tarde"));
  assert(!xtepg_proximo("xtream:10", base, 2, &p));
  n = xtepg_faixa("xtream:10", base, base + 1200, f, 8);
  assert(n == 2 && !strcmp(f[1].titulo, "Depois"));
  assert(xtepg_agora("xtream:10", base + 700, &p) && !strcmp(p.titulo, "Depois"));
  puts("ok  pede uma vez, entra no passo, agora/proximo/faixa");

  xtepg_querer("xtream:10");                 // fresco: sem pedido novo
  { int k; for (k = 0; k < 20; k++) { struct timespec ts = { 0, 2000000 }; xtepg_passo(); nanosleep(&ts, NULL); } }
  assert(pedidos == 1);
  puts("ok  fresco nao pede de novo");

  falhar = 1;
  xtepg_querer("xtream:11");
  { int k; for (k = 0; k < 200 && pedidos < 2; k++) { struct timespec ts = { 0, 5000000 }; xtepg_passo(); nanosleep(&ts, NULL); } }
  { int k; for (k = 0; k < 20; k++) { struct timespec ts = { 0, 5000000 }; xtepg_passo(); nanosleep(&ts, NULL); } }
  assert(pedidos == 2 && !xtepg_tem("xtream:11") && !xtepg_agora("xtream:11", base, &p));
  xtepg_querer("xtream:11");                 // falhou ha pouco: espera
  { int k; for (k = 0; k < 20; k++) { struct timespec ts = { 0, 2000000 }; xtepg_passo(); nanosleep(&ts, NULL); } }
  assert(pedidos == 2);
  falhar = 0;
  puts("ok  falha (429) nao vira grade nem repete na hora");

  xtepg_limpar();
  assert(!xtepg_tem("xtream:10"));
  puts("ok  limpar esquece");
  puts("xtepg: tudo ok");
  return 0;
}
