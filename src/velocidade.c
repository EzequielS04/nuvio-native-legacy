// Velocidade de reproducao — ver velocidade.h.
#include "velocidade.h"
#include <stdio.h>

const int VEL_LISTA[VEL_N] = { 75, 100, 125, 150, 175, 200 };

int vel_indice(int cent) {
  int i, melhor = 1, dMelhor = 1 << 30;
  for (i = 0; i < VEL_N; i++) {
    int d = VEL_LISTA[i] - cent;
    if (d < 0) d = -d;
    if (d < dMelhor) { dMelhor = d; melhor = i; }
  }
  return melhor;
}

int vel_passo(int cent, int dir) {
  int i = vel_indice(cent);
  if (dir > 0 && i + 1 < VEL_N) i++;
  else if (dir < 0 && i > 0) i--;
  return VEL_LISTA[i];
}

double vel_tempo_real(double segMidia, int cent) {
  if (cent <= 0) cent = VEL_NORMAL;
  return segMidia * (double)VEL_NORMAL / (double)cent;
}

void vel_rotulo(char *b, size_t n, int cent) {
  int inteiro = cent / 100, resto = cent % 100;
  if (!b || !n) return;
  if (cent <= 0) { snprintf(b, n, "1x"); return; }
  if (!resto) snprintf(b, n, "%dx", inteiro);
  else if (resto % 10 == 0) snprintf(b, n, "%d.%dx", inteiro, resto / 10);
  else snprintf(b, n, "%d.%02dx", inteiro, resto);
}

void vel_log(int cent, const char *estado) {
  printf("[player] velocidade %d.%02dx (%s)\n", cent / 100, cent % 100, estado ? estado : "?");
  fflush(stdout);
}
