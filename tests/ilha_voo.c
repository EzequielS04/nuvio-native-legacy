#include "ilha_voo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void testar(GfxRect alvo) {
  float anterior = -1.0f, w = 1921.0f, h = 1081.0f, f;
  for (unsigned ms = 0; ms <= 800; ms++) {
    float t = ilha_voo_fracao(ms);
    GfxRect r = ilha_voo_rect(alvo, t, 1920, 1080, &f);
    assert(t >= anterior && t >= 0 && t <= 1);
    assert(r.w <= w + .001f && r.h <= h + .001f && r.w >= alvo.w - .001f && r.h >= alvo.h - .001f);
    assert(r.x >= -.001f && r.y >= -.001f && r.x+r.w <= 1920.001f && r.y+r.h <= 1080.001f);
    if (ms == 0) assert(r.x == 0 && r.y == 0 && r.w == 1920 && r.h == 1080);
    if (ms >= NV_ILHA_VOO_MS) {
      assert(fabsf(r.x-alvo.x) < .001f && fabsf(r.y-alvo.y) < .001f);
      assert(fabsf(r.w-alvo.w) < .001f && fabsf(r.h-alvo.h) < .001f);
    }
    anterior = t; w = r.w; h = r.h;
  }
}
int main(void) {
  testar((GfxRect){180, 44, 32, 48});
  testar((GfxRect){1530, 44, 32, 48});
  assert(ilha_voo_fracao(200) == .5f);
  assert(ilha_voo_fracao(399) < 1 && ilha_voo_fracao(400) == 1);
  puts("ilha_voo: dois destinos, 400 ms, limites e movimento monotono ok");
}
