// #371 (Samsung S95C .tpk, 2.0.2): on "Profile & Stats", Left at the left edge
// opens the sidebar OVER the page (app.c: saiuPorEsquerda -> menu_abrir). The
// page used to call perfil_fechar() there, which clears `aberto`, and
// perfil_desenhar / perfil_evento both return early when !aberto: the sidebar
// opened over an empty screen and, once closed, the page stayed blank and dead.
// Left at the edge must ask for the bar WITHOUT closing the page; Back still closes.
#include "../src/perfil.c"
#include "dados.h"
#include <assert.h>

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  perfil_evento(&e);
}

static void abrir_com_dados(int nDias, int nDestaques) {
  PerfilDados d;
  memset(&d, 0, sizeof d);
  snprintf(d.nome, sizeof d.nome, "Test");
  d.plays = 5; d.nDias = nDias; d.nDestaques = nDestaques;
  for (int i = 0; i < nDestaques; i++) snprintf(d.destaques[i].titulo, sizeof d.destaques[i].titulo, "T%d", i);
  perfil_abrir();                         // opens first, as the router does; data arrives after
  perfil_definir_dados(&d);
}

int main(void) {
  dados_iniciar("deploy/app/art");

  abrir_com_dados(30, 3);                 // calendar column, first day
  tecla(SDLK_LEFT);
  assert(perfil_quer_sair());             // the router opens the bar
  assert(perfil_aberto());                // ...and the page must stay alive under it

  abrir_com_dados(0, 3);                  // no calendar: the cards column is the edge
  assert(secao == 1);
  tecla(SDLK_LEFT);
  assert(perfil_quer_sair());
  assert(perfil_aberto());

  abrir_com_dados(30, 3);                 // Back is still the way out to Home
  tecla(SDLK_AC_BACK);
  assert(perfil_quer_sair());
  assert(!perfil_aberto());

  puts("PASS: perfil_esquerda_barra");
  return 0;
}
