// FUNDO ATRAS DOS PAINEIS. Ver fundo.h.
#include "fundo.h"
#include "ajustes.h"
#include "corviva.h"
#include "layout.h"
#include "tex_cache.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int fundo_modo(void) { return ajustes_fundo(); }

// veu = 1: a arte de tela cheia e o veu do mockup saem numa passada so
// (gfx_arte_veu) quando o caminho permite; devolve 1 se o veu ja foi aplicado.
static int arte(GfxRect r, float raioPx, const char *c, float a, int veu) {
  GLuint t = (c && c[0]) ? tex_obter_larg(c, r.w > r.h * 1.78f ? r.w : r.h * 1.78f) : 0;
  int feito = 0;
  if (!t) { gfx_cor(r, raioPx / r.h, NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, a); return 0; }
  gfx_tex_aspect_atual = tex_aspecto(c);
  gfx_arte_opaca_atual = tex_opaca(c);   // tela cheia opaca: a luz por baixo nao e pintada
  // O veu: 40% no meio, 62% na borda esquerda (+0,367 do que falta) e 50% na direita (+0,167).
  if (veu && raioPx <= 0.0f && a >= 0.999f)
    feito = gfx_arte_veu(r, t, 0.40f, 0.367f, 0.167f, a);
  if (!feito) gfx_rect(r, t, GFX_ARTE, 0, 0, 0, raioPx / r.h, 1, 1, 1, a);
  gfx_arte_opaca_atual = 0;
  gfx_tex_aspect_atual = 0;
  return feito;
}
// A cor de uma regiao da paleta com saturacao +35% e brilho 62%.
#ifndef FUNDO_BRILHO
#define FUNDO_BRILHO 1.0f
#endif
static void tingir(const float *q, float *o) {
  float l = 0.2126f * q[0] + 0.7152f * q[1] + 0.0722f * q[2];
  int k;
  for (k = 0; k < 3; k++) {
    float v = (l + (q[k] - l) * 1.35f) * FUNDO_BRILHO;
    o[k] = v < 0 ? 0 : v > 1 ? 1 : v;
  }
}
static int telaCheia(GfxRect r) {
  return r.x <= 0.5f && r.y <= 0.5f && r.w >= NV_TELA_W / gfx_escala() - 1 && r.h >= NV_TELA_H / gfx_escala() - 1;
}
// Assa (so assa) a arte borrada no quadro pequeno; pinta se alfa > 0. O mesmo
// assado serve ao vidro fosco (gfx_vidro_fosco): a luz da arte, em tela.
static void assar(const CorvivaPaleta *p, float alfa) {
  float amb[4][3], forca = nv_ambiente_forca, tempo = nv_tempo_viva;
  int i;
  memcpy(amb, nv_ambiente_viva, sizeof amb);
  for (i = 0; i < 4; i++) tingir(p->regiao[i], nv_ambiente_viva[i]);
  nv_ambiente_forca = 1.0f; nv_tempo_viva = 0.0f;   // parado: o assado nao refaz
  gfx_ambiente_preparar();
  if (alfa > 0.0f) gfx_ambiente(alfa * 0.998f);   // < 1: pinta agora, nao fica pendente para o clear
  memcpy(nv_ambiente_viva, amb, sizeof amb);
  nv_ambiente_forca = forca; nv_tempo_viva = tempo;
}
// Fundo de arte nitida/Frost com "Vidro fosco" ligado: o assado existe so para
// o vidro. Com a Imersiva a luz real ja e o assado, entao nada a fazer.
static void foscoPreparar(GfxRect r, float raioPx, const char *c) {
  CorvivaPaleta p;
  if (!ajustes_vidro() || !ajustes_vidro_fosco() || nv_ambiente_forca > 0.001f ||
      !telaCheia(r) || raioPx > 0.0f || !c || !c[0] || !corviva_paleta(c, &p) || !p.ok) return;
  assar(&p, 0.0f);
}
void fundo_fosco_quadro(void) {
  CorvivaPaleta p;
  if (!ajustes_vidro() || !ajustes_vidro_fosco() || nv_ambiente_forca > 0.001f ||
      !corviva_cena_paleta(&p)) return;
  assar(&p, 0.0f);
}
// O FUNDO DE TELA CHEIA PELO CAMINHO DA LUZ IMERSIVA (gfx_luz_canal). O
// desenho de sempre vai para um quadro pequeno so quando a chave muda — pelo
// MESMO assado da luz imersiva, que a C9 mostra certo — e cada quadro paga UM
// quad opaco (GFX_SNAP, a passada de tela da luz). A "Arte borrada" assa
// exatamente a luz que o assar() acima assava (as quatro regioes da paleta,
// forca 1, parada); o veu de 28% vai na mesma passada, e o quadro sem veu e a
// fonte do vidro fosco. O Frost assa o desenho direto inteiro.
//
// CONFERENCIA DE UMA VEZ (C9, 05/10/2026: o assado do 1bcd6ebe saiu escuro na
// TV e certo no Mac). Na primeira vez que cada fundo sai opaco, um pixel da
// TELA, lido logo depois do quad, e comparado com a mesma conta feita aqui no
// CPU, num ponto onde a luz pesa (>= 12 niveis acima do fundo sem luz). Errou:
// log "[cor] fundo ... ERRADO" e o desenho direto de sempre pela sessao. Uma
// leitura de 1 px por fundo por sessao.
#define CANAL_BORRADA 0
#define CANAL_FROST   1
#define K_BORRADA 0
#define K_FROST   1
static const float VEU_BORRADA[4] = { 0.024f, 0.027f, 0.035f, 0.28f };
static const char *const NOME_K[2] = { "borrada", "frost" };
static int conferido[2] = { -1, -1 };   // -1 a conferir, 1 certo, 0 errado (desenho direto)
static int conferidoAssado[2] = { -1, -1 };
int fundo_conferencia(int modo) {
  return modo == FUNDO_FROST ? conferido[K_FROST] : modo == FUNDO_BORRADA ? conferido[K_BORRADA] : -1;
}

static float ss(float e0, float e1, float x) {
  float t = (x - e0) / (e1 - e0);
  t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
  return t * t * (3.0f - 2.0f * t);
}
// GFX_AMBIENTE com uTempo 0 e uCor.a 1 sobre o clear na cor do fundo (o
// assado), e o veu por cima: o pixel da tela em (u, v), em 0..1.
static void refBorrada(const float amb[4][3], float u, float v, int luz, float o[3]) {
  const float A = 1920.0f / 1080.0f;
  float qx = u * A, qy = v, w[4], cx[4], cy[4], r[4], k[4], tw = 0.0f, al;
  int i, j;
  cx[0] = -0.12f * A;                    cy[0] = 0.55f;          r[0] = 1.30f; k[0] = 0.85f;
  cx[1] = 1.12f * A + 0.03f * sinf(2.0f); cy[1] = 0.45f + 0.06f;  r[1] = 1.30f; k[1] = 0.85f + 0.15f * sinf(1.7f);
  cx[2] = 0.55f * A + 0.08f * sinf(1.0f); cy[2] = -0.28f;         r[2] = 1.15f; k[2] = 0.85f + 0.15f * sinf(3.1f);
  cx[3] = 0.45f * A + 0.08f;              cy[3] = 1.28f;          r[3] = 1.15f; k[3] = 0.85f + 0.15f * sinf(4.4f);
  for (i = 0; i < 4; i++) {
    float d = sqrtf((qx - cx[i]) * (qx - cx[i]) + (qy - cy[i]) * (qy - cy[i])), l = 1.0f - ss(0.0f, r[i], d);
    w[i] = l * l * k[i]; tw += w[i];
  }
  al = luz ? (tw < 1.0f ? tw : 1.0f) * 0.72f : 0.0f;
  for (j = 0; j < 3; j++) {
    float c = 0.0f, f = j == 0 ? NV_COR_FUNDO_R : j == 1 ? NV_COR_FUNDO_G : NV_COR_FUNDO_B;
    for (i = 0; i < 4; i++) c += amb[i][j] * w[i];
    c /= tw > 0.001f ? tw : 0.001f;
    c = c * al + f * (1.0f - al);
    o[j] = c * (1.0f - VEU_BORRADA[3]) + VEU_BORRADA[j] * VEU_BORRADA[3];
  }
}
// O Frost direto em (u, v) de uma tela cheia: base, degrade de cima e as tres
// luzes (GFX_LUZ: queda suave ao quadrado).
static void refFrost(const float ac[3], float u, float v, int luz, float o[3]) {
  static const float L[3][4] = { { 0.14f, 0.08f, 1.05f, 0.70f }, { 0.92f, 0.96f, 0.90f, 0.45f },
                                 { 0.70f, 0.30f, 0.60f, 0.18f } };
  static const float B[3] = { 0.043f, 0.047f, 0.055f }, T[3] = { 0.082f, 0.086f, 0.102f };
  const float A = 1920.0f / 1080.0f;
  float g = v / 0.70f;
  int i, j;
  g = 1.0f - (g < 0.0f ? 0.0f : g > 1.0f ? 1.0f : g);
  for (j = 0; j < 3; j++) o[j] = B[j] * (1.0f - g) + T[j] * g;
  if (!luz) return;
  for (i = 0; i < 3; i++) {
    float px = (u - L[i][0]) * A, py = v - L[i][1], t = 1.0f - sqrtf(px * px + py * py) / L[i][2], sv, al;
    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    sv = t * t * (3.0f - 2.0f * t);
    al = sv * sv * L[i][3];
    for (j = 0; j < 3; j++) o[j] = o[j] * (1.0f - al) + ac[j] * al;
  }
}
static int nivel(float c) { return (int)(c * 255.0f + 0.5f); }
// Ponto onde a luz mais pesa entre os candidatos. Devolve o desvio (niveis)
// entre "com luz" e "sem luz" e preenche u, v e as duas referencias.
static int pontoDeLuz(int k, const void *dados, float *u, float *v, int ref[3], int base[3]) {
  static const float CB[4][2] = { { 0.04f, 0.5f }, { 0.96f, 0.5f }, { 0.5f, 0.04f }, { 0.5f, 0.96f } };
  static const float CF[3][2] = { { 0.14f, 0.08f }, { 0.92f, 0.94f }, { 0.5f, 0.5f } };
  int n = k == K_BORRADA ? 4 : 3, i, j, melhor = -1;
  for (i = 0; i < n; i++) {
    float uu = k == K_BORRADA ? CB[i][0] : CF[i][0], vv = k == K_BORRADA ? CB[i][1] : CF[i][1];
    float c[3], b[3];
    int dev = 0;
    if (k == K_BORRADA) { refBorrada(dados, uu, vv, 1, c); refBorrada(dados, uu, vv, 0, b); }
    else { refFrost(dados, uu, vv, 1, c); refFrost(dados, uu, vv, 0, b); }
    for (j = 0; j < 3; j++) { int d = abs(nivel(c[j]) - nivel(b[j])); if (d > dev) dev = d; }
    if (dev > melhor) {
      melhor = dev; *u = uu; *v = vv;
      for (j = 0; j < 3; j++) { ref[j] = nivel(c[j]); base[j] = nivel(b[j]); }
    }
  }
  return melhor;
}
// DESPEJO DE UMA VEZ (dono, 05/10/2026: ver na TV o que foi mesmo desenhado).
// Na PRIMEIRA vez por execucao que cada fundo de tela cheia sai opaco, grava
// /tmp/nuvio-fundo-<frost|borrada>.bmp (a tela logo depois do fundo, em meia
// resolucao) e /tmp/nuvio-fundo-<...>-assado.bmp (o quadro pequeno de 320x180,
// quando ha um), e loga o pixel conferido. Sem arquivo de pedido: um pedido que
// o app nao conseguia apagar derrubou a C9 para 9 fps. A marca e posta ANTES de
// gravar, entao uma gravacao que falha tambem nao repete; sem /tmp gravavel
// (Android) nada e lido. NUVIO_DUMP_FUNDO_DIR troca o /tmp (testes no Mac).
static const char *dumpDir(void) {
  const char *d = getenv("NUVIO_DUMP_FUNDO_DIR");
  return d && d[0] ? d : "/tmp";
}
static int despejado[2];
static int despejoDevido(int k, float a) {
  if (despejado[k] || a * gfx_opacidade_grupo < 0.999f) return 0;
  if (access(dumpDir(), W_OK) != 0) { despejado[k] = 1; return 0; }
  return 1;
}
// Depois do quad do fundo `k` (t = o quadro pequeno, 0 no desenho direto):
// a conferencia de uma vez e o despejo de uma vez.
static void depoisDoFundo(int k, GLuint t, const void *dados, float a) {
  int despejar = despejoDevido(k, a), conferir = 0, dev, ref[3], base[3], j, erro = 0, tol;
  unsigned char tela[3] = { 0, 0, 0 }, pq[3] = { 0, 0, 0 };
  float u = 0.5f, v = 0.5f;
  if (t && conferido[k] < 0 && conferidoAssado[k] != gfx_n_fundo_assados &&
      a * gfx_opacidade_grupo >= 0.999f && !gfx_efeitos_leves() && !gfx_modos_desligados)
    conferir = 1;
  if (!conferir && !despejar) return;
  dev = pontoDeLuz(k, dados, &u, &v, ref, base);
  tol = 5 + dev / 5;
  if (!gfx_tela_px(u, v, tela)) return;   // dentro de um snapshot: nada foi lido, fica para o proximo
  if (despejar) despejado[k] = 1;
  if (t) gfx_luz_canal_px(t, u, v, pq);
  for (j = 0; j < 3; j++) { int d = abs((int)tela[j] - ref[j]); if (d > erro) erro = d; }
  if (conferir) {
    if (dev < 12) conferidoAssado[k] = gfx_n_fundo_assados;   // luz fraca demais: confere no proximo assado
    else conferido[k] = erro <= tol;
  }
  if ((conferir && dev >= 12) || despejar) {
    printf("[cor] fundo %s %s: em %.2f,%.2f esperado %d,%d,%d (sem luz %d,%d,%d); tela %d,%d,%d; "
           "quadro pequeno %d,%d,%d%s\n", NOME_K[k],
           !t ? "direto" : conferido[k] == 1 ? "conferido" : conferido[k] == 0 ? "CONFERIDO ERRADO (desenho direto)" : "lido",
           u, v, ref[0], ref[1], ref[2], base[0], base[1], base[2], tela[0], tela[1], tela[2], pq[0], pq[1], pq[2],
           t ? "" : " (sem quadro pequeno)");
    fflush(stdout);
  }
  if (despejar) {
    char cam[640];
    snprintf(cam, sizeof cam, "%s/nuvio-fundo-%s.bmp", dumpDir(), NOME_K[k]);
    printf("[fundo-dump] %s: %s", cam, gfx_tela_bmp(cam) ? "gravado" : "FALHOU");
    if (t) {
      snprintf(cam, sizeof cam, "%s/nuvio-fundo-%s-assado.bmp", dumpDir(), NOME_K[k]);
      printf(", %s: %s", cam, gfx_luz_canal_bmp(t, cam) ? "gravado" : "FALHOU");
    }
    printf("\n");
    fflush(stdout);
  }
}

static void pintarBorrada(void *ctx) {
  const float (*amb)[3] = ctx;
  GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
  float ant[4][3], forca = nv_ambiente_forca, tempo = nv_tempo_viva;
  memcpy(ant, nv_ambiente_viva, sizeof ant);
  memcpy(nv_ambiente_viva, amb, sizeof ant);
  nv_ambiente_forca = 1.0f; nv_tempo_viva = 0.0f;   // o assar(): forca 1, parado
  gfx_rect(tela, 0, GFX_AMBIENTE, 0, 0, 0, 0, 1, 1, 1, nv_ambiente_forca);
  memcpy(nv_ambiente_viva, ant, sizeof ant);
  nv_ambiente_forca = forca; nv_tempo_viva = tempo;
}
static int borrada(GfxRect r, float raioPx, const char *c, float a) {
  CorvivaPaleta p;
  int i;
  if (!c || !c[0] || !corviva_paleta(c, &p) || !p.ok) return 0;
  if (telaCheia(r) && raioPx <= 0.0f) {
    float amb[4][3], k[15];
    GLuint t = 0;
    for (i = 0; i < 4; i++) tingir(p.regiao[i], amb[i]);
    if (conferido[K_BORRADA] != 0) {
      memcpy(k, amb, sizeof amb);
      k[12] = NV_COR_FUNDO_R; k[13] = NV_COR_FUNDO_G; k[14] = NV_COR_FUNDO_B;
      t = gfx_luz_canal(CANAL_BORRADA, k, 15, pintarBorrada, amb);
    }
    if (t) {
      if (ajustes_vidro() && ajustes_vidro_fosco()) gfx_vidro_fosco_fonte(t);
      gfx_luz_canal_desenhar(t, a, VEU_BORRADA, 0);
      depoisDoFundo(K_BORRADA, t, amb, a);
      return 1;
    }
    assar(&p, a);
    gfx_cor(r, 0.0f, VEU_BORRADA[0], VEU_BORRADA[1], VEU_BORRADA[2], VEU_BORRADA[3] * a);
    depoisDoFundo(K_BORRADA, 0, amb, a);
    return 1;
  } else {
    static const float PX[4][2] = { { 0.0f, 0.5f }, { 1.0f, 0.5f }, { 0.5f, 0.0f }, { 0.5f, 1.0f } };
    gfx_cor(r, raioPx / r.h, NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, a);
    for (i = 0; i < 4; i++) {
      float cc[3];
      tingir(p.regiao[i], cc);
      gfx_luz_canto(r, raioPx / r.h, r.w * PX[i][0], r.h * PX[i][1], r.h * 1.1f, cc[0], cc[1], cc[2], a);
    }
  }
  gfx_cor(r, raioPx / r.h, 0.024f, 0.027f, 0.035f, 0.28f * a);
  return 1;
}
// FROST (acentos-mockup.html, quadro 6): a luz NAO e o acento cru, e o mesmo
// matiz com L 0,42 e croma <= 0,11 (ajustes_acento_luz) — Branco vira nevoa
// neutra e Jade nao acende a tela. Fundo linear(165deg, #15161A, #0B0C0E 70%)
// e as tres luzes a 70%, 45% e 18%.
static void frost(GfxRect r, float raioPx, float a) {
  float ar, ag, ab, rr = raioPx / r.h;
  ajustes_acento_luz(&ar, &ag, &ab);
  gfx_cor(r, rr, 0.043f, 0.047f, 0.055f, a);   // #0B0C0E
  gfx_rect(r, 0, GFX_VEU_CSS, 1.0f, 1.0f, 0.70f, rr, 0.082f, 0.086f, 0.102f, a);   // #15161A em cima
  gfx_luz_canto(r, rr, r.w * 0.14f, r.h * 0.08f, r.h * 1.05f, ar, ag, ab, 0.70f * a);
  gfx_luz_canto(r, rr, r.w * 0.92f, r.h * 0.96f, r.h * 0.90f, ar, ag, ab, 0.45f * a);
  gfx_luz_canto(r, rr, r.w * 0.70f, r.h * 0.30f, r.h * 0.60f, ar, ag, ab, 0.18f * a);
}
static void pintarFrost(void *ctx) {
  (void)ctx;
  frost((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 1.0f);
}
static void frostTela(GfxRect r, float raioPx, float a) {
  float k[6];
  GLuint t = 0;
  if (!telaCheia(r) || raioPx > 0.0f) { frost(r, raioPx, a); return; }
  ajustes_acento_luz(&k[0], &k[1], &k[2]);
  k[3] = NV_COR_FUNDO_R; k[4] = NV_COR_FUNDO_G; k[5] = NV_COR_FUNDO_B;
  if (conferido[K_FROST] != 0) t = gfx_luz_canal(CANAL_FROST, k, 6, pintarFrost, NULL);
  if (t) gfx_luz_canal_desenhar(t, a, NULL, 1);
  else frost(r, raioPx, a);
  depoisDoFundo(K_FROST, t, k, a);
}
void fundo_desenhar_modo(int modo, GfxRect r, float raioPx, const char *c, float a) {
  if (r.w < 1 || r.h < 1 || a <= 0.003f) return;
  if (modo == FUNDO_FROST) {
    foscoPreparar(r, raioPx, c);
    frostTela(r, raioPx, a);
    return;
  }
  if (modo == FUNDO_BORRADA && borrada(r, raioPx, c, a)) return;
  foscoPreparar(r, raioPx, c);
  if (arte(r, raioPx, c, a, raioPx <= 0.0f)) return;
  if (raioPx > 0.0f) { gfx_cor(r, raioPx / r.h, 0, 0, 0, 0.42f * a); return; }
  // O veu do mockup: linear-gradient(90deg, 62%, 40% no meio, 50%). Um
  // chapado de 40% e, por cima, as duas metades em degrade (GFX_VEU_CSS,
  // nv_dither) com o que falta para chegar a 62% e 50% nas bordas.
  // O chapado vai DENTRO das duas metades (gfx_veu_css_base): uma camada de
  // tela cheia misturada a menos por quadro, o mesmo pixel.
  gfx_veu_css_base((GfxRect){ r.x, r.y, r.w * 0.5f, r.h }, 2, 1.0f, 1.0f, 0.367f * a, 0.40f * a);
  gfx_veu_css_base((GfxRect){ r.x + r.w * 0.5f, r.y, r.w * 0.5f, r.h }, 3, 1.0f, 1.0f, 0.167f * a, 0.40f * a);
}
void fundo_desenhar(GfxRect area, const char *arteUrl, float a) {
  fundo_desenhar_modo(fundo_modo(), area, 0.0f, arteUrl, a);
}
