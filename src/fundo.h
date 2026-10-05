// FUNDO ATRAS DOS PAINEIS (Ajustes › Aparência › Fundo, Ajustes v2).
//
// Um modulo so para quem pinta o pano de fundo de uma tela inteira com a arte
// do titulo: Ajustes (e as telas que usam ajustes_ui_fundo) e, depois do
// merge, a pagina do titulo (detail.c). O modo e a escolha LOCAL desta TV:
//   FUNDO_ARTE    (0, padrao) a arte nitida em "cover" com o veu escuro do
//                 mockup (linear 90deg: 62%, 40% no meio, 50%), em degrade
//                 pelo nv_dither;
//   FUNDO_BORRADA (1) so as cores da arte: a luz assada de 320x180 da
//                 "Dinâmica imersiva" (gfx_ambiente) com a paleta da arte,
//                 saturacao +35% e brilho 62%, e um veu de 28%. Nenhum
//                 desfoque por quadro; o assado so refaz quando a paleta muda.
//                 Sem paleta ainda (arte nao decodificada), cai no ARTE;
//   FUNDO_FROST   (2) sem arte: cinza frio com tres luzes largas na cor de
//                 ajustes_acento() (58%, 38%, 14%), tudo por GFX_VEU_CSS e
//                 GFX_LUZ (nv_dither: painel de 8 bits nao faz faixa).
#ifndef NV_FUNDO_H
#define NV_FUNDO_H
#include "gfx.h"

#define FUNDO_ARTE    0
#define FUNDO_BORRADA 1
#define FUNDO_FROST   2

// O modo escolhido nesta TV (ajustes_fundo).
int  fundo_modo(void);
// Pinta o fundo no modo escolhido em `area` (coordenadas de layout; a tela
// inteira e o caso normal), com a arte `arteUrl` (a mesma chave do cache de
// texturas; NULL ou "" = sem arte, so a cor de fundo e o veu) e alfa `a`.
void fundo_desenhar(GfxRect area, const char *arteUrl, float a);
// O mesmo, num modo pedido (as previas de Ajustes › Fundo). `raioPx` arredonda
// os cantos (0 na tela cheia); fora da tela cheia o Borrada usa quatro luzes
// com a paleta no lugar do assado, que e de tela inteira.
void fundo_desenhar_modo(int modo, GfxRect area, float raioPx, const char *arteUrl, float a);

// Uma vez por quadro (main.c, depois de gfx_ambiente_preparar): com "Vidro
// fosco" ligado e sem luz imersiva, assa a arte borrada do TITULO EM CENA para o
// vidro (gfx_vidro_fosco) desenhar dentro dos paineis. Nada se nao ha arte em
// cena. Refaz so quando a paleta muda.
void fundo_fosco_quadro(void);

// A conferencia de uma vez do fundo de tela cheia `modo` (FUNDO_BORRADA ou
// FUNDO_FROST, fundo.c): -1 ainda nao, 1 o pixel da tela bateu com a conta do
// CPU, 0 errou (o fundo segue no desenho direto pela sessao).
int fundo_conferencia(int modo);

#endif
