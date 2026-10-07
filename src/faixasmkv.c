// Ver faixasmkv.h (#206).
#include "faixasmkv.h"
#include "idioma.h"
#include "linguas.h"
#include <stdio.h>
#include <string.h>

#define SEP "  \xc2\xb7  "
#define MKV_AUDIO 2
#define MKV_LEG   17

const char *faixasmkv_canais(int c) {
  return c == 8 ? "7.1" : c == 6 ? "5.1" : c == 2 ? "2.0" : "";
}

// Idioma da faixa: o do ARQUIVO quando ele etiqueta, senao o do player; e o
// NOME ganha dos dois quando cita outro idioma com todas as letras (mesma
// regra do video.c da LG: "Português" etiquetado eng e comum em release
// remontado).
//
// O ARQUIVO PRIMEIRO desde o #269. Antes era o player, e o player do .tpk da
// Samsung troca o codigo: "jp" para ja, "cz" para cs, "du" para nl, e "fr"
// para as legendas fil e fi do mesmo MKV (medido no log p2p-20). O cabecalho
// diz "pt-BR"/"es-419" onde o player diz "pt"/"es". Com as faixas casadas,
// o MKV e a fonte da verdade.
static void idiomaFinal(const VideoFaixa *f, const MkvFaixa *m, char *id, size_t t) {
  const char *peloNome = ling_do_nome(m->nome);
  snprintf(id, t, "%s", (m->idioma[0] && strcmp(m->idioma, "und")) ? m->idioma : f->idioma);
  if (peloNome && (!id[0] || !ling_casa(peloNome, id))) snprintf(id, t, "%s", peloNome);
}

// O nome repete o idioma ("English", uma palavra so)? Entao nao acrescenta.
static int nomeUtil(const MkvFaixa *m) {
  return m->nome[0] && !(ling_do_nome(m->nome) && !strchr(m->nome, ' '));
}

const char *faixasmkv_codec(const char *c) {
  static const struct { const char *pre, *nome; } T[] = {
    { "A_EAC3", "E-AC3" }, { "A_AC3", "AC3" }, { "A_TRUEHD", "TrueHD" },
    { "A_DTS/LOSSLESS", "DTS-HD MA" }, { "A_DTS/EXPRESS", "DTS-HD" }, { "A_DTS", "DTS" },
    { "A_AAC", "AAC" }, { "A_OPUS", "Opus" }, { "A_FLAC", "FLAC" }, { "A_VORBIS", "Vorbis" },
    { "A_MPEG/L3", "MP3" }, { "A_MPEG/L2", "MP2" }, { "A_PCM", "PCM" }, { "A_MLP", "MLP" },
  };
  size_t i;
  if (!c || !*c) return "";
  for (i = 0; i < sizeof T / sizeof *T; i++)
    if (!strncmp(c, T[i].pre, strlen(T[i].pre))) return T[i].nome;
  return "";
}

// Legenda de IMAGEM (PGS do Blu-ray, VobSub do DVD, DVB): o player do .tpk nao
// a lista. MEDIDO (p2p-20): MKV com 1 legenda S_HDMV/PGS, player com 0.
static int legendaImagem(const MkvFaixa *m) {
  return !strncmp(m->codec, "S_HDMV/", 7) || !strcmp(m->codec, "S_VOBSUB") ||
         !strcmp(m->codec, "S_DVBSUB");
}

// CASAMENTO DE UM TIPO. 1) Contagem igual: ordinal, como sempre. 2) Contagem
// diferente na legenda: tira as de imagem, que o player do .tpk nao lista, e
// tenta de novo. Fora disso fica como veio: rotulo errado e pior que
// "Audio 1". (Casar "pelo comeco" quando o player lista MENOS — ele para nas
// 27 primeiras de um MKV com 41, medido — foi tentado e ficou de fora: uma
// faixa escondida no meio desalinha tudo dali em diante sem que o idioma
// denuncie, e "Letreiros" na faixa de dialogo e pior que o codigo do player.)
// `mapa[k]` recebe o indice em `fx` da faixa k do player (-1 = nenhum).
static int casarTipo(int nF, const MkvFaixa *fx, int n, int tipo, int *mapa) {
  int cand[64], nc = 0, j, k, pulouImagem = 0;
  for (k = 0; k < nF; k++) mapa[k] = -1;
  if (nF < 1) return 0;
  for (j = 0; j < n && nc < 64; j++) if (fx[j].tipo == tipo) cand[nc++] = j;
  if (nc != nF && tipo == MKV_LEG) {
    int m = 0;
    for (j = 0; j < nc; j++) if (!legendaImagem(&fx[cand[j]])) cand[m++] = cand[j];
    pulouImagem = m != nc;
    nc = m;
  }
  if (nc != nF) return 0;
  for (k = 0; k < nF; k++) mapa[k] = cand[k];
  return pulouImagem ? 2 : 1;
}

int faixasmkv_overlay(const char *c, int textoSimples) {
  if (!c || !*c) return 0;
  if (!strncmp(c, "S_TEXT/ASS", 10) || !strncmp(c, "S_TEXT/SSA", 10)) return 1;
  return textoSimples && (!strcmp(c, "S_TEXT/UTF8") || !strcmp(c, "S_TEXT/WEBVTT"));
}

// Posicao da TrackEntry `j` entre as legendas do arquivo (o ordinal do mkvass).
static int ordinalLegenda(const MkvFaixa *fx, int j) {
  int k, o = 0;
  for (k = 0; k < j; k++) if (fx[k].tipo == MKV_LEG) o++;
  return o;
}

static int aplicarUma(VideoFaixa *f, const MkvFaixa *m, int ordinal) {
  char id[8], rot[sizeof f->rotulo], base[sizeof f->rotulo];
  idiomaFinal(f, m, id, sizeof id);
  // Sem idioma nenhum, a base e a de sempre ("Audio 1"), refeita e nao lida
  // do rotulo: aplicar duas vezes nao pode dar "Audio 1 · 5.1 · 5.1".
  if (id[0]) snprintf(base, sizeof base, "%s", i18n(ling_nome(id)));
  else if (m->tipo == MKV_AUDIO) snprintf(base, sizeof base, "%s %d", i18n("Áudio"), ordinal);
  else snprintf(base, sizeof base, "%s %d", i18n("Legenda"), ordinal);
  if (m->tipo == MKV_AUDIO) {
    // "Inglês · E-AC3 5.1" (#269: "nem o codec nem o 5.1/7.1"). O nome da
    // faixa, quando util, ja costuma trazer os dois ("DD 5.1", "Japanese 5.1
    // Opus") e vai no lugar do codec; os canais so entram se ele nao disser.
    const char *ch = faixasmkv_canais(m->canais);
    const char *cod = faixasmkv_codec(m->codec);
    int temNome = nomeUtil(m);
    const char *meio = temNome ? m->nome : cod;
    // #293: o player mostra o codec/canais da faixa tocando; a TrackEntry e a
    // unica fonte no .tpk (o Player nao entrega isso).
    snprintf(f->codec, sizeof f->codec, "%s", m->codec);
    f->canais = m->canais;
    int poeCh = ch[0] && !(meio[0] && strstr(meio, ch));
    snprintf(rot, sizeof rot, "%s%s%s%s%s", base,
             (meio[0] || poeCh) ? SEP : "", meio,
             (meio[0] && poeCh) ? " " : "", poeCh ? ch : "");
  } else {
    // LETREIROS/FORCADA (FlagForced, "Forced", "Signs & Songs"): a pessoa
    // precisa saber que essa nao traduz o dialogo. O nome do arquivo, quando
    // ele ja diz isso ("Forced"), e o que o .wgt mostra; so a flag vira
    // "Forçada" (#287: a flag e de forcada, nao de placas).
    int letreiro = ling_letreiro(m->nome, m->forcado);
    f->letreiro = letreiro;
    f->tipoLeg = ling_tipo_legenda(m->nome, m->forcado, m->sdh);
    if (letreiro && !(m->nome[0] && ling_letreiro(m->nome, 0)))
      snprintf(rot, sizeof rot, "%s%s%s", base, SEP, i18n(ling_tipo_legenda_rotulo(f->tipoLeg)));
    else if (nomeUtil(m))
      snprintf(rot, sizeof rot, "%s%s%s", base, SEP, m->nome);
    else
      snprintf(rot, sizeof rot, "%s", base);
  }
  // Rotulo por ULTIMO, de uma vez, depois do idioma: o desenho le sem trava.
  snprintf(f->idioma, sizeof f->idioma, "%s", id);
  if (strcmp(rot, f->rotulo)) { snprintf(f->rotulo, sizeof f->rotulo, "%s", rot); return 1; }
  return 0;
}

int faixasmkv_aplicar(VideoFaixa *aud, int nAud, VideoFaixa *leg, int nLeg,
                      const MkvFaixa *fx, int n) {
  int mapa[64], k, mudou = 0, comoA, comoL;
  if (!fx || n < 1) return 0;
  if (nAud > 64) nAud = 64;
  if (nLeg > 64) nLeg = 64;
  comoA = casarTipo(nAud, fx, n, MKV_AUDIO, mapa);
  for (k = 0; k < nAud; k++) if (mapa[k] >= 0) mudou += aplicarUma(&aud[k], &fx[mapa[k]], k + 1);
  comoL = casarTipo(nLeg, fx, n, MKV_LEG, mapa);
  for (k = 0; k < nLeg; k++) {
    if (mapa[k] < 0) continue;
    mudou += aplicarUma(&leg[k], &fx[mapa[k]], k + 1);
    snprintf(leg[k].codec, sizeof leg[k].codec, "%s", fx[mapa[k]].codec);
    leg[k].ordinalMkv = ordinalLegenda(fx, mapa[k]);
  }
  if ((nAud && comoA != 1) || (nLeg && comoL != 1)) {
    int nA = 0, nL = 0, j;
    for (j = 0; j < n; j++) { if (fx[j].tipo == MKV_AUDIO) nA++; else if (fx[j].tipo == MKV_LEG) nL++; }
    printf("[mkv] contagem: arquivo %d audio / %d legenda, player %d / %d; casamento audio=%s legenda=%s\n",
           nA, nL, nAud, nLeg,
           !nAud ? "-" : comoA == 1 ? "ordinal" : "none, labels kept",
           !nLeg ? "-" : comoL == 1 ? "ordinal" : comoL == 2 ? "without image subs" : "none, labels kept");
    fflush(stdout);
  }
  return mudou;
}
