// Geometria barata do voo da arte ate a mini capa. Compartilhada com a
// regressao de limites/timing; nenhum estado de desenho ou textura aqui.
#ifndef NV_ILHA_VOO_H
#define NV_ILHA_VOO_H
#include "gfx.h"

#define NV_ILHA_VOO_MS 400u
static inline float ilha_voo_fracao(unsigned decorrido) {
  float t;
  if (decorrido >= NV_ILHA_VOO_MS) return 1.0f;
  t = (float)decorrido / (float)NV_ILHA_VOO_MS;
  return t * t * t * (t * (6.0f * t - 15.0f) + 10.0f);
}
static inline GfxRect ilha_voo_rect(GfxRect alvo, float t, float W0, float H0, float *f) {
  float u = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
  float w = W0 + (alvo.w - W0) * u, h = H0 + (alvo.h - H0) * u;
  float cx = W0 * .5f + (alvo.x + alvo.w * .5f - W0 * .5f) * u;
  float cy = H0 * .5f + (alvo.y + alvo.h * .5f - H0 * .5f) * u;
  *f = u;
  return (GfxRect){cx - w * .5f, cy - h * .5f, w, h};
}
#endif
