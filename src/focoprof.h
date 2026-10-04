// FOCO COMUM DOS CARTOES (Home e Detalhe): a mesma regra, os mesmos Ajustes.
//
//  - Anel ligado (Ajustes > Foco no cartaz): o cartao mantem a caixa e leva o
//    anel de NV_ANEL_FOCO no realce (ou o cartao de vidro). Desligado: nenhum
//    contorno, e quem marca o foco e o crescimento (FOCO_ZOOM).
//  - Profundidade (AJ_PROF*, borda/brilho/cobertura): o realce de topo do
//    cartao em foco, por tipo de cartao (`ligadaAqui`).
// `f` e a animacao de foco 0..1 que cada tela ja tem; nada aqui guarda estado.
#ifndef NV_FOCOPROF_H
#define NV_FOCOPROF_H
#include "gfx.h"
#include "ajustes.h"
#include "layout.h"

#define FOCO_ZOOM 0.06f   // o mesmo 6 % da Home com o anel desligado

// Caixa do cartao em foco: cresce em volta do centro so com o anel desligado.
static inline GfxRect foco_zoom(GfxRect r, float f) {
  float s;
  if (ajustes_borda_foco() || f <= 0.0f) return r;
  s = FOCO_ZOOM * f;
  return (GfxRect){ r.x - r.w * s * 0.5f, r.y - r.h * s * 0.5f,
                    r.w * (1.0f + s), r.h * (1.0f + s) };
}

// Anel de foco. `raio` na convencao do gfx_cor (fracao do MENOR lado).
static inline void foco_anel(GfxRect r, float raio, float f, float a) {
  float menor = r.w < r.h ? r.w : r.h, ar, ag, ab;
  if (f <= 0.01f || !ajustes_borda_foco()) return;
  if (ajustes_vidro()) { gfx_vidro_cartao(r, raio * menor / r.h, f, a); return; }
  ajustes_acento(&ar, &ag, &ab);
  gfx_cor((GfxRect){ r.x - NV_ANEL_FOCO, r.y - NV_ANEL_FOCO,
                     r.w + 2 * NV_ANEL_FOCO, r.h + 2 * NV_ANEL_FOCO },
          (raio * menor + NV_ANEL_FOCO) / (menor + 2 * NV_ANEL_FOCO),
          ar, ag, ab, f * a);
}

// Realce de profundidade (borda + reflexo) por cima da arte do cartao.
static inline void foco_profundidade(GfxRect card, float raio, int ligadaAqui, float a) {
  float borda, brilho, cobertura;
  if (!ajustes_profundidade() || !ligadaAqui) return;
  borda = ajustes_profundidade_borda();
  brilho = ajustes_profundidade_brilho();
  cobertura = ajustes_profundidade_cobertura();
  if (borda > 0.001f) {
    float alcance = (12.0f + 18.0f * cobertura) / (card.h > 1.0f ? card.h : 1.0f);
    gfx_brilho_topo(card, raio, alcance, 1.0f, 1.0f, 1.0f, borda * 0.55f * a);
  }
  if (brilho > 0.001f)
    gfx_brilho_topo(card, raio, 0.34f, 1.0f, 1.0f, 1.0f, brilho * 0.18f * a);
}
#endif
