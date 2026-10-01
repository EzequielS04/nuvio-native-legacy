// CARTAZ DO ADDON LIDO NA RAIZ DO ITEM (#200, mnguzd, LG G5, 1.6.4).
//
// O relato: com a URL de poster personalizada no AIOMetadata (BetterPosters,
// PostersPlus), alguns titulos (Breaking Bad, Ted Lasso) apareciam com o
// cartaz do TMDB no idioma da TV e SEM a nota; os outros com o cartaz do
// servico, com a nota. No app oficial, nao.
//
// A CAUSA (lida no codigo do AIOMetadata, addon/lib/getCache.ts): o meta que
// sai do CACHE dele e remontado com o bloco "basic" primeiro — e o basic leva
// `_providerArt: { poster, background, logo }`, a arte do provedor (TMDB, no
// idioma configurado). O `poster` da raiz, ja trocado pela URL do servico de
// cartaz, entra DEPOIS. deMeta lia "poster" com js_texto, que acha a PRIMEIRA
// chave com esse nome em qualquer nivel: o cartaz do _providerArt. Titulo que
// o AIOMetadata montou na hora (poster antes do _providerArt) saia certo — dai
// o "nao da para prever quais".
//
// O teste usa a forma reduzida das duas ordens e confere que o cartaz, o fundo
// e o logo sao os da raiz; e que um item sem cartaz na raiz ainda aproveita o
// aninhado (o comportamento antigo), em vez de sumir da fileira.
//
//   bash tests/posterprov.sh   (roda este tambem)
#define main detalheanime_main
#include "detalheanime.c"
#undef main

int main(void) {
  CatItem d;
  // 1) Ordem do meta remontado do cache do AIOMetadata: _providerArt antes.
  { const char *js =
      "{\"id\":\"tt0903747\",\"name\":\"Во все тяжкие\",\"type\":\"series\","
      "\"imdb_id\":\"tt0903747\",\"posterShape\":\"poster\",\"_hasPoster\":true,"
      "\"_metaProvider\":\"tmdb\","
      "\"_providerArt\":{\"poster\":\"https://image.tmdb.org/t/p/w500/ru.jpg\","
      "\"background\":\"https://image.tmdb.org/t/p/original/ru-bg.jpg\","
      "\"logo\":\"https://image.tmdb.org/t/p/original/ru-logo.png\",\"anime\":false},"
      "\"poster\":\"https://btttr.cc/K/imdb/poster-default/tt0903747.jpg?lang=ru\","
      "\"background\":\"https://cdn.exemplo/bg/tt0903747.jpg\","
      "\"logo\":\"https://cdn.exemplo/logo/tt0903747.png\"}";
    assert(deMeta(js, js + strlen(js), "series", &d));
    assert(!strcmp(d.poster, "https://btttr.cc/K/imdb/poster-default/tt0903747.jpg?lang=ru"));
    assert(!strcmp(d.backdrop, "https://cdn.exemplo/bg/tt0903747.jpg"));
    assert(!strcmp(d.logo, "https://cdn.exemplo/logo/tt0903747.png"));
    assert(!strcmp(d.imdb, "tt0903747")); }
  puts("ok  _providerArt antes do poster: vale o poster da raiz (servico de cartaz)");

  // 2) Ordem do meta montado na hora: ja saia certo e continua.
  { const char *js =
      "{\"id\":\"tt10986410\",\"name\":\"Тед Лассо\",\"type\":\"series\","
      "\"poster\":\"https://btttr.cc/K/imdb/poster-default/tt10986410.jpg\","
      "\"_providerArt\":{\"poster\":\"https://image.tmdb.org/t/p/w500/ru2.jpg\"}}";
    assert(deMeta(js, js + strlen(js), "series", &d));
    assert(!strcmp(d.poster, "https://btttr.cc/K/imdb/poster-default/tt10986410.jpg")); }
  puts("ok  poster antes do _providerArt: sem mudanca");

  // 3) Sem cartaz na raiz (null): o aninhado ainda serve, o item nao some.
  { const char *js =
      "{\"id\":\"tt1\",\"name\":\"X\",\"_providerArt\":{\"poster\":\"https://p/aninhado.jpg\"},"
      "\"poster\":null}";
    assert(deMeta(js, js + strlen(js), "movie", &d));
    assert(!strcmp(d.poster, "https://p/aninhado.jpg")); }
  puts("ok  sem poster na raiz: usa o aninhado, como antes");

  // 4) A ficha (/meta) passa pelo mesmo deMeta com o ponteiro em "meta":{...}.
  { const char *js =
      "{\"meta\":{\"id\":\"tt0903747\",\"name\":\"BB\","
      "\"_providerArt\":{\"poster\":\"https://image.tmdb.org/t/p/w500/ru.jpg\"},"
      "\"poster\":\"https://btttr.cc/K/p.jpg\"}}";
    const char *m = strstr(js, "\"meta\"");
    assert(deMeta(m, NULL, "series", &d));
    assert(!strcmp(d.poster, "https://btttr.cc/K/p.jpg")); }
  puts("ok  /meta do addon: mesma regra");

  puts("posteraddon: tudo ok");
  return 0;
}
