// DETALHE DE ANIME: TIPO INCERTO NAO VIRA FILME (relato do Mizikashi1, v1.4.2).
//
// O log 2043 (LG, v1.4.2) mostrava, ao abrir Mushoku Tensei (tt13293588):
//   [desc] Mushoku Tensei: Jobless Reincarnation: 3 atores, dir='Miklós Jancsó', 0 temporadas
// O item vinha de um catalogo do AIOMetadata com `type: "anime"`; buscarEps
// fazia `ehFilme = tipo != "series"` e pedia /meta/movie/tt13293588 ao
// Cinemeta, que responde com OUTRO titulo ("My Way Home", 1965, dir. Miklos
// Jancso, 3 atores, 0 videos). A chave do cache de /meta era so o id, entao a
// abertura seguinte, ja como serie, lia o corpo do filme ("meta do cache",
// 0 episodios).
//
// O QUE ESTE TESTE PROVA (fixtures reduzidas das respostas reais do Cinemeta):
//   1. item de catalogo "anime" cujo meta diz "series" entra como serie;
//   2. item "anime" sem `type` proprio: buscarEps pergunta /meta/series
//      primeiro, publica episodios e grava tipo "series" no item, sem o
//      diretor do filme homonimo, e zera um id TMDB sem tipo provado;
//   3. o cache de /meta separa filme de serie do mesmo id;
//   4. filme de verdade continua um pedido so, /meta/movie;
//   5. id que nao e do IMDb (kitsu:) nao vai ao Cinemeta.
//
//   bash tests/detalheanime.sh
#include "../src/descoberta.c"
#include "../src/progresso.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

// --- DUBLES: nenhum participa da regra, so fazem descoberta.c linkar ---------
// (o conjunto e o de tests/colfileiras.c, menos os cat_* — que catalogo.c ja
// traz — e mais os que este teste ja tinha)
int         ajustes_idioma_ingles(void) { return 0; }
int   ajustes_cw_ordem(void)               { return 0; }   // Padrao (issue #127)
int   ajustes_cw_mostrar_nao_exibidos(void) { return 1; }
const char *i18n(const char *s)         { return s; }
const char *dados_dir(void)             { return ""; }
const char *sessao_usuario(void)        { return ""; }
int         perfis_ativo(void)          { return 1; }
int   ajustes_itens_fileira(void)          { return 12; }   // padrao (#163)
unsigned homeestado_geracao(void) { return 1; }
int homeestado_contexto_valido(void) { return 0; }
int homeestado_tem_fileira(const char *chave) { (void)chave; return 0; }
int homeestado_ordem_fileira(const char *chave) { (void)chave; return -1; }
int homeestado_salvar_se_geracao(const CatFileira *f, int n, unsigned g) {
  (void)f; (void)n; return g == 1;
}
int homeestado_identidade_geracao(unsigned g, char *d, unsigned z, int *p) {
  if (g != 1) return 0;
  if (d && z) d[0] = 0;
  if (p) *p = 1;
  return 1;
}
// Contexto em partes (homeestado.h, 1.4.5): constante aqui, entao nada muda
// no meio da montagem e o fim dela segue o caminho de sempre.
void homeestado_contexto(HomeContexto *c) { *c = (HomeContexto){0}; c->perfil = 1; }
int homeestado_mudancas(const HomeContexto *a, const HomeContexto *b) { (void)a; (void)b; return 0; }
const char *homeestado_mudancas_texto(int m, char *b, unsigned t) { (void)m; if (b && t) b[0] = 0; return b; }
int prog_ler(ProgRegistro *saida, int max) { (void)saida; (void)max; return 0; }
int prog_gravar_local(const char *imdb, int t, int e, double p, double d) {
  (void)imdb; (void)t; (void)e; (void)p; (void)d; return 0;
}

void  SDL_Delay(Uint32 ms)                 { usleep(ms * 1000); }
int   ajustes_cw_fonte(void)               { return 0; }
int   ajustes_tmdb_ligado(void)            { return 1; }
static int fakeMetaExterno, fakeTmdbBasico;
int   ajustes_meta_externo(void)           { return fakeMetaExterno; }
int   ajustes_tmdb_basico(void)            { return fakeTmdbBasico; }
int   ajustes_tmdb_arte(void)              { return 0; }
int   ajustes_tmdb_elenco(void)            { return 0; }
int   ajustes_tmdb_cw(void)                { return 0; }
static const char *fakeIdioma = "pt-BR";
const char *ajustes_tmdb_idioma(void)      { return fakeIdioma; }
const char *ajustes_tmdb_chave(void)       { return ""; }
void  fil_gravar_registro(void)            { }
int   fil_podar_catalogos(const char *const *ids, const char *const *bases, int n,
                          int perfilDaLista) {
  (void)ids; (void)bases; (void)n; (void)perfilDaLista; return 0; }
int   fil_addon_novo(const char *id, const char *base) { (void)id; (void)base; return 0; }
int   addons_perfil_da_lista(void)         { return 0; }
int   addons_ativo(int i)                  { (void)i; return 1; }
int   fil_limite(void)                     { return 16; }
int   fil_oculta(const char *c)            { (void)c; return 0; }
// Dubles da escolha da cota (#126): nada escolhido na TV, e o registro dos
// catalogos fora da cota nao interessa a este teste.
int fil_escolhida(const char *c) { (void)c; return -1; }
void fil_registrar_se_couber(const char *c, const char *t, const char *a,
                             const char *tp) { (void)c; (void)t; (void)a; (void)tp; }
void  fil_registrar(const char *c, const char *t, const char *a,
                    const char *tp, int itens) {
  (void)c; (void)t; (void)a; (void)tp; (void)itens;
}
int   fil_tem_ordem(void)                  { return 0; }
int   fil_unir(const char *const *c, int n, int *s, int m) {
  int i; (void)c; for (i = 0; i < n && i < m; i++) s[i] = i; return i;
}
void  marco(const char *n)                 { (void)n; }
void  prog_chave(char *d, unsigned n, const char *c, int t, int e) {
  (void)c; (void)t; (void)e; if (n) d[0] = 0;
}
void  prog_content_id(char *d, unsigned n, const char *i, int *t, int *e) {
  (void)i; (void)t; (void)e; if (n) d[0] = 0;
}
int   prog_por_chave(const char *c, ProgRegistro *s) { (void)c; (void)s; return 0; }
// "Tirar de Continuar assistindo" (desc_tirar_continuar, tests/cwremover.sh).
void  prog_remover(const char *c)          { (void)c; }
void  prog_marcar_removido(const char *i)  { (void)i; }
int   prog_removido_vence(const char *i, long long ms) { (void)i; (void)ms; return 0; }
int   trakt_continuar(CatItem *s, int m)   { (void)s; (void)m; return 0; }
int   trakt_continuar_falhou(void)        { return 0; }
// Simkl (issue #110): sem vinculo nos testes de fileira, como o Trakt acima.
int   simkl_ativo(void)                    { return 0; }
int   simkl_continuar(CatItem *s, int m)   { (void)s; (void)m; return 0; }
int   simkl_e_a_seguir(const char *id)     { (void)id; return 0; }
int   simkl_plantowatch(CatItem *s, int m) { (void)s; (void)m; return 0; }
int   ajustes_salvos_no_simkl(void)        { return 0; }
int   trakt_enfeitar_lote(CatItem *s, int n) { (void)s; (void)n; return 0; }
int   trakt_lista(const char *q, CatItem *s, int m) { (void)q; (void)s; (void)m; return 0; }
int   trakt_social(CatItem *s, int m)      { (void)s; (void)m; return 0; }
const char *nuvem_trakt_cliente(void)      { return ""; }
// Um addon de metadados, ligado so no caso 7 (o resto do teste roda sem addon).
static int addonMeta;
int   addons_n(void)                       { return addonMeta ? 1 : 0; }
int   addons_sondado(int i)                { (void)i; return 1; }
int   addons_fornece(int i, int oque)      { (void)i; return oque == ADD_META; }
const char *addons_base(int i)             { (void)i; return addonMeta ? "https://addon.test/SEGREDO" : ""; }
const char *addons_id_manifesto(int i)     { (void)i; return ""; }
const char *addons_nome(int i)            { (void)i; return "addon"; }
unsigned addons_versao(void)             { return 1; }   // estatico no teste
const char *addons_base_por_id(const char *id) { (void)id; return ""; }
void  addons_manifesto_lido(int i, const char *corpo) { (void)i; (void)corpo; }

// --- REDE FALSA: respostas do Cinemeta, reduzidas --------------------------
static const char *META_FILME_ERRADO =
  "{\"meta\":{\"id\":\"tt13293588\",\"type\":\"movie\",\"name\":\"My Way Home\","
  "\"director\":[\"Mikl\xc3\xb3s Jancs\xc3\xb3\"],"
  "\"cast\":[\"Andr\xc3\xa1s Koz\xc3\xa1k\",\"Sergey Nikonenko\",\"B\xc3\xa9la Barsi\"],"
  "\"moviedb_id\":94664,\"videos\":[]}}";
static const char *META_SERIE =
  "{\"meta\":{\"id\":\"tt13293588\",\"type\":\"series\","
  "\"name\":\"Mushoku Tensei: Jobless Reincarnation\",\"director\":[],"
  "\"cast\":[\"Yumi Uchiyama\",\"Tomokazu Sugita\",\"Ai Kayano\"],"
  "\"moviedb_id\":94664,\"videos\":["
  "{\"id\":\"tt13293588:1:1\",\"season\":1,\"episode\":1,\"name\":\"Jobless Reincarnation\"},"
  "{\"id\":\"tt13293588:1:2\",\"season\":1,\"episode\":2,\"name\":\"Getting Ahead of Myself\"},"
  "{\"id\":\"tt13293588:2:1\",\"season\":2,\"episode\":1,\"name\":\"The Brokenhearted Mage\"}]}}";
// O addon do usuario conhece 5 episodios da mesma serie; o Cinemeta so 3.
static const char *META_SERIE_ADDON =
  "{\"meta\":{\"id\":\"tt13293588\",\"type\":\"series\","
  "\"name\":\"Mushoku Tensei\",\"videos\":["
  "{\"season\":1,\"episode\":1,\"name\":\"A\"},{\"season\":1,\"episode\":2,\"name\":\"B\"},"
  "{\"season\":1,\"episode\":3,\"name\":\"C\"},{\"season\":2,\"episode\":1,\"name\":\"D\"},"
  "{\"season\":2,\"episode\":2,\"name\":\"E\"}]}}";
static const char *META_FILME_OK =
  "{\"meta\":{\"id\":\"tt0111161\",\"type\":\"movie\",\"name\":\"The Shawshank Redemption\","
  "\"director\":[\"Frank Darabont\"],\"cast\":[\"Tim Robbins\"],\"videos\":[]}}";

// Meta completo, como o Cinemeta devolve (#176): descricao, fundo, logo, ano.
static const char *META_COMPLETO =
  "{\"meta\":{\"id\":\"tt0000176\",\"type\":\"movie\",\"name\":\"Filme Salvo\","
  "\"poster\":\"https://p.test/poster.jpg\",\"background\":\"https://p.test/fundo.jpg\","
  "\"logo\":\"https://p.test/logo.png\",\"description\":\"Sinopse de verdade.\","
  "\"releaseInfo\":\"2019\",\"runtime\":\"2h 1min\",\"moviedb_id\":4242,"
  "\"imdbRating\":\"8.1\",\"genres\":[\"Drama\"],\"cast\":[\"Ator Um\"],\"videos\":[]}}";

// O que os addons/TMDB falsos respondem; cada caso liga o seu.
static const char *addonResp, *addonTipo = "/series/";
static const char *tmdbFind, *tmdbTemp1, *tmdbFilme, *cineSerie;
static char pedidos[64][600];
static int nPedidos;

char *rede_baixar(const char *u, int t) {
  (void)t;
  if (nPedidos < 64) snprintf(pedidos[nPedidos], sizeof pedidos[0], "%s", u);
  nPedidos++;
  if (strstr(u, "cinemeta")) {
    if (strstr(u, "/meta/movie/tt13293588.json"))  return strdup(META_FILME_ERRADO);
    if (strstr(u, "/meta/series/tt13293588.json")) return strdup(cineSerie ? cineSerie : META_SERIE);
    if (strstr(u, "/meta/movie/tt0111161.json"))   return strdup(META_FILME_OK);
    if (strstr(u, "/meta/movie/tt0000176.json"))   return strdup(META_COMPLETO);
  }
  // O addon de metadados responde o que o teste pos em `addonResp`, no tipo que
  // o teste pos em `addonTipo` (o de serie e o padrao dos casos antigos).
  if (strstr(u, "addon.test/SEGREDO/meta/") && addonResp &&
      strstr(u, addonTipo)) return strdup(addonResp);
  // TMDB (#176): so o que o teste liga em `tmdbResp*`.
  if (strstr(u, "themoviedb.org/3/find/")) return tmdbFind ? strdup(tmdbFind) : NULL;
  if (strstr(u, "themoviedb.org/3/tv/555/season/1?") && tmdbTemp1) return strdup(tmdbTemp1);
  if (strstr(u, "themoviedb.org/3/tv/555/season/")) return strdup("{\"episodes\":[]}");
  if (strstr(u, "themoviedb.org/3/movie/278?") && tmdbFilme) return strdup(tmdbFilme);
  return NULL;   // outros addons: fora do teste
}
char *rede_baixar_com(const char *u, int t, const char *const *c) {
  (void)c; return rede_baixar(u, t); }
int arte_reserva_registrar(const char *url, const char *imdb, int poster) {
  (void)url; (void)imdb; (void)poster; return 1; }

static int pediu(const char *trecho) {
  int i;
  for (i = 0; i < nPedidos && i < 64; i++) if (strstr(pedidos[i], trecho)) return 1;
  return 0;
}
int arte_reserva_episodios(const char *imdb, const char *corpo) { (void)imdb; (void)corpo; return 0; }

static void limparCacheMeta(void) {
  int i;
  for (i = 0; i < META_CACHE_N; i++) { free(metaCache[i].corpo); metaCache[i].corpo = NULL; }
}

static void catalogoCom(const char *imdb, const char *tipo, const char *titulo) {
  CatItem it;
  memset(&it, 0, sizeof it);
  snprintf(it.imdb, sizeof it.imdb, "%s", imdb);
  snprintf(it.tipo, sizeof it.tipo, "%s", tipo);
  snprintf(it.titulo, sizeof it.titulo, "%s", titulo);
  // Um tmdb qualquer, como o de um catalogo que o trouxesse com o tipo errado.
  it.tmdb = 94664;
  cat_definir_tudo(&it, 1, NULL, 0);
}

static void abrir(void) {
  nPedidos = 0;
  epItem = 0;
  fioEpVivo = 1;
  buscarEps(NULL);
}

int main(void) {
  // 1) deMeta: o `type` do item vence o "anime" do catalogo; o "type" de
  //    trailers[] (aninhado, e vem ANTES) nao conta.
  { const char *js =
      "{\"trailers\":[{\"source\":\"x\",\"type\":\"Trailer\"}],"
      "\"id\":\"tt13293588\",\"type\":\"series\",\"name\":\"Mushoku Tensei\","
      "\"poster\":\"https://p/x.jpg\"}";
    CatItem d;
    assert(deMeta(js, js + strlen(js), "anime", &d));
    assert(!strcmp(d.tipo, "series"));
    const char *js2 =
      "{\"id\":\"tt13293588\",\"name\":\"Mushoku Tensei\",\"poster\":\"https://p/x.jpg\"}";
    assert(deMeta(js2, js2 + strlen(js2), "anime", &d));
    assert(!strcmp(d.tipo, "anime"));   // sem prova, nao inventa
    const char *js3 =
      "{\"id\":\"tt1\",\"type\":\"other\",\"name\":\"X\",\"poster\":\"https://p/x.jpg\"}";
    assert(deMeta(js3, js3 + strlen(js3), "movie", &d));
    assert(!strcmp(d.tipo, "movie")); }
  puts("ok  tipo do item (movie/series) vence o do catalogo; o resto fica");

  // 2) Item "anime" sem tipo proprio: pergunta como serie, publica episodios,
  //    grava "series", nao traz o diretor do filme homonimo.
  limparCacheMeta();
  catalogoCom("tt13293588", "anime", "Mushoku Tensei: Jobless Reincarnation");
  abrir();
  assert(pediu("/meta/series/tt13293588.json"));
  assert(!pediu("/meta/movie/tt13293588.json"));
  { const CatItem *ci = cat_item(0);
    assert(ci);
    assert(!strcmp(ci->tipo, "series"));
    assert(ci->direcao[0] == 0);
    assert(ci->nTemporadas == 2);
    assert(ci->tmdb == 0);           // 94664 sem tipo provado nao fica
    assert(cat_n_episodios(0) == 3); }
  puts("ok  anime vira serie pelo /meta: 3 episodios, 2 temporadas, sem Jancso");

  // 3) Cache por tipo: o corpo do filme guardado para o mesmo id NAO serve a
  //    serie. Simula a sequencia do log: aberto como filme, depois como serie.
  limparCacheMeta();
  catalogoCom("tt13293588", "movie", "Mushoku Tensei: Jobless Reincarnation");
  abrir();
  assert(pediu("/meta/movie/tt13293588.json"));
  catalogoCom("tt13293588", "series", "Mushoku Tensei: Jobless Reincarnation");
  abrir();
  assert(pediu("/meta/series/tt13293588.json"));   // nao leu o do filme
  assert(cat_n_episodios(0) == 3);
  assert(cat_item(0)->direcao[0] == 0);
  puts("ok  cache do /meta separa filme e serie do mesmo id");

  // 4) Filme de verdade: um pedido, /meta/movie, tipo intacto.
  limparCacheMeta();
  catalogoCom("tt0111161", "movie", "The Shawshank Redemption");
  abrir();
  assert(nPedidos >= 1 && strstr(pedidos[0], "/meta/movie/tt0111161.json"));
  assert(!pediu("/meta/series/"));
  assert(!strcmp(cat_item(0)->tipo, "movie"));
  assert(!strcmp(cat_item(0)->direcao, "Frank Darabont"));
  puts("ok  filme continua um pedido so a /meta/movie");

  // 5) Id que nao e do IMDb nao vai ao Cinemeta (nem vira a chave "kitsu").
  limparCacheMeta();
  catalogoCom("kitsu:41370", "anime", "Mushoku Tensei");
  abrir();
  assert(!pediu("cinemeta"));
  assert(fioEpVivo == 0);
  puts("ok  id kitsu: nao pede /meta ao Cinemeta");

  // 6) As puras.
  { const char *t[2];
    assert(desc_meta_tipos("series", t) == 1 && !strcmp(t[0], "series"));
    assert(desc_meta_tipos("movie", t) == 1 && !strcmp(t[0], "movie"));
    assert(desc_meta_tipos("anime", t) == 2 && !strcmp(t[0], "series") && !strcmp(t[1], "movie"));
    assert(desc_meta_tipos("", t) == 2);
    assert(desc_meta_tem_temporadas(META_SERIE));
    assert(!desc_meta_tem_temporadas(META_FILME_ERRADO)); }
  // 7) Cinemeta com MENOS episodios que o addon de metadados (#174, #175): a
  //    lista do addon entra no lugar. Com o addon trazendo igual ou menos, a
  //    do Cinemeta fica.
  limparCacheMeta();
  addonMeta = 1;
  addonResp = META_SERIE_ADDON;
  catalogoCom("tt13293588", "series", "Mushoku Tensei: Jobless Reincarnation");
  abrir();
  assert(pediu("addon.test/SEGREDO/meta/series/tt13293588.json"));
  assert(cat_n_episodios(0) == 5);
  puts("ok  addon com mais episodios que o Cinemeta: lista do addon");
  limparCacheMeta();
  assert(desc_meta_n_episodios(META_SERIE) == 3);
  assert(desc_meta_n_episodios(META_FILME_ERRADO) == 0);
  assert(desc_meta_n_episodios(NULL) == 0);
  addonMeta = 0;
  catalogoCom("tt13293588", "series", "Mushoku Tensei: Jobless Reincarnation");
  abrir();
  assert(!pediu("addon.test"));
  assert(cat_n_episodios(0) == 3);
  puts("ok  sem addon de meta a lista do Cinemeta fica como era");
  // 8) ITEM RASO (#176): o que "Salvos" e a lista do Trakt entregam (titulo,
  //    poster, fundo/logo do metahub, "14" do Trakt) sai do detalhe igual ao
  //    que a busca entrega: sinopse, ano, tmdb, fundo e logo do /meta.
  limparCacheMeta();
  { CatItem it;
    memset(&it, 0, sizeof it);
    snprintf(it.imdb, sizeof it.imdb, "tt0000176");
    snprintf(it.tipo, sizeof it.tipo, "movie");
    snprintf(it.titulo, sizeof it.titulo, "Filme Salvo");
    snprintf(it.poster, sizeof it.poster, "https://p.test/poster.jpg");
    snprintf(it.backdrop, sizeof it.backdrop, "https://images.metahub.space/background/medium/tt0000176/img");
    snprintf(it.logo, sizeof it.logo, "https://images.metahub.space/logo/medium/tt0000176/img");
    snprintf(it.classificacao, sizeof it.classificacao, "14");
    it.naLista = 1;
    cat_definir_tudo(&it, 1, NULL, 0); }
  abrir();
  { const CatItem *ci = cat_item(0);
    assert(ci);
    assert(!strcmp(ci->sinopse, "Sinopse de verdade."));
    assert(!strncmp(ci->meta, "2019", 4));
    assert(ci->tmdb == 4242);
    assert(!strcmp(ci->backdrop, "https://p.test/fundo.jpg"));
    assert(!strcmp(ci->logo, "https://p.test/logo.png"));
    assert(ci->classificacao[0] == 0);
    assert(ci->naLista == 1);                            // o que era do item fica
    assert(!strcmp(ci->poster, "https://p.test/poster.jpg")); }
  puts("ok  item raso (salvos/Trakt) sai do detalhe com o meta da busca");
  // Item que JA tem sinopse nao e tocado (o do catalogo/busca).
  limparCacheMeta();
  { CatItem it;
    memset(&it, 0, sizeof it);
    snprintf(it.imdb, sizeof it.imdb, "tt0000176");
    snprintf(it.tipo, sizeof it.tipo, "movie");
    snprintf(it.titulo, sizeof it.titulo, "Filme Salvo");
    snprintf(it.poster, sizeof it.poster, "https://p.test/poster.jpg");
    snprintf(it.backdrop, sizeof it.backdrop, "https://x.test/outro-fundo.jpg");
    snprintf(it.sinopse, sizeof it.sinopse, "Sinopse do catalogo.");
    cat_definir_tudo(&it, 1, NULL, 0); }
  abrir();
  assert(!strcmp(cat_item(0)->sinopse, "Sinopse do catalogo."));
  assert(!strcmp(cat_item(0)->backdrop, "https://x.test/outro-fundo.jpg"));
  puts("ok  item completo nao e sobrescrito");

  // ===========================================================================
  // #176: CONTEUDO LOCALIZADO. O usuario tem um addon de metadados em ucraniano
  // e via o titulo, os episodios e a sinopse em ingles do Cinemeta.
  // ===========================================================================
  { static const char *EN =
      "{\"meta\":{\"id\":\"tt13293588\",\"type\":\"series\",\"name\":\"Jobless Reincarnation\","
      "\"description\":\"A 34-year-old is reborn.\",\"genres\":[\"Animation\",\"Fantasy\"],"
      "\"cast\":[\"Yumi Uchiyama\"],\"videos\":["
      "{\"id\":\"tt13293588:1:1\",\"season\":1,\"episode\":1,\"name\":\"Jobless Reincarnation\","
      "\"overview\":\"English 1\",\"thumbnail\":\"https://c/t1.jpg\"},"
      "{\"id\":\"tt13293588:1:2\",\"season\":1,\"episode\":2,\"name\":\"Getting Ahead of Myself\","
      "\"overview\":\"English 2\",\"thumbnail\":\"https://c/t2.jpg\"},"
      "{\"id\":\"tt13293588:2:1\",\"season\":2,\"episode\":1,\"name\":\"The Brokenhearted Mage\","
      "\"overview\":\"English 3\",\"thumbnail\":\"https://c/t3.jpg\"}]}}";
    static const char *UK3 =
      "{\"meta\":{\"id\":\"tt13293588\",\"type\":\"series\","
      "\"name\":\"Реінкарнація безробітного\",\"description\":\"Тридцятичотирирічний чоловік renasce.\","
      "\"genres\":[\"Анімація\",\"Фентезі\"],\"videos\":["
      "{\"season\":1,\"episode\":1,\"name\":\"Перший епізод\",\"overview\":\"Опис 1\"},"
      "{\"season\":1,\"episode\":2,\"name\":\"Другий епізод\",\"overview\":\"Опис 2\"},"
      "{\"season\":2,\"episode\":1,\"name\":\"Третій епізод\",\"overview\":\"Опис 3\"}]}}";
    static const char *UK2 =
      "{\"meta\":{\"id\":\"tt13293588\",\"type\":\"series\",\"name\":\"Реінкарнація\","
      "\"videos\":[{\"season\":1,\"episode\":1,\"name\":\"Перший епізод\",\"overview\":\"Опис 1\"},"
      "{\"season\":1,\"episode\":2,\"name\":\"Другий епізод\",\"overview\":\"Опис 2\"}]}}";
    const CatEp *e;
    cineSerie = EN;
    addonMeta = 1;
    addonTipo = "/series/";
    fakeIdioma = "en-US";

    // 8) SEM a preferencia e em ingles: nada muda (o Cinemeta continua mandando).
    limparCacheMeta();
    fakeMetaExterno = 0;
    addonResp = UK3;
    catalogoCom("tt13293588", "series", "Jobless Reincarnation");
    abrir();
    assert(cat_n_episodios(0) == 3);
    e = cat_episodio(0, 0);
    assert(!strcmp(e->nome, "Jobless Reincarnation") && !strcmp(e->sinopse, "English 1"));
    assert(!strcmp(cat_item(0)->titulo, "Jobless Reincarnation"));
    puts("ok  #176: sem a preferencia e em ingles o Cinemeta segue mandando");

    // 9) COM "Prefere a ficha do addon" (ajustes_meta_externo, que ninguem lia):
    //    titulo, sinopse, generos, nome e sinopse dos episodios vem do addon; o
    //    still que o addon nao tem vem do Cinemeta. A URL do addon nao vai ao log
    //    (o teste so ve o nome, "addon").
    limparCacheMeta();
    fakeMetaExterno = 1;
    catalogoCom("tt13293588", "series", "Jobless Reincarnation");
    abrir();
    assert(cat_n_episodios(0) == 3);
    e = cat_episodio(0, 0);
    assert(!strcmp(e->nome, "Перший епізод") && !strcmp(e->sinopse, "Опис 1"));
    assert(!strcmp(e->thumb, "https://c/t1.jpg"));       /* preenchido do Cinemeta */
    e = cat_episodio(0, 2);
    assert(e->temporada == 2 && !strcmp(e->nome, "Третій епізод"));
    assert(!strcmp(cat_item(0)->titulo, "Реінкарнація безробітного"));
    assert(strstr(cat_item(0)->sinopse, "Тридцятичотирирічний"));
    assert(strstr(cat_item(0)->genero, "Анімація"));
    assert(cat_item(0)->nElenco == 1);                    /* elenco do Cinemeta fica */
    assert(cat_item(0)->nTemporadas == 2);
    puts("ok  #176: com a preferencia, titulo/sinopse/generos/episodios saem do addon");

    // 10) Addon com MENOS episodios: a lista do Cinemeta fica (nao some episodio)
    //     e leva nome/sinopse do addon nos que os dois tem; o 3o segue em ingles.
    limparCacheMeta();
    addonResp = UK2;
    catalogoCom("tt13293588", "series", "Jobless Reincarnation");
    abrir();
    assert(cat_n_episodios(0) == 3);
    assert(!strcmp(cat_episodio(0, 0)->nome, "Перший епізод"));
    assert(!strcmp(cat_episodio(0, 1)->sinopse, "Опис 2"));
    assert(!strcmp(cat_episodio(0, 2)->nome, "The Brokenhearted Mage"));
    puts("ok  #176: addon com menos episodios: lista do Cinemeta com o texto do addon");

    // 11) Addon sem resposta valida ("meta":null) nao apaga o texto do Cinemeta.
    limparCacheMeta();
    addonResp = "{\"meta\":null}";
    catalogoCom("tt13293588", "series", "Jobless Reincarnation");
    abrir();
    assert(!strcmp(cat_item(0)->titulo, "Jobless Reincarnation"));
    assert(!strcmp(cat_episodio(0, 0)->nome, "Jobless Reincarnation"));
    puts("ok  #176: meta invalida do addon nao apaga nada");

    // 12) SEM a preferencia, UI em ucraniano-like (idioma nao ingles) e TMDB
    //     ligado: quem traduz e o TMDB. O addon NAO manda; o nome do episodio vem
    //     do TMDB, e "Episodio 2" (sem traducao) nao entra.
    limparCacheMeta();
    fakeMetaExterno = 0;
    fakeIdioma = "uk-UA";
    fakeTmdbBasico = 1;
    desc_tmdb_definir("0123456789abcdef0123456789abcdef");
    addonResp = UK3;
    tmdbFind = "{\"tv_results\":[{\"id\":555}]}";
    tmdbTemp1 =
      "{\"episodes\":["
      "{\"episode_number\":1,\"name\":\"Безробітне переродження\",\"overview\":\"TMDB 1\",\"vote_average\":8.1},"
      "{\"episode_number\":2,\"name\":\"Епізод 2\",\"overview\":\"\"}]}";
    catalogoCom("tt13293588", "series", "Jobless Reincarnation");
    abrir();
    assert(pediu("themoviedb.org/3/tv/555/season/1?"));
    assert(!strcmp(cat_item(0)->titulo, "Jobless Reincarnation"));    /* addon nao mandou */
    e = cat_episodio(0, 0);
    assert(!strcmp(e->nome, "Безробітне переродження") && !strcmp(e->sinopse, "TMDB 1"));
    assert(e->nota == 81);
    e = cat_episodio(0, 1);
    assert(!strcmp(e->nome, "Getting Ahead of Myself"));              /* generico fica de fora */
    assert(!strcmp(e->sinopse, "English 2"));
    puts("ok  #176: com TMDB num idioma nao ingles o nome do episodio vem traduzido; generico nao entra");

    // 13) COM a preferencia e o TMDB tambem ligado: o addon manda e o TMDB so
    //     preenche o que ficou vazio (nao pisa no texto do addon).
    limparCacheMeta();
    fakeMetaExterno = 1;
    catalogoCom("tt13293588", "series", "Jobless Reincarnation");
    abrir();
    assert(!strcmp(cat_episodio(0, 0)->nome, "Перший епізод"));
    assert(!strcmp(cat_episodio(0, 0)->sinopse, "Опис 1"));
    assert(cat_episodio(0, 0)->nota == 81);                           /* nota do TMDB entra */
    assert(!strcmp(cat_item(0)->titulo, "Реінкарнація безробітного")); /* TMDB nao troca */
    puts("ok  #176: addon preferido + TMDB: o texto do addon nao e pisado, a nota entra");

    // 14) FILME com a preferencia: /meta/movie do addon manda no titulo/sinopse.
    limparCacheMeta();
    fakeTmdbBasico = 0;
    tmdbFind = NULL; tmdbTemp1 = NULL;
    addonTipo = "/movie/";
    addonResp = "{\"meta\":{\"id\":\"tt0111161\",\"type\":\"movie\",\"name\":\"Втеча з Шоушенка\","
                "\"description\":\"Опис фільму\",\"genres\":[\"Драма\"]}}";
    catalogoCom("tt0111161", "movie", "The Shawshank Redemption");
    abrir();
    assert(!strcmp(cat_item(0)->titulo, "Втеча з Шоушенка"));
    assert(!strcmp(cat_item(0)->sinopse, "Опис фільму"));
    assert(!strcmp(cat_item(0)->genero, "Драма"));
    assert(!strcmp(cat_item(0)->direcao, "Frank Darabont"));          /* resto do Cinemeta */
    puts("ok  #176: filme com a preferencia: titulo/sinopse/generos do addon, resto do Cinemeta");

    // 15) O NOME GENERICO (funcao pura).
    assert(desc_nome_episodio_generico("Episode 3", 3));
    assert(desc_nome_episodio_generico("Episódio 12", 12));
    assert(desc_nome_episodio_generico("Серія 3", 3));
    assert(desc_nome_episodio_generico("#3", 3));
    assert(desc_nome_episodio_generico("", 3));
    assert(!desc_nome_episodio_generico("Episode 3", 4));
    assert(!desc_nome_episodio_generico("The Brokenhearted Mage", 1));
    assert(!desc_nome_episodio_generico("Season Finale 3", 3));
    assert(!desc_nome_episodio_generico("Apollo 13 Pt 2", 2));
    puts("ok  #176: nome generico do TMDB reconhecido, titulo de verdade nao");

    // 16) Mescla pura: modo TEXTO troca; modo VAZIOS so preenche.
    { CatEp a[2], o[2];
      memset(a, 0, sizeof a); memset(o, 0, sizeof o);
      a[0].temporada = 1; a[0].episodio = 1; snprintf(a[0].nome, sizeof a[0].nome, "A");
      a[1].temporada = 1; a[1].episodio = 2;
      o[0].temporada = 1; o[0].episodio = 1; snprintf(o[0].nome, sizeof o[0].nome, "O");
      snprintf(o[0].thumb, sizeof o[0].thumb, "t");
      o[1].temporada = 1; o[1].episodio = 2; snprintf(o[1].nome, sizeof o[1].nome, "O2");
      desc_mesclar_episodios(a, 2, o, 2, DESC_MESCLA_VAZIOS);
      assert(!strcmp(a[0].nome, "A") && !strcmp(a[1].nome, "O2") && !strcmp(a[0].thumb, "t"));
      desc_mesclar_episodios(a, 2, o, 2, DESC_MESCLA_TEXTO);
      assert(!strcmp(a[0].nome, "O")); }
    puts("ok  #176: mescla de episodios: texto sobrepoe, vazios so preenche");

    // CW / SPOTLIGHT ----------------------------------------------------------
    // 17) Texto localizado de um item do Continuar (titulo em ingles do Trakt):
    //     pela FICHA DO ADDON quando preferida, com um pedido por titulo.
    limparCacheMeta();
    memset(locCache, 0, sizeof locCache);
    fakeMetaExterno = 1;
    { CatItem it[2];
      memset(it, 0, sizeof it);
      snprintf(it[0].imdb, sizeof it[0].imdb, "tt0111161");
      snprintf(it[0].tipo, sizeof it[0].tipo, "movie");
      snprintf(it[0].titulo, sizeof it[0].titulo, "The Shawshank Redemption");
      snprintf(it[0].sinopse, sizeof it[0].sinopse, "English overview");
      it[0].poster[0] = 'x';
      snprintf(it[1].imdb, sizeof it[1].imdb, "cs:channel:abc");   /* canal: fora */
      snprintf(it[1].tipo, sizeof it[1].tipo, "channel");
      snprintf(it[1].titulo, sizeof it[1].titulo, "Canal");
      cat_definir_tudo(it, 2, NULL, 0);
      nPedidos = 0;
      desc_localizar_indices((int[]){ 0, 1 }, 2);
      while (locVivo) usleep(2000);
      assert(!strcmp(cat_item(0)->titulo, "Втеча з Шоушенка"));
      assert(!strcmp(cat_item(0)->sinopse, "Опис фільму"));
      assert(!strcmp(cat_item(1)->titulo, "Canal"));
      assert(nPedidos == 1);                                  /* canal nao pergunta */
      /* de novo: cache, nenhum pedido */
      nPedidos = 0;
      desc_localizar_indices((int[]){ 0 }, 1);
      while (locVivo) usleep(2000);
      assert(nPedidos == 0);
      /* refazer a fileira: o texto ja localizado entra sem rede */
      { CatItem v[1];
        memset(v, 0, sizeof v);
        snprintf(v[0].imdb, sizeof v[0].imdb, "tt0111161:1:2");
        snprintf(v[0].tipo, sizeof v[0].tipo, "movie");
        snprintf(v[0].titulo, sizeof v[0].titulo, "The Shawshank Redemption");
        assert(aplicarLocCache(v, 1) == 1);
        assert(!strcmp(v[0].titulo, "Втеча з Шоушенка"));
        assert(nPedidos == 0); } }
    puts("ok  #176: Continuar/destaque: titulo e sinopse do addon, um pedido por titulo, cache, canal fora");

    // 18) Sem addon e sem preferencia, o TMDB no idioma configurado traduz.
    limparCacheMeta();
    memset(locCache, 0, sizeof locCache);
    fakeMetaExterno = 0;
    addonMeta = 0;
    fakeTmdbBasico = 1;
    tmdbFind = "{\"movie_results\":[{\"id\":278}]}";
    tmdbFilme = "{\"id\":278,\"title\":\"Втеча з Шоушенка (TMDB)\",\"overview\":\"Опис TMDB\","
                "\"genres\":[{\"id\":18,\"name\":\"Драма\"}]}";
    { CatItem it[1];
      memset(it, 0, sizeof it);
      snprintf(it[0].imdb, sizeof it[0].imdb, "tt0111161");
      snprintf(it[0].tipo, sizeof it[0].tipo, "movie");
      snprintf(it[0].titulo, sizeof it[0].titulo, "The Shawshank Redemption");
      it[0].poster[0] = 'x';
      cat_definir_tudo(it, 1, NULL, 0);
      desc_localizar_indices((int[]){ 0 }, 1);
      while (locVivo) usleep(2000);
      assert(!strcmp(cat_item(0)->titulo, "Втеча з Шоушенка (TMDB)"));
      assert(!strcmp(cat_item(0)->sinopse, "Опис TMDB")); }
    puts("ok  #176: Continuar/destaque: sem addon, o TMDB no idioma configurado traduz");

    // 19) Em ingles e sem a preferencia nao ha o que localizar: zero pedidos.
    memset(locCache, 0, sizeof locCache);
    fakeIdioma = "en-US";
    nPedidos = 0;
    { CatItem it[1];
      memset(it, 0, sizeof it);
      snprintf(it[0].imdb, sizeof it[0].imdb, "tt0111161");
      snprintf(it[0].tipo, sizeof it[0].tipo, "movie");
      snprintf(it[0].titulo, sizeof it[0].titulo, "The Shawshank Redemption");
      it[0].poster[0] = 'x';
      cat_definir_tudo(it, 1, NULL, 0);
      desc_localizar_indices((int[]){ 0 }, 1);
      assert(!locVivo);
      assert(nPedidos == 0);
      assert(!strcmp(cat_item(0)->titulo, "The Shawshank Redemption")); }
    puts("ok  #176: em ingles e sem a preferencia nada e localizado (zero pedidos)");

    tmdbFind = tmdbTemp1 = tmdbFilme = NULL;
    cineSerie = NULL; addonResp = NULL; addonTipo = "/series/";
    fakeMetaExterno = fakeTmdbBasico = 0; fakeIdioma = "pt-BR";
    tmdbChave[0] = 0;
  }
  puts("detalheanime: tudo ok");
  return 0;
}
