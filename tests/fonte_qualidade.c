// R9 (04/10): "a escolha automatica pegou o menor e de pior resolucao em todos
// os filmes". Reproduz a lista do dono — debrid/Torrentio 4k, 1080p, 720p mais
// um plugin MegaEmbed 1080 MP4 sem tamanho — e fixa quem vence em cada estado
// do orcamento do StreamFit. Sem rede, sem GL: a lista em memoria e o
// stream_automatico (a mesma pontuacao da verificacao e da folha).
#include "streams.h"
#include "streamfit.h"
#include "ajustes.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void fonte(Stream *s, const char *prov, const char *rot, int altura, int mp4,
                  int foraCache, unsigned long long bytes, const char *host) {
  memset(s, 0, sizeof *s);
  snprintf(s->provedor, sizeof s->provedor, "%s", prov);
  snprintf(s->rotulo, sizeof s->rotulo, "%s", rot);
  snprintf(s->url, sizeof s->url, "https://%s/v/%s.%s", host, rot, mp4 ? "mp4" : "mkv");
  s->altura = altura; s->mp4 = mp4; s->foraCache = foraCache;
  s->tamanhoBytes = bytes; s->fileIdx = -1;
}
static void plugin(Stream *s, const char *rot, int altura) {
  fonte(s, "MegaEmbed", rot, altura, 1, 0, 0, "streamtape.invalid");
  snprintf(s->bingeGroup, sizeof s->bingeGroup, "nuvio-plugin|megaembed|%d", altura);
}
static const char *vence(const Stream *l, int n) {
  int i;
  usleep(250000);   // a foto do StreamFit no automatico vale 200 ms
  stream_definir_alvo("tt6933238");
  stream_definir_lista(l, n);
  i = stream_automatico();
  assert(i >= 0);
  return stream_item(i)->rotulo;
}
#define GB(x) ((unsigned long long)(x) * 1000000000ULL)

int main(void) {
  Stream l[8];
  int kb[8] = {5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000}, n;
  ajustes_dir("/tmp");   // qualidade "Automatica" por padrao
  streamfit_limpar();

  // 1) A LISTA DO DONO, sem medida nenhuma (hosts=0 na C9).
  n = 0;
  plugin(&l[n++], "MegaEmbed - 1080", 1080);
  fonte(&l[n++], "Torrentio", "Torrentio 720p", 720, 0, 0, 0, "td.invalid");
  fonte(&l[n++], "Torrentio", "Torrentio 1080p", 1080, 0, 0, 0, "td.invalid");
  fonte(&l[n++], "Torrentio", "Torrentio 4k", 2160, 0, 0, 0, "td.invalid");
  assert(!strcmp(vence(l, n), "Torrentio 4k"));
  // sem 4K: o 1080p de debrid (MKV) ganha do 1080 MP4 do embed, mesmo com o +5000 do MP4.
  assert(!strcmp(vence(l, n - 1), "Torrentio 1080p"));
  // 720p de debrid em cache tambem ganha de um embed 1080 (origem vem antes de altura).
  { Stream m[2]; plugin(&m[0], "MegaEmbed - 1080", 1080);
    fonte(&m[1], "Torrentio", "Torrentio 720p", 720, 0, 0, 0, "td.invalid");
    assert(!strcmp(vence(m, 2), "Torrentio 720p")); }
  // debrid FORA do cache so abre um aviso: o embed que toca agora passa na frente.
  { Stream m[2]; plugin(&m[0], "MegaEmbed - 1080", 1080);
    fonte(&m[1], "Debridio", "[AD] Debridio 1080p", 1080, 0, 1, 0, "td.invalid");
    assert(!strcmp(vence(m, 2), "MegaEmbed - 1080")); }
  // so o embed: ele toca como sempre.
  { Stream m[1]; plugin(&m[0], "MegaEmbed - 480", 480);
    assert(!strcmp(vence(m, 1), "MegaEmbed - 480")); }
  // a ordem de chegada nao decide: o embed chegando por ultimo ou por primeiro da no mesmo.
  { Stream m[2]; fonte(&m[0], "Torrentio", "Torrentio 1080p", 1080, 0, 0, 0, "td.invalid");
    plugin(&m[1], "MegaEmbed - 1080", 1080);
    assert(!strcmp(vence(m, 2), "Torrentio 1080p")); }

  // 2) ORCAMENTO DO STREAMFIT. 4K de 60 GB e 1080p de 1,5 GB, filme de 2 h.
  n = 0;
  plugin(&l[n++], "MegaEmbed - 1080", 1080);
  fonte(&l[n++], "Torrentio", "Torrentio 1080p", 1080, 0, 0, GB(1.5), "td.invalid");
  fonte(&l[n++], "Torrentio", "Torrentio 4k", 2160, 0, 0, GB(60), "td.invalid");
  stream_fit_duracao("tt6933238", 7200, SF_DUR_METADATA);
  streamfit_rede(7);
  // 2a) sem medida: NAO rebaixa, a 4K vence.
  assert(!strcmp(vence(l, n), "Torrentio 4k"));
  // 2b) pouca confianca (4 intervalos < 5): continua sem medida, a 4K vence.
  assert(streamfit_diagnostico(7, "https://td.invalid/x", kb, 4, streamfit_agora_ms()) == 0);
  assert(!strcmp(vence(l, n), "Torrentio 4k"));
  // 2c) medida confiavel de ~5 Mbps: a 4K (66 Mbps) e pesada e desce; o 1080p de 1,5 GB cabe.
  assert(streamfit_diagnostico(7, "https://td.invalid/x", kb, 8, streamfit_agora_ms()) == 8);
  assert(!strcmp(vence(l, n), "Torrentio 1080p"));
  // 2d) medida de outro host nao vale para este: volta a nao rebaixar.
  streamfit_limpar(); streamfit_rede(7);
  assert(streamfit_diagnostico(7, "https://outro.invalid/x", kb, 8, streamfit_agora_ms()) == 8);
  assert(!strcmp(vence(l, n), "Torrentio 4k"));
  // 2e) rede rapida: a 4K cabe e vence.
  { int rapido[8] = {90000, 90000, 90000, 90000, 90000, 90000, 90000, 90000};
    streamfit_limpar(); streamfit_rede(7);
    assert(streamfit_diagnostico(7, "https://td.invalid/x", rapido, 8, streamfit_agora_ms()) == 8);
    assert(!strcmp(vence(l, n), "Torrentio 4k")); }
  // 2f) tudo pesado e so sobra o embed: a debrid pesada AINDA ganha do embed.
  streamfit_limpar(); streamfit_rede(7);
  assert(streamfit_diagnostico(7, "https://td.invalid/x", kb, 8, streamfit_agora_ms()) == 8);
  { Stream m[2]; plugin(&m[0], "MegaEmbed - 1080", 1080);
    fonte(&m[1], "Torrentio", "Torrentio 4k", 2160, 0, 0, GB(60), "td.invalid");
    assert(!strcmp(vence(m, 2), "Torrentio 4k")); }

  streamfit_limpar();
  puts("fonte_qualidade: ok");
  return 0;
}
