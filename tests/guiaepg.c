// Guia de TV: o EPG externo (epg.c) precisa casar o NOME que o addon da com o
// canal da grade XMLTV, e responder "o que esta no ar" e "o que vem a seguir".
//
// Molde minimo de XMLTV com os casos medidos no FrostView real:
//   - "Globo RJ" no addon vs "Globo.RJ.br" na grade (chave pelo id);
//   - "RecordTV Paulista" vs "Record TV" (prefixo depois de normalizar);
//   - "Canal Sony" vs "SONY" (palavra inutil na variante curta);
//   - "H2" vs "History 2" (apelido da tabela);
//   - "Sao.Paulo/SP..Cartoonito.br": id regional — a chave sai depois do "..";
//   - "TV.Aparecida.(aberta).br": parentese nao entra na chave;
//   - "SBT Thathi Vale": afiliada herda a rede "sbt" pelo primeiro token;
//   - "TV Cidade - RecordTV": substring — a chave mais comprida ("recordtv")
//     vence "record", e a mesma chave em dois canais nao e ambiguidade;
//   - canal "24h" sem grade nenhuma (nao casa, nunca).
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/epg.h"
#include "../src/rede.h"
#include <zlib.h>

// dubles do que epg.c referencia mas o teste nao exercita
char *rede_baixar_bin(const char *u, int s, long *n) { (void)u;(void)s;(void)n; return 0; }

// A GRADE DO PROVEDOR (#158) vem por aqui. `extraCorpo`/`extraN` e o que o
// "servidor" responde; `extraLimitado` simula o teto de bytes atingido.
static char *extraCorpo; static long extraN; static int extraLimitado, extraPedidos;
static char extraUltimaUrl[1100];
char *rede_baixar_bin_medido_controle(const char *url, int segundos,
                                      const char *const *cab, const RedeControle *c,
                                      long *tam, RedeMedida *m) {
  char *b;
  (void)segundos; (void)cab; (void)c;
  extraPedidos++;
  snprintf(extraUltimaUrl, sizeof extraUltimaUrl, "%s", url);
  if (m) { memset(m, 0, sizeof *m); m->status = 200; m->limitado = extraLimitado; }
  if (!extraCorpo) { if (m) m->status = 0; return NULL; }
  b = malloc((size_t)extraN);
  memcpy(b, extraCorpo, (size_t)extraN);
  *tam = extraN;
  return b;
}

static long gz(const char *in, char **out) {
  z_stream z; long cap = (long)strlen(in) + 256;
  *out = malloc((size_t)cap);
  memset(&z, 0, sizeof z);
  deflateInit2(&z, 6, Z_DEFLATED, 16 + 15, 8, Z_DEFAULT_STRATEGY);
  z.next_in = (Bytef *)in; z.avail_in = (uInt)strlen(in);
  z.next_out = (Bytef *)*out; z.avail_out = (uInt)cap;
  deflate(&z, Z_FINISH);
  deflateEnd(&z);
  return (long)z.total_out;
}

// Roda o fio da carga ate publicar (epg_passo e quem publica).
static int carregar(void) {
  int k;
  for (k = 0; k < 2000; k++) {
    struct timespec ts = { 0, 2000000 };
    epg_passo();
    if (epg_estado() == EPG_PRONTO || epg_estado() == EPG_FALHOU) return epg_estado();
    nanosleep(&ts, NULL);
  }
  return -1;
}
char *dados_ler(const char *n)        { (void)n; return 0; }
int   dados_gravar_leve(const char *n, const char *c) { (void)n;(void)c; return 1; }
void  dados_marcar_sujo(int l)        { (void)l; }
void  dados_fs_travar(void)           {}
void  dados_fs_liberar(void)          {}
char *dados_caminho(char *d, unsigned t, const char *n) { (void)d;(void)t;(void)n; return 0; }
int   dados_apagar(const char *n)     { (void)n; return 0; }
const char *dados_dir(void)           { return "/tmp"; }

static int falhas;
static void confere(const char *o_que, int ok) {
  printf("  %-64s %s\n", o_que, ok ? "ok" : "FALHOU");
  if (!ok) falhas++;
}

// Datas relativas a agora, em formato XMLTV, para o teste nao depender do dia.
static void xtvtime(char *dst, time_t t) {
  struct tm *m = gmtime(&t);
  strftime(dst, 24, "%Y%m%d%H%M%S +0000", m);
}

int main(void) {
  time_t agora = time(NULL);
  char ini1[24], fim1[24], ini2[24], fim2[24], ini3[24], fim3[24];
  char *xml;
  EpgProg p;

  xtvtime(ini1, agora - 1800); xtvtime(fim1, agora + 1800);   // no ar
  xtvtime(ini2, agora + 1800); xtvtime(fim2, agora + 5400);   // proximo
  xtvtime(ini3, agora + 5400); xtvtime(fim3, agora + 7200);   // depois

  const char *molde =
    "<?xml version=\"1.0\"?><tv>"
    "<channel id=\"Globo.RJ.br\"><display-name>Globo RJ</display-name></channel>"
    "<channel id=\"Record.TV.br\"><display-name>Record TV</display-name></channel>"
    "<channel id=\"Record.br\"><display-name>RECORD</display-name></channel>"
    "<channel id=\"SP..Record.TV.br\"><display-name>Record TV SP</display-name></channel>"
    "<channel id=\"Sony.br\"><display-name>SONY CHANNEL</display-name></channel>"
    "<channel id=\"History.2.br\"><display-name>History 2</display-name></channel>"
    "<channel id=\"Sao.Paulo/SP..Cartoonito.br\"><display-name>SP  Cartoonito HD</display-name></channel>"
    "<channel id=\"MG..TV.Aparecida.(aberta).br\"><display-name>MG  TV Aparecida</display-name></channel>"
    "<channel id=\"SBT.br\"><display-name>SBT</display-name></channel>"
    "<programme channel=\"Globo.RJ.br\" start=\"%s\" stop=\"%s\"><title>Jornal Nacional</title></programme>"
    "<programme channel=\"Globo.RJ.br\" start=\"%s\" stop=\"%s\"><title>Novela &amp; Cia</title></programme>"
    "<programme channel=\"Globo.RJ.br\" start=\"%s\" stop=\"%s\"><title>Filme da Noite</title></programme>"
    "<programme channel=\"Record.TV.br\" start=\"%s\" stop=\"%s\"><title>Jornal da Record</title></programme>"
    "</tv>";

  size_t cap = strlen(molde) + 400;
  xml = malloc(cap);
  snprintf(xml, cap, molde, ini1, fim1, ini2, fim2, ini3, fim3, ini1, fim1);

  confere("processa o molde sem erro", epg_xml_processar(xml) == 4);

  { int g = epg_match("Globo RJ");
    confere("Globo RJ casa pela forma do id", g >= 0);
    confere("agora = Jornal Nacional",
            g >= 0 && epg_agora(g, agora, &p) && !strcmp(p.titulo, "Jornal Nacional"));
    confere("proximo = Novela & Cia (entidade decodificada)",
            g >= 0 && epg_proximo(g, agora, 0, &p) && !strcmp(p.titulo, "Novela & Cia"));
    confere("depois = Filme da Noite",
            g >= 0 && epg_proximo(g, agora, 1, &p) && !strcmp(p.titulo, "Filme da Noite"));
  }
  confere("RecordTV Paulista herda a grade Record TV",
          epg_match("RecordTV Paulista") >= 0);
  confere("Canal Sony casa com SONY CHANNEL",
          epg_match("Canal Sony") >= 0);
  confere("H2 casa pelo apelido com History 2",
          epg_match("H2") >= 0);
  confere("id regional Sao.Paulo/SP..Cartoonito casa",
          epg_match("Cartoonito") >= 0);
  confere("parentese do id nao quebra TV Aparecida",
          epg_match("TV Aparecida") >= 0);
  confere("afiliada SBT Thathi Vale herda a rede sbt",
          epg_match("SBT Thathi Vale") >= 0);
  confere("TV Cidade - RecordTV casa pela chave mais comprida",
          epg_match("TV Cidade - RecordTV") >= 0);
  confere("canal 24h sem grade nao casa",
          epg_match("Aladdin 24h") < 0);
  confere("agora fora de canal valido devolve 0",
          !epg_agora(9999, agora, &p));

  // --- pelo id do canal (#158) ---------------------------------------------
  confere("id exato casa (Globo.RJ.br)",
          epg_match_id("Globo.RJ.br") >= 0 && epg_match_id("Globo.RJ.br") == epg_match("Globo RJ"));
  confere("id de canal sem programa nao e resposta (Sony.br)",
          epg_match_id("Sony.br") < 0);
  confere("id vazio ou desconhecido: -1",
          epg_match_id("") < 0 && epg_match_id("ProTV.ro") < 0);

  // --- a grade do provedor como sexta fonte ---------------------------------
  { char molde2[1200], *z = NULL; long nz;
    int e;
    snprintf(molde2, sizeof molde2,
      "\xEF\xBB\xBF\n<?xml version=\"1.0\"?><tv>"
      "<channel id=\"ProTV.ro\"><display-name>PRO TV HD</display-name></channel>"
      "<programme channel=\"ProTV.ro\" start=\"%s\" stop=\"%s\"><title>Stirile Pro TV</title></programme>"
      "</tv>", ini1, fim1);
    extraCorpo = molde2; extraN = (long)strlen(molde2);
    epg_fonte_extra("http://painel.exemplo:8080/xmltv.php?username=u&password=SEGREDO");
    epg_iniciar();
    e = carregar();
    confere("XML puro (com BOM) do provedor carrega", e == EPG_PRONTO);
    { int g = epg_match_id("ProTV.ro");
      confere("canal romeno casa pelo epg_channel_id",
              g >= 0 && epg_agora(g, agora, &p) && !strcmp(p.titulo, "Stirile Pro TV")); }
    confere("pediu a URL do provedor", strstr(extraUltimaUrl, "xmltv.php") != NULL);

    // gzip: o xmltv.php de alguns paineis responde comprimido
    nz = gz(molde2 + 4, &z);
    extraCorpo = z; extraN = nz;
    epg_fonte_extra("http://outro.exemplo/xmltv.php?username=u&password=p");
    e = carregar();                        // epg_passo ve a troca e recarrega
    confere("gzip do provedor carrega", e == EPG_PRONTO && epg_match_id("ProTV.ro") >= 0);

    // acima do teto: ignorada, e o resto da grade segue
    extraLimitado = 1;
    epg_fonte_extra("http://grande.exemplo/xmltv.php?username=u&password=p");
    e = carregar();
    confere("acima do teto: ignorada sem derrubar a carga", epg_match_id("ProTV.ro") < 0);
    extraLimitado = 0;

    // "" tira a fonte
    { int antes = extraPedidos;
      epg_fonte_extra("");
      e = carregar();
      confere("sem fonte extra, nada e pedido", extraPedidos == antes); }
    free(z); extraCorpo = NULL; }

  if (falhas) { printf("FALHOU: %d checagem(ns)\n", falhas); return 1; }
  puts("PASS: EPG casa nomes do addon com a grade e responde agora/proximos.");
  free(xml);
  return 0;
}
