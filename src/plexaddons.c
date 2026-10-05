// Liga o Plex como origem extra de fontes do addons.c (um arquivo a parte para
// plex.c compilar e ser testado sem arrastar o modulo de addons).
#include "addons.h"
#include "plex.h"

static int origem(const char *id, const char *tipo, int (*cancelado)(void *), void *ctx,
                  OrigemAviso aviso, void *avisoU, void *saida) {
  (void)aviso; (void)avisoU;   // one quiet lookup, no per-source progress rows
  return plex_consultar(id, tipo, cancelado, ctx, (Stream **)saida);
}

void plex_ligar_aos_addons(void) { addons_definir_origem_extra(origem, plex_casamento_ativo); }
