// Cartoes da ilha — ver ilhacart.h.
#include "ilhacart.h"
#include "ilha.h"
#include "agenda.h"
#include "ajustes.h"
#include "avisos.h"
#include "catalogo.h"
#include "home.h"
#include "idioma.h"
#include <stdio.h>
#include <string.h>

#define VIVO_OCIOSO_MS (30u * 60u * 1000u)
#define ESTREIA_SONDA_MS 1000u

static IlhaCartao vivo, estreia;
static int temVivo, temEstreia;
static Uint32 ultimaTecla, ultimaSonda;
static unsigned vivoSeq;

unsigned ilhacart_vivo_seq(void) { return vivoSeq; }

// O episodio (T, E) na lista que o catalogo ja tem desse titulo: nome, sinopse
// e o still. Sem lista (serie que nunca abriu nesta sessao), fica o do titulo.
static void doEpisodio(int idx, int t, int e, IlhaCartao *c) {
  int i, n = idx >= 0 ? cat_n_episodios(idx) : 0;
  for (i = 0; i < n; i++) {
    const CatEp *ep = cat_episodio(idx, i);
    if (ep && ep->temporada == t && ep->episodio == e) {
      if (ep->nome[0]) snprintf(c->epNome, sizeof c->epNome, "%s", ep->nome);
      if (ep->sinopse[0]) snprintf(c->sinopse, sizeof c->sinopse, "%s", ep->sinopse);
      if (ep->thumb[0]) snprintf(c->arte, sizeof c->arte, "%s", ep->thumb);
      return;
    }
  }
}

static void doTitulo(const CatItem *ci, IlhaCartao *c) {
  snprintf(c->imdb, sizeof c->imdb, "%s", ci->imdb);
  snprintf(c->titulo, sizeof c->titulo, "%s", ci->titulo);
  snprintf(c->poster, sizeof c->poster, "%s", ci->poster);
  snprintf(c->logo, sizeof c->logo, "%s", ci->logo);
  c->serie = !strcmp(ci->tipo, "series");
}

void ilhacart_player_saiu(int indice, double posSeg, double durSeg, int t, int e) {
  const CatItem *ci = home_retorno_vale(indice, posSeg, durSeg) ? cat_item(indice) : NULL;
  if (!ci || !ci->imdb[0]) { temVivo = 0; ilha_cartao(ILHA_VIVO, NULL); return; }
  memset(&vivo, 0, sizeof vivo);
  doTitulo(ci, &vivo);
  if (vivo.serie && t > 0 && e > 0) {
    vivo.t = t; vivo.e = e;
    if (t == ci->temporada && e == ci->episodio && ci->nomeEpisodio[0])
      snprintf(vivo.epNome, sizeof vivo.epNome, "%s", ci->nomeEpisodio);
    doEpisodio(indice, t, e, &vivo);
  }
  if (!vivo.arte[0]) snprintf(vivo.arte, sizeof vivo.arte, "%s", ci->backdrop);
  if (!vivo.sinopse[0]) snprintf(vivo.sinopse, sizeof vivo.sinopse, "%s", ci->sinopse);
  vivo.progresso = (float)(posSeg / durSeg);
  vivo.restanteMin = (int)((durSeg - posSeg) / 60.0 + 0.5);
  snprintf(vivo.chave, sizeof vivo.chave, "vivo:%s:%d:%d", vivo.imdb, vivo.t, vivo.e);
  temVivo = 1;
  vivoSeq++;
  ultimaTecla = SDL_GetTicks();
  ilha_cartao(ILHA_VIVO, &vivo);
  printf("[ilha] atividade ao vivo: %s T%dE%d %.0f%%, faltam %d min\n", vivo.imdb, vivo.t,
         vivo.e, vivo.progresso * 100.0f, vivo.restanteMin);
}

void ilhacart_tecla(Uint32 agora) { ultimaTecla = agora; }

void ilhacart_dispensar(int qual) {
  if (qual == ILHA_VIVO) { temVivo = 0; ilha_cartao(ILHA_VIVO, NULL); return; }
  if (temEstreia) avisos_marcar_visto(estreia.avisoId);
  temEstreia = 0;
  ilha_cartao(ILHA_ESTREIA, NULL);
}

static void montarEstreia(const char *id, const char *imdb) {
  const AgItem *ag = agenda_registro(imdb);
  int idx = cat_indice_por_imdb(imdb);
  const CatItem *ci = idx >= 0 ? cat_item(idx) : NULL;
  memset(&estreia, 0, sizeof estreia);
  snprintf(estreia.avisoId, sizeof estreia.avisoId, "%s", id);
  snprintf(estreia.imdb, sizeof estreia.imdb, "%s", imdb);
  estreia.progresso = -1.0f;
  if (ci) doTitulo(ci, &estreia);
  estreia.serie = 1;
  // O registro da agenda vale ate a proxima escrita: copiado aqui, na hora.
  if (ag) {
    if (!estreia.titulo[0]) snprintf(estreia.titulo, sizeof estreia.titulo, "%s", ag->titulo);
    if (!estreia.poster[0]) snprintf(estreia.poster, sizeof estreia.poster, "%s", ag->poster);
    estreia.t = ag->temporada; estreia.e = ag->episodio;
    snprintf(estreia.epNome, sizeof estreia.epNome, "%s", ag->nomeEp);
    snprintf(estreia.sinopse, sizeof estreia.sinopse, "%s", ag->sinopse);
    if (agenda_dias(ag->dataProx) == 0)
      snprintf(estreia.quando, sizeof estreia.quando, "%s", i18n("hoje"));
  }
  if (idx >= 0 && estreia.t > 0 && estreia.e > 0) doEpisodio(idx, estreia.t, estreia.e, &estreia);
  if (!estreia.arte[0] && ci) snprintf(estreia.arte, sizeof estreia.arte, "%s", ci->backdrop);
  if (!estreia.sinopse[0] && ci) snprintf(estreia.sinopse, sizeof estreia.sinopse, "%s", ci->sinopse);
  snprintf(estreia.chave, sizeof estreia.chave, "estreia:%s", id);
}

void ilhacart_atualizar(Uint32 agora, const char *imdbAberto) {
  if (temVivo && agora - ultimaTecla > VIVO_OCIOSO_MS) {
    printf("[ilha] atividade ao vivo saiu: 30 min sem tecla\n");
    temVivo = 0;
    ilha_cartao(ILHA_VIVO, NULL);
  }
  if (ultimaSonda && agora - ultimaSonda < ESTREIA_SONDA_MS) return;
  ultimaSonda = agora ? agora : 1;
  { char id[72], imdb[64];
    int ha = ajustes_relogio_ligado() && avisos_estreia_pendente(id, sizeof id, imdb, sizeof imdb);
    // A PAGINA DO TITULO ABERTA E "VI": o aviso ja cumpriu o papel.
    if (ha && imdbAberto && !strcmp(imdbAberto, imdb)) {
      avisos_marcar_visto(id);
      ha = 0;
    }
    if (!ha) {
      if (temEstreia) { temEstreia = 0; ilha_cartao(ILHA_ESTREIA, NULL); }
      return;
    }
    // Remonta a cada sonda: o catalogo ou a lista de episodios podem ter
    // chegado depois (logo, still). A chave so muda se o aviso mudar.
    montarEstreia(id, imdb);
    temEstreia = 1;
    ilha_cartao(ILHA_ESTREIA, &estreia); }
}
