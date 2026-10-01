// Spotlight: a caixa de busca por cima da tela. O porque e quem abre estao em
// spotlight.h; aqui so as decisoes de forma e de custo.
//
// FORMA. Um painel so, centrado: campo no topo, teclado a esquerda, lista a
// direita. A lista e VERTICAL e agrupada (o Spotlight do Mac, o Google TV), e
// nao as fileiras horizontais da tela de Busca: numa caixa por cima de outra
// tela o que importa e chegar no titulo com poucos toques, e uma coluna com o
// melhor resultado no alto poe o provavel em UM toque (direita + OK).
//
// O TECLADO FICA. Sem ele o D-pad nao digita (ver o topo de busca.c). E o mesmo
// alfabeto de teclado_alfabeto(), num tamanho menor que o da tela de Busca.
//
// AS FONTES SAO AS QUE O APP JA TEM, nenhuma rede nova:
//   - titulos: o catalogo em memoria (as fileiras da home) + desc_buscar, a
//     mesma busca nos addons da tela de Busca;
//   - pessoas: o ELENCO dos titulos ja carregados (CatItem.elenco), com o id do
//     TMDB que abre a filmografia, e depois o /search/person do TMDB
//     (spotpessoa.h) — com debounce, um pedido quando o texto para, nunca um
//     por letra. A do elenco vem antes: e da biblioteca do dono;
//   - colecoes (pastas), canais da Live TV (lista publicada do guia),
//     catalogos (titulos das fileiras) e addons instalados.
//   Fora isso so a busca nos addons (desc_buscar) e a de pessoas vao a rede.
//
// CUSTO (60 fps na C9): remontar so roda quando o texto muda ou a resposta da
// rede chega, nunca por quadro. O desenho e um veu, um painel e no maximo
// ~12 linhas visiveis; com a entrada assentada app.c congela o fundo numa
// copia (como o painel de Salvos), e o quadro passa a ser copia + painel.
#include "spotlight.h"
#include "buscasrec.h"
#include "buscanorm.h"
#include "catalogo.h"
#include "descoberta.h"
#include "colecoes.h"
#include "guia.h"
#include "spotpessoa.h"
#include "addons.h"
#include "teclado.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "anim.h"
#include "layout.h"
#include "ajustes.h"
#include "idioma.h"
#include "posterprov.h"
#include "ponteiro.h"
#ifdef NV_ANDROID
#include "android.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Geometria (1920x1080) ----------------------------------------------------
#define SP_PX        220.0f
#define SP_PW        1480.0f
#define SP_PY        72.0f
#define SP_PH        936.0f
#define SP_RAIO      34.0f          // px; vira fracao na hora de desenhar
#define SP_PAD       44.0f
#define SP_CAMPO_Y   (SP_PY + 36.0f)
#define SP_CAMPO_H   96.0f
#define SP_CORPO_Y   (SP_CAMPO_Y + SP_CAMPO_H + 34.0f)
#define SP_RODAPE_Y  (SP_PY + SP_PH - 58.0f)
#define SP_CORPO_H   (SP_RODAPE_Y - 22.0f - SP_CORPO_Y)
#define SP_TECLA     62.0f
#define SP_TECLA_GAP 10.0f
#define SP_KB_COLS   6
#define SP_KB_PASSO  (SP_TECLA + SP_TECLA_GAP)
#define SP_KB_X      (SP_PX + SP_PAD)
#define SP_KB_W      (SP_KB_COLS * SP_TECLA + (SP_KB_COLS - 1) * SP_TECLA_GAP)
#define SP_LISTA_X   (SP_KB_X + SP_KB_W + 52.0f)
#define SP_LISTA_W   (SP_PX + SP_PW - SP_PAD - SP_LISTA_X)
#define SP_MAX_TXT   48             // = BUSCASREC_TERMO
#define SP_MAX_LIN   48
#define SP_KB_MAX_FIL 8

// --- Linhas da lista -----------------------------------------------------------
enum {
  L_CAB = 0,      // cabecalho de grupo (nao focavel)
  L_AVISO,        // "Buscando...", "Nada encontrado" (nao focavel)
  L_TOPO,         // melhor resultado
  L_TITULO,
  L_PESSOA,
  L_COLECAO,
  L_CANAL,
  L_CATALOGO,
  L_ADDON,
  L_RECENTE,
  L_LIMPAR,
};
static const float ALTURA[] = { 50, 64, 210, 92, 92, 92, 92, 80, 80, 72, 72 };

typedef struct {
  int  tipo;
  int  ref;          // indice de catalogo / pasta / canal / fileira / addon / termo
  int  ref2;         // pessoa: indice do titulo de onde ela veio
  long tmdb;
  long tituloTmdb;   // pessoa do TMDB: o titulo pelo qual a filmografia abre
  char tituloTipo[8];
  char t1[160];
  char t2[200];
  char arte[1024];
  char icone[32];
  char id[80];
  char base[600];
  char chave[96];    // identidade, para foco e entrada sobreviverem a remontagem
  float y, h;
} Linha;

static Linha lin[SP_MAX_LIN];
static int   nLin;
static float animLin[SP_MAX_LIN], entraLin[SP_MAX_LIN];

// --- Estado ----------------------------------------------------------------------
static int   aberto;
static float entrada;              // 0..1 (mola)
static int   painel;               // 0 teclado, 1 lista
static int   focoL = -1;           // linha focada (indice em lin)
static int   kbF, kbC;             // tecla focada
static char  consulta[SP_MAX_TXT];
static int   nConsulta;
static char  montada[SP_MAX_TXT];  // consulta da ultima remontagem
static int   ultimoRemoto = -1, ultimoBuscando = -1, ultimaGeracao = -1;
static unsigned ultimaGerPessoa;
static float scrollY, scrollAlvo, velY;
static float animTecla[SP_KB_MAX_FIL + 1][SP_KB_COLS];
static float animCampo;
static SpotPedido pedido;
static int   temPedido;
static int   okPress, okLongo;
static Uint32 okDesde;
static int   ouvindo;              // ditado do sistema em andamento
static int   ditadoFalhou;

// Soma do que cada alvo de busca ja devolveu para o termo corrente: a resposta
// de um addon lento chega depois da tecla e tem de aparecer sozinha.
static int remotoTotal(void) {
  int a, n = 0, nA = desc_busca_n_alvos();
  for (a = 0; a < nA; a++) n += desc_busca_alvo_n(a, consulta);
  return n + desc_busca_n(consulta) * 1000;
}

// Teclado: 36 caracteres em 6 fileiras e a fileira de baixo de comandos.
static char  kbTeclas[64][5];
static int   kbN, kbFil;
enum { K_ESPACO, K_APAGAR, K_LIMPAR, K_FALAR, K_TECLADO };
static int   kbCmd[6], kbNCmd;

static int ditadoDisponivel(void) {
#ifdef NV_ANDROID
  return 1;
#else
  return 0;
#endif
}
static int imeDisponivel(void) {
#ifdef NV_ANDROID
  return 1;
#else
  return 0;
#endif
}

static void kbMontar(void) {
  const unsigned char *p = (const unsigned char *)teclado_alfabeto();
  kbN = 0;
  while (*p && kbN < 48) {
    int len = *p < 0x80 ? 1 : (*p >= 0xF0 ? 4 : (*p >= 0xE0 ? 3 : 2)), i;
    for (i = 0; i < len; i++) kbTeclas[kbN][i] = (char)p[i];
    kbTeclas[kbN][len] = 0;
    kbN++; p += len;
  }
  kbFil = (kbN + SP_KB_COLS - 1) / SP_KB_COLS;
  kbNCmd = 0;
  kbCmd[kbNCmd++] = K_ESPACO;
  kbCmd[kbNCmd++] = K_APAGAR;
  kbCmd[kbNCmd++] = K_LIMPAR;
  if (ditadoDisponivel()) kbCmd[kbNCmd++] = K_FALAR;
  if (imeDisponivel()) kbCmd[kbNCmd++] = K_TECLADO;
}

static int kbColunas(int f) {
  if (f < kbFil) {
    int resto = kbN - f * SP_KB_COLS;
    return resto > SP_KB_COLS ? SP_KB_COLS : resto;
  }
  return kbNCmd;
}

static GfxRect teclaRect(int f, int c) {
  GfxRect r;
  r.y = SP_CORPO_Y + f * SP_KB_PASSO;
  r.h = SP_TECLA;
  if (f < kbFil) { r.x = SP_KB_X + c * SP_KB_PASSO; r.w = SP_TECLA; }
  else {
    r.w = (SP_KB_W - (kbNCmd - 1) * SP_TECLA_GAP) / (float)kbNCmd;
    r.x = SP_KB_X + c * (r.w + SP_TECLA_GAP);
  }
  return r;
}

// --- Montagem da lista --------------------------------------------------------------
static int focavel(int tipo) { return tipo != L_CAB && tipo != L_AVISO; }

static Linha *nova(int tipo) {
  Linha *l;
  if (nLin >= SP_MAX_LIN) return NULL;
  l = &lin[nLin++];
  memset(l, 0, sizeof *l);
  l->tipo = tipo;
  l->ref = l->ref2 = -1;
  return l;
}

static void cabecalho(const char *t) {
  Linha *l = nova(L_CAB);
  if (!l) return;
  snprintf(l->t1, sizeof l->t1, "%s", t);
  snprintf(l->chave, sizeof l->chave, "cab|%s", t);
}

// Quanto o titulo `t` (normalizado) casa com o alvo: igual > comeca com >
// uma palavra comeca com > contem. 0 = nao casa.
static int pontuar(const char *t, const char *alvo) {
  size_t n = strlen(alvo);
  const char *p;
  if (!strcmp(t, alvo)) return 100;
  if (!strncmp(t, alvo, n)) return 80;
  for (p = t; (p = strstr(p, alvo)) != NULL; p++)
    if (p > t && (p[-1] == ' ' || p[-1] == ':' || p[-1] == '-')) return 60;
  return strstr(t, alvo) ? 40 : 0;
}

static const char *rotuloTipo(const char *tipo) {
  if (!strcmp(tipo, "movie")) return i18n("Filme");
  if (!strcmp(tipo, "series")) return i18n("Série");
  if (!strcmp(tipo, "channel") || !strcmp(tipo, "tv")) return i18n("Canal");
  return "";
}

// "2022 · 3 temporadas · Série · ★ 8.1": o que a lista mostra sob o nome.
static void metaTitulo(const CatItem *ci, char *dst, size_t n) {
  const char *tp = rotuloTipo(ci->tipo);
  char nota[24] = "";
  if (ci->nota > 0) snprintf(nota, sizeof nota, "\xe2\x98\x85 %d.%d", ci->nota / 10, ci->nota % 10);
  snprintf(dst, n, "%s%s%s%s%s", ci->meta,
           ci->meta[0] && tp[0] ? "  \xc2\xb7  " : "", tp,
           (ci->meta[0] || tp[0]) && nota[0] ? "  \xc2\xb7  " : "", nota);
}

static const char *arteDe(const CatItem *ci, int paisagem) {
  const char *a;
  if (paisagem && ci->backdrop[0]) return ci->backdrop;
  a = posterprov_card_addon(ci->origem, ci->imdb, ci->tmdb, ci->tipo, ci->poster);
  if (a && a[0]) return a;
  return ci->backdrop[0] ? ci->backdrop : "";
}

static void linhaTitulo(int tipo, int idx) {
  const CatItem *ci = cat_item(idx);
  Linha *l;
  if (!ci || !(l = nova(tipo))) return;
  l->ref = idx;
  snprintf(l->t1, sizeof l->t1, "%s", ci->titulo);
  metaTitulo(ci, l->t2, sizeof l->t2);
  snprintf(l->arte, sizeof l->arte, "%s", arteDe(ci, tipo == L_TOPO));
  snprintf(l->chave, sizeof l->chave, "t|%s|%s", ci->imdb, ci->titulo);
}

typedef struct { int idx, pont, ordem; } Cand;
static int candCmp(const void *a, const void *b) {
  const Cand *x = a, *y = b;
  if (x->pont != y->pont) return y->pont - x->pont;
  return x->ordem - y->ordem;
}

#define SP_MAX_TIT 9
static void montarTitulos(const char *alvo) {
  Cand c[64];
  int nc = 0, r, i, ordem = 0;
  char nome[320];
  // REDE primeiro na ORDEM (o addon ja ranqueou), local depois; o placar
  // decide o resto. Remoto que nao contem o texto (o addon casou por outro
  // campo) ainda entra, abaixo de qualquer casamento de nome.
  { int a, nAlvos = desc_busca_n_alvos();
    for (a = 0; a < nAlvos && nc < 40; a++) {
      int nRem = desc_busca_alvo_n(a, consulta);
      CatItem novos[8];
      int posNovo[8], idxNovos[8], nNovos = 0;
      for (i = 0; i < nRem && i < 8 && nc < 40; i++) {
        CatItem it;
        int idx, k, dup = 0;
        if (!desc_busca_alvo_item(a, i, &it)) continue;
        idx = it.imdb[0] ? cat_indice_por_imdb(it.imdb) : -1;
        if (idx >= 0) for (k = 0; k < nc; k++) if (c[k].idx == idx) { dup = 1; break; }
        if (dup) continue;
        busca_normalizar(it.titulo, nome, sizeof nome);
        c[nc].pont = pontuar(nome, alvo);
        if (c[nc].pont < 20) c[nc].pont = 20;
        c[nc].pont += 6 - (a < 6 ? a : 6);   // primeiro addon desempata
        c[nc].ordem = ordem++;
        if (idx >= 0) c[nc++].idx = idx;
        else if (nNovos < 8) { novos[nNovos] = it; posNovo[nNovos++] = nc; c[nc++].idx = -1; }
      }
      // Os que nao estao no catalogo entram numa troca de bloco so (o porque
      // esta em refiltrar, busca.c: cat_acrescentar por item copia tudo).
      if (nNovos > 0) {
        int entraram = cat_acrescentar_lote(novos, nNovos, idxNovos);
        for (i = 0; i < nNovos; i++) c[posNovo[i]].idx = i < entraram ? idxNovos[i] : -1;
      }
    } }
  for (r = 0; r < cat_n_fileiras() && nc < 64; r++) {
    const CatFileira *cf = cat_fileira(r);
    if (!cf) break;
    for (i = 0; i < cf->n && nc < 64; i++) {
      const CatItem *ci = cat_item(cf->ini + i);
      int p, k, dup = 0;
      if (!ci || !strcmp(ci->tipo, "channel")) continue;
      busca_normalizar(ci->titulo, nome, sizeof nome);
      if (!(p = pontuar(nome, alvo))) continue;
      for (k = 0; k < nc; k++) {
        const CatItem *o = c[k].idx >= 0 ? cat_item(c[k].idx) : NULL;
        if (c[k].idx == cf->ini + i || (o && ci->imdb[0] && !strcmp(o->imdb, ci->imdb))) {
          if (p + 5 > c[k].pont) c[k].pont = p + 5;   // esta nas fileiras do dono
          dup = 1; break;
        }
      }
      if (dup) continue;
      c[nc].idx = cf->ini + i; c[nc].pont = p + 5; c[nc].ordem = ordem++; nc++;
    }
  }
  // tira os -1 (catalogo no teto) antes de ordenar
  { int w = 0; for (i = 0; i < nc; i++) if (c[i].idx >= 0) c[w++] = c[i]; nc = w; }
  qsort(c, (size_t)nc, sizeof *c, candCmp);
  // O mesmo titulo vindo de dois addons sem imdb conhecido so ganha o indice
  // no lote: fica o de placar maior (o primeiro depois da ordenacao).
  { int w = 0, k;
    for (i = 0; i < nc; i++) {
      for (k = 0; k < w; k++) if (c[k].idx == c[i].idx) break;
      if (k == w) c[w++] = c[i];
    }
    nc = w; }
  if (nc > 0) {
    cabecalho(i18n("Melhor resultado"));
    linhaTitulo(L_TOPO, c[0].idx);
  }
  if (nc > 1) {
    cabecalho(i18n("Títulos"));
    for (i = 1; i < nc && i < SP_MAX_TIT; i++) linhaTitulo(L_TITULO, c[i].idx);
  }
}

static void montarPessoas(const char *alvo) {
  long vistos[SPP_MAX + 4];
  int nv = 0, i, j, n = cat_n(), cab = 0;
  char nome[160];
  for (i = 0; i < n && nv < 4; i++) {
    const CatItem *ci = cat_item(i);
    if (!ci || ci->nElenco <= 0) continue;
    for (j = 0; j < ci->nElenco && nv < 4; j++) {
      int k, dup = 0;
      Linha *l;
      if (ci->elenco[j].tmdb <= 0 || !ci->elenco[j].nome[0]) continue;
      for (k = 0; k < nv; k++) if (vistos[k] == ci->elenco[j].tmdb) { dup = 1; break; }
      if (dup) continue;
      busca_normalizar(ci->elenco[j].nome, nome, sizeof nome);
      if (pontuar(nome, alvo) < 60) continue;   // pessoa: so inicio de nome/sobrenome
      if (!cab) { cabecalho(i18n("Pessoas")); cab = 1; }
      if (!(l = nova(L_PESSOA))) return;
      vistos[nv++] = ci->elenco[j].tmdb;
      l->ref2 = i;
      l->tmdb = ci->elenco[j].tmdb;
      snprintf(l->t1, sizeof l->t1, "%s", ci->elenco[j].nome);
      snprintf(l->t2, sizeof l->t2, i18n("Em %s"), ci->titulo);
      snprintf(l->arte, sizeof l->arte, "%s", ci->elenco[j].foto);
      snprintf(l->chave, sizeof l->chave, "p|%ld", l->tmdb);
    }
  }
  // O TMDB completa ate SPP_MAX, na ordem de popularidade dele. Sem resposta
  // ainda (debounce, rede) a lista fica com as do elenco e remonta quando a
  // resposta chega (spot_atualizar).
  { int nt = spotpessoa_n(consulta);
    for (i = 0; i < nt && nv < SPP_MAX; i++) {
      SpotPessoa sp;
      int k, dup = 0;
      Linha *l;
      if (!spotpessoa_item(consulta, i, &sp)) break;
      for (k = 0; k < nv; k++) if (vistos[k] == sp.tmdb) { dup = 1; break; }
      if (dup) continue;
      if (!cab) { cabecalho(i18n("Pessoas")); cab = 1; }
      if (!(l = nova(L_PESSOA))) return;
      vistos[nv++] = sp.tmdb;
      l->tmdb = sp.tmdb;
      l->tituloTmdb = sp.tituloTmdb;
      snprintf(l->tituloTipo, sizeof l->tituloTipo, "%s", sp.tituloTipo);
      snprintf(l->t1, sizeof l->t1, "%s", sp.nome);
      if (sp.conhecido[0]) snprintf(l->t2, sizeof l->t2, i18n("Conhecido por  %s"), sp.conhecido);
      snprintf(l->arte, sizeof l->arte, "%s", sp.foto);
      snprintf(l->chave, sizeof l->chave, "p|%ld", l->tmdb);
    } }
}

static void montarColecoes(const char *alvo) {
  int i, n = col_n(), achou = 0;
  char t[300];
  for (i = 0; i < n && achou < 3; i++) {
    const ColFolder *f = col_folder(i);
    Linha *l;
    if (!f || !f->title[0]) continue;
    busca_normalizar(f->title, t, sizeof t);
    if (!pontuar(t, alvo)) {
      busca_normalizar(f->group, t, sizeof t);
      if (!f->group[0] || pontuar(t, alvo) < 60) continue;
    }
    if (!achou) cabecalho(i18n("Coleções"));
    if (!(l = nova(L_COLECAO))) return;
    achou++;
    l->ref = i;
    snprintf(l->t1, sizeof l->t1, "%s", f->title);
    snprintf(l->t2, sizeof l->t2, "%s%s%s", i18n("Coleção"), f->group[0] ? "  \xc2\xb7  " : "", f->group);
    snprintf(l->arte, sizeof l->arte, "%s", col_capa(f) ? col_capa(f) : "");
    snprintf(l->chave, sizeof l->chave, "c|%s", f->id);
  }
}

static void montarCanais(const char *alvo) {
  int idx[4], n = guia_buscar_canais(alvo, idx, 4), i;
  if (n > 0) cabecalho(i18n("Canais ao vivo"));
  for (i = 0; i < n; i++) {
    const char *id, *nome, *logo, *cat, *base;
    Linha *l;
    if (!guia_canal_campos(idx[i], &id, &nome, &logo, &cat, &base) || !(l = nova(L_CANAL))) continue;
    l->ref = idx[i];
    snprintf(l->id, sizeof l->id, "%s", id);
    snprintf(l->t1, sizeof l->t1, "%s", nome);
    snprintf(l->t2, sizeof l->t2, "%s%s%s", i18n("Canal"), cat[0] ? "  \xc2\xb7  " : "", cat);
    snprintf(l->arte, sizeof l->arte, "%s", logo);
    snprintf(l->base, sizeof l->base, "%s", base);
    snprintf(l->chave, sizeof l->chave, "k|%.90s", id);
  }
}

static void montarCatalogos(const char *alvo) {
  int r, achou = 0, i;
  char t[300];
  for (r = 0; r < cat_n_fileiras() && achou < 3; r++) {
    const CatFileira *cf = cat_fileira(r);
    Linha *l;
    if (!cf || !cf->base[0] || !cf->catId[0]) continue;
    busca_normalizar(cf->titulo, t, sizeof t);
    if (pontuar(t, alvo) < 60) continue;
    if (!achou) cabecalho(i18n("Catálogos"));
    if (!(l = nova(L_CATALOGO))) return;
    achou++;
    l->ref = r;
    snprintf(l->t1, sizeof l->t1, "%s", cf->titulo);
    snprintf(l->t2, sizeof l->t2, "%s  \xc2\xb7  %d %s", i18n("Catálogo"), cf->n,
             i18n(cf->n == 1 ? "título" : "títulos"));
    snprintf(l->icone, sizeof l->icone, "aj_rows-3");
    snprintf(l->chave, sizeof l->chave, "f|%.90s", cf->chave);
  }
  for (i = 0, achou = 0; i < addons_n() && achou < 2; i++) {
    const char *nm = addons_nome(i);
    Linha *l;
    if (!nm || !nm[0]) continue;
    busca_normalizar(nm, t, sizeof t);
    if (pontuar(t, alvo) < 60) continue;
    if (!achou) cabecalho(i18n("Addons"));
    if (!(l = nova(L_ADDON))) return;
    achou++;
    l->ref = i;
    snprintf(l->t1, sizeof l->t1, "%s", nm);
    snprintf(l->t2, sizeof l->t2, "%s", i18n(addons_ativo(i) ? "Addon ativo" : "Addon desligado"));
    snprintf(l->icone, sizeof l->icone, "addon");
    snprintf(l->chave, sizeof l->chave, "a|%.90s", nm);
  }
}

// Campo vazio: pesquisas recentes e "Em alta" (os primeiros da primeira
// fileira de catalogo de addon — o que o dono ja ve no topo da home, e o unico
// "em alta" que existe sem uma viagem de rede).
static void montarVazio(void) {
  int i, n = buscasrec_n();
  if (n > 0) {
    cabecalho(i18n("Pesquisas recentes"));
    for (i = 0; i < n && i < 6; i++) {
      Linha *l = nova(L_RECENTE);
      if (!l) return;
      l->ref = i;
      snprintf(l->t1, sizeof l->t1, "%s", buscasrec_termo(i));
      snprintf(l->icone, sizeof l->icone, "aj_rotate-ccw-clock");
      snprintf(l->chave, sizeof l->chave, "r|%s", l->t1);
    }
    { Linha *l = nova(L_LIMPAR);
      if (l) { snprintf(l->t1, sizeof l->t1, "%s", i18n("Limpar pesquisas recentes"));
               snprintf(l->chave, sizeof l->chave, "limpar"); } }
  }
  { int r, achou = 0;
    for (r = 0; r < cat_n_fileiras() && !achou; r++) {
      const CatFileira *cf = cat_fileira(r);
      if (!cf || !cf->base[0] || cf->n <= 0 || !strcmp(cf->tipo, "channel")) continue;
      for (i = 0; i < cf->n && achou < 6; i++) {
        const CatItem *ci = cat_item(cf->ini + i);
        if (!ci || !ci->titulo[0]) continue;
        if (!achou) cabecalho(i18n("Em alta"));
        linhaTitulo(L_TITULO, cf->ini + i);
        achou++;
      }
    } }
  if (nLin == 0) {
    Linha *l = nova(L_AVISO);
    if (l) snprintf(l->t1, sizeof l->t1, "%s", i18n("Digite ou fale o nome de um filme, série, pessoa ou canal."));
  }
}

static void remontar(void) {
  char alvo[SP_MAX_TXT * 2];
  char chaveFoco[96] = "";
  static char chavesAntes[SP_MAX_LIN][96];
  float entraAntes[SP_MAX_LIN];
  int nAntes = nLin, i, j;
  if (focoL >= 0 && focoL < nLin) snprintf(chaveFoco, sizeof chaveFoco, "%s", lin[focoL].chave);
  for (i = 0; i < nLin; i++) { memcpy(chavesAntes[i], lin[i].chave, 96); entraAntes[i] = entraLin[i]; }
  snprintf(montada, sizeof montada, "%s", consulta);
  nLin = 0;
  busca_normalizar(consulta, alvo, sizeof alvo);
  // Sempre, inclusive com o campo vazio: o debounce precisa saber que o
  // texto mudou para nao disparar um termo que ja nao esta no campo.
  spotpessoa_pedir(consulta, SDL_GetTicks());
  if (busca_codepoints(alvo) < 2) montarVazio();
  else {
    desc_buscar(consulta);
    montarTitulos(alvo);
    montarPessoas(alvo);
    montarColecoes(alvo);
    montarCanais(alvo);
    montarCatalogos(alvo);
    if (desc_buscando()) {
      Linha *l = nova(L_AVISO);
      if (l) { snprintf(l->t1, sizeof l->t1, "%s", i18n("Buscando nos seus addons…"));
               snprintf(l->chave, sizeof l->chave, "aviso"); }
    } else if (nLin == 0) {
      Linha *l = nova(L_AVISO);
      if (l) { snprintf(l->t1, sizeof l->t1, i18n("Nada encontrado para “%s”."), consulta);
               snprintf(l->t2, sizeof l->t2, "%s", i18n("Confira a grafia ou tente o nome original."));
               snprintf(l->chave, sizeof l->chave, "aviso"); }
    }
  }
  // Posicoes, e quem ja estava herda foco e entrada.
  { float y = 0.0f;
    for (i = 0; i < nLin; i++) {
      lin[i].h = ALTURA[lin[i].tipo];
      if (lin[i].tipo == L_AVISO && lin[i].t2[0]) lin[i].h += 30.0f;
      if (lin[i].tipo == L_CAB && i > 0) y += 14.0f;
      lin[i].y = y; y += lin[i].h;
      entraLin[i] = 0.0f;
      for (j = 0; j < nAntes; j++)
        if (lin[i].chave[0] && !strcmp(chavesAntes[j], lin[i].chave)) { entraLin[i] = entraAntes[j]; break; }
    } }
  memset(animLin, 0, sizeof animLin);
  focoL = -1;
  if (chaveFoco[0])
    for (i = 0; i < nLin; i++) if (!strcmp(lin[i].chave, chaveFoco)) { focoL = i; break; }
  if (focoL < 0) for (i = 0; i < nLin; i++) if (focavel(lin[i].tipo)) { focoL = i; break; }
  if (focoL < 0 && painel == 1) painel = 0;
  ultimoRemoto = busca_codepoints(alvo) >= 2 ? remotoTotal() : -1;
  ultimoBuscando = desc_buscando();
  ultimaGeracao = desc_busca_geracao();
  ultimaGerPessoa = spotpessoa_geracao();
}

static int temResultados(void) {
  int i;
  for (i = 0; i < nLin; i++)
    if (focavel(lin[i].tipo) && lin[i].tipo != L_RECENTE && lin[i].tipo != L_LIMPAR) return 1;
  return 0;
}
// QUANDO A BUSCA CONTA COMO FEITA: a mesma regra da tela de Busca (busca.c,
// registrarConsulta) — abriu um resultado, fechou com resultado na tela, ou
// o ditado trouxe o texto. Nunca por letra.
static void registrar(void) {
  if (nConsulta >= 2 && temResultados()) buscasrec_registrar(consulta);
}

// --- Campo -------------------------------------------------------------------------
static void acrescentar(const char *t) {
  size_t n = strlen(t);
  if ((size_t)nConsulta + n + 1 > SP_MAX_TXT) return;
  memcpy(consulta + nConsulta, t, n);
  nConsulta += (int)n;
  consulta[nConsulta] = 0;
}
static void apagar(void) { nConsulta = (int)busca_apagar_ultimo(consulta, (size_t)nConsulta); }

void spot_texto_externo(const char *t) {
  size_t i, w = 0;
  if (!t) return;
  // Sem quebra de linha nem espaco nas pontas: o reconhecedor as vezes devolve.
  while (*t == ' ' || *t == '\n') t++;
  for (i = 0; t[i] && w + 1 < sizeof consulta; i++)
    consulta[w++] = (t[i] == '\n' || t[i] == '\t') ? ' ' : t[i];
  while (w > 0 && consulta[w - 1] == ' ') w--;
  // nao deixa meia sequencia UTF-8 no fim (o corte em 47 bytes pode cair nela)
  if (w > 0) {
    size_t k = w;
    while (k > 0 && ((unsigned char)consulta[k - 1] & 0xC0) == 0x80) k--;
    if (k > 0) {
      unsigned char c0 = (unsigned char)consulta[k - 1];
      size_t len = c0 < 0x80 ? 1 : (c0 >= 0xF0 ? 4 : (c0 >= 0xE0 ? 3 : 2));
      if (k - 1 + len > w) w = k - 1;
    }
  }
  consulta[w] = 0;
  nConsulta = (int)w;
  remontar();
}

static void ditar(void) {
  ditadoFalhou = 0;
#ifdef NV_ANDROID
  if (android_ditado_iniciar()) ouvindo = 1;
  else ditadoFalhou = 1;
#endif
}

// --- Ciclo de vida -------------------------------------------------------------------
void spot_abrir(int voz) {
  kbMontar();
  aberto = 1;
  painel = 0; kbF = 0; kbC = 0;
  nConsulta = 0; consulta[0] = 0; montada[0] = 0;
  scrollY = scrollAlvo = velY = 0.0f;
  temPedido = 0; okPress = okLongo = 0;
  ouvindo = 0; ditadoFalhou = 0;
  memset(animTecla, 0, sizeof animTecla);
  nLin = 0; focoL = -1;
  memset(entraLin, 0, sizeof entraLin);
  remontar();
  printf("[spotlight] aberto (%s)\n", voz ? "voz" : "tecla");
  fflush(stdout);
  if (voz && ditadoDisponivel()) ditar();
}

void spot_fechar(void) {
  if (!aberto) return;
  aberto = 0;
  okPress = okLongo = 0;
  ouvindo = 0;
#ifdef NV_ANDROID
  SDL_StopTextInput();
#endif
}

int spot_aberto(void)  { return aberto; }
int spot_visivel(void) { return aberto || entrada > 0.004f; }
int spot_cheio(void)   { return aberto && entrada >= 0.999f; }
const char *spot_consulta(void) { return consulta; }
int spot_n_linhas(void) { return nLin; }
int spot_linha_tipo(int i) { return i >= 0 && i < nLin ? lin[i].tipo : -1; }
const char *spot_linha_texto(int i) { return i >= 0 && i < nLin ? lin[i].t1 : ""; }
int spot_linha_focada(void) { return painel == 1 ? focoL : -1; }

int spot_pediu(SpotPedido *p) {
  if (!temPedido) return 0;
  temPedido = 0;
  if (p) *p = pedido;
  return 1;
}

static void acionar(int i) {
  Linha *l;
  if (i < 0 || i >= nLin) return;
  l = &lin[i];
  memset(&pedido, 0, sizeof pedido);
  switch (l->tipo) {
    case L_RECENTE:
      snprintf(consulta, sizeof consulta, "%s", buscasrec_termo(l->ref));
      nConsulta = (int)strlen(consulta);
      buscasrec_registrar(consulta);
      remontar();
      return;
    case L_LIMPAR:
      buscasrec_limpar();
      painel = 0;
      remontar();
      return;
    case L_TOPO: case L_TITULO:
      pedido.tipo = SPOT_TITULO; pedido.indice = l->ref; break;
    case L_PESSOA:
      pedido.tipo = SPOT_PESSOA; pedido.indice = l->ref2; pedido.tmdb = l->tmdb;
      pedido.tituloTmdb = l->tituloTmdb;
      snprintf(pedido.tituloTipo, sizeof pedido.tituloTipo, "%s", l->tituloTipo);
      snprintf(pedido.nome, sizeof pedido.nome, "%s", l->t1);
      snprintf(pedido.arte, sizeof pedido.arte, "%s", l->arte);
      break;
    case L_COLECAO:  pedido.tipo = SPOT_COLECAO;  pedido.indice = l->ref; break;
    case L_CATALOGO: pedido.tipo = SPOT_CATALOGO; pedido.indice = l->ref; break;
    case L_ADDON:    pedido.tipo = SPOT_ADDONS;   pedido.indice = l->ref; break;
    case L_CANAL:
      pedido.tipo = SPOT_CANAL;
      snprintf(pedido.id, sizeof pedido.id, "%s", l->id);
      snprintf(pedido.nome, sizeof pedido.nome, "%s", l->t1);
      snprintf(pedido.base, sizeof pedido.base, "%s", l->base);
      break;
    default: return;
  }
  registrar();
  temPedido = 1;
  spot_fechar();
}

static void removerRecente(int i) {
  if (i < 0 || i >= nLin) return;
  if (lin[i].tipo == L_RECENTE) buscasrec_remover(lin[i].ref);
  else if (lin[i].tipo == L_LIMPAR) buscasrec_limpar();
  else return;
  remontar();
}

static void moverLista(int d) {
  int i = focoL;
  if (nLin == 0) return;
  for (;;) {
    i += d;
    if (i < 0 || i >= nLin) return;
    if (focavel(lin[i].tipo)) { focoL = i; return; }
  }
}

static void entrarLista(void) {
  int i;
  if (focoL < 0 || focoL >= nLin || !focavel(lin[focoL].tipo))
    for (focoL = -1, i = 0; i < nLin; i++) if (focavel(lin[i].tipo)) { focoL = i; break; }
  if (focoL >= 0) painel = 1;
}

static void aplicarTecla(void) {
  if (kbF < kbFil) {
    int k = kbF * SP_KB_COLS + kbC;
    if (k < kbN) acrescentar(kbTeclas[k]);
  } else {
    switch (kbCmd[kbC]) {
      case K_ESPACO: if (nConsulta > 0 && consulta[nConsulta - 1] != ' ') acrescentar(" "); break;
      case K_APAGAR: apagar(); break;
      case K_LIMPAR: registrar(); nConsulta = 0; consulta[0] = 0; break;
      case K_FALAR:  ditar(); return;
      case K_TECLADO:
        // IME do sistema (Android): o texto volta como SDL_TEXTINPUT e as
        // letras ASCII tambem como KEYDOWN — os dois caminhos de spot_evento.
        SDL_StartTextInput();
        return;
      default: break;
    }
  }
  remontar();
}

static void kbMover(int dx, int dy) {
  if (dy) {
    int nf = kbF + dy;
    if (nf < 0 || nf > kbFil) return;
    // Entre a grade e a fileira de comandos o x e o que conta: a coluna da
    // tecla mais perto do centro da tecla de onde se saiu.
    { GfxRect a = teclaRect(kbF, kbC);
      float cx = a.x + a.w * 0.5f, melhor = 1e9f;
      int c, nc = kbColunas(nf), alvo = 0;
      for (c = 0; c < nc; c++) {
        GfxRect b = teclaRect(nf, c);
        float d = b.x + b.w * 0.5f - cx;
        if (d < 0) d = -d;
        if (d < melhor) { melhor = d; alvo = c; }
      }
      kbF = nf; kbC = alvo; }
    return;
  }
  if (dx < 0) { if (kbC > 0) kbC--; return; }
  if (kbC + 1 < kbColunas(kbF)) kbC++;
  else entrarLista();
}

static void focarTecla(int f, int c) { painel = 0; kbF = f; kbC = c; }
static void focarLinha(int i, int b) {
  (void)b;
  if (i >= 0 && i < nLin && focavel(lin[i].tipo)) { painel = 1; focoL = i; }
}

void spot_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto) return;
  if (e->type == SDL_TEXTINPUT) {
    // ASCII alfanumerico chega TAMBEM como KEYDOWN (tratado abaixo); o resto
    // (acentos, cirilico, pontuacao do IME) so por aqui.
    unsigned char c = (unsigned char)e->text.text[0];
    if (c >= 0x80 || (c > ' ' && c < 0x7f && !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                                              (c >= '0' && c <= '9')))) {
      acrescentar(e->text.text);
      remontar();
    }
    return;
  }
  if (e->type != SDL_KEYDOWN && e->type != SDL_KEYUP) return;
  k = e->key.keysym.sym;

  // OK numa pesquisa recente decide na SOLTURA (toque = buscar, segurar =
  // remover), como as pilulas da tela de Busca.
  if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && painel == 1 && focoL >= 0 && focoL < nLin &&
      (lin[focoL].tipo == L_RECENTE || lin[focoL].tipo == L_LIMPAR)) {
    if (e->type == SDL_KEYDOWN) {
      if (!okPress) { okPress = 1; okLongo = 0; okDesde = SDL_GetTicks(); }
    } else if (okPress) {
      okPress = 0;
      if (okLongo) okLongo = 0;
      else acionar(focoL);
    }
    return;
  }
  if (e->type != SDL_KEYDOWN) { okPress = 0; return; }

  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SPOT_TECLA_ABRIR ||
      e->key.keysym.scancode == NV_SCANCODE_BACK || e->key.keysym.scancode == NV_SCANCODE_YELLOW) {
    registrar(); spot_fechar(); return;
  }
  if (k == SPOT_TECLA_VOZ) {
    if (ditadoDisponivel()) ditar();
    else { registrar(); spot_fechar(); }
    return;
  }
  if (k == SDLK_BACKSPACE || k == SDLK_DELETE) {
    if (nConsulta > 0) { apagar(); remontar(); painel = 0; }
    else spot_fechar();
    return;
  }
  if (!(e->key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) &&
      ((k >= SDLK_a && k <= SDLK_z) || (k >= SDLK_0 && k <= SDLK_9) || k == SDLK_SPACE)) {
    if (k != SDLK_SPACE || (nConsulta && consulta[nConsulta - 1] != ' ')) {
      char um[2] = { (char)k, 0 };
      acrescentar(um);
      remontar();
    }
    painel = 0;
    return;
  }
  if (painel == 0) {
    switch (k) {
      case SDLK_LEFT:  kbMover(-1, 0); break;
      case SDLK_RIGHT: kbMover(1, 0);  break;
      case SDLK_UP:    kbMover(0, -1); break;
      case SDLK_DOWN:  kbMover(0, 1);  break;
      case SDLK_TAB:   entrarLista();  break;
      case SDLK_RETURN: case SDLK_KP_ENTER: aplicarTecla(); break;
      default: break;
    }
    return;
  }
  switch (k) {
    case SDLK_LEFT: case SDLK_TAB: painel = 0; break;
    case SDLK_UP:   moverLista(-1); break;
    case SDLK_DOWN: moverLista(1);  break;
    case SDLK_RETURN: case SDLK_KP_ENTER: acionar(focoL); break;
    default: break;
  }
}

void spot_atualizar(float dt, Uint32 agora) {
  int i, f, c;
  entrada = anim_mola(entrada, aberto ? 1.0f : 0.0f, dt, aberto ? 16.0f : 22.0f);
  if (!aberto) { if (entrada < 0.004f) entrada = 0.0f; return; }

#ifdef NV_ANDROID
  if (ouvindo) {
    char buf[256];
    int r = android_ditado_ler(buf, sizeof buf);
    if (r >= 0) {
      ouvindo = 0;
      if (r == 1 && buf[0]) {
        printf("[spotlight] ditado: %d bytes\n", (int)strlen(buf));
        fflush(stdout);
        spot_texto_externo(buf);
        registrar();
        entrarLista();
      } else ditadoFalhou = 1;
    }
  }
#endif
  // A RESPOSTA DA REDE CHEGA DEPOIS DA TECLA: remonta quando a contagem do termo
  // corrente muda ou quando a busca termina (o aviso "Buscando..." sai).
  spotpessoa_atualizar(agora);
  if (strcmp(montada, consulta)) remontar();
  else if (nConsulta >= 2) {
    int n = remotoTotal(), b = desc_buscando(), g = desc_busca_geracao();
    if (n != ultimoRemoto || b != ultimoBuscando || g != ultimaGeracao ||
        spotpessoa_geracao() != ultimaGerPessoa) remontar();
  }
  for (f = 0; f <= kbFil && f <= SP_KB_MAX_FIL; f++)
    for (c = 0; c < SP_KB_COLS; c++) {
      float alvo = (painel == 0 && f == kbF && c == kbC) ? 1.0f : 0.0f;
      animTecla[f][c] = anim_mola(animTecla[f][c], alvo, dt, NV_MOLA_FOCO);
    }
  animCampo = anim_mola(animCampo, painel == 0 ? 1.0f : 0.0f, dt, NV_MOLA_FOCO);
  for (i = 0; i < nLin; i++) {
    float alvo = (painel == 1 && i == focoL) ? 1.0f : 0.0f;
    animLin[i] = anim_mola(animLin[i], alvo, dt, NV_MOLA_FOCO);
    entraLin[i] = anim_mola(entraLin[i], 1.0f, dt, 14.0f);
  }
  if (painel == 1 && okPress && !okLongo && agora - okDesde >= NV_HOLD_MS) {
    okLongo = 1;
    removerRecente(focoL);
  }
  if (painel != 1) okPress = okLongo = 0;
  // Rolagem: so o necessario para a linha focada caber (com o cabecalho do
  // grupo dela visivel, quando ele e a linha de cima).
  if (painel == 1 && focoL >= 0) {
    float topo = lin[focoL].y, base = topo + lin[focoL].h;
    if (focoL > 0 && lin[focoL - 1].tipo == L_CAB) topo = lin[focoL - 1].y;
    if (topo - scrollAlvo < 0.0f) scrollAlvo = topo;
    if (base - scrollAlvo > SP_CORPO_H) scrollAlvo = base - SP_CORPO_H;
  } else if (painel == 0) scrollAlvo = 0.0f;
  if (scrollAlvo < 0.0f) scrollAlvo = 0.0f;
  scrollY = anim_mola2(&velY, scrollY, scrollAlvo, dt, NV_MOLA2_SCROLL);
}

// --- Desenho -----------------------------------------------------------------------
void spot_veu(void) {
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0.0f, 0.0f, 0.01f, 0.62f);
}

static void desenhaPainel(GfxRect p, float a) {
  float raio = SP_RAIO / p.h, ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  // Uma luz curta da cor do tema atras do painel, no alto: e o que o tira do
  // plano da tela de tras sem uma sombra de tela inteira.
  gfx_rect((GfxRect){ p.x - 60.0f, p.y - 50.0f, p.w + 120.0f, 420.0f }, 0, GFX_SOMBRA,
           1.0f, 0, 0, 0.5f, ar, ag, ab, 0.10f * a);
  // No vidro a folha sozinha deixa os cartazes de tras competirem com as
  // teclas (captura de 01/10): um miolo escuro a 55 % por baixo dela.
  if (ajustes_vidro()) { gfx_cor(p, raio, 0.03f, 0.032f, 0.04f, 0.55f * a); gfx_vidro_folha(p, raio, a); }
  else {
    gfx_cor(p, raio, 0.058f, 0.062f, 0.074f, 0.92f * a);
    gfx_luz_canto(p, raio, p.w * 0.18f, -40.0f, 760.0f, ar, ag, ab, 0.07f * a);
  }
  gfx_vidro_aro(p, raio, 1.5f, 1.0f, 1.0f, 1.0f, 0.10f * a);
}

static void desenhaCampo(float dy, float a, Uint32 agora) {
  GfxRect campo = { SP_PX + SP_PAD, SP_CAMPO_Y + dy, SP_PW - 2 * SP_PAD, SP_CAMPO_H };
  float ar, ag, ab, raio = 0.5f, tx;
  float lum = 0.12f + 0.03f * animCampo;
  ajustes_acento(&ar, &ag, &ab);
  if (animCampo > 0.01f)
    gfx_rect((GfxRect){ campo.x - 30, campo.y - 22, campo.w + 60, campo.h + 44 }, 0, GFX_SOMBRA,
             1.0f, 0, 0, 0.5f, ar, ag, ab, 0.20f * animCampo * a);
  if (ajustes_vidro()) gfx_vidro_painel(campo, raio, 0.7f, a);
  else gfx_cor(campo, raio, lum, lum + 0.005f, lum + 0.016f, a);
  gfx_icone((GfxRect){ campo.x + 34.0f, campo.y + (campo.h - 38.0f) * 0.5f, 38.0f, 38.0f },
            "menu_search", 0.75f, 0.76f, 0.80f, a);
  tx = campo.x + 34.0f + 38.0f + 22.0f;
  if (nConsulta) {
    TxtLinha l = txt_linha_corta(TXT_HEADLINE, consulta, 246, 247, 251, 255, campo.w - 300.0f);
    txt_desenhar_alpha(l, tx, campo.y + (campo.h - l.h) * 0.5f, a);
    tx += l.w + 6.0f;
  } else {
    const char *ph = ouvindo ? i18n("Ouvindo…") : i18n("Buscar filmes, séries, pessoas e canais");
    TxtLinha l = txt_linha(TXT_HEADLINE, ph, 255, 255, 255, 255);
    txt_desenhar_alpha(l, tx, campo.y + (campo.h - l.h) * 0.5f, 0.42f * a);
  }
  if (painel == 0 && nConsulta > 0 && (agora / 500) % 2 == 0)
    gfx_cor((GfxRect){ tx, campo.y + 24.0f, 3.0f, campo.h - 48.0f }, 0.5f, ar, ag, ab, 0.95f * a);
  // Microfone a direita do campo: aceso enquanto o ditado ouve; apagado onde
  // ha ditado; ausente onde nao ha (nao se promete o que a TV nao faz).
  if (ditadoDisponivel()) {
    float d = 60.0f, cx = campo.x + campo.w - 18.0f - d, cy = campo.y + (campo.h - d) * 0.5f;
    float pulso = ouvindo ? 0.5f + 0.5f * SDL_sinf(agora * 0.008f) : 0.0f;
    if (ouvindo) {
      gfx_rect((GfxRect){ cx - 18, cy - 18, d + 36, d + 36 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
               ar, ag, ab, (0.25f + 0.25f * pulso) * a);
      gfx_cor((GfxRect){ cx, cy, d, d }, 0.5f, ar, ag, ab, a);
    } else gfx_cor((GfxRect){ cx, cy, d, d }, 0.5f, 0.2f, 0.205f, 0.225f, a);
    { int t = ouvindo ? ajustes_tinta_foco() : 220;
      gfx_icone((GfxRect){ cx + 15, cy + 15, d - 30, d - 30 }, "aj_mic",
                t / 255.0f, t / 255.0f, t / 255.0f, a); }
  }
}

static void desenhaTeclado(float dy, float a) {
  int f, c;
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  for (f = 0; f <= kbFil; f++)
    for (c = 0; c < kbColunas(f); c++) {
      float k = animTecla[f][c], esc = 1.0f + 0.08f * k;
      GfxRect b = teclaRect(f, c), t;
      const char *s = "", *ic = NULL;
      int tom;
      b.y += dy;
      t = (GfxRect){ b.x - b.w * (esc - 1) * 0.5f, b.y - b.h * (esc - 1) * 0.5f, b.w * esc, b.h * esc };
      gfx_cor(t, 0.16f, 0.13f, 0.138f, 0.158f, 0.95f * a);
      if (k > 0.01f) {
        gfx_rect((GfxRect){ t.x - 12, t.y - 12, t.w + 24, t.h + 24 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
                 ar, ag, ab, 0.28f * k * a);
        gfx_cor(t, 0.16f, ar, ag, ab, k * a);
      }
      if (ponteiro_ativo()) ponteiro_alvo(b.x, b.y, b.w, b.h, focarTecla, NULL, f, c);
      if (f < kbFil) s = kbTeclas[f * SP_KB_COLS + c];
      else switch (kbCmd[c]) {
        case K_ESPACO:  s = i18n("espaço"); break;
        case K_APAGAR:  s = i18n("apagar"); break;
        case K_LIMPAR:  s = i18n("limpar"); break;
        case K_FALAR:   ic = "aj_mic"; break;
        case K_TECLADO: ic = "aj_keyboard"; break;
      }
      tom = (int)anim_mistura(226.0f, (float)ajustes_tinta_foco(), k);
      if (ic) gfx_icone((GfxRect){ t.x + (t.w - 30) * 0.5f, t.y + (t.h - 30) * 0.5f, 30, 30 }, ic,
                        tom / 255.0f, tom / 255.0f, tom / 255.0f, a);
      else {
        TxtLinha l = txt_linha(f < kbFil ? TXT_PAINEL_ITEM : TXT_CAPTION, s, tom, tom, tom, 255);
        txt_desenhar_alpha(l, t.x + (t.w - l.w) * 0.5f, t.y + (t.h - l.h) * 0.5f, a);
      }
    }
  // Onde ha IME/ditado, uma linha diz o que as teclas de icone fazem.
  { float y = SP_CORPO_Y + dy + (kbFil + 1) * SP_KB_PASSO + 18.0f;
    if (ditadoFalhou) {
      TxtLinha l = txt_linha(TXT_CAPTION2, i18n("O ditado não respondeu. Tente de novo ou digite."),
                             235, 180, 120, 255);
      txt_desenhar_alpha(l, SP_KB_X, y, a);
    } else if (ditadoDisponivel() || imeDisponivel()) {
      TxtLinha l = txt_linha(TXT_CAPTION2, i18n("Microfone: ditado  ·  Teclado: o do sistema"),
                             150, 154, 163, 255);
      txt_desenhar_alpha(l, SP_KB_X, y, 0.9f * a);
    } }
}

// Arte de uma linha no retangulo `r`, com esqueleto enquanto nao chega.
static void arte(GfxRect r, const char *url, float raio, int circulo, float a) {
  GLuint tex = (url && url[0]) ? tex_obter_larg(url, r.w) : 0;
  if (tex) {
    if (circulo) gfx_rect(r, tex, GFX_AVATAR, 0, 0, 0, 0.0f, 0, 0, 0, a);
    else {
      gfx_tex_aspect_atual = tex_aspecto(url);
      gfx_rect(r, tex, GFX_CARD, 0.0f, 0.0f, 0.0f, raio, 0, 0, 0, a);
      gfx_tex_aspect_atual = 0.0f;
    }
  } else if (url && url[0] && !tex_falhou(url))
    gfx_esqueleto(r, circulo ? 0.5f : raio, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, a);
  else gfx_cor(r, circulo ? 0.5f : raio, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, a);
}

static void desenhaLinha(int i, float x, float y, float a) {
  Linha *l = &lin[i];
  float f = animLin[i], w = SP_LISTA_W, ar, ag, ab;
  int vidro = ajustes_vidro();
  int t1 = 246, t2 = 168;
  GfxRect r = { x, y, w, l->h - 8.0f };
  ajustes_acento(&ar, &ag, &ab);
  if (l->tipo == L_CAB) {
    TxtLinha t = txt_linha(TXT_CW_BADGE, l->t1, 150, 156, 168, 255);
    txt_desenhar_alpha(t, x + 18.0f, y + l->h - t.h - 10.0f, a);
    return;
  }
  if (l->tipo == L_AVISO) {
    TxtLinha t = txt_linha_corta(TXT_PAINEL_ITEM, l->t1, 220, 222, 228, 255, w - 36.0f);
    txt_desenhar_alpha(t, x + 18.0f, y + 16.0f, a);
    if (l->t2[0]) {
      TxtLinha s = txt_linha_corta(TXT_CAPTION, l->t2, 160, 164, 175, 255, w - 36.0f);
      txt_desenhar_alpha(s, x + 18.0f, y + 16.0f + t.h + 8.0f, a);
    }
    return;
  }
  if (ponteiro_ativo()) ponteiro_alvo(r.x, r.y, r.w, r.h, focarLinha, NULL, i, 0);
  // Foco: a linha inteira acende na cor do tema (o realce do Spotlight); no
  // vidro, a superficie clareia e ganha o contorno branco.
  if (f > 0.01f) {
    float rr = 20.0f / r.h;
    if (vidro) { gfx_vidro_painel(r, rr, 0.55f, f * a); gfx_vidro_foco(r, rr, f, a); }
    else {
      gfx_rect((GfxRect){ r.x - 14, r.y - 14, r.w + 28, r.h + 28 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
               ar, ag, ab, 0.22f * f * a);
      gfx_cor(r, rr, ar, ag, ab, f * a);
      t1 = (int)anim_mistura(246.0f, (float)ajustes_tinta_foco(), f);
      t2 = (int)anim_mistura(168.0f, (float)ajustes_tinta_foco(), f * 0.85f);
    }
  }
  if (l->tipo == L_TOPO) {
    const CatItem *ci = cat_item(l->ref);
    GfxRect art = { r.x + 14.0f, r.y + 14.0f, 0, r.h - 28.0f };
    float tx;
    art.w = art.h * 16.0f / 9.0f;
    // Sem paisagem, o cartaz em pe ocupa o mesmo lugar (e a caixa encolhe).
    if (!(ci && ci->backdrop[0])) art.w = art.h * 2.0f / 3.0f;
    arte(art, l->arte, 16.0f / art.h, 0, a);
    tx = art.x + art.w + 28.0f;
    { TxtLinha t = txt_linha_corta(TXT_ROW_TITULO, l->t1, t1, t1, t1, 255, r.x + r.w - tx - 24.0f);
      TxtLinha m = txt_linha_corta(TXT_CAPTION, l->t2, t2, t2, t2, 255, r.x + r.w - tx - 24.0f);
      float ty = r.y + 34.0f;
      txt_desenhar_alpha(t, tx, ty, a);
      txt_desenhar_alpha(m, tx, ty + t.h + 10.0f, a);
      if (ci && ci->genero[0]) {
        TxtLinha g = txt_linha_corta(TXT_CAPTION2, ci->genero, t2, t2, t2, 255, r.x + r.w - tx - 24.0f);
        txt_desenhar_alpha(g, tx, ty + t.h + 10.0f + m.h + 8.0f, 0.85f * a);
      }
      if (f > 0.5f) {
        TxtLinha o = txt_linha(TXT_CAPTION2, i18n("OK   Abrir"), t2, t2, t2, 255);
        txt_desenhar_alpha(o, tx, r.y + r.h - o.h - 22.0f, (f - 0.5f) * 2.0f * a);
      } }
    return;
  }
  { GfxRect ic = { r.x + 14.0f, r.y + 8.0f, 0, r.h - 16.0f };
    float tx;
    switch (l->tipo) {
      case L_TITULO:  ic.w = ic.h * 2.0f / 3.0f; arte(ic, l->arte, 8.0f / ic.h, 0, a); break;
      case L_PESSOA:
        ic.w = ic.h;
        if (l->arte[0]) arte(ic, l->arte, 0.5f, 1, a);
        else {
          // Sem foto no TMDB: o disco com o icone de pessoa, nao um buraco.
          gfx_cor(ic, 0.5f, 0.20f, 0.205f, 0.225f, a);
          gfx_icone((GfxRect){ ic.x + ic.w * 0.25f, ic.y + ic.h * 0.25f, ic.w * 0.5f, ic.h * 0.5f },
                    "aj_user-round", 0.78f, 0.79f, 0.82f, a);
        }
        break;
      case L_COLECAO: ic.w = ic.h * 16.0f / 9.0f; arte(ic, l->arte, 10.0f / ic.h, 0, a); break;
      case L_CANAL:
        ic.w = ic.h * 16.0f / 9.0f;
        gfx_cor(ic, 10.0f / ic.h, 0.16f, 0.165f, 0.185f, a);
        guia_logo_desenhar(l->arte, l->t1, ic, ic.w - 16.0f, ic.h - 16.0f, 0.965f, a);
        break;
      default: {
        // Icone num disco: recente, limpar, catalogo, addon.
        float d = 46.0f;
        GfxRect dc = { r.x + 18.0f, r.y + (r.h - d) * 0.5f, d, d };
        int tt = (f > 0.5f && !vidro) ? ajustes_tinta_foco() : 210;
        gfx_cor(dc, 0.5f, 1.0f, 1.0f, 1.0f, (0.08f + 0.06f * (1.0f - f)) * a);
        gfx_icone((GfxRect){ dc.x + 11, dc.y + 11, d - 22, d - 22 },
                  l->icone[0] ? l->icone : "aj_rotate-ccw-clock",
                  tt / 255.0f, tt / 255.0f, tt / 255.0f, l->icone[0] ? a : 0.6f * a);
        ic.w = d + 4.0f;
        break; }
    }
    tx = ic.x + ic.w + 24.0f;
    if (l->tipo == L_RECENTE || l->tipo == L_LIMPAR) {
      TxtLinha t = txt_linha_corta(l->tipo == L_LIMPAR ? TXT_CAPTION : TXT_PAINEL_ITEM, l->t1,
                                   t1, t1, t1, 255, r.x + r.w - tx - 24.0f);
      txt_desenhar_alpha(t, tx, r.y + (r.h - t.h) * 0.5f, l->tipo == L_LIMPAR ? 0.8f * a : a);
      // A barra da pressao longa: solte antes de encher e nao apaga.
      if (painel == 1 && i == focoL && okPress && !okLongo) {
        float p = anim_clamp((SDL_GetTicks() - okDesde) / (float)NV_HOLD_MS, 0.0f, 1.0f);
        if (p > 0.02f) gfx_cor((GfxRect){ r.x + 18.0f, r.y + r.h - 8.0f, (r.w - 36.0f) * p, 4.0f },
                               0.5f, t1 / 255.0f, t1 / 255.0f, t1 / 255.0f, 0.9f * a);
      }
      return;
    }
    { TxtLinha t = txt_linha_corta(TXT_PAINEL_ITEM, l->t1, t1, t1, t1, 255, r.x + r.w - tx - 24.0f);
      TxtLinha m = txt_linha_corta(TXT_CAPTION, l->t2, t2, t2, t2, 255, r.x + r.w - tx - 24.0f);
      float bloco = t.h + 6.0f + (l->t2[0] ? m.h : 0);
      float ty = r.y + (r.h - bloco) * 0.5f;
      txt_desenhar_alpha(t, tx, ty, a);
      if (l->t2[0]) txt_desenhar_alpha(m, tx, ty + t.h + 6.0f, a); }
  }
}

static void desenhaLista(float dy, float a) {
  int i;
  float topo = SP_CORPO_Y + dy;
  gfx_recorte(SP_LISTA_X - 30.0f, topo - 16.0f, SP_LISTA_W + 60.0f, SP_CORPO_H + 32.0f);
  for (i = 0; i < nLin; i++) {
    float y = topo + lin[i].y - scrollY;
    float e = entraLin[i];
    if (y > topo + SP_CORPO_H + 20.0f || y + lin[i].h < topo - 20.0f) continue;
    // Linha nova sobe 10 px e acende; a que ja estava fica parada.
    desenhaLinha(i, SP_LISTA_X, y + (1.0f - e) * 10.0f, a * e);
  }
  gfx_sem_recorte();
}

// Dicas do controle em pares tecla + acao, cada uma desenhada separada: a
// frase inteira numa linha so perdia os espacos entre os pares.
static float dica(float x, float y, const char *tecla, const char *acao, float a) {
  TxtLinha t = txt_linha(TXT_CAPTION2, tecla, 222, 224, 230, 255);
  TxtLinha r = txt_linha(TXT_CAPTION2, acao, 150, 154, 163, 255);
  txt_desenhar_alpha(t, x, y, a);
  txt_desenhar_alpha(r, x + t.w + 12.0f, y, a);
  return x + t.w + 12.0f + r.w + 44.0f;
}

static void desenhaRodape(float dy, float a) {
  float x = SP_PX + SP_PAD, y = SP_RODAPE_Y + dy;
  int recente = painel == 1 && focoL >= 0 && focoL < nLin && lin[focoL].tipo == L_RECENTE;
  if (painel == 0) {
    x = dica(x, y, "OK", i18n("Digitar"), a);
    x = dica(x, y, "\xe2\x86\x92", i18n("Resultados"), a);
  } else {
    x = dica(x, y, "OK", i18n(recente ? "Buscar de novo" : "Abrir"), a);
    if (recente) x = dica(x, y, i18n("Segure OK"), i18n("Remover"), a);
    x = dica(x, y, "\xe2\x86\x90", i18n("Teclado"), a);
  }
  dica(x, y, i18n("Voltar"), i18n("Fechar"), a);
}

void spot_desenhar(Uint32 agora, int veuPronto) {
  float a, dy;
  if (entrada < 0.004f) return;
  a = anim_suave(entrada);
  dy = (1.0f - a) * -24.0f;
  ponteiro_camada();
  if (!veuPronto) {
    float ga = gfx_opacidade_grupo;
    gfx_opacidade_grupo = ga * a;
    spot_veu();
    gfx_opacidade_grupo = ga;
  }
  // O PAINEL TODO e um anteparo para o ponteiro: clique fora das teclas e das
  // linhas nao vaza para a tela de tras.
  if (ponteiro_ativo()) ponteiro_alvo(SP_PX, SP_PY, SP_PW, SP_PH, NULL, NULL, 0, 0);
  desenhaPainel((GfxRect){ SP_PX, SP_PY + dy, SP_PW, SP_PH }, a);
  desenhaCampo(dy, a, agora);
  desenhaTeclado(dy, a);
  desenhaLista(dy, a);
  desenhaRodape(dy, a);
}
