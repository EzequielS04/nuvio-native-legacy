// O backend falso conta os custos que o retorno rapido deve eliminar. Nenhuma
// janela, conta ou fonte real: o estado de pausa e confirmado separadamente.
#include "player.h"
#include "catalogo.h"
#include "perfis.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int cargas, paradas, buscas, pausas, ativo, pronto, tocando, confirmado, falha, conflito;
static double posicao = 600, duracao = 3600;
static char fonte[4096], conta[96] = "fixture-conta-a";
const char *nv_test_usuario(void) { return conta; }
int video_tocar(const char *url) {
  cargas++; ativo = pronto = tocando = 1; confirmado = falha = conflito = 0;
  snprintf(fonte, sizeof fonte, "%s", url); return 1;
}
void video_parar(void) { paradas++; ativo = pronto = tocando = confirmado = 0; fonte[0] = 0; }
void video_pausar(int p) { pausas++; tocando = !p; confirmado = 0; }
void video_buscar(double p) { buscas++; posicao = p; }
int video_pausa_confirmada(void) { return confirmado && ativo && pronto && !tocando; }
int video_tocando(void) { return tocando; }
int video_pronto(void) { return pronto; }
int video_ativo(void) { return ativo; }
int video_falhou(void) { return falha; }
int video_conflito_recurso(void) { return conflito; }
double video_pos(void) { return posicao; }
double video_duracao(void) { return duracao; }
const char *video_url_atual(void) { return fonte; }

static void abrir(const char *tipo) {
  CatItem c = {0};
  snprintf(c.imdb, sizeof c.imdb, "fixture-titulo");
  snprintf(c.titulo, sizeof c.titulo, "Titulo ficticio");
  snprintf(c.tipo, sizeof c.tipo, "%s", tipo);
  cat_definir(&c, 1);
  posicao = 600; duracao = 3600;
  player_abrir(0, "https://example.invalid/fixture.mp4");
  player_atualizar(.016f, SDL_GetTicks());
}
static void guardar(void) {
  player_preparar_retencao();
  confirmado = 1; // o evento do backend, depois da intencao
  assert(player_suspender());
  assert(player_retido() && !player_aberto() && !tocando);
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  puts("player_retido: inicio");
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  perfis_definir_ativo(1);
  abrir("movie");
  player_preparar_retencao();
  assert(!player_suspender()); // tocando local=0 nao e confirmacao
  assert(!player_retido());
  confirmado = 1;
  assert(player_suspender());
  assert(cargas == 1 && paradas == 0 && buscas == 0 && !tocando);
  assert(player_retomar_retido("fixture-titulo", 0, 0));
  assert(player_aberto() && !player_retido() && tocando);
  assert(cargas == 1 && paradas == 0 && buscas == 0);
  player_atualizar(.016f, SDL_GetTicks());
  assert(buscas == 0); // nao reaplica progresso salvo na suspensao
  puts("ok retorno: 1 load total, 0 stop, 0 seek, pausa confirmada");

  guardar();
  int p = paradas;
  player_validar_retido(SDL_GetTicks() + 120000u);
  assert(!player_retido() && paradas == p + 1);
  puts("ok teto de 120 s libera pipeline");

  abrir("movie"); guardar(); p = paradas;
  perfis_definir_ativo(2); player_validar_retido(SDL_GetTicks());
  assert(!player_retido() && paradas == p + 1); perfis_definir_ativo(1);
  abrir("movie"); guardar(); p = paradas;
  snprintf(conta, sizeof conta, "fixture-conta-b"); player_validar_retido(SDL_GetTicks());
  assert(!player_retido() && paradas == p + 1);
  puts("ok troca de conta e perfil descarta");

  abrir("movie"); guardar(); p = paradas;
  assert(!player_retomar_retido("outro-titulo", 0, 0));
  assert(paradas == p + 1 && !player_retido());
  abrir("movie"); guardar(); p = paradas;
  assert(!player_retomar_retido("fixture-titulo", 1, 2));
  assert(paradas == p + 1 && !player_retido());
  puts("ok identidade e episodio diferentes usam fluxo normal");

  abrir("movie"); guardar(); p = paradas; falha = 1;
  player_validar_retido(SDL_GetTicks()); assert(paradas == p + 1 && !player_retido());
  abrir("movie"); guardar(); p = paradas; conflito = 1;
  player_validar_retido(SDL_GetTicks()); assert(paradas == p + 1 && !player_retido());
  abrir("movie"); guardar(); p = paradas;
  snprintf(fonte, sizeof fonte, "https://example.invalid/substituto.mp4");
  player_validar_retido(SDL_GetTicks()); assert(paradas == p && !player_retido());
  puts("ok erro/recurso perdido e backend substituido nao retomam");

  abrir("movie"); guardar(); p = paradas;
  player_abrir(0, "https://example.invalid/novo.mp4");
  assert(paradas == p + 1 && !player_retido()); player_encerrar();
  abrir("channel"); player_preparar_retencao(); confirmado = 1;
  assert(!player_suspender()); player_encerrar();
  abrir("movie"); duracao = 30; player_atualizar(.016f, SDL_GetTicks());
  player_preparar_retencao(); confirmado = 1;
  assert(!player_suspender()); player_encerrar();
  puts("ok novo player, canal e clipe curto seguem fechamento normal");
  SDL_Quit(); puts("player_retido: tudo ok");
}
