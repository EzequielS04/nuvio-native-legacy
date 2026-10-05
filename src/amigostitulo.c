// Amigos de um titulo. Ver amigostitulo.h.
#include "amigostitulo.h"
#include "idioma.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#define AMT_NOTA_BOA 70    // nota de tracker (0..100) a partir da qual conta como "gostou"

static AmigosTitulo idx[AMT_TITULOS_MAX];
static int nIdx;
static unsigned rev, socialRev = ~0u;
static Uint32 conferidoEm;

static int acha(const char *imdb) {
  int i;
  for (i = 0; i < nIdx; i++) if (!strcmp(idx[i].imdb, imdb)) return i;
  return -1;
}

// Entre dois pontos do mesmo amigo no mesmo titulo, vale o mais adiantado.
static int adiante(int t1, int e1, int t2, int e2) {
  return t1 != t2 ? t1 > t2 : e1 > e2;
}

static void aplica(AmigoTit *a, const SvEvento *e) {
  switch (e->acao) {
    case SV_AGORA:
      a->viu = 1; a->agora = 1; break;
    case SV_INICIO: case SV_FIM:
      a->viu = 1; break;
    case SV_REACAO:
      // Quem reagiu ao "O que achou?" ja viu. So a reacao POSITIVA e "gostou".
      a->viu = 1;
      if (e->reacao == SV_REAC_GOSTOU) a->gostou = 1;
      break;
    case SV_AVALIOU:
      a->viu = 1;
      if (e->nota >= AMT_NOTA_BOA) a->gostou = 1;
      break;
    default: return;       // salvo, abandono, mandou, atividade: nao e "viu" nem "gostou"
  }
  if (e->temporada > 0 && e->episodio > 0 && (e->acao == SV_AGORA || e->acao == SV_INICIO || e->acao == SV_FIM) &&
      adiante(e->temporada, e->episodio, a->temporada, a->episodio)) {
    a->temporada = e->temporada; a->episodio = e->episodio;
  }
}

int amigostitulo_montar(const SvEvento *ev, int n) {
  AmigoTit tmp[AMT_MAX * 2];     // por titulo; o que passa disto e gente demais para o chip
  int i, j, k;
  nIdx = 0;
  for (i = 0; ev && i < n; i++) {
    const SvEvento *e = &ev[i];
    AmigosTitulo *t;
    int nt = 0;
    if (!e->imdb[0] || !e->pessoaId[0] || acha(e->imdb) >= 0 || nIdx >= AMT_TITULOS_MAX) continue;
    t = &idx[nIdx];
    memset(t, 0, sizeof *t);
    snprintf(t->imdb, sizeof t->imdb, "%s", e->imdb);
    // Todos os eventos deste titulo, um registro por amigo (dedupe por pessoaId).
    for (j = i; j < n; j++) {
      int p;
      if (strcmp(ev[j].imdb, e->imdb) || !ev[j].pessoaId[0]) continue;
      for (p = 0; p < nt; p++) if (!strcmp(tmp[p].id, ev[j].pessoaId)) break;
      if (p == nt) {
        if (nt >= AMT_MAX * 2) continue;
        memset(&tmp[p], 0, sizeof tmp[p]);
        snprintf(tmp[p].id, sizeof tmp[p].id, "%s", ev[j].pessoaId);
        snprintf(tmp[p].nome, sizeof tmp[p].nome, "%s", ev[j].pessoaNome);
        snprintf(tmp[p].avatar, sizeof tmp[p].avatar, "%s", ev[j].pessoaAvatar);
        nt++;
      }
      aplica(&tmp[p], &ev[j]);
    }
    // Gostou antes de so assistiu; dentro de cada grupo, a ordem de chegada (mais novo).
    for (k = 0; k < 2; k++)
      for (j = 0; j < nt; j++) {
        if (!tmp[j].viu && !tmp[j].gostou) continue;
        if ((k == 0) != (tmp[j].gostou != 0)) continue;
        t->total++;
        if (k == 0) t->nGostou++; else t->nViu++;
        if (t->n < AMT_MAX) t->a[t->n++] = tmp[j];
      }
    if (t->total > 0) nIdx++;
  }
  rev++;
  return nIdx;
}

void amigostitulo_atualizar(void) {
  Uint32 agora = SDL_GetTicks();
  unsigned sr;
  int i, n;
  if (rev && agora - conferidoEm < 250u) return;
  conferidoEm = agora;
  socialvis_atualizar();
  sr = socialvis_revisao();
  if (rev && sr == socialRev) return;
  socialRev = sr;
  {
    static SvEvento ev[SV_EVENTOS_MAX];
    n = socialvis_n_eventos();
    if (n > SV_EVENTOS_MAX) n = SV_EVENTOS_MAX;
    for (i = 0; i < n; i++) ev[i] = *socialvis_evento(i);
    amigostitulo_montar(ev, n);
  }
  // O nome e a foto do CONTATO ganham os do evento (e o que a pessoa escolheu).
  for (i = 0; i < nIdx; i++) {
    int j;
    for (j = 0; j < idx[i].n; j++) {
      int k = socialvis_amigo_indice(idx[i].a[j].id);
      const SvAmigo *am = k >= 0 ? socialvis_amigo(k) : NULL;
      if (!am) continue;
      if (am->nome[0]) snprintf(idx[i].a[j].nome, sizeof idx[i].a[j].nome, "%s", am->nome);
      if (am->avatar[0]) snprintf(idx[i].a[j].avatar, sizeof idx[i].a[j].avatar, "%s", am->avatar);
    }
  }
}

unsigned amigostitulo_revisao(void) { return rev; }

int amigostitulo_obter(const char *imdb, AmigosTitulo *saida) {
  int i;
  if (!imdb || !imdb[0]) return 0;
  i = acha(imdb);
  if (i < 0) return 0;
  if (saida) *saida = idx[i];
  return 1;
}

void amigostitulo_primeiro_nome(const char *nome, char *dst, size_t tam) {
  size_t i = 0;
  if (!tam) return;
  while (nome && nome[i] == ' ') nome++;
  while (nome && nome[i] && nome[i] != ' ' && i + 1 < tam) { dst[i] = nome[i]; i++; }
  dst[i] = 0;
  if (!dst[0]) snprintf(dst, tam, "?");
  // Minuscula inicial vira maiuscula so no ASCII (o apelido "fabi" -> "Fabi").
  if (dst[0] >= 'a' && dst[0] <= 'z') dst[0] = (char)(dst[0] - 32);
}

// "X gostou" / "X e Y gostaram" / "X, Y e mais N gostaram" (e o mesmo para assistiram).
static void grupo(const AmigoTit *a, int n, int total, int gostou, char *dst, size_t tam) {
  char p[3][64];
  int i, q = 0;
  dst[0] = 0;
  for (i = 0; i < n && q < 2; i++) {
    if ((a[i].gostou != 0) != (gostou != 0)) continue;
    amigostitulo_primeiro_nome(a[i].nome, p[q++], sizeof p[0]);
  }
  if (total <= 0 || q == 0) return;
  if (total == 1)
    snprintf(dst, tam, i18n(gostou ? "%s gostou" : "%s assistiu"), p[0]);
  else if (total == 2)
    snprintf(dst, tam, i18n(gostou ? "%s e %s gostaram" : "%s e %s assistiram"), p[0], p[1]);
  else
    snprintf(dst, tam, i18n(gostou ? "%s, %s e mais %d gostaram" : "%s, %s e mais %d assistiram"),
             p[0], p[1], total - 2);
}

void amigostitulo_linha_destaque(const AmigosTitulo *t, char *dst, size_t tam) {
  char g[160], v[160];
  if (!tam) return;
  dst[0] = 0;
  if (!t || t->total <= 0) return;
  grupo(t->a, t->n, t->nGostou, 1, g, sizeof g);
  grupo(t->a, t->n, t->nViu, 0, v, sizeof v);
  snprintf(dst, tam, "%s%s%s", g, g[0] && v[0] ? "  \xc2\xb7  " : "", v);
}

void amigostitulo_linha_ilha(const AmigosTitulo *t, char *dst, size_t tam) {
  char frase[2][128];
  int i, q = 0;
  if (!tam) return;
  dst[0] = 0;
  if (!t || t->n <= 0) return;
  for (i = 0; i < t->n && q < 2; i++) {
    const AmigoTit *a = &t->a[i];
    char nome[64];
    amigostitulo_primeiro_nome(a->nome, nome, sizeof nome);
    if (a->gostou) snprintf(frase[q], sizeof frase[q], i18n("%s gostou"), nome);
    else if (a->episodio > 0) snprintf(frase[q], sizeof frase[q], i18n("%s viu até o E%d"), nome, a->episodio);
    else snprintf(frase[q], sizeof frase[q], i18n("%s viu"), nome);
    q++;
  }
  snprintf(dst, tam, "%s%s%s", frase[0], q > 1 ? "  \xc2\xb7  " : "", q > 1 ? frase[1] : "");
  if (t->total > q) {
    size_t z = strlen(dst);
    snprintf(dst + z, tam - z, "  \xc2\xb7  +%d", t->total - q);
  }
}
