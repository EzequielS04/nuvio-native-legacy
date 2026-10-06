// O grafico de temporadas da pagina de serie. Ver temporadas_grafico.h.
#include "temporadas_grafico.h"
#include "catalogo.h"
#include "vistoep.h"
#include "extras.h"
#include "svdesenho.h"
#include "notasui.h"
#include "gfx.h"
#include "text.h"
#include "layout.h"
#include "ajustes.h"
#include "idioma.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// --- MEDIDAS (1920x1080) -----------------------------------------------------
// Cabecalho no mesmo TXT_HEADLINE dos "Numeros da temporada", e dois cartoes do
// mesmo material das Notas: o grafico (com a coluna de numeros a esquerda) e,
// quando ha amigos com posicao, o cartao deles a direita.
#define TG_CAB       78.0f
#define TG_CARD_H   300.0f
#define TG_GAP       24.0f
#define TG_RAIO      30.0f
#define TG_PAD       32.0f
#define TG_AMG_W    540.0f
#define TG_STAT_W   300.0f
#define TG_SLOT_MAX 128.0f
#define TG_BARRA_MAX 56.0f
#define TG_BARRA_MIN 12.0f
#define TG_ROSTO     30.0f
#define TG_LIN_H     54.0f
#define TG_LIN_AV    42.0f
#define TG_ANIM_MS  750u
#define TG_ANIM_PASSO 45u     // atraso entre colunas vizinhas

// --- CONTA (pura) --------------------------------------------------------------

static int naoExibido(int t, int e, int agT, int agE) {
  if (agT <= 0 || agE <= 0) return 0;
  if (t != agT) return t > agT;
  return e >= agE;
}

static int depois(int t1, int e1, int t2, int e2) {
  return t1 > t2 || (t1 == t2 && e1 > e2);
}

int tgraf_coluna(const TgDados *d, int numero) {
  int i;
  if (!d) return -1;
  for (i = 0; i < d->n; i++) if (d->t[i].numero == numero) return i;
  return -1;
}

int tgraf_montar(TgDados *d, const TgEp *eps, int n, int sabe, int agT, int agE,
                 int especiais, const AmigosTitulo *amg) {
  int i, j;
  char imdb[sizeof d->imdb];
  memcpy(imdb, d->imdb, sizeof imdb);
  memset(d, 0, sizeof *d);
  memcpy(d->imdb, imdb, sizeof imdb);
  d->sabe = sabe;
  for (i = 0; i < n; i++) {
    const TgEp *e = &eps[i];
    TgTemp *t;
    int c;
    if (e->temporada < 0 || e->episodio < 1) continue;
    if (e->temporada == 0 && !especiais) continue;
    c = tgraf_coluna(d, e->temporada);
    if (c < 0) {
      if (d->n >= TG_TEMP_MAX) continue;
      // Insere na ordem (as listas do meta quase sempre ja vem ordenadas, mas
      // nada garante: o Cinemeta lista na ordem que o addon publicou).
      for (c = d->n; c > 0 && d->t[c - 1].numero > e->temporada; c--) d->t[c] = d->t[c - 1];
      memset(&d->t[c], 0, sizeof d->t[c]);
      d->t[c].numero = e->temporada;
      d->n++;
    }
    t = &d->t[c];
    t->total++;
    // Nao exibido GANHA de visto (a mesma ordem de contarTemporada em detail.c):
    // o Trakt aceita marcar o que nem estreou, e contar isso faria a coluna
    // passar de 100%.
    if (naoExibido(e->temporada, e->episodio, agT, agE)) continue;
    t->exibidos++;
    if (sabe && e->visto == 1) {
      t->vistos++;
      if (e->episodio > t->ultVisto) t->ultVisto = e->episodio;
      if (depois(e->temporada, e->episodio, d->meuT, d->meuE)) {
        d->meuT = e->temporada; d->meuE = e->episodio;
      }
    }
  }
  for (i = 0; i < d->n; i++) {
    TgTemp *t = &d->t[i];
    t->completa = t->exibidos > 0 && t->vistos >= t->exibidos;
    d->total += t->total; d->exibidos += t->exibidos; d->vistos += t->vistos;
    d->completas += t->completa;
  }
  if (!amg) return d->n;
  d->totalAmg = amg->total;
  for (i = 0; i < amg->n && d->nAmg < AMT_MAX; i++) {
    const AmigoTit *a = &amg->a[i];
    TgAmigo *g;
    // Sem posicao (filme, ou so reagiu) nao entra: o grafico e de onde cada
    // um esta. Especial so com especiais no grafico.
    if (a->temporada < 0 || a->episodio <= 0) continue;
    if (a->temporada == 0 && !especiais) continue;
    g = &d->amg[d->nAmg++];
    memset(g, 0, sizeof *g);
    snprintf(g->id, sizeof g->id, "%s", a->id);
    snprintf(g->nome, sizeof g->nome, "%s", a->nome);
    snprintf(g->avatar, sizeof g->avatar, "%s", a->avatar);
    g->temporada = a->temporada; g->episodio = a->episodio;
    g->reacao = a->reacao; g->nota = a->nota; g->agora = a->agora;
    // A FRENTE so quando sabemos onde VOCE esta: sem mapa nao ha "sua frente".
    g->frente = sabe && depois(a->temporada, a->episodio, d->meuT, d->meuE);
    g->col = tgraf_coluna(d, a->temporada);
  }
  // Ordem: os da frente primeiro; dentro de cada grupo, o mais adiantado antes.
  // Insercao estavel (AMT_MAX e 8): empate fica na ordem de amigostitulo.
  for (i = 1; i < d->nAmg; i++) {
    TgAmigo x = d->amg[i];
    for (j = i; j > 0; j--) {
      const TgAmigo *p = &d->amg[j - 1];
      int antes = x.frente > p->frente ||
                  (x.frente == p->frente && depois(x.temporada, x.episodio, p->temporada, p->episodio));
      if (!antes) break;
      d->amg[j] = d->amg[j - 1];
    }
    d->amg[j] = x;
  }
  for (i = 0; i < d->nAmg; i++) { if (d->amg[i].frente) d->nFrente++; else d->nAtras++; }
  return d->n;
}

int tgraf_existe(const TgDados *d) {
  return d && d->n > 0 && ((d->sabe && d->vistos > 0) || d->nAmg > 0);
}

void tgraf_frase_amigos(const TgDados *d, char *dst, size_t tam) {
  char n1[64], n2[64];
  dst[0] = 0;
  if (!d || d->nAmg <= 0) return;
  if (d->nFrente > 0) {
    amigostitulo_primeiro_nome(d->amg[0].nome, n1, sizeof n1);
    if (d->nFrente == 1) snprintf(dst, tam, i18n("%s está na sua frente"), n1);
    else if (d->nFrente == 2) {
      amigostitulo_primeiro_nome(d->amg[1].nome, n2, sizeof n2);
      snprintf(dst, tam, i18n("%s e %s estão na sua frente"), n1, n2);
    } else snprintf(dst, tam, i18n("%s e mais %d estão na sua frente"), n1, d->nFrente - 1);
    return;
  }
  // Ninguem na frente: com mapa, voce lidera; sem mapa, so onde eles estao.
  if (!d->sabe) { snprintf(dst, tam, "%s", i18n("Onde seus amigos estão")); return; }
  amigostitulo_primeiro_nome(d->amg[0].nome, n1, sizeof n1);
  if (d->nAtras == 1) snprintf(dst, tam, i18n("Você está na frente de %s"), n1);
  else snprintf(dst, tam, i18n("Você está na frente de %d amigos"), d->nAtras);
}

// --- DADOS DA PAGINA (com revisao) ------------------------------------------

static TgDados dados;
static int     dIdx = -1;
static unsigned dCat, dVisto, dAmg;
static int     dAgT, dAgE, dNEps, dValido;
static TgEp    epsBuf[1024];

const TgDados *tgraf_dados(int idx) {
  const CatItem *ci = cat_item(idx);
  int agT = extras_agenda_temporada(), agE = extras_agenda_episodio();
  int n = cat_n_episodios(idx), i, k = 0, sabe;
  AmigosTitulo at;
  int temAmg;
  if (!ci || !ci->imdb[0] || n < 1) {
    memset(&dados, 0, sizeof dados);
    dValido = 0;
    return &dados;
  }
  if (dValido && idx == dIdx && !strncmp(dados.imdb, ci->imdb, sizeof dados.imdb - 1) &&
      dCat == cat_revisao() && dVisto == vistoep_revisao() &&
      dAmg == amigostitulo_revisao() && dAgT == agT && dAgE == agE && dNEps == n)
    return &dados;
  sabe = vistoep_conhecido(ci->imdb);
  for (i = 0; i < n && k < (int)(sizeof epsBuf / sizeof epsBuf[0]); i++) {
    const CatEp *e = cat_episodio(idx, i);
    if (!e) continue;
    epsBuf[k].temporada = (short)e->temporada;
    epsBuf[k].episodio = (short)e->episodio;
    epsBuf[k].visto = (signed char)(sabe ? vistoep_estado(ci->imdb, e->temporada, e->episodio) : -1);
    k++;
  }
  temAmg = amigostitulo_obter(ci->imdb, &at);
  snprintf(dados.imdb, sizeof dados.imdb, "%s", ci->imdb);
  { char *dp = strchr(dados.imdb, ':'); if (dp) *dp = 0; }
  tgraf_montar(&dados, epsBuf, k, sabe, agT, agE, 0, temAmg ? &at : NULL);
  dIdx = idx; dCat = cat_revisao(); dVisto = vistoep_revisao();
  dAmg = amigostitulo_revisao(); dAgT = agT; dAgE = agE; dNEps = n; dValido = 1;
  return &dados;
}

// --- DESENHO -----------------------------------------------------------------

static char animImdb[24];
static Uint32 animIni;

void tgraf_reiniciar(void) { animImdb[0] = 0; animIni = 0; }

float tgraf_altura(void) { return TG_CAB + TG_CARD_H; }

// raio em PIXELS -> o normalizado pela altura do gfx (teto: pilula).
static float raioPx(GfxRect r, float px) {
  float m = r.w < r.h ? r.w : r.h;
  if (px > m * 0.5f) px = m * 0.5f;
  return r.h > 0.0f ? px / r.h : 0.0f;
}

static float suaveSaida(float x) {
  if (x <= 0.0f) return 0.0f;
  if (x >= 1.0f) return 1.0f;
  x = 1.0f - x;
  return 1.0f - x * x * x;
}

// Fracao da animacao de crescer da coluna `c` (0..1).
static float crescer(int c, Uint32 agora) {
  Uint32 atraso = (Uint32)c * TG_ANIM_PASSO;
  if (ajustes_animacoes_reduzidas()) return 1.0f;
  if (!animIni) return 0.0f;
  if (agora - animIni < atraso) return 0.0f;
  return suaveSaida((float)(agora - animIni - atraso) / (float)TG_ANIM_MS);
}

static void textoTemp(int numero, char *dst, size_t tam) {
  snprintf(dst, tam, i18n("T%d"), numero);
}

static void reacaoTexto(const TgAmigo *g, char *dst, size_t tam) {
  dst[0] = 0;
  if (g->agora) snprintf(dst, tam, "%s", i18n("Vendo agora"));
  else if (g->reacao == SV_REAC_GOSTOU) snprintf(dst, tam, "%s", i18n("Gostou"));
  else if (g->reacao == SV_REAC_NAO) snprintf(dst, tam, "%s", i18n("Não gostou"));
  else if (g->reacao == SV_REAC_MEIO) snprintf(dst, tam, "%s", i18n("Mais ou menos"));
  else if (g->nota > 0) snprintf(dst, tam, i18n("Nota %d/10"), (g->nota + 5) / 10);
}

// A COLUNA NUMERICA a esquerda do grafico: a serie inteira em repouso, a
// temporada focada quando o foco anda pelas colunas.
static void colunaNumeros(const TgDados *d, float x, float y, float w, int foco, float a) {
  char grande[24], l1[96], l2[96], l3[96];
  TxtLinha lg, ls1, ls2, ls3;
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  l1[0] = l2[0] = l3[0] = 0;
  if (foco >= 0 && foco < d->n) {
    const TgTemp *t = &d->t[foco];
    char tt[32];
    if (t->numero == 0) snprintf(tt, sizeof tt, "%s", i18n("Especiais"));
    else snprintf(tt, sizeof tt, i18n("Temporada %d"), t->numero);
    if (d->sabe) snprintf(grande, sizeof grande, "%d/%d", t->vistos, t->exibidos);
    else snprintf(grande, sizeof grande, "%d", t->exibidos);
    snprintf(l1, sizeof l1, "%s", tt);
    { size_t k;
      if (t->completa) snprintf(l2, sizeof l2, "%s", i18n("Temporada completa"));
      else if (d->sabe) snprintf(l2, sizeof l2, i18n("%d de %d assistidos"), t->vistos, t->exibidos);
      else snprintf(l2, sizeof l2, i18n(t->exibidos == 1 ? "%d episódio" : "%d episódios"), t->exibidos);
      k = strlen(l2);
      // A mesma clausula do resumo acima das pilulas (detail.c).
      if (t->total > t->exibidos)
        snprintf(l2 + k, sizeof l2 - k, i18n(t->total - t->exibidos == 1 ? " · %d ainda não exibido"
                                                                         : " · %d ainda não exibidos"),
                 t->total - t->exibidos); }
    snprintf(l3, sizeof l3, "%s", i18n("OK abre a temporada"));
  } else {
    int pct = d->exibidos > 0 ? (int)((d->vistos * 100L + d->exibidos / 2) / d->exibidos) : 0;
    if (d->sabe) snprintf(grande, sizeof grande, "%d%%", pct);
    else snprintf(grande, sizeof grande, "%d", d->exibidos);
    if (d->sabe) snprintf(l1, sizeof l1, i18n("%d de %d episódios"), d->vistos, d->exibidos);
    else snprintf(l1, sizeof l1, "%s", i18n("Episódios"));
    if (d->sabe && d->completas > 0)
      snprintf(l2, sizeof l2, i18n("Temporadas completas: %d de %d"), d->completas, d->n);
    else snprintf(l2, sizeof l2, i18n(d->n == 1 ? "%d temporada" : "%d temporadas"), d->n);
    if (d->sabe && d->exibidos > 0 && d->vistos >= d->exibidos)
      snprintf(l3, sizeof l3, "%s", i18n("Você está em dia"));
    else if (d->sabe && d->meuT > 0)
      snprintf(l3, sizeof l3, i18n("Último visto: T%dE%d"), d->meuT, d->meuE);
  }
  lg = txt_linha(TXT_V2_NUM, grande, 245, 246, 248, 255);
  ls1 = txt_linha_corta(TXT_ILHA_NOME, l1, 243, 242, 239, 255, w);
  ls2 = txt_linha_corta(TXT_ILHA_SUB, l2, 243, 242, 239, 255, w);
  ls3 = txt_linha_corta(TXT_ILHA_SUB, l3, 243, 242, 239, 255, w);
  txt_desenhar_alpha(lg, x, y, a);
  // O fio de realce sob o numero grande: a assinatura do bloco.
  gfx_cor((GfxRect){ x, y + (float)lg.h + 6.0f, 44.0f, 4.0f }, 0.5f, ar, ag, ab, a);
  y += (float)lg.h + 26.0f;
  txt_desenhar_alpha(ls1, x, y, a); y += (float)ls1.h + 8.0f;
  if (l2[0]) { txt_desenhar_alpha(ls2, x, y, a * 0.72f); y += (float)ls2.h + 6.0f; }
  if (l3[0]) txt_desenhar_alpha(ls3, x, y, a * 0.55f);
}

// Rostinho de um amigo na coluna: cheio e com aro de realce quando esta na
// frente, apagado quando esta atras.
static void rostoNaColuna(const TgAmigo *g, float cx, float cy, float a) {
  GfxRect r = { cx - TG_ROSTO * 0.5f, cy - TG_ROSTO * 0.5f, TG_ROSTO, TG_ROSTO };
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  // Disco escuro por baixo separa o rosto da barra clara.
  gfx_cor((GfxRect){ r.x - 3.0f, r.y - 3.0f, r.w + 6.0f, r.h + 6.0f }, 0.5f, 0.05f, 0.05f, 0.07f, a * 0.85f);
  if (g->frente) gfx_anel_fora(r, 0.5f, 1.0f, 2.5f, ar, ag, ab, a);
  svd_avatar(r, g->avatar, g->nome, g->id, g->frente ? a : a * 0.5f);
}

static void desenhaGrafico(const TgDados *d, GfxRect card, int foco, int selNumero,
                           float a, Uint32 agora) {
  float bx = card.x + TG_PAD + TG_STAT_W + TG_PAD, bw = card.x + card.w - TG_PAD - bx;
  float topo = card.y + 30.0f, contY = topo, colTopo = topo + 36.0f;
  float colBase = card.y + card.h - 30.0f - 36.0f, colH = colBase - colTopo;
  float slot, barW, ar, ag, ab;
  int i, passoRot, vidro = ajustes_vidro();
  if (d->n <= 0) return;
  ajustes_acento(&ar, &ag, &ab);
  slot = bw / (float)d->n;
  if (slot > TG_SLOT_MAX) slot = TG_SLOT_MAX;
  // Poucas temporadas: o grupo fica no meio da area, nao encostado a esquerda.
  bx += (bw - slot * (float)d->n) * 0.5f;
  barW = slot * 0.46f;
  if (barW > TG_BARRA_MAX) barW = TG_BARRA_MAX;
  if (barW < TG_BARRA_MIN) barW = TG_BARRA_MIN;
  // Rotulo de temporada a cada `passoRot` colunas quando apertado ("T12" mede
  // ~44 px); o focado e o escolhido sempre levam o seu.
  passoRot = (int)ceilf(52.0f / slot);
  if (passoRot < 1) passoRot = 1;
  // Fio separador da coluna de numeros.
  gfx_cor((GfxRect){ card.x + TG_PAD + TG_STAT_W + TG_PAD * 0.5f - 1.0f, card.y + 28.0f, 1.0f, card.h - 56.0f }, 0.0f,
          1.0f, 1.0f, 1.0f, a * 0.08f);
  // Linha de base.
  gfx_cor((GfxRect){ bx, colBase + 1.0f, slot * (float)d->n, 1.0f }, 0.0f, 1.0f, 1.0f, 1.0f, a * 0.10f);

  for (i = 0; i < d->n; i++) {
    const TgTemp *t = &d->t[i];
    float cx = bx + slot * ((float)i + 0.5f), x0 = cx - barW * 0.5f;
    float g = crescer(i, agora);
    int focado = (i == foco), escolhida = (t->numero == selNumero);
    float fTot = t->total > 0 ? 1.0f : 0.0f;
    float fExib = t->total > 0 ? (float)t->exibidos / (float)t->total : 0.0f;
    float fVisto = t->total > 0 ? (float)t->vistos / (float)t->total : 0.0f;
    float hExib = colH * fExib, hVisto = colH * fVisto * g;
    GfxRect trilho = { x0, colBase - hExib, barW, hExib };
    char rot[16], cont[24];
    (void)fTot;
    // Foco: um halo de realce atras da coluna inteira e a coluna um tom acima.
    if (focado) {
      GfxRect halo = { cx - slot * 0.5f + 4.0f, colTopo - 34.0f, slot - 8.0f, colH + 34.0f + 40.0f };
      gfx_cor(halo, raioPx(halo, 18.0f), ar, ag, ab, a * 0.14f);
      gfx_anel(halo, raioPx(halo, 18.0f), 2.0f, ar, ag, ab, a * 0.65f);
    } else if (escolhida) {
      GfxRect halo = { cx - slot * 0.5f + 4.0f, colTopo - 34.0f, slot - 8.0f, colH + 34.0f + 40.0f };
      gfx_cor(halo, raioPx(halo, 18.0f), 1.0f, 1.0f, 1.0f, a * 0.045f);
    }
    // Trilho (o que ja foi ao ar e voce nao viu).
    if (hExib > 0.5f) {
      if (vidro) gfx_cor(trilho, raioPx(trilho, barW * 0.5f), 1.0f, 1.0f, 1.0f, a * (focado ? 0.16f : 0.09f));
      else gfx_cor(trilho, raioPx(trilho, barW * 0.5f), 0.17f, 0.18f, 0.21f, a * (focado ? 1.0f : 0.85f));
    }
    // O que ainda vai ao ar: tracejado acima do trilho.
    if (t->total > t->exibidos) {
      GfxRect fut = { x0, colTopo, barW, colH - hExib - (hExib > 0.5f ? 4.0f : 0.0f) };
      if (fut.h > 6.0f) {
        float esp = 2.0f;
        int tracos = (int)((fut.w + fut.h) * 2.0f / 14.0f);
        if (tracos < 6) tracos = 6;
        gfx_rect(fut, 0, GFX_ANEL, 0, esp / fut.h, (float)tracos, raioPx(fut, barW * 0.5f),
                 1.0f, 1.0f, 1.0f, a * 0.38f);
      }
    }
    // O que voce viu.
    if (d->sabe && hVisto > 0.5f) {
      GfxRect ch = { x0, colBase - hVisto, barW, hVisto };
      if (t->completa) gfx_cor(ch, raioPx(ch, barW * 0.5f), ar, ag, ab, a);
      // Pela metade: o mesmo branco, mais baixo — completa e o unico cheio
      // (com realce branco, o degrau ainda separa as duas).
      else gfx_cor(ch, raioPx(ch, barW * 0.5f), 0.93f, 0.94f, 0.96f, a * (focado ? 0.62f : 0.46f));
    }
    // Contagem acima da coluna: em todas quando cabe, senao so na focada/escolhida.
    if (slot >= 72.0f || focado || escolhida) {
      TxtLinha lc;
      if (d->sabe) snprintf(cont, sizeof cont, "%d/%d", t->vistos, t->exibidos);
      else snprintf(cont, sizeof cont, "%d", t->exibidos);
      lc = txt_linha(focado || t->completa ? TXT_G20B : TXT_ILHA_SUB, cont,
                     t->completa && !focado ? (int)(ar * 255) : 243,
                     t->completa && !focado ? (int)(ag * 255) : 242,
                     t->completa && !focado ? (int)(ab * 255) : 239, 255);
      txt_desenhar_alpha(lc, cx - (float)lc.w * 0.5f, contY, a * (focado ? 1.0f : 0.72f) * (0.35f + 0.65f * g));
    }
    // Rotulo da temporada abaixo.
    if (i % passoRot == 0 || focado || escolhida) {
      TxtLinha lr;
      textoTemp(t->numero, rot, sizeof rot);
      lr = txt_linha(focado || escolhida ? TXT_G20B : TXT_ILHA_SUB, rot, 243, 242, 239, 255);
      txt_desenhar_alpha(lr, cx - (float)lr.w * 0.5f, colBase + 14.0f,
                         a * (focado ? 1.0f : escolhida ? 0.9f : 0.55f));
      if (escolhida && !focado)
        gfx_cor((GfxRect){ cx - 3.0f, colBase + 14.0f + (float)lr.h + 5.0f, 6.0f, 6.0f }, 0.5f,
                ar, ag, ab, a);
    }
  }

  // Os amigos na coluna e na altura do episodio. Os de tras primeiro, para os
  // da frente ficarem por cima quando se sobrepoem.
  { int passo;
    for (passo = 0; passo < 2; passo++) {
      int k;
      for (k = d->nAmg - 1; k >= 0; k--) {
        const TgAmigo *g = &d->amg[k];
        const TgTemp *t;
        float cx, cy, fr, al;
        int m = 0, pos = 0, q;
        if (g->col < 0 || g->frente != passo) continue;
        t = &d->t[g->col];
        if (t->total <= 0) continue;
        // Varios amigos na mesma temporada: lado a lado, centrados na coluna.
        for (q = 0; q < d->nAmg; q++)
          if (d->amg[q].col == g->col) { if (q < k) pos++; m++; }
        fr = (float)g->episodio / (float)t->total;
        if (fr > 1.0f) fr = 1.0f;
        cx = bx + slot * ((float)g->col + 0.5f) + ((float)pos - (float)(m - 1) * 0.5f) * (TG_ROSTO * 0.72f);
        cy = colBase - colH * fr;
        if (cy < colTopo + TG_ROSTO * 0.5f) cy = colTopo + TG_ROSTO * 0.5f;
        al = crescer(g->col, agora);
        al = al > 0.6f ? (al - 0.6f) / 0.4f : 0.0f;
        rostoNaColuna(g, cx, cy, a * al);
      }
    } }
}

static void desenhaAmigos(const TgDados *d, GfxRect card, int foco, float a, Uint32 agora) {
  char frase[160];
  float x = card.x + TG_PAD, w = card.w - 2.0f * TG_PAD, y = card.y + 28.0f;
  TxtLinha lf;
  int i, mostra;
  tgraf_frase_amigos(d, frase, sizeof frase);
  lf = txt_linha_corta(TXT_ILHA_NOME, frase, 243, 242, 239, 255, w);
  txt_desenhar_alpha(lf, x, y, a);
  y += (float)lf.h + 18.0f;
  if (foco < 0) {
    // EM REPOUSO: os rostos em fila e quem esta mais adiante, em uma linha.
    float rx = x;
    char l[128];
    mostra = d->nAmg < 6 ? d->nAmg : 6;
    for (i = 0; i < mostra; i++) {
      const TgAmigo *g = &d->amg[i];
      GfxRect r = { rx, y, 56.0f, 56.0f };
      gfx_cor((GfxRect){ r.x - 3.0f, r.y - 3.0f, r.w + 6.0f, r.h + 6.0f }, 0.5f, 0.06f, 0.06f, 0.08f, a);
      svd_avatar(r, g->avatar, g->nome, g->id, g->frente || !d->sabe ? a : a * 0.55f);
      if (g->agora) svd_ponto_vivo(r.x + r.w - 9.0f, r.y + r.h - 9.0f, 16.0f, 3.0f, a, agora);
      rx += 44.0f;
    }
    if (d->nAmg > mostra) {
      char b[16];
      TxtLinha lb;
      snprintf(b, sizeof b, "+%d", d->nAmg - mostra);
      lb = txt_linha(TXT_ILHA_SUB, b, 243, 242, 239, 255);
      txt_desenhar_alpha(lb, rx + 22.0f, y + (56.0f - (float)lb.h) * 0.5f, a * 0.7f);
    }
    y += 56.0f + 22.0f;
    { char n1[64];
      const TgAmigo *g = &d->amg[0];
      amigostitulo_primeiro_nome(g->nome, n1, sizeof n1);
      snprintf(l, sizeof l, i18n("%s está no T%dE%d"), n1, g->temporada, g->episodio);
      { TxtLinha ll = txt_linha_corta(TXT_ILHA_SUB, l, 243, 242, 239, 255, w);
        txt_desenhar_alpha(ll, x, y, a * 0.72f); y += (float)ll.h + 8.0f; }
      { TxtLinha lh = txt_linha_corta(TXT_ILHA_HORA, i18n("Navegue no gráfico para ver todos"),
                                      243, 242, 239, 255, w);
        txt_desenhar_alpha(lh, x, y, a * 0.45f); } }
    return;
  }
  // COM O FOCO: a lista. Ate quatro linhas; com mais, a quarta vira "+N".
  mostra = d->nAmg;
  if (mostra > 4) mostra = 3;
  for (i = 0; i < mostra; i++) {
    const TgAmigo *g = &d->amg[i];
    char ep[24], rea[48];
    TxtLinha ln, le, lr;
    float tx = x + TG_LIN_AV + 16.0f, alfa = g->frente || !d->sabe ? a : a * 0.6f;
    GfxRect av = { x, y + (TG_LIN_H - TG_LIN_AV) * 0.5f, TG_LIN_AV, TG_LIN_AV };
    svd_avatar(av, g->avatar, g->nome, g->id, alfa);
    if (g->agora) svd_ponto_vivo(av.x + av.w - 8.0f, av.y + av.h - 8.0f, 14.0f, 3.0f, a, agora);
    snprintf(ep, sizeof ep, i18n("T%dE%d"), g->temporada, g->episodio);
    reacaoTexto(g, rea, sizeof rea);
    le = txt_linha(TXT_G20B, ep, 243, 242, 239, 255);
    ln = txt_linha_corta(TXT_ILHA_NOME, g->nome, 243, 242, 239, 255, w - TG_LIN_AV - 16.0f - (float)le.w - 20.0f);
    lr = txt_linha_corta(TXT_ILHA_HORA, rea, 243, 242, 239, 255, w - TG_LIN_AV - 16.0f);
    { float bloco = (float)ln.h + (rea[0] ? 2.0f + (float)lr.h : 0.0f);
      float ty = y + (TG_LIN_H - bloco) * 0.5f;
      txt_desenhar_alpha(ln, tx, ty, alfa);
      if (rea[0]) txt_desenhar_alpha(lr, tx, ty + (float)ln.h + 2.0f, alfa * 0.6f); }
    txt_desenhar_alpha(le, x + w - (float)le.w, y + (TG_LIN_H - (float)le.h) * 0.5f,
                       g->frente ? a : a * 0.6f);
    y += TG_LIN_H;
  }
  if (d->nAmg > mostra) {
    char b[48];
    TxtLinha lb;
    snprintf(b, sizeof b, "+%d", d->nAmg - mostra);
    lb = txt_linha(TXT_ILHA_SUB, b, 243, 242, 239, 255);
    txt_desenhar_alpha(lb, x + TG_LIN_AV + 16.0f, y + (TG_LIN_H - (float)lb.h) * 0.5f, a * 0.6f);
  }
}

void tgraf_desenhar(const TgDados *d, float x, float y, float w, int foco,
                    int selNumero, float a, Uint32 agora) {
  float alt = tgraf_altura();
  GfxRect cg, ca;
  int temAmg;
  if (!tgraf_existe(d) || a <= 0.001f) return;
  if (y >= NV_TELA_H || y + alt <= 0.0f) return;
  // A animacao comeca quando o bloco aparece pela primeira vez nesta serie.
  if (strcmp(animImdb, d->imdb)) {
    snprintf(animImdb, sizeof animImdb, "%s", d->imdb);
    animIni = agora ? agora : 1u;
  }
  { TxtLinha lt = txt_linha(TXT_HEADLINE, i18n("Seu progresso"), 245, 248, 255, 255);
    txt_desenhar_alpha(lt, x, y, a); }
  temAmg = d->nAmg > 0;
  cg = (GfxRect){ x, y + TG_CAB, temAmg ? w - TG_AMG_W - TG_GAP : w, TG_CARD_H };
  notasui_painel(cg, TG_RAIO, a);
  colunaNumeros(d, cg.x + TG_PAD, cg.y + 30.0f, TG_STAT_W - 8.0f, foco, a);
  desenhaGrafico(d, cg, foco, selNumero, a, agora);
  if (temAmg) {
    ca = (GfxRect){ x + w - TG_AMG_W, y + TG_CAB, TG_AMG_W, TG_CARD_H };
    notasui_painel(ca, TG_RAIO, a);
    desenhaAmigos(d, ca, foco, a, agora);
  }
}
