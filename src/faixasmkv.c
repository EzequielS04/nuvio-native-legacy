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

// Idioma da faixa: o do player quando ha; senao o do arquivo; e o NOME ganha
// dos dois quando cita outro idioma com todas as letras (mesma regra do
// video.c da LG: "Português" etiquetado eng e comum em release remontado).
static void idiomaFinal(const VideoFaixa *f, const MkvFaixa *m, char *id, size_t t) {
  const char *peloNome = ling_do_nome(m->nome);
  snprintf(id, t, "%s", f->idioma[0] ? f->idioma
           : (m->idioma[0] && strcmp(m->idioma, "und")) ? m->idioma : "");
  if (peloNome && (!id[0] || !ling_casa(peloNome, id))) snprintf(id, t, "%s", peloNome);
}

// O nome repete o idioma ("English", uma palavra so)? Entao nao acrescenta.
static int nomeUtil(const MkvFaixa *m) {
  return m->nome[0] && !(ling_do_nome(m->nome) && !strchr(m->nome, ' '));
}

int faixasmkv_aplicar(VideoFaixa *aud, int nAud, VideoFaixa *leg, int nLeg,
                      const MkvFaixa *fx, int n) {
  int j, kA = 0, kL = 0, nA = 0, nL = 0, mudou = 0;
  if (!fx || n < 1) return 0;
  for (j = 0; j < n; j++) {
    if (fx[j].tipo == MKV_AUDIO) nA++;
    else if (fx[j].tipo == MKV_LEG) nL++;
  }
  for (j = 0; j < n; j++) {
    const MkvFaixa *m = &fx[j];
    VideoFaixa *f;
    char id[8], rot[sizeof aud[0].rotulo], base[sizeof aud[0].rotulo];
    if (m->tipo == MKV_AUDIO && nA == nAud && kA < nAud) f = &aud[kA++];
    else if (m->tipo == MKV_LEG && nL == nLeg && kL < nLeg) f = &leg[kL++];
    else continue;
    idiomaFinal(f, m, id, sizeof id);
    // Sem idioma nenhum, a base e a de sempre ("Audio 1"), refeita e nao lida
    // do rotulo: aplicar duas vezes nao pode dar "Audio 1 · 5.1 · 5.1".
    if (id[0]) snprintf(base, sizeof base, "%s", i18n(ling_nome(id)));
    else if (m->tipo == MKV_AUDIO) snprintf(base, sizeof base, "%s %d", i18n("Áudio"), kA);
    else snprintf(base, sizeof base, "%s %d", i18n("Legenda"), kL);
    if (m->tipo == MKV_AUDIO) {
      const char *ch = faixasmkv_canais(m->canais);
      int temNome = nomeUtil(m);
      int poeCh = ch[0] && !(temNome && strstr(m->nome, ch));
      snprintf(rot, sizeof rot, "%s%s%s%s%s", base,
               (temNome || poeCh) ? SEP : "", temNome ? m->nome : "",
               (temNome && poeCh) ? " " : "", poeCh ? ch : "");
    } else {
      // LETREIROS (FlagForced, "Forced", "Signs & Songs"): a pessoa precisa
      // saber que essa nao traduz o dialogo. O nome do arquivo, quando ele ja
      // diz isso ("Forced"), e o que o .wgt mostra; so a flag vira "Letreiros".
      int letreiro = ling_letreiro(m->nome, m->forcado);
      f->letreiro = letreiro;
      if (letreiro && !(m->nome[0] && ling_letreiro(m->nome, 0)))
        snprintf(rot, sizeof rot, "%s%s%s", base, SEP, i18n("Letreiros"));
      else if (nomeUtil(m))
        snprintf(rot, sizeof rot, "%s%s%s", base, SEP, m->nome);
      else
        snprintf(rot, sizeof rot, "%s", base);
    }
    // Rotulo por ULTIMO, de uma vez, depois do idioma: o desenho le sem trava.
    snprintf(f->idioma, sizeof f->idioma, "%s", id);
    if (strcmp(rot, f->rotulo)) { snprintf(f->rotulo, sizeof f->rotulo, "%s", rot); mudou++; }
  }
  if (nA != nAud || nL != nLeg) {
    printf("[mkv] contagem: arquivo %d audio / %d legenda, player %d / %d (tipo que nao bate fica como veio)\n",
           nA, nL, nAud, nLeg);
    fflush(stdout);
  }
  return mudou;
}
