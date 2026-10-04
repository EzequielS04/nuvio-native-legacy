// Apresentacao do AutoSync (F05): textos traduzidos, rotulos das acoes e a
// linha minima da folha de legendas atual. Fica separado de legsync.c para o
// nucleo ser testado sem GL e para o coordenador mover a linha para o seletor
// do F04 (legendasui.c) no merge sem tocar o nucleo.
#include "legsync.h"
#include "idioma.h"
#include "gfx.h"
#include "text.h"
#include "plrui.h"
#include "ajustes.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

static const int ORDEM[] = { LEGSYNC_ACAO_RAPIDA, LEGSYNC_ACAO_COMPLETA, LEGSYNC_ACAO_DESFAZER,
                             LEGSYNC_ACAO_OUTRA, LEGSYNC_ACAO_PARAR };
#define N_ACOES ((int)(sizeof ORDEM / sizeof *ORDEM))
static int acaoSel;

const char *legsync_acao_rotulo(int a) {
  switch (a) {
    case LEGSYNC_ACAO_RAPIDA:   return i18n("R\xc3\xa1pida");
    case LEGSYNC_ACAO_COMPLETA: return i18n("Completa");
    case LEGSYNC_ACAO_DESFAZER: return i18n("Desfazer");
    case LEGSYNC_ACAO_OUTRA:    return i18n("Outra refer\xc3\xaancia");
    case LEGSYNC_ACAO_PARAR:    return i18n("Parar");
  }
  return "";
}

static void segundos(int ms, char *dst, unsigned tam) {
  snprintf(dst, tam, "%+.2f s", ms / 1000.0);
  plrui_decimal(dst);
}

void legsync_texto(const LegSyncVisao *v, char *dst, unsigned tam) {
  char s[32];
  if (!dst || !tam) return;
  dst[0] = 0;
  switch (v->fase) {
    case LEGSYNC_DEPOIS:     snprintf(dst, tam, "%s", i18n("Segundo idioma: dispon\xc3\xadvel depois")); return;
    case LEGSYNC_AGUARDANDO: snprintf(dst, tam, "%s", i18n("Aguardando a legenda baixar")); return;
    case LEGSYNC_PRONTA:     snprintf(dst, tam, "%s", i18n("Pronta: compara com a legenda incorporada do arquivo")); return;
    case LEGSYNC_LENDO:      snprintf(dst, tam, i18n("Lendo a legenda incorporada\xe2\x80\xa6 %d%%"), v->progresso); return;
    case LEGSYNC_ANALISANDO: snprintf(dst, tam, "%s", i18n("Analisando\xe2\x80\xa6")); return;
    case LEGSYNC_ACEITA:     segundos(v->offsetAutoMs, s, sizeof s);
                             snprintf(dst, tam, i18n("Sincronizada: %s"), s); return;
    case LEGSYNC_RECUSADA:   snprintf(dst, tam, "%s", i18n("Sem confian\xc3\xa7" "a suficiente; nada foi alterado")); return;
    case LEGSYNC_PAUSADA:    snprintf(dst, tam, "%s", i18n("Pausada pela busca no v\xc3\xad" "deo; retoma sozinha")); return;
    case LEGSYNC_DESFEITA:   snprintf(dst, tam, "%s", i18n("Corre\xc3\xa7\xc3\xa3o desfeita; o atraso manual continua")); return;
    case LEGSYNC_INDISPONIVEL: break;
  }
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

static int acaoValida(int acoes) {
  int i;
  if (acaoSel & acoes) return acaoSel;
  for (i = 0; i < N_ACOES; i++) if (ORDEM[i] & acoes) return acaoSel = ORDEM[i];
  return acaoSel = 0;
}

const char *legsync_linha_acao_chave(void) {
  LegSyncVisao v = legsync_visao(0);
  switch (acaoValida(v.acoes)) {
    case LEGSYNC_ACAO_RAPIDA:   return "R\xc3\xa1pida";
    case LEGSYNC_ACAO_COMPLETA: return "Completa";
    case LEGSYNC_ACAO_DESFAZER: return "Desfazer";
    case LEGSYNC_ACAO_OUTRA:    return "Outra refer\xc3\xaancia";
    case LEGSYNC_ACAO_PARAR:    return "Parar";
  }
  return NULL;
}

// Ha acao possivel antes/depois da escolhida (os discos < > apagam sem ela).
static int vizinha(int acoes, int atual, int passo) {
  int i, pos = -1;
  for (i = 0; i < N_ACOES; i++) if (ORDEM[i] == atual) pos = i;
  if (pos < 0) return 0;
  for (i = pos + passo; i >= 0 && i < N_ACOES; i += passo) if (ORDEM[i] & acoes) return 1;
  return 0;
}

int legsync_linha_tecla(int k) {
  LegSyncVisao v = legsync_visao(0);
  int i, atual = acaoValida(v.acoes), pos = -1;
  for (i = 0; i < N_ACOES; i++) if (ORDEM[i] == atual) pos = i;
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER) { if (atual) legsync_acao(atual); return 1; }
  if (k == SDLK_LEFT) {
    for (i = pos - 1; i >= 0; i--) if (ORDEM[i] & v.acoes) { acaoSel = ORDEM[i]; break; }
    return 1;
  }
  if (k == SDLK_RIGHT) {
    for (i = pos + 1; i < N_ACOES; i++) if (pos >= 0 && (ORDEM[i] & v.acoes)) { acaoSel = ORDEM[i]; return 1; }
    return 0;   // depois da ultima acao: a folha leva ao Estilo, como antes
  }
  return 0;
}

void legsync_linha_desenhar(float x, float y, float w, float h, int foco, float a) {
  LegSyncVisao v = legsync_visao(0);
  char sub[200], titulo[96];
  float ar, ag, ab, dir = 0.0f, tx, tw;
  int atual = acaoValida(v.acoes);
  GfxRect face = { x + 22.0f, y + (h - 52.0f) * 0.5f, 52.0f, 52.0f };
  ajustes_acento(&ar, &ag, &ab);
  if (foco) plrui_linha_foco((GfxRect){ x, y, w, h }, 22.0f, a);
  if (ajustes_vidro()) gfx_cor(face, 16.0f / face.h, 1, 1, 1, (foco ? 0.12f : 0.07f) * a);
  else gfx_cor(face, 16.0f / face.h, foco ? 0.22f : 0.125f, foco ? 0.227f : 0.129f, foco ? 0.259f : 0.153f, a);
  gfx_icone((GfxRect){ face.x + 15.0f, face.y + 15.0f, 22.0f, 22.0f }, "aj_wand", 1, 1, 1, (foco ? 1.0f : 0.7f) * a);
  legsync_texto(&v, sub, sizeof sub);
  snprintf(titulo, sizeof titulo, "%s", i18n("Sincroniza\xc3\xa7\xc3\xa3o autom\xc3\xa1tica"));
  // A DIREITA: com foco, a acao escolhida entre < >; sem foco, o offset aceito.
  if (foco && atual) {
    TxtLinha l = txt_linha(TXT_G16B, legsync_acao_rotulo(atual), 243, 242, 239, 255);
    float cx = x + w - 22.0f, cy = y + h * 0.5f, dw = 30.0f;
    float ad = vizinha(v.acoes, atual, 1) ? 1.0f : 0.3f, ae = vizinha(v.acoes, atual, -1) ? 1.0f : 0.3f;
    cx -= dw; gfx_cor((GfxRect){ cx, cy - 15.0f, dw, 30.0f }, 0.5f, 1, 1, 1, 0.10f * ad * a);
    gfx_icone((GfxRect){ cx + 6.0f, cy - 9.0f, 18.0f, 18.0f }, "pl_chevron-right", 1, 1, 1, ad * a);
    cx -= 10.0f + (float)l.w;
    txt_desenhar_alpha(l, cx, cy - (float)l.h * 0.5f, a);
    cx -= 10.0f + dw; gfx_cor((GfxRect){ cx, cy - 15.0f, dw, 30.0f }, 0.5f, 1, 1, 1, 0.10f * ae * a);
    gfx_icone((GfxRect){ cx + 6.0f, cy - 9.0f, 18.0f, 18.0f }, "pl_chevron-left", 1, 1, 1, ae * a);
    dir = x + w - cx + 12.0f;
  } else if (v.fase == LEGSYNC_ACEITA) {
    char s[32]; TxtLinha l;
    segundos(v.offsetAutoMs, s, sizeof s);
    l = txt_linha(TXT_G16B, s, (int)(ar * 255), (int)(ag * 255), (int)(ab * 255), 255);
    txt_desenhar_alpha(l, x + w - 22.0f - (float)l.w, y + (h - (float)l.h) * 0.5f, a);
    dir = (float)l.w + 34.0f;
  }
  tx = face.x + face.w + 18.0f; tw = w - (tx - x) - 22.0f - dir;
  { int c = foco ? 255 : 219, cs = v.fase == LEGSYNC_INDISPONIVEL || v.fase == LEGSYNC_DEPOIS ? 110 : 140;
    TxtLinha ln = txt_linha_corta(TXT_ILHA_FORTE, titulo, c, c, c - 2, 255, tw);
    TxtLinha ls = txt_linha_corta(TXT_ILHA_GENERO, sub, cs, cs, cs - 2, 255, tw);
    float th = (float)ln.h + 5.0f + (float)ls.h, ty = y + (h - th) * 0.5f;
    txt_desenhar_alpha(ln, tx, ty, a);
    txt_desenhar_alpha(ls, tx, ty + (float)ln.h + 5.0f, a); }
}
