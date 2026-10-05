// Avanco segurado: fluxo de teclas simulado a 100 ms e a 40 ms.
#include <stdio.h>
#include <math.h>
#include "../src/salto.h"

static int falhas;
#define CHECK(c, ...) do { if (!(c)) { printf("FALHA: " __VA_ARGS__); printf("\n"); falhas++; } } while (0)

// Segura a tecla por `holdMs` com repeticao a cada `repMs` (repeat=0, como o
// firmware da TV); devolve segundos.
static float segurar(unsigned holdMs, unsigned repMs, float dur) {
  SaltoEst st = {0}; float tot = 0;
  for (unsigned t = 0; t <= holdMs; t += repMs)
    tot += salto_tecla(&st, t == 0, 0, 1000u + t, dur);
  return tot;
}

// `n` toques separados por `gapMs`, cada um uma rajada de uma tecla so.
static float toques(int n, unsigned gapMs, float dur) {
  SaltoEst st = {0}; float tot = 0;
  for (int i = 0; i < n; i++)
    tot += salto_tecla(&st, i == 0, 0, 1000u + i * gapMs, dur);
  return tot;
}

int main(void) {
  const float filme = 7200.0f;
  CHECK(toques(5, 200, filme) == 50.0f, "5 toques em 1 s = 50 s, deu %.0f", toques(5, 200, filme));
  CHECK(toques(2, 0, filme) == 20.0f, "dois toques no mesmo instante = 20 s");
  CHECK(toques(3, 160, filme) == 30.0f, "toques a 160 ms nao sao repeticao");
  { SaltoEst st = {0}; float t = salto_tecla(&st, 1, 0, 1000, filme);   // flag do SDL
    t += salto_tecla(&st, 0, 1, 1050, filme);
    CHECK(t == 10.0f, "repeat=1 dentro de 300 ms e ignorado"); }
  CHECK(segurar(0, 100, filme) == 10.0f, "toque unico deve ser 10 s");
  unsigned holds[] = {1000, 3000, 10000};
  float lo[] = {30, 150, 800}, hi[] = {50, 400, 2400};
  for (int i = 0; i < 3; i++) {
    float a = segurar(holds[i], 100, filme), b = segurar(holds[i], 40, filme);
    printf("hold %5u ms: 100ms=%6.0f s  40ms=%6.0f s\n", holds[i], a, b);
    CHECK(a >= lo[i] && a <= hi[i], "%u ms a 100 ms fora da faixa: %.0f", holds[i], a);
    CHECK(b >= lo[i] && b <= hi[i], "%u ms a 40 ms fora da faixa: %.0f", holds[i], b);
    CHECK(fabsf(a - b) <= 0.15f * a, "repeticoes divergem: %.0f x %.0f", a, b);
  }
  // Episodio curto: teto de 60 s por passo.
  CHECK(salto_passo(20000, 1500.0f) == 60.0f, "teto de episodio curto");
  CHECK(salto_passo(20000, 7200.0f) == 120.0f, "teto de filme");
  CHECK(salto_passo(1499, 7200.0f) == 10.0f && salto_passo(1500, 7200.0f) == 30.0f, "degrau 1");
  { float c = segurar(10000, 100, 1500.0f); printf("episodio 25 min, 10 s: %.0f s\n", c);
    CHECK(c < segurar(10000, 100, filme), "episodio curto deve andar menos"); }
  printf(falhas ? "salto: %d falha(s)\n" : "salto OK\n", falhas);
  return falhas != 0;
}
