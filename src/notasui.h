// DESENHO DAS NOTAS na pagina de titulo: a linha do hero (marca + valor na
// escala do site) e a secao "Notas" (heatmap por fonte + resumo + grade de
// episodios). As decisoes — escala, cor, ordem, encaixe — moram em
// notasfontes.c; aqui so se desenha.
#ifndef NV_NOTASUI_H
#define NV_NOTASUI_H
#include "extras.h"
#include "notasfontes.h"
#include "gfx.h"

// --- linha do titulo ---------------------------------------------------------
typedef struct {
  int   n;                       // itens que ficaram
  int   fonte[EX_NFONTES];
  int   cru[EX_NFONTES];
  float larg[EX_NFONTES];        // largura do item, SEM o vao que o precede
} NotasPlano;

// Escolhe as fontes (ajustes_nota_titulo E nota > 0), na ordem fixa da linha, e
// tira as de menor prioridade ate caber em `disp` (largura livre a partir do
// ponto em que a linha comeca). `leadPrimeiro` e o que precede o PRIMEIRO item
// (o ponto separador, ou 0 no comeco da linha); os demais levam 24 px.
// `cru[f]` e o extras_nota(f) — 0 = sem nota. `querer` (opcional, EX_NFONTES
// bytes) substitui ajustes_nota_titulo: serve ao teste.
void  notasui_planejar(NotasPlano *p, const int cru[EX_NFONTES], float disp,
                       float leadPrimeiro, const unsigned char *querer);
// Desenha o plano com o centro vertical em `yc`. Devolve o x onde parou.
float notasui_desenhar_linha(const NotasPlano *p, float x, float yc, float a);

// --- secao "Notas" -------------------------------------------------------------
typedef struct {
  int cru[EX_NFONTES];           // extras_nota(f) (IMDb ja com a reserva do catalogo)
  // Notas por episodio (serie). nTemp 0 = sem grade. Decimos: 72 = 7.2.
  int nTemp;
  int (*tempNum)(int t);
  int (*nEps)(int t);
  int (*epNum)(int t, int i);
  int (*epNota)(int t, int i);
} NotasSecao;

// A secao se divide em DUAS fileiras do D-pad, como a audiencia: o heatmap por
// fonte com o resumo, e a grade de episodios. Numa fileira so, a grade ficava
// abaixo da dobra sem nenhum jeito de o foco chegar nela.
int   notasui_fontes_tem(const NotasSecao *s);
int   notasui_grade_tem(const NotasSecao *s);
// Alturas do CONTEUDO (sem o cabecalho que detail.c desenha). Baratas: nao
// rasterizam nada.
float notasui_fontes_altura(const NotasSecao *s);
float notasui_grade_altura(const NotasSecao *s);
// Desenham com o canto superior esquerdo em (x, y); `y` pode estar fora da
// tela (nao custa nada). Devolvem a altura ocupada.
float notasui_fontes_desenhar(const NotasSecao *s, float x, float y, float a);
float notasui_grade_desenhar(const NotasSecao *s, float x, float y, float a);
// Volta o portao do texto (nova pagina de titulo).
void  notasui_reiniciar(void);

// Uma marca sozinha (para os cartoes da aba de notas): centralizada em (xc, yc)
// dentro de uma caixa `h` de altura. Devolve a largura usada.
float notasui_marca_cartao(int fonte, int cru, float xc, float yc, float h, float a);

#endif
