#ifndef NV_DTS_TV_H
#define NV_DTS_TV_H
/* Does this TV decode DTS itself? Decided from the starfish release major
 * (/etc/starfish-release, "release 11.2" -> 11), never from the TV's answer:
 * newer LGs play the DTS track SILENT instead of raising errorCode 200 (#285,
 * webOS 26), so waiting for the error never converts anything.
 * The LG C9 (2019, webOS 4) was the last LG with DTS: release major >= 5
 * (every 2020+ LG, and anything newer or marketing-numbered such as 26) is
 * assumed to have NO DTS. Below 5, or an unknown release (<= 0), keep the
 * native path and the errorCode 200 fallback. */
#define NV_DTS_TV_PRIMEIRA_SEM_DTS 5
static inline int nv_dts_tv_decodifica(int release_major) {
  return release_major <= 0 || release_major < NV_DTS_TV_PRIMEIRA_SEM_DTS;
}
/* Convert now (before any native play) a DTS track on a TV with no DTS. */
static inline int nv_dts_converter_ja(int release_major, int faixa_dts,
                                      int conversao_disponivel, int ja_tentou) {
  return faixa_dts && conversao_disponivel && !ja_tentou &&
         !nv_dts_tv_decodifica(release_major);
}
/* Silent playback is certain: no DTS on this TV and no conversion to run. */
static inline int nv_dts_sem_som(int release_major, int faixa_dts,
                                 int conversao_disponivel) {
  return faixa_dts && !conversao_disponivel && !nv_dts_tv_decodifica(release_major);
}
#endif
