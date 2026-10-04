#include "colfileiras.h"
#include "colecoes.h"
#include "catordem.h"
#include "fileiras.h"
#include "addons.h"
#include "sessao.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int chaveFonte(const ColSource *s, char *dst) {
  if (!s || s->prov[0] || !s->type[0] || !s->catId[0]) return 0;
  const char *id = s->addonId;
  if (!id[0] && s->base[0]) {
    for (int i = 0; i < addons_n(); i++)
      if (!strcmp(addons_base(i), s->base)) { id = addons_id_manifesto(i); break; }
    if (!id || !id[0]) id = s->base;
  }
  if (!id || !id[0]) return 0;
  int n = snprintf(dst, FIL_CHAVE, "%s_%s_%s", id, s->type, s->catId);
  return n > 0 && n < FIL_CHAVE;
}

void colfileiras_contexto(void) {
  if (fil_conta_dono(sessao_usuario()) && col_tem_conta()) col_esquecer_perfil();
}

int colfileiras_receber(const char *json) {
  colfileiras_contexto();
  unsigned rev = col_revisao();
  int cap = 0, n = 0;
  if (col_tem_conta() && col_resposta_valida(json))
    for (int i = 0; i < col_n(); i++) {
      const ColFolder *f = col_folder(i);
      if (f && !f->extra) cap += f->nSources;
    }
  char (*antes)[FIL_CHAVE] = cap ? calloc((size_t)cap, FIL_CHAVE) : NULL;
  if (cap && !antes) {
    printf("[collections] account snapshot deferred: allocation failed\n");
    return 0;
  }
  if (antes) for (int i = 0; i < col_n(); i++) {
    const ColFolder *f = col_folder(i);
    if (!f || f->extra) continue;
    char grupo[FIL_CHAVE]; col_chave_pasta(f, grupo, sizeof grupo);
    // A hidden group does not represent the independently selected rows.
    if (fil_oculta(grupo)) continue;
    for (int j = 0; j < f->nSources && n < cap; j++)
      if (chaveFonte(&f->sources[j], antes[n])) n++;
  }
  int r = col_definir_json(json);
  if (antes && rev != col_revisao()) for (int i = 0; i < n; i++) {
    int existe = 0;
    for (int j = 0; j < col_n() && !existe; j++) {
      const ColFolder *f = col_folder(j);
      for (int k = 0; f && k < f->nSources && !existe; k++) {
        char chave[FIL_CHAVE];
        existe = chaveFonte(&f->sources[k], chave) && !strcmp(chave, antes[i]);
      }
    }
    for (int j = 0; !existe && j < catordem_n(); j++)
      existe = !strcmp(catordem_chave(j), antes[i]) && !catordem_oculta(antes[i], antes[i]);
    if (!existe) fil_colecao_catalogo_removido(antes[i]);
  }
  free(antes);
  return r;
}

void colfileiras_sincronizar(void) {
  colfileiras_contexto();
  char ids[COL_MAX][FIL_CHAVE];
  const char *chaves[COL_MAX], *titulos[COL_MAX];
  int ocultas[COL_MAX], n = 0;
  for (int i = 0; i < col_n() && n < COL_MAX; i++) {
    const ColFolder *f = col_folder(i);
    if (!f || !f->group[0]) continue;
    for (int j = 0; j < f->nSources; j++) {
      char chave[FIL_CHAVE];
      if (chaveFonte(&f->sources[j], chave)) fil_colecao_catalogo_restaurado(chave);
    }
    col_chave_pasta(f, ids[n], sizeof ids[n]);
    int j;
    for (j = 0; j < n; j++) if (!strcmp(ids[j], ids[n])) break;
    if (j < n) continue;
    chaves[n] = ids[n];
    titulos[n] = f->group;
    ocultas[n] = catordem_oculta(chaves[n], chaves[n]);
    n++;
  }
  fil_colecoes_reconciliar(chaves, titulos, ocultas, n, col_tem_conta());
  // Use a locked registry snapshot: discovery may be registering catalogues
  // on its worker while the account pull is applied on the drawing thread.
  static char todas[FIL_MAX][FIL_CHAVE];
  const char *conhecidas[FIL_MAX], *ordenadas[FIL_MAX];
  int ordem[FIL_MAX], fora[FIL_MAX], dentro[FIL_MAX] = {0};
  int q = fil_copiar_chaves(todas, FIL_MAX);
  for (int i = 0; i < q; i++) conhecidas[i] = todas[i];
  q = catordem_unir(conhecidas, q, ordem, FIL_MAX);
  for (int i = 0; i < q; i++) {
    ordenadas[i] = conhecidas[ordem[i]];
    fora[i] = catordem_oculta(ordenadas[i], ordenadas[i]);
    if (!fora[i]) for (int j = 0; j < catordem_n(); j++)
      if (!strcmp(ordenadas[i], catordem_chave(j)))
        fil_colecao_catalogo_restaurado(ordenadas[i]);
  }
  // Restoring an automatic exclusion does not make a wrapped source a
  // standalone Home row. Keep collection membership separate from personal
  // visibility, or these invisible sources steal catalogue quota (#233).
  for (int i = 0; i < col_n(); i++) {
    const ColFolder *f = col_folder(i);
    if (!f) continue;
    char grupo[FIL_CHAVE]; col_chave_pasta(f, grupo, sizeof grupo);
    if (fil_oculta(grupo) || catordem_oculta(grupo, grupo)) continue;
    for (int j = 0; j < f->nSources; j++) {
      char chave[FIL_CHAVE];
      if (chaveFonte(&f->sources[j], chave))
        for (int k = 0; k < q; k++)
          if (!strcmp(ordenadas[k], chave)) dentro[k] = 1;
    }
  }
  fil_conta_reconciliar(ordenadas, fora, dentro, q);
}
