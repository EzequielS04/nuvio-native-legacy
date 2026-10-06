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
    ok(!strcmp(a[0].rotulo, "Áudio 1  \xc2\xb7  AAC 5.1"), "sem idioma: Audio 1 + codec + 5.1", a[0].rotulo); }

  // Contagem diferente (o player escondeu uma legenda): legendas ficam como
  // vieram, o audio (que bate) ainda ganha rotulo.
  player(a, 2, l, 3);
  faixasmkv_aplicar(a, 2, l, 3, fx, nf);
  ok(!strcmp(l[1].rotulo, "Húngaro") && !l[1].letreiro, "legendas 3x4: nada aplicado", l[1].rotulo);
  ok(strstr(a[1].rotulo, "2.0") != NULL, "audio 2x2: aplicado mesmo assim", a[1].rotulo);

  // #269: o player do .tpk nao lista legenda de IMAGEM (PGS). Arquivo com 4
  // legendas, uma delas PGS no meio; player com 3: casa sem a PGS.
  { MkvFaixa g[MKV_MAX_FAIXAS]; memcpy(g, fx, sizeof g);
    snprintf(g[4].codec, sizeof g[4].codec, "S_HDMV/PGS");   // a legenda 2 (Forced)
    player(a, 2, l, 3);
    faixasmkv_aplicar(a, 2, l, 3, g, nf);
    ok(!strcmp(l[0].rotulo, "Húngaro") && strstr(l[1].rotulo, "Letreiros") && strstr(l[2].rotulo, "English SDH"),
       "legendas 3x4 com uma PGS: casa sem ela", l[2].rotulo); }

  // #269: codigos do player da Samsung e o MKV mandando no idioma.
  { MkvFaixa g[MKV_MAX_FAIXAS]; memcpy(g, fx, sizeof g);
    player(a, 2, l, 4);
    snprintf(l[3].idioma, 8, "jp");            // o que o player diz
    snprintf(g[6].idioma, sizeof g[6].idioma, "pt-BR");   // o que o arquivo diz
    g[6].nome[0] = 0;
    faixasmkv_aplicar(a, 2, l, 4, g, nf);
    ok(!strcmp(l[3].idioma, "pt-BR") && !strcmp(l[3].rotulo, "Português (BR)"),
       "idioma do MKV vence o do player", l[3].rotulo);
    snprintf(g[2].codec, sizeof g[2].codec, "A_EAC3"); g[2].canais = 8; g[2].nome[0] = 0;
    player(a, 2, l, 4);
    faixasmkv_aplicar(a, 2, l, 4, g, nf);
    ok(!strcmp(a[1].rotulo, "Inglês  \xc2\xb7  E-AC3 7.1"), "audio: codec + 7.1", a[1].rotulo); }
  ok(!strcmp(faixasmkv_codec("A_DTS/LOSSLESS"), "DTS-HD MA") && !strcmp(faixasmkv_codec("A_AAC/MPEG4/LC"), "AAC") &&
     !faixasmkv_codec("V_MPEGH")[0], "nomes de codec", NULL);

  printf(falhas ? "faixasmkv: %d FALHA(S)\n" : "faixasmkv: tudo ok\n", falhas);
  return falhas != 0;
}
