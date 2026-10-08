#include "dts_tv.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
int main(void) {
  /* TVs that decode DTS keep the native path (and the errorCode 200 fallback). */
  CHECK(nv_dts_tv_decodifica(5)); CHECK(nv_dts_tv_decodifica(10));
  CHECK(!nv_dts_converter_ja(10, 1, 1, 0));
  /* webOS 26 (starfish 11.x) and anything newer, or a marketing number: no DTS. */
  CHECK(!nv_dts_tv_decodifica(11)); CHECK(!nv_dts_tv_decodifica(12));
  CHECK(!nv_dts_tv_decodifica(26)); CHECK(!nv_dts_tv_decodifica(99));
  CHECK(nv_dts_converter_ja(11, 1, 1, 0));
  CHECK(nv_dts_converter_ja(26, 1, 1, 0));
  /* Not a DTS track, already tried, or no converter: do not start one. */
  CHECK(!nv_dts_converter_ja(26, 0, 1, 0));
  CHECK(!nv_dts_converter_ja(26, 1, 1, 1));
  CHECK(!nv_dts_converter_ja(26, 1, 0, 0));
  /* No DTS and no converter: say so instead of playing silent. */
  CHECK(nv_dts_sem_som(26, 1, 0)); CHECK(!nv_dts_sem_som(26, 1, 1));
  CHECK(!nv_dts_sem_som(26, 0, 0)); CHECK(!nv_dts_sem_som(7, 1, 0));
  if (!fails) puts("dts_tv ok");
  return fails != 0;
}
