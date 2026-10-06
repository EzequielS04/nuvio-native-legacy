// VELOCIDADE DE REPRODUCAO (#202): a lista, o passo e as contas.
//
// Pedido: "sinto falta de mudar a velocidade do video: 1.25x, 1.50x...". A
// linha mora na folha de AUDIO, logo abaixo do Volume (faixas.c), com a mesma
// anatomia e o mesmo gesto: ESQUERDA/DIREITA mudam na hora, sem OK.
//
// Seis valores e nao um continuo: no controle remoto cada valor e um toque, e
// de 0,75x a 2x em degraus de 0,25 cobre o que os players de referencia
// oferecem (o app web do Nuvio vai de 0,25x a 2x) sem virar uma lista longa.
//
// A velocidade e da REPRODUCAO, nao do aparelho: volta a 1x a cada titulo novo
// (faixas_reiniciar) e ao fechar o player (o trailer da home nunca herda 1,5x).
//
// Este modulo e so estado puro (sem SDL, sem video): o backend aplica
// (video_velocidade, video.h) e a UI le. Valores em CENTESIMOS (150 = 1,5x)
// para nao comparar float. Testes: tests/velocidade.c.
#ifndef NV_VELOCIDADE_H
#define NV_VELOCIDADE_H
#include <stddef.h>

#define VEL_NORMAL 100
#define VEL_N      6
extern const int VEL_LISTA[VEL_N];

// Indice do valor da lista mais proximo de `cent` (100 -> 1).
int vel_indice(int cent);
// O vizinho na lista, sem dar a volta nas pontas (dir > 0 sobe, < 0 desce).
// Um valor fora da lista anda a partir do mais proximo.
int vel_passo(int cent, int dir);
// `segMidia` segundos do arquivo levam quantos segundos de relogio a `cent`:
// o "termina as" da ilha e a contagem do proximo episodio usam isto.
double vel_tempo_real(double segMidia, int cent);
// "0.75x", "1x", "1.25x", "1.5x", "2x": ponto decimal; quem mostra passa por
// plrui_decimal (virgula nas linguas que escrevem com virgula).
void vel_rotulo(char *b, size_t n, int cent);
// A linha unica do registro: "[player] velocidade 1.50x (plataforma ok)".
void vel_log(int cent, const char *estado);

#endif
