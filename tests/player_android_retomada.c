// Fluxo real do player; so o pipeline e falso. Nao abre janela, rede ou TV.
#define NV_ANDROID 1
#include "player.h"
#include "catalogo.h"
#include "perfis.h"
#include "progresso.h"
#include "linguas.h"
#include "dados.h"
#include "ajustes.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int cargas, buscas, ativo, pronto, tocando, ack, pausaAck;
static double posicao, duracao = 1800, inicio;
static char fonte[4096];
int video_tocar_posicao(const char *url, double pos) {
  cargas++; inicio = pos; posicao = pos; ativo = tocando = 1; pronto = 0; ack = 0;
  snprintf(fonte, sizeof fonte, "%s", url); return 1;
}
int video_tocar(const char *url) { return video_tocar_posicao(url, 0); }
int video_retomada_inicial_estado(void) { return ack; }
void video_parar(void) { ativo = pronto = tocando = 0; fonte[0] = 0; }
void video_pausar(int p) { tocando = !p; pausaAck = p; }
void video_buscar(double pos) { buscas++; posicao = pos; }
int video_pausa_confirmada(void) { return pausaAck && ativo && pronto && !tocando; }
int video_tocando(void) { return tocando; }
int video_pronto(void) { return pronto; }
int video_ativo(void) { return ativo; }
int video_falhou(void) { return 0; }
int video_conflito_recurso(void) { return 0; }
double video_pos(void) { return posicao; }
double video_duracao(void) { return duracao; }
const char *video_url_atual(void) { return fonte; }

static void quadro(void) { player_atualizar(.016f, SDL_GetTicks()); }
static void abrir(const char *id, const char *tipo, int pct, int t, int e) {
  CatItem c = {0};
  snprintf(c.imdb, sizeof c.imdb, "%s", id);
  snprintf(c.titulo, sizeof c.titulo, "Fixture");
  snprintf(c.tipo, sizeof c.tipo, "%s", tipo);
  c.progresso = pct; c.temporada = t; c.episodio = e;
  // De proposito: meta anuncia duracao diferente da gravada. Nao pode ser
  // usada para preparar uma posicao que a pessoa nunca salvou.
  snprintf(c.meta, sizeof c.meta, "2026 · 240 min");
  cat_definir(&c, 1);
  player_abrir(0, NULL); player_definir_episodio(t, e);
}
static void fonteAbrir(void) { player_definir_fonte("https://example.invalid/fixture.mp4"); }
int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  dados_iniciar("");
  perfis_definir_ativo(1); ling_local_legenda("none");
  assert(prog_gravar_local("fixture-filme", 0, 0, 612.345, 1800));
  abrir("fixture-filme", "movie", 34, 0, 0); fonteAbrir();
  assert(cargas == 1 && fabs(inicio - 612.345) < .0001 && buscas == 0);
  pronto = 1; quadro(); assert(buscas == 0); // ack ainda em voo
  ack = 1; quadro(); quadro(); assert(buscas == 0);
  puts("ok posicao salva chega no prepare, sem segundo seek ou uso de meta");

  // Reter e opt-in (05/10, "Manter o video pronto ao sair"): de fabrica nao retem.
  player_preparar_retencao(); assert(!player_suspender());
  { char cam[600]; const char *d = getenv("NUVIO_DADOS"); FILE *f;
    assert(d && *d);
    snprintf(cam, sizeof cam, "%s/ajustes.txt", d);
    f = fopen(cam, "w"); assert(f); fputs("manterVideoLocal 0\n", f); fclose(f);
    ajustes_dir(d); }
  player_preparar_retencao(); assert(player_suspender());
  int n = cargas;
  assert(player_retomar_retido("fixture-filme", 0, 0)); quadro();
  assert(cargas == n && buscas == 0);
  player_descartar_retido(); player_encerrar();
  puts("ok sessao retida nao recarrega nem busca");

  abrir("fixture-filme", "movie", 34, 0, 0); fonteAbrir();
  pronto = 1; ack = -1; quadro(); quadro();
  assert(buscas == 1 && fabs(posicao - 612) < .01); player_encerrar();
  puts("ok preparacao recusada recua a um seek normal");

  n = buscas;
  abrir("fixture-percentual", "movie", 34, 0, 0); fonteAbrir();
  assert(inicio == 0); pronto = 1; quadro(); quadro();
  assert(buscas == n + 1 && fabs(posicao - 612) < .01); player_encerrar();
  puts("ok percentual sozinho aguarda a duracao real");

  abrir("fixture-filme", "movie", 34, 0, 0); player_do_inicio();
  player_definir_episodio(0, 0); fonteAbrir(); assert(inicio == 0);
  n = buscas; pronto = 1; quadro(); assert(buscas == n); player_encerrar();
  puts("ok assistir do inicio persiste na definicao tardia do episodio");

  abrir("fixture-filme", "channel", 34, 0, 0); fonteAbrir();
  assert(inicio == 0); n = buscas; pronto = 1; quadro();
  assert(buscas == n); player_encerrar(); puts("ok ao vivo nao busca");

  assert(prog_gravar_local("fixture-serie", 2, 4, 612.345, 1800));
  abrir("fixture-serie", "series", 34, 2, 4);
  player_definir_episodio(2, 5); fonteAbrir(); assert(inicio == 0);
  n = buscas; pronto = 1; quadro(); assert(buscas == n); player_encerrar();
  // Sair do E5 salva um registro novo dele. Recoloca o E4 como o episodio
  // atual antes de verificar sua retomada (mesmo contrato do catalogo real).
  assert(prog_gravar_local("fixture-serie", 2, 4, 612.345, 1800));
  abrir("fixture-serie", "series", 34, 2, 4); fonteAbrir();
  assert(fabs(inicio - 612.345) < .0001); pronto = 1; ack = 1; quadro(); player_encerrar();
  puts("ok episodio salvo nao vaza para o seguinte");

  perfis_definir_ativo(2);
  abrir("fixture-filme", "movie", 34, 0, 0); fonteAbrir(); assert(inicio == 0);
  player_encerrar(); SDL_Quit();
  puts("player Android retomada: PASS (prepare, fallback, retido, live, episodio e perfil)");
}
