// FUNDO ATRAS DOS PAINEIS. Ver fundo.h.
#include "fundo.h"
#include "ajustes.h"
#include "corviva.h"
#include "layout.h"
#include "tex_cache.h"
#include <string.h>

int fundo_modo(void) { return ajustes_fundo(); }

static void arte(GfxRect r, float raioPx, const char *c, float a) {
  GLuint t = (c && c[0]) ? tex_obter_larg(c, r.w > r.h * 1.78f ? r.w : r.h * 1.78f) : 0;
  if (!t) { gfx_cor(r, raioPx / r.h, NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, a); return; }
  gfx_tex_aspect_atual = tex_aspecto(c);
  gfx_arte_opaca_atual = tex_opaca(c);   // tela cheia opaca: a luz por baixo nao e pintada
  gfx_rect(r, t, GFX_ARTE, 0, 0, 0, raioPx / r.h, 1, 1, 1, a);
  gfx_arte_opaca_atual = 0;
  gfx_tex_aspect_atual = 0;
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
static int borrada(GfxRect r, float raioPx, const char *c, float a) {
  CorvivaPaleta p;
  int i;
  if (!c || !c[0] || !corviva_paleta(c, &p) || !p.ok) return 0;
  if (telaCheia(r) && raioPx <= 0.0f) {
    assar(&p, a);
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
void fundo_desenhar_modo(int modo, GfxRect r, float raioPx, const char *c, float a) {
  if (r.w < 1 || r.h < 1 || a <= 0.003f) return;
  if (modo == FUNDO_FROST) { foscoPreparar(r, raioPx, c); frost(r, raioPx, a); return; }
  if (modo == FUNDO_BORRADA && borrada(r, raioPx, c, a)) return;
  foscoPreparar(r, raioPx, c);
  arte(r, raioPx, c, a);
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
