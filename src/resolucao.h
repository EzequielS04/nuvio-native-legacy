// INTERFACE RESOLUTION: the "Resolucao da interface" setting and the 4K
// fallback. Header-only on purpose: ajustes.c is compiled by many tests with a
// short source list, and these are pure functions (tests/resolucao.c).
//
// THE OWNER'S RULE (06/10/2026, after the 2.0.1 log triage: 14 Android TVs in
// 4K below 20 fps, Mali-G52 at 53-59 ms of GPU per frame against 13-16 at
// 1080p): "720p is the worst case, it is the ugliest".
//   - AUTOMATIC (the default) = 1080p. It never drops to 720p on its own: a
//     slow GPU loses EFFECTS (gpunivel.h levels 1 and 2), never pixels.
//   - 4K only when the person picks it. If the GPU cannot keep up (gpu-tempo
//     median > 30 ms, or FPS < 40 where the GPU clock is not available) for a
//     sustained stretch, the session falls back to 1080p (internal 1920x1080
//     target scaled to the 4K surface, gpun_alvo_1080), the island says so
//     once, and RES_ARQ_RECUO makes the next launches start at 1080p. Picking
//     4K again in Settings deletes the file and tries again.
//   - 720p only when the person picks it (gpun_forcar_720).
#ifndef NV_RESOLUCAO_H
#define NV_RESOLUCAO_H

// Values of AJ_RESOLUCAO, saved under the key "resolucaoUi".
enum { RES_AUTO = 0, RES_1080 = 1, RES_4K = 2, RES_720 = 3, RES_N };

// The old key "resolucao_ui" (up to 2.0.1) was { 0 1080p, 1 4K, 2 720p } with
// 1080p as the default, so a saved 0 cannot be told apart from "never chose":
// it becomes Automatic. 4K and 720p were explicit choices and stay.
static inline int res_migrar(int antigo) {
  if (antigo == 1) return RES_4K;
  if (antigo == 2) return RES_720;
  return RES_AUTO;
}

// Per-device memory of a 4K that did not hold (dados.h, data folder).
#define RES_ARQ_RECUO "resolucao-4k-recuo.txt"

// THE 4K WATCH. One sample per 3 s report (main.c). `valida` = the interface
// is what is being drawn (no player, no opening animation, past the warm-up).
#ifndef RES_4K_GPU_MS
#define RES_4K_GPU_MS     30.0
#endif   // gpu-tempo median above this = GPU-bound
#ifndef RES_4K_FPS_MIN
#define RES_4K_FPS_MIN    40.0
#endif   // without the GPU clock, FPS below this
#ifndef RES_4K_SEGUIDAS
#define RES_4K_SEGUIDAS   4
#endif      // 4 bad reports in a row = ~12 s sustained
typedef struct { int ruins; int recuou; } ResVigia;

// Returns 1 exactly once: on the sample that decides the fallback. A good
// sample resets the streak (a scroll burst is not "sustained"); an invalid one
// neither counts nor resets. `gpuMs` <= 0 = no GPU clock on this device.
static inline int res_vigia_amostra(ResVigia *v, double fps, double gpuMs, int valida) {
  int ruim;
  if (!v || v->recuou || !valida) return 0;
  ruim = gpuMs > 0.0 ? gpuMs > RES_4K_GPU_MS : fps < RES_4K_FPS_MIN;
  v->ruins = ruim ? v->ruins + 1 : 0;
  if (v->ruins < RES_4K_SEGUIDAS) return 0;
  v->recuou = 1;
  return 1;
}
#endif
