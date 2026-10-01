// Ver tests/faixasmkv.sh (#206).
#include "../src/faixasmkv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *i18n(const char *s) { return s; }   // sem tabela: o texto pt

static int falhas;
static void ok(int c, const char *o, const char *v) {
  printf("  %-56s %s  [%s]\n", o, c ? "ok" : "FALHOU", v ? v : "");
  if (!c) falhas++;
}
// O que o host do .tpk manda hoje (nv_tpk_video_faixa): idioma do player
// ("hu" nas legendas, vazio no audio, como no log da QE65Q80A) e "Audio N".
static void player(VideoFaixa *a, int nA, VideoFaixa *l, int nL) {
  int i;
  memset(a, 0, sizeof(VideoFaixa) * nA); memset(l, 0, sizeof(VideoFaixa) * nL);
  for (i = 0; i < nA; i++) { a[i].numero = i; a[i].ordinalMkv = -1; snprintf(a[i].rotulo, 48, "Áudio %d", i + 1); }
  for (i = 0; i < nL; i++) {
    l[i].numero = l[i].ordinalMkv = i;
    snprintf(l[i].idioma, 8, "%s", i < 2 ? "hu" : "en");
    snprintf(l[i].rotulo, 48, "%s", i < 2 ? "Húngaro" : "Inglês");
  }
}

int main(int argc, char **argv) {
  FILE *fp = fopen(argv[1], "rb");
  static unsigned char buf[4 << 20];
  long n = fp ? (long)fread(buf, 1, sizeof buf, fp) : 0;
  MkvFaixa fx[MKV_MAX_FAIXAS];
  VideoFaixa a[4], l[8];
  int nf, m;
  if (fp) fclose(fp);
  if (argc < 2) return 2;
  nf = mkv_faixas_do_trecho(buf, n, fx, MKV_MAX_FAIXAS, NULL, 0, NULL);
  printf("faixas no arquivo: %d\n", nf);
  ok(nf == 7, "cabecalho: 7 faixas", NULL);
  ok(nf >= 3 && fx[1].canais == 6 && fx[2].canais == 2, "Audio > Channels lido (6 e 2)", NULL);
  ok(nf >= 6 && fx[5].forcado == 1 && fx[3].forcado == 0, "FlagForced lida", NULL);

  player(a, 2, l, 4);
  m = faixasmkv_aplicar(a, 2, l, 4, fx, nf);
  ok(m == 5, "cinco rotulos mudaram (legenda 1 ja estava certa)", NULL);
  ok(!strcmp(a[0].rotulo, "HUN  \xc2\xb7  DD 5.1") || strstr(a[0].rotulo, "DD 5.1") != NULL,
     "audio 1: idioma + nome, sem repetir 5.1", a[0].rotulo);
  ok(a[0].idioma[0] && !strstr(a[0].rotulo, "Áudio") && !strstr(a[0].rotulo, "5.1 5.1"),
     "audio 1: nao e mais \"Audio 1\"", a[0].rotulo);
  ok(strstr(a[1].rotulo, "2.0") != NULL && strstr(a[1].rotulo, "Áudio") == NULL,
     "audio 2: idioma + 2.0", a[1].rotulo);
  ok(!l[0].letreiro && !strchr(l[0].rotulo, 0xc2), "legenda 1: so o idioma", l[0].rotulo);
  ok(l[1].letreiro && strstr(l[1].rotulo, "Forced") != NULL, "legenda 2: nome Forced, letreiro", l[1].rotulo);
  ok(l[2].letreiro && strstr(l[2].rotulo, "Letreiros") != NULL, "legenda 3: so a flag -> Letreiros", l[2].rotulo);
  ok(!l[3].letreiro && strstr(l[3].rotulo, "English SDH") != NULL, "legenda 4: nome SDH", l[3].rotulo);
  ok(l[1].numero == 1 && l[1].ordinalMkv == 1 && a[1].numero == 1, "indices do player intactos", NULL);

  // De novo, como na releitura do audio (#165): nada duplica.
  { char antes[48]; snprintf(antes, sizeof antes, "%s", a[0].rotulo);
    m = faixasmkv_aplicar(a, 2, l, 4, fx, nf);
    ok(m == 0 && !strcmp(antes, a[0].rotulo), "aplicar duas vezes nao muda nada", a[0].rotulo); }

  // Sem idioma em lugar nenhum: "Audio 1 · 5.1", e de novo sem duplicar.
  { MkvFaixa g[MKV_MAX_FAIXAS]; memcpy(g, fx, sizeof g);
    g[1].idioma[0] = 0; g[1].nome[0] = 0;
    player(a, 2, l, 4);
    faixasmkv_aplicar(a, 2, l, 4, g, nf);
    faixasmkv_aplicar(a, 2, l, 4, g, nf);
    ok(!strcmp(a[0].rotulo, "Áudio 1  \xc2\xb7  5.1"), "sem idioma: Audio 1 + 5.1", a[0].rotulo); }

  // Contagem diferente (o player escondeu uma legenda): legendas ficam como
  // vieram, o audio (que bate) ainda ganha rotulo.
  player(a, 2, l, 3);
  faixasmkv_aplicar(a, 2, l, 3, fx, nf);
  ok(!strcmp(l[1].rotulo, "Húngaro") && !l[1].letreiro, "legendas 3x4: nada aplicado", l[1].rotulo);
  ok(strstr(a[1].rotulo, "2.0") != NULL, "audio 2x2: aplicado mesmo assim", a[1].rotulo);

  printf(falhas ? "faixasmkv: %d FALHA(S)\n" : "faixasmkv: tudo ok\n", falhas);
  return falhas != 0;
}
