// IDIOMA AUTOMATICO: o mapa codigo -> IDIOMA_* e a ordem de precedencia.
// Funcao pura (src/idiomaauto.h), sem SDL, sem disco.
#include "../src/idiomaauto.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define M(cod, esperado) do { \
  int r_ = idiomaauto_mapear(cod); \
  if (r_ != (esperado)) { printf("FALHOU mapear(\"%s\") = %d, esperava %d\n", \
    (cod) ? (cod) : "(nulo)", r_, (esperado)); return 1; } } while (0)

#define R(tmdb, leg, sis, idioma, origem) do { \
  int f_ = -1, r_ = idiomaauto_resolver(tmdb, leg, sis, &f_); \
  if (r_ != (idioma) || f_ != (origem)) { \
    printf("FALHOU resolver(%s, %s, %s) = %d/%d, esperava %d/%d\n", \
      (tmdb) ? (tmdb) : "NULL", (leg) ? (leg) : "NULL", (sis) ? (sis) : "NULL", \
      r_, f_, (idioma), (origem)); return 1; } } while (0)

int main(void) {
  // tmdb_language e locales, com regiao
  M("pt-BR", IDIOMA_PT);  M("pt-PT", IDIOMA_PT);  M("pt", IDIOMA_PT);
  M("pt_BR", IDIOMA_PT);  M("PT-br", IDIOMA_PT);
  M("en-US", IDIOMA_EN);  M("en-GB", IDIOMA_EN);  M("en", IDIOMA_EN);
  M("ro-RO", IDIOMA_RO);  M("ro", IDIOMA_RO);
  M("uk", IDIOMA_UK);     M("uk-UA", IDIOMA_UK);
  M("ru-RU", IDIOMA_RU);  M("ru", IDIOMA_RU);
  M("fr-FR", IDIOMA_FR);  M("fr-CA", IDIOMA_FR);
  M("de-DE", IDIOMA_DE);  M("de-AT", IDIOMA_DE);
  M("es-ES", IDIOMA_ES);  M("es-419", IDIOMA_ES);  M("es-MX", IDIOMA_ES);
  M("es", IDIOMA_ES);
  // locale de sistema com codificacao e modificador
  M("en_US.UTF-8", IDIOMA_EN);  M("pt_BR.UTF-8", IDIOMA_PT);
  M("ro_RO@euro", IDIOMA_RO);   M("de_DE.utf8", IDIOMA_DE);
  // ISO 639-2 (legenda): bibliografico e terminologico
  M("por", IDIOMA_PT);  M("pob", IDIOMA_PT);  M("eng", IDIOMA_EN);
  M("rum", IDIOMA_RO);  M("ron", IDIOMA_RO);  M("ukr", IDIOMA_UK);
  M("rus", IDIOMA_RU);  M("fre", IDIOMA_FR);  M("fra", IDIOMA_FR);
  M("ger", IDIOMA_DE);  M("deu", IDIOMA_DE);  M("spa", IDIOMA_ES);
  M("RUM", IDIOMA_RO);  M(" ron", IDIOMA_RO);
  // o que NAO e um dos oito, ou nem e idioma
  M(NULL, -1);   M("", -1);   M("none", -1);  M("off", -1);  M("DEVICE", -1);
  M("it", -1);   M("it-IT", -1);  M("ita", -1);  M("ja-JP", -1);  M("ko", -1);
  M("zh-CN", -1);  M("zh-Hans", -1);  M("nl", -1);  M("pl-PL", -1);
  M("C", -1);  M("POSIX", -1);  M("-", -1);  M("e", -1);
  M("portugues", -1);   // nome por extenso nao e codigo
  M("xxxxxxxxxxxxxxxxxxxxxxxx", -1);   // muito longo: nao estoura o buffer

  // precedencia: tmdb_language > legenda > sistema > ingles
  R("pt-BR", "ro", "de-DE", IDIOMA_PT, IDA_TMDB);
  R("ro-RO", NULL, NULL,    IDIOMA_RO, IDA_TMDB);
  R("es-419", "", "fr-FR",  IDIOMA_ES, IDA_TMDB);
  R("uk", "rus", "en-US",   IDIOMA_UK, IDA_TMDB);
  R(NULL, "rum", "en-US",   IDIOMA_RO, IDA_LEGENDA);
  R("", "fre", "de-DE",     IDIOMA_FR, IDA_LEGENDA);
  R("", "ger", NULL,        IDIOMA_DE, IDA_LEGENDA);
  R(NULL, NULL, "pt-BR",    IDIOMA_PT, IDA_SISTEMA);
  R("", "", "ru_RU.UTF-8",  IDIOMA_RU, IDA_SISTEMA);
  R(NULL, NULL, NULL,       IDIOMA_EN, IDA_PADRAO);
  R("", "", "",             IDIOMA_EN, IDA_PADRAO);
  // uma fonte que existe mas nao e um dos oito passa a vez para a proxima
  R("it-IT", "spa", "de-DE", IDIOMA_ES, IDA_LEGENDA);
  R("ja", "none", "fr-FR",   IDIOMA_FR, IDA_SISTEMA);
  R("ja", "ko", "zh-CN",     IDIOMA_EN, IDA_PADRAO);
  R("nl", "off", "C",        IDIOMA_EN, IDA_PADRAO);
  // fonte NULL no ponteiro de saida e aceita
  assert(idiomaauto_resolver("de", NULL, NULL, NULL) == IDIOMA_DE);

  assert(!strcmp(idiomaauto_fonte_nome(IDA_TMDB), "tmdb_language"));
  assert(!strcmp(idiomaauto_fonte_nome(IDA_LEGENDA), "legenda"));
  assert(!strcmp(idiomaauto_fonte_nome(IDA_SISTEMA), "sistema"));
  assert(!strcmp(idiomaauto_fonte_nome(IDA_PADRAO), "padrao"));
  { int i;
    for (i = 0; i < IDIOMA_N; i++)
      assert(idiomaauto_mapear(idiomaauto_codigo(i)) == i);   // codigo e mapa fecham o ciclo
  }
  puts("idiomaauto: ok");
  return 0;
}
