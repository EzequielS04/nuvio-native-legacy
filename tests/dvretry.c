#include <assert.h>
#include <stdio.h>
#include "../src/video_dvretry.h"
int main(void) {
  NvDvSonda s = {0, 0};
  int n = 0;
  assert(nv_dvsonda_falhou(&s, &n) == 3000 && n == 1);
  assert(nv_dvsonda_falhou(&s, &n) == 8000 && n == 2);
  assert(nv_dvsonda_falhou(&s, &n) == 20000 && n == 3);
  assert(!s.desistiu);
  assert(nv_dvsonda_falhou(&s, &n) == 0 && s.desistiu && n == 3);   // esgotou
  nv_dvsonda_zerar(&s);
  assert(!s.desistiu && s.falhas == 0 && nv_dvsonda_falhou(&s, NULL) == 3000);
  assert(nv_dvsonda_pronta(1, 0) && !nv_dvsonda_pronta(1, 1) && !nv_dvsonda_pronta(0, 0));
  puts("dvretry ok");
  return 0;
}
