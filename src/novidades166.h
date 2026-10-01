// Cartao de novidades da 1.6.6: o layout Apple TV (barra em pilula, abertura
// em carrossel) e a Live TV que toca na LG. Previa viva a esquerda (tres cenas
// com os componentes de verdade), a lista agrupada a direita e tres pilulas.
#ifndef NV_NOVIDADES166_H
#define NV_NOVIDADES166_H
#include <SDL2/SDL.h>

// O numero da versao mora SO aqui: trocar o nome do lancamento e uma linha.
#define N166_VERSAO "1.6.6"

void novidades166_dir(const char *dirArte);   // pasta da arte do pacote
void novidades166_primeira_vez(void);
int  novidades166_aberto(void);
void novidades166_abrir(void);
void novidades166_evento(const SDL_Event *e);
void novidades166_atualizar(float dt, Uint32 agora);
void novidades166_desenhar(Uint32 agora);

#define N166_PEDIU_NADA   0
#define N166_PEDIU_LAYOUT 1   // Ajustes › Layout, na linha do layout da home
#define N166_PEDIU_GUIA   2   // o Guia de TV
int novidades166_pedido(void);

// Para a captura (tests/novidades166_shot.c): quantas cenas, qual esta na
// tela, e pular direto para uma delas com o relogio interno em `t` segundos.
int  novidades166_cenas(void);
int  novidades166_cena(void);
void novidades166_ir(int cena, float t);
// A lista: quantas linhas, e a largura da descricao da linha `i` no idioma
// atual contra o `limite` da coluna (a captura confere os 30 idiomas).
int  novidades166_itens(void);
int  novidades166_item_largura(int i, int *limite, const char **nome);
// 1 = a previa esta com o foco (cima), 0 = os botoes.
int  novidades166_foco_na_previa(void);

#endif
