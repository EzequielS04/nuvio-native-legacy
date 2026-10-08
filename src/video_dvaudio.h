// Faixa de audio que o caminho Dolby Vision nao alimenta (A_TRUEHD, DTS-HD MA
// sem conversao etc.): em vez de entregar o MKV inteiro ao player da TV (HDR10),
// troca por uma faixa do MESMO idioma que o caminho aceita. Remux de DV costuma
// trazer TrueHD Atmos + um E-AC-3 (DD+ 7.1) do mesmo idioma; o E-AC-3 mantem o DV.
#ifndef NV_VIDEO_DVAUDIO_H
#define NV_VIDEO_DVAUDIO_H

#include <string.h>
#include <ctype.h>

// 0 = o caminho nao alimenta; 1..3 = alimenta (3 = E-AC-3, melhor).
static inline int nv_dvaudio_nota_codec(const char *c) {
  if (!strcmp(c, "A_EAC3")) return 3;
  if (!strcmp(c, "A_AC3")) return 2;
  if (!strncmp(c, "A_DTS", 5)) return 1;
  return 0;
}

static inline int nv_dvaudio_alimentavel(const char *c) { return nv_dvaudio_nota_codec(c) > 0; }

static inline int nv_dvaudio_tem_atmos(const char *nome) {
  size_t n = strlen(nome), i, k;
  static const char *chave[2] = {"atmos", "joc"};
  for (k = 0; k < 2; k++) {
    size_t m = strlen(chave[k]);
    for (i = 0; i + m <= n; i++) {
      size_t j;
      for (j = 0; j < m && tolower((unsigned char)nome[i + j]) == chave[k][j]; j++) {}
      if (j == m) return 1;
    }
  }
  return 0;
}

// Escolhe a faixa que mantem o DV no lugar de `atual`. Devolve o indice ou -1.
// codec[i] = CodecID do Matroska; idioma[i] = idioma da lista da TV; rotulo[i] = nome
// mostrado. `casa` compara dois idiomas (ling_casa no app). Idioma vazio so casa
// com idioma vazio. Ordem: E-AC-3 > AC-3 > DTS; no empate, a que cita Atmos/JOC;
// no empate final, a primeira do arquivo.
static inline int nv_dvaudio_escolher(int n, int atual, const char (*codec)[16],
                                      const char (*idioma)[8], const char (*rotulo)[48],
                                      int (*casa)(const char *, const char *)) {
  int i, melhor = -1, notaMelhor = 0, atmosMelhor = 0;
  if (atual < 0 || atual >= n) return -1;
  for (i = 0; i < n; i++) {
    int nota = nv_dvaudio_nota_codec(codec[i]), atmos;
    if (i == atual || !nota) continue;
    if (!idioma[atual][0] ? idioma[i][0] != 0 : (!idioma[i][0] || !casa(idioma[i], idioma[atual]))) continue;
    atmos = nv_dvaudio_tem_atmos(rotulo[i]);
    if (nota > notaMelhor || (nota == notaMelhor && atmos && !atmosMelhor)) {
      melhor = i; notaMelhor = nota; atmosMelhor = atmos;
    }
  }
  return melhor;
}

// Decisao como o dvPronto a usa. A escolha vem SO da lista do MKV (nMkv faixas):
// o player da TV costuma listar UMA faixa de audio num remux com TrueHD+E-AC-3
// (C9: "faixas: audio=1"), entao a lista dele nao pode limitar os candidatos.
// nTv fica na assinatura so para o teste deixar essa regra explicita.
// Devolve o indice da faixa NO MKV (ordinal entre as faixas de audio), ou -1.
static inline int nv_dvaudio_decidir(int nTv, int nMkv, int atual, const char (*codec)[16],
                                     const char (*idioma)[8], const char (*rotulo)[48],
                                     int (*casa)(const char *, const char *)) {
  (void)nTv;
  return nv_dvaudio_escolher(nMkv, atual, codec, idioma, rotulo, casa);
}

#endif
