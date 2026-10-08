#include "tirarremoto.h"
#include "trakt.h"
#include "syncprog.h"

void tirarremoto_executar(const char *imdb, const char *chave, int ocultar) {
  (void)ocultar;
  trakt_playback_remover(imdb);
  trakt_progresso_ocultar(imdb, 1);
  syncprog_remover(chave);
}
