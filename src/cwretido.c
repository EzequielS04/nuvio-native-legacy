// Ver cwretido.h.
#include "cwretido.h"
#include <stdio.h>
#include <string.h>

static char base[64];
static unsigned rev;

int cw_retido_definir(const char *imdb) {
  char b[64] = "";
  if (imdb && imdb[0]) idbase_copiar(imdb, b, sizeof b);
  if (!strcmp(b, base)) return 0;
  snprintf(base, sizeof base, "%s", b);
  rev++;
  printf("[cw] retido fora de Continuar assistindo: %s\n", base[0] ? base : "(nenhum)");
  fflush(stdout);
  return 1;
}

const char *cw_retido(void) { return base; }
unsigned cw_retido_rev(void) { return rev; }

int cw_retido_exclui(const char *imdb) {
  size_t L = strlen(base);
  return L && imdb && idbase_len(imdb) == L && !strncmp(imdb, base, L);
}
