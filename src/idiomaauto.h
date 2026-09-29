// IDIOMA AUTOMATICO DA INTERFACE — a parte pura: codigo de idioma (da conta ou
// da TV) -> um dos oito IDIOMA_* de idiomacod.h, e a ordem de precedencia.
// Header-only e sem dependencia de proposito (como idiomacod.h): o teste
// tests/idiomaauto.c o inclui sozinho, e ajustes.c so cuida de guardar e gravar.
//
// DE ONDE VEM A REGRA. No app web oficial (NuvioWeb 0.3.38) a conta NAO
// sincroniza o idioma da interface — ele mora em ThemeStore.language, local — e
// sem escolha manual o web usa o locale do sistema. A conta sincroniza, sim,
// tmdb_language e subtitle_preferred_language (profileSettingsSyncService.js),
// que dizem em que lingua a pessoa le. Aqui isso decide a interface enquanto a
// pessoa nunca escolheu uma na TV:
//   1. tmdb_language           ("pt-BR", "ro-RO", "uk", "es-419"...)
//   2. subtitle_preferred_language ("ro", "rum", "ron", "ukr", "fre", "ger"...)
//   3. o locale da TV          ("pt-BR", "en_US.UTF-8"...)
//   4. ingles
#ifndef NV_IDIOMAAUTO_H
#define NV_IDIOMAAUTO_H

#include <stddef.h>
#include "idiomacod.h"

// Origem da decisao, para o log e para o aviso.
enum { IDA_TMDB = 0, IDA_LEGENDA = 1, IDA_SISTEMA = 2, IDA_PADRAO = 3 };

static inline const char *idiomaauto_fonte_nome(int fonte) {
  switch (fonte) {
    case IDA_TMDB:    return "tmdb_language";
    case IDA_LEGENDA: return "legenda";
    case IDA_SISTEMA: return "sistema";
    default:          return "padrao";
  }
}

// Codigo curto do idioma (o de ISO 639-1), para o log.
static inline const char *idiomaauto_codigo(int idioma) {
  static const char *C[IDIOMA_N] = { "pt", "en", "ro", "uk", "ru", "fr", "de", "es" };
  return idioma >= 0 && idioma < IDIOMA_N ? C[idioma] : "en";
}

static inline int ida_igual(const char *a, const char *b) {
  for (; *a && *a == *b; a++, b++) {}
  return *a == *b;
}

// Um codigo de idioma qualquer -> IDIOMA_*, ou -1 se nao e um dos oito (ou nao
// e idioma: "", "none", "off", "DEVICE"). Aceita o que aparece de fato:
//   BCP-47 / locale   pt-BR  pt_BR  es-419  ro-RO  en_US.UTF-8  zh-Hans
//   ISO 639-1         pt en ro uk ru fr de es
//   ISO 639-2         por eng ron/rum ukr rus fra/fre deu/ger spa
//   OpenSubtitles     pob (portugues do Brasil), pb
// Sem distinguir caixa. Olha so o idioma PRIMARIO (antes do primeiro - _ . @):
// a regiao nao muda a lingua da interface (pt-PT tambem e "pt", es-419 e "es").
static inline int idiomaauto_mapear(const char *cod) {
  char p[8];
  size_t n = 0;
  if (!cod) return -1;
  while (*cod == ' ' || *cod == '\t') cod++;
  for (; *cod && *cod != '-' && *cod != '_' && *cod != '.' && *cod != '@' &&
         *cod != ' ' && *cod != ',' && *cod != ';'; cod++) {
    if (n + 1 >= sizeof p) return -1;             // mais longo que qualquer codigo
    p[n++] = (*cod >= 'A' && *cod <= 'Z') ? (char)(*cod + 32) : *cod;
  }
  p[n] = 0;
  if (n == 2) {
    if (ida_igual(p, "pt") || ida_igual(p, "pb")) return IDIOMA_PT;
    if (ida_igual(p, "en")) return IDIOMA_EN;
    if (ida_igual(p, "ro") || ida_igual(p, "mo")) return IDIOMA_RO;
    if (ida_igual(p, "uk")) return IDIOMA_UK;
    if (ida_igual(p, "ru")) return IDIOMA_RU;
    if (ida_igual(p, "fr")) return IDIOMA_FR;
    if (ida_igual(p, "de")) return IDIOMA_DE;
    if (ida_igual(p, "es")) return IDIOMA_ES;
    return -1;
  }
  if (n == 3) {
    if (ida_igual(p, "por") || ida_igual(p, "pob")) return IDIOMA_PT;
    if (ida_igual(p, "eng")) return IDIOMA_EN;
    if (ida_igual(p, "ron") || ida_igual(p, "rum")) return IDIOMA_RO;
    if (ida_igual(p, "ukr")) return IDIOMA_UK;
    if (ida_igual(p, "rus")) return IDIOMA_RU;
    if (ida_igual(p, "fra") || ida_igual(p, "fre")) return IDIOMA_FR;
    if (ida_igual(p, "deu") || ida_igual(p, "ger")) return IDIOMA_DE;
    if (ida_igual(p, "spa")) return IDIOMA_ES;
  }
  return -1;
}

// A REGRA. Cada fonte pode ser NULL ou vazia; uma que nao mapeia para um dos
// oito (a conta em italiano, digamos) e pulada e a proxima decide. `fonte`
// (opcional) recebe IDA_*.
static inline int idiomaauto_resolver(const char *tmdb, const char *legenda,
                                      const char *sistema, int *fonte) {
  int r, f;
  if ((r = idiomaauto_mapear(tmdb)) >= 0)         f = IDA_TMDB;
  else if ((r = idiomaauto_mapear(legenda)) >= 0) f = IDA_LEGENDA;
  else if ((r = idiomaauto_mapear(sistema)) >= 0) f = IDA_SISTEMA;
  else { r = IDIOMA_EN; f = IDA_PADRAO; }
  if (fonte) *fonte = f;
  return r;
}

#endif
