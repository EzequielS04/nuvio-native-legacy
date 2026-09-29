// Scrobble do Trakt (issue #179): start ao tocar, pause ao pausar/avancar, stop
// (>= 80%) ao sair, 409 conta como sucesso, sem Trakt nao sai nada, e o token
// nunca aparece no log. Rede e trakt_* sao dubles.
#include "scrobble.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ligado = 1, chamadas, proximoStatus = 201;
static char urls[16][96], corpos[16][400];

int trakt_ativo(void) { return ligado; }
int trakt_cabecalhos(const char **cab, char *aut, size_t na, char *chave, size_t nc) {
  if (!ligado) return 0;
  snprintf(aut, na, "Authorization: Bearer SEGREDO-DO-TOKEN");
  snprintf(chave, nc, "trakt-api-key: SEGREDO-DO-CLIENTE");
  cab[0] = aut; cab[1] = "trakt-api-version: 2"; cab[2] = chave; cab[3] = NULL;
  return 1;
}
char *rede_postar_st(const char *url, int s, const char *const *cab, const char *corpo, int *st) {
  (void)s; (void)cab;
  if (chamadas < 16) {
    snprintf(urls[chamadas], sizeof urls[0], "%s", url);
    snprintf(corpos[chamadas], sizeof corpos[0], "%s", corpo);
  }
  chamadas++;
  *st = proximoStatus;
  return strdup("");
}

static void esperar(void) { scrobble_drenar(); }

int main(void) {
  char b[400];
  unsigned t = 1000;

  // ---- payloads
  assert(scrobble_corpo("tt111", 12.345, b, sizeof b));
  assert(!strcmp(b, "{\"movie\":{\"ids\":{\"imdb\":\"tt111\"}},\"progress\":12.35}") ||
         !strcmp(b, "{\"movie\":{\"ids\":{\"imdb\":\"tt111\"}},\"progress\":12.34}"));
  assert(scrobble_corpo("tt222:1:2", 50, b, sizeof b));
  assert(strstr(b, "\"show\":{\"ids\":{\"imdb\":\"tt222\"}}") &&
         strstr(b, "\"episode\":{\"season\":1,\"number\":2}") && strstr(b, "\"progress\":50.00"));
  assert(scrobble_corpo("tmdb:m603", 1, b, sizeof b) && strstr(b, "\"tmdb\":603"));
  assert(scrobble_corpo("tmdb:t1399:2:5", 1, b, sizeof b) &&
         strstr(b, "\"show\":{\"ids\":{\"tmdb\":1399}}") && strstr(b, "\"season\":2"));
  assert(!scrobble_corpo("kitsu:7442:3", 1, b, sizeof b));
  assert(scrobble_corpo("tt1", 250, b, sizeof b) && strstr(b, "\"progress\":100.00"));
  assert(scrobble_corpo("tt1", -5, b, sizeof b) && strstr(b, "\"progress\":0.00"));

  // ---- start so depois de 1,2 s estavel; nada antes
  scrobble_zerar();
  scrobble_passo("tt222:1:2", 10, 2400, 1, 1, t);
  scrobble_passo("tt222:1:2", 11, 2400, 1, 1, t + 500);
  esperar(); assert(chamadas == 0);
  scrobble_passo("tt222:1:2", 12, 2400, 1, 1, t + 1300);
  esperar(); assert(chamadas == 1);
  assert(!strcmp(urls[0], "https://api.trakt.tv/scrobble/start"));
  assert(strstr(corpos[0], "\"season\":1"));

  // ---- sem repetir enquanto segue tocando
  scrobble_passo("tt222:1:2", 30, 2400, 1, 1, t + 5000);
  esperar(); assert(chamadas == 1);

  // ---- avanco: pausa curta (<1,2 s) que volta a tocar nao emite nada
  scrobble_passo("tt222:1:2", 30, 2400, 0, 1, t + 6000);
  scrobble_passo("tt222:1:2", 30, 2400, 1, 1, t + 6400);
  esperar(); assert(chamadas == 1);

  // ---- pause estavel, depois start ao retomar
  scrobble_passo("tt222:1:2", 600, 2400, 0, 1, t + 7000);
  scrobble_passo("tt222:1:2", 600, 2400, 0, 1, t + 8300);
  esperar(); assert(chamadas == 2 && strstr(urls[1], "/scrobble/pause"));
  assert(strstr(corpos[1], "\"progress\":25.00"));
  scrobble_passo("tt222:1:2", 600, 2400, 1, 1, t + 9000);
  scrobble_passo("tt222:1:2", 600, 2400, 1, 1, t + 10300);
  esperar(); assert(chamadas == 3 && strstr(urls[2], "/scrobble/start"));

  // ---- sair >= 80% -> stop; 409 e sucesso (sem segunda tentativa)
  proximoStatus = 409;
  scrobble_sair("tt222:1:2", 2000, 2400);   // 83,3%
  esperar(); assert(chamadas == 4 && strstr(urls[3], "/scrobble/stop"));

  // ---- sair < 80% -> pause
  proximoStatus = 201;
  scrobble_sair("tt111", 1000, 7200);
  esperar(); assert(chamadas == 5 && strstr(urls[4], "/scrobble/pause"));
  assert(strstr(corpos[4], "\"movie\""));

  // ---- filme: pausado antes de qualquer start nao envia
  scrobble_passo("tt333", 0, 7200, 0, 1, t + 20000);
  scrobble_passo("tt333", 0, 7200, 0, 1, t + 22000);
  esperar(); assert(chamadas == 5);
  // fluxo curto (clipe de erro) e video nao pronto nao escrobla
  scrobble_passo("tt333", 5, 30, 1, 1, t + 30000);
  scrobble_passo("tt333", 5, 30, 1, 1, t + 32000);
  scrobble_passo("tt333", 5, 7200, 1, 0, t + 34000);
  scrobble_passo("tt333", 5, 7200, 1, 0, t + 36000);
  esperar(); assert(chamadas == 5);

  // ---- id sem equivalente: nao vai para a rede
  scrobble_sair("kitsu:7442:3", 2400, 2400);
  esperar(); assert(chamadas == 5);

  // ---- Trakt desvinculado: nenhuma chamada
  ligado = 0;
  scrobble_passo("tt444", 5, 7200, 1, 1, t + 40000);
  scrobble_passo("tt444", 5, 7200, 1, 1, t + 42000);
  scrobble_sair("tt444", 7000, 7200);
  esperar(); assert(chamadas == 5);
  puts("scrobble: PASS");
  return 0;
}
