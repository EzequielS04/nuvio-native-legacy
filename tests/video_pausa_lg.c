// Compila o ramo LS2 real no host. Precarrega os headers da plataforma antes
// de selecionar Luna; chamadas do barramento e payloads sao dublados.
#include "video.h"
#include "video_escala.h"
#include "video_reconexao.h"
#include "idioma.h"
#include "linguas.h"
#include "marco.h"
#include "mkv.h"
#include "mkvass.h"
#include "js.h"
#include "lsregistro.h"
#include "rede.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <dlfcn.h>
#include <assert.h>
#undef __APPLE__
#include "../src/video.c"

static int pedidos, aceita = 1;
static int fakeCall(LSHandle *h, const char *uri, const char *carga, Filtro cb,
                    void *ctx, unsigned long *tok, void *erro) {
  (void)h; (void)uri; (void)carga; (void)cb; (void)ctx; (void)tok; (void)erro;
  pedidos++; return aceita;
}
static const char *fakePayload(LSMessage *m) { return (const char *)m; }
static void evento(const char *p, unsigned geracao) {
  aoEvento(NULL, (LSMessage *)p, (void *)(uintptr_t)geracao);
}
int main(void) {
  lsCall = fakeCall; lsPayload = fakePayload;
  ligado = pronto = tocando = 1; sessao = 7;
  snprintf(midia, sizeof midia, "fixture-media-id");
  video_pausar(1);
  assert(pedidos == 1 && !video_tocando() && !video_pausa_confirmada());
  evento("{\"paused\":true}", 6);
  assert(!video_pausa_confirmada()); // callback da sessao anterior
  evento("{\"paused\":true}", 7);
  assert(video_pausa_confirmada());
  video_pausar(0); assert(!video_pausa_confirmada());
  evento("{\"paused\":true}", 7); assert(!video_pausa_confirmada());
  evento("{\"playing\":true}", 7); assert(!video_pausa_confirmada());
  video_pausar(1); evento("{\"paused\":true}", 7);
  falhou = 1; assert(!video_pausa_confirmada()); falhou = 0;
  terminou = 1; assert(!video_pausa_confirmada()); terminou = 0;
  video_parar(); assert(!video_pausa_confirmada());
  pronto = ligado = 1; snprintf(midia, sizeof midia, "fixture-nova");
  evento("{\"paused\":true}", 7); assert(!video_pausa_confirmada());
  aceita = 0; video_pausar(1); assert(!video_pausa_confirmada());
  puts("video_pausa_lg: intencao, ack, geracao, play, erro e unload ok");
}
