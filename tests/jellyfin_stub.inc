// Stubs for tests that link descoberta.c / app.c without jellyfin.c and a real
// ajustes.c: no personal server is connected, so no Jellyfin row or page exists.
#ifndef NV_JELLYFIN_STUB_INC
#define NV_JELLYFIN_STUB_INC
#include "catalogo.h"
int ajustes_jellyfin_ligado(void) { return 0; }
int jellyfin_conectado(void) { return 0; }
int jellyfin_chave_fileira(const char *chave) { (void)chave; return 0; }
int jellyfin_fileiras_copiar(CatItem *i, int mi, CatFileira *f, int mf, int *n) {
  (void)i; (void)mi; (void)f; (void)mf; if (n) *n = 0; return 0; }
int jellyfin_ficha(CatItem *item, CatEp *eps, int maxEps) { (void)item; (void)eps; (void)maxEps; return -1; }
unsigned jellyfin_fileiras_versao(void) { return 0; }
#endif
