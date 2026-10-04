// Apresentacao do AutoSync (F05): textos traduzidos, rotulos das acoes e o
// PROVEDOR da linha de sincronizacao do seletor de legendas do F04
// (legendasui.h, LegendasSyncProvider). Fica separado de legsync.c para o
// nucleo ser testado sem GL.
#include "legsync.h"
#include "idioma.h"
#include "plrui.h"
#include "player.h"
#include "legendasui.h"
#include <stdio.h>
#include <string.h>

static const int ORDEM[] = { LEGSYNC_ACAO_RAPIDA, LEGSYNC_ACAO_COMPLETA, LEGSYNC_ACAO_AUDIO, LEGSYNC_ACAO_DESFAZER,
                             LEGSYNC_ACAO_OUTRA, LEGSYNC_ACAO_PARAR };
#define N_ACOES ((int)(sizeof ORDEM / sizeof *ORDEM))

const char *legsync_acao_rotulo(int a) {
  switch (a) {
    case LEGSYNC_ACAO_RAPIDA:   return i18n("R\xc3\xa1pida");
    case LEGSYNC_ACAO_COMPLETA: return i18n("Completa");
    case LEGSYNC_ACAO_DESFAZER: return i18n("Desfazer");
    case LEGSYNC_ACAO_OUTRA:    return i18n("Outra refer\xc3\xaancia");
    case LEGSYNC_ACAO_PARAR:    return i18n("Parar");
    case LEGSYNC_ACAO_AUDIO:    return i18n("Por \xc3\xa1udio");
  }
  return "";
}

static void segundos(int ms, char *dst, unsigned tam) {
  snprintf(dst, tam, "%+.2f s", ms / 1000.0);
  plrui_decimal(dst);
}

// F06: por que "Por audio" nao aparece entre as acoes (o ajuste esta ligado).
static const char *motivoAudioTexto(LegSyncMotivo m) {
  switch (m) {
    case LEGSYNC_M_AUD_PLATAFORMA:  return i18n("Por \xc3\xa1udio: indispon\xc3\xadvel nesta plataforma");
    case LEGSYNC_M_AUD_PASSTHROUGH: return i18n("Por \xc3\xa1udio: indispon\xc3\xadvel com passthrough ligado");
    case LEGSYNC_M_AUD_SEM_AUDIO:   return i18n("Por \xc3\xa1udio: sem \xc3\xa1udio decodificado");
    case LEGSYNC_M_AUD_SEM_FALA:    return i18n("Por \xc3\xa1udio: sem falas claras; nada foi alterado");
    default: return NULL;
  }
}

static void textoBase(const LegSyncVisao *v, char *dst, unsigned tam);

void legsync_texto(const LegSyncVisao *v, char *dst, unsigned tam) {
  const char *a;
  if (!dst || !tam) return;
  textoBase(v, dst, tam);
  // O motivo do audio entra junto do estado, uma vez, quando ele ja nao e o
  // proprio estado (passthrough/plataforma da escuta pedida).
  a = motivoAudioTexto(v->motivoAudio);
  // Em repouso (pronta/desfeita) o motivo do audio SUBSTITUI o estado: as
  // acoes da embutida continuam visiveis entre < >, e o motivo cabe na linha.
  if (a && (v->fase == LEGSYNC_PRONTA || v->fase == LEGSYNC_DESFEITA)) { snprintf(dst, tam, "%s", a); return; }
  if (a && !motivoAudioTexto(v->motivo) && v->fase != LEGSYNC_DEPOIS) {
    size_t n = strlen(dst);
    if (n) snprintf(dst + n, tam > n ? tam - n : 0, " \xc2\xb7 %s", a);
    else snprintf(dst, tam, "%s", a);
  }
}

static void textoBase(const LegSyncVisao *v, char *dst, unsigned tam) {
  char s[32];
  const char *a;
  dst[0] = 0;
  switch (v->fase) {
    case LEGSYNC_DEPOIS:     snprintf(dst, tam, "%s", i18n("Segundo idioma: dispon\xc3\xadvel depois")); return;
    case LEGSYNC_AGUARDANDO: snprintf(dst, tam, "%s", i18n("Aguardando a legenda baixar")); return;
    case LEGSYNC_PRONTA:
      if (!(v->acoes & LEGSYNC_ACAO_RAPIDA) && (v->acoes & LEGSYNC_ACAO_AUDIO)) {
        snprintf(dst, tam, "%s", i18n("Pronta: compara com as falas do \xc3\xa1udio")); return;
      }
      snprintf(dst, tam, "%s", i18n("Pronta: compara com a legenda incorporada do arquivo")); return;
    case LEGSYNC_LENDO:      snprintf(dst, tam, i18n("Lendo a legenda incorporada\xe2\x80\xa6 %d%%"), v->progresso); return;
    case LEGSYNC_ANALISANDO: snprintf(dst, tam, "%s", i18n("Analisando\xe2\x80\xa6")); return;
    case LEGSYNC_ACEITA:     segundos(v->offsetAutoMs, s, sizeof s);
                             snprintf(dst, tam, i18n("Sincronizada: %s"), s); return;
    case LEGSYNC_OUVINDO:    snprintf(dst, tam, i18n("Ouvindo as falas\xe2\x80\xa6 %d%%"), v->progresso); return;
    case LEGSYNC_RECUSADA:
      if ((a = motivoAudioTexto(v->motivo)) != NULL) { snprintf(dst, tam, "%s", a); return; }
      snprintf(dst, tam, "%s", i18n("Sem confian\xc3\xa7" "a suficiente; nada foi alterado")); return;
    case LEGSYNC_PAUSADA:    snprintf(dst, tam, "%s", i18n("Pausada pela busca no v\xc3\xad" "deo; retoma sozinha")); return;
    case LEGSYNC_DESFEITA:   snprintf(dst, tam, "%s", i18n("Corre\xc3\xa7\xc3\xa3o desfeita; o atraso manual continua")); return;
    case LEGSYNC_INDISPONIVEL: break;
  }
  if ((a = motivoAudioTexto(v->motivo)) != NULL) { snprintf(dst, tam, "%s", a); return; }
  switch (v->motivo) {
    case LEGSYNC_M_EMBUTIDA:   snprintf(dst, tam, "%s", i18n("A legenda incorporada j\xc3\xa1 acompanha o v\xc3\xad" "deo")); break;
    case LEGSYNC_M_PLATAFORMA: snprintf(dst, tam, "%s", i18n("Indispon\xc3\xadvel nesta plataforma")); break;
    case LEGSYNC_M_EXTERNA_INCOMPLETA:
      snprintf(dst, tam, "%s", i18n("Legenda externa incompleta; n\xc3\xa3o d\xc3\xa1 para sincronizar")); break;
    case LEGSYNC_M_SEM_REFERENCIA:
      snprintf(dst, tam, "%s", i18n("Indispon\xc3\xadvel: sem legenda de texto indexada no arquivo")); break;
    case LEGSYNC_M_SEM_RANGE:  snprintf(dst, tam, "%s", i18n("Indispon\xc3\xadvel: o servidor n\xc3\xa3o aceita leitura parcial")); break;
    case LEGSYNC_M_REDE:       snprintf(dst, tam, "%s", i18n("Indispon\xc3\xadvel: falha de rede ao ler a refer\xc3\xaancia")); break;
    case LEGSYNC_M_ORCAMENTO:  snprintf(dst, tam, "%s", i18n("Indispon\xc3\xadvel: limite de dados atingido")); break;
    case LEGSYNC_M_SEM_OUTRA:  snprintf(dst, tam, "%s", i18n("Sem outra refer\xc3\xaancia completa neste arquivo")); break;
    default:                   snprintf(dst, tam, "%s", i18n("Escolha uma legenda externa para sincronizar")); break;
  }
}

// --- provedor do seletor (legendasui.c) ----------------------------------------
// So o slot PRINCIPAL e so com legenda EXTERNA ativa (embutida ja acompanha o
// video; nenhuma nao tem o que sincronizar); nunca em canal ao vivo. O segundo
// idioma ainda nao sincroniza: a linha dele nao aparece.
static int acoesAgora(int slot, int *lista, int max) {
  LegSyncVisao v;
  int i, n = 0;
  if (slot != 0) return 0;
  v = legsync_visao(0);
  for (i = 0; i < N_ACOES && n < max; i++) if (v.acoes & ORDEM[i]) lista[n++] = ORDEM[i];
  return n;
}

static const char *pEstado(int slot, void *u) {
  static char b[200];
  LegSyncVisao v;
  (void)u;
  if (slot != 0 || player_id_canal()[0]) return NULL;
  v = legsync_visao(0);
  if (v.fase == LEGSYNC_INDISPONIVEL && (v.motivo == LEGSYNC_M_SEM_EXTERNA || v.motivo == LEGSYNC_M_EMBUTIDA))
    return NULL;
  legsync_texto(&v, b, sizeof b);
  return b;
}

static int pAcoes(int slot, const char **rot, int max, void *u) {
  int l[N_ACOES], n = acoesAgora(slot, l, max < N_ACOES ? max : N_ACOES), i;
  (void)u;
  for (i = 0; i < n; i++) rot[i] = legsync_acao_rotulo(l[i]);
  return n;
}

static void pExecutar(int slot, int acao, void *u) {
  int l[N_ACOES], n = acoesAgora(slot, l, N_ACOES);
  (void)u;
  if (acao >= 0 && acao < n) legsync_acao(l[acao]);
}

void legsync_ui_ligar(void) {
  static const LegendasSyncProvider p = { pEstado, pAcoes, pExecutar, NULL };
  legendasui_definir_sync(&p);
}
