// #202: velocidade de reproducao (src/velocidade.c): a lista, o passo sem dar
// a volta nas pontas, o rotulo e a conta do tempo de relogio que o "termina
// as" e a contagem do proximo episodio usam.
#include "../src/velocidade.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int perto(double a, double b) { return fabs(a - b) < 1e-9; }

int main(void) {
  char b[16];
  int i;
  // A lista: curta, crescente, com o 1x dentro.
  assert(VEL_N == 6);
  for (i = 1; i < VEL_N; i++) assert(VEL_LISTA[i] > VEL_LISTA[i - 1]);
  assert(VEL_LISTA[vel_indice(VEL_NORMAL)] == VEL_NORMAL);
  assert(VEL_LISTA[0] == 75 && VEL_LISTA[VEL_N - 1] == 200);

  // Passo: um toque = um valor; nas pontas fica parado (sem volta).
  assert(vel_passo(100, 1) == 125);
  assert(vel_passo(125, 1) == 150);
  assert(vel_passo(150, -1) == 125);
  assert(vel_passo(100, -1) == 75);
  assert(vel_passo(75, -1) == 75);
  assert(vel_passo(200, 1) == 200);
  assert(vel_passo(150, 0) == 150);
  // Fora da lista anda a partir do mais proximo.
  assert(vel_indice(130) == vel_indice(125));
  assert(vel_passo(130, 1) == 150);
  assert(vel_passo(0, 1) == 100);
  { int v = 75, n = 0; while (v != 200) { v = vel_passo(v, 1); n++; } assert(n == VEL_N - 1); }

  // Rotulo com ponto (plrui_decimal troca pela virgula onde a lingua pede).
  vel_rotulo(b, sizeof b, 100); assert(!strcmp(b, "1x"));
  vel_rotulo(b, sizeof b, 75);  assert(!strcmp(b, "0.75x"));
  vel_rotulo(b, sizeof b, 125); assert(!strcmp(b, "1.25x"));
  vel_rotulo(b, sizeof b, 150); assert(!strcmp(b, "1.5x"));
  vel_rotulo(b, sizeof b, 175); assert(!strcmp(b, "1.75x"));
  vel_rotulo(b, sizeof b, 200); assert(!strcmp(b, "2x"));
  vel_rotulo(b, sizeof b, 130); assert(!strcmp(b, "1.3x"));
  vel_rotulo(b, 3, 125); assert(strlen(b) == 2);   // corta sem estourar

  // Tempo de relogio: 60 min de arquivo a 1,5x sao 40 min; a 0,75x, 80 min.
  assert(perto(vel_tempo_real(3600.0, 100), 3600.0));
  assert(perto(vel_tempo_real(3600.0, 150), 2400.0));
  assert(perto(vel_tempo_real(3600.0, 200), 1800.0));
  assert(perto(vel_tempo_real(3600.0, 75), 4800.0));
  assert(perto(vel_tempo_real(5.0, 125), 4.0));      // contagem final do proximo episodio
  assert(perto(vel_tempo_real(3600.0, 0), 3600.0));  // valor invalido = 1x
  assert(perto(vel_tempo_real(0.0, 150), 0.0));

  printf("velocidade: lista, passo, rotulo e tempo de relogio ok\n");
  return 0;
}
