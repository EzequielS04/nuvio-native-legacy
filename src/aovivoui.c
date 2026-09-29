// Desenho do OSD do canal ao vivo, do banner do zapping e do cartao de erro.
// Ver aovivo.h; separado da conta para ela ser testada sem GL.
//
// A MESMA CARA DO GUIA (revisao de 29/09/2026, pedido do dono: "ver se esta
// bom e se esta seguindo nossa identidade e o estilo do guia de TV atual").
// A primeira versao tinha vocabulario proprio para coisas que o guia ja
// resolve, e cada uma virou a do guia:
//   - marca do canal numa PLACA escura translucida -> guia_logo_desenhar, a
//     regra do dono de 16/09 ("sem azulejo"; recortado vira branco);
//   - numero numa pilula cinza e nome em 56 px, disputando com o titulo do
//     programa -> numero cinza solto + nome em HEADLINE, como a linha do canal
//     do heroi do guia; o degrau grande fica com o PROGRAMA (TITULO2);
//   - AO VIVO com ponto, em TXT_MINI, ao lado do nome -> o selo do guia, na
//     linha do horario do programa (e o programa que esta no ar);
//   - "HD" num chip de texto -> a MARCA de formato (badges.h), como no resto
//     do app; categoria como texto solto -> a etiqueta do guia;
//   - "A SEGUIR" num bloco a direita, cortado em 520 px e sem alinhar com
//     nada -> a linha unica do guia (rotulo, hora, titulo) na coluna do texto;
//   - barra de 8 px na largura do bloco -> o trilho de 4 px do heroi do guia;
//   - pilulas de botao com numeros proprios (68 px, texto de 20 px, repouso em
//     branco a 14%) -> botao_pilula secundario de botoes.c, a tabela unica;
//   - "Carregando o fluxo" em AMBAR cravado -> texto secundario neutro.
#include "aovivo.h"
#include "ajustes.h"
#include "badges.h"
#include "botoes.h"
#include "guia.h"
#include "gfx.h"
#include "text.h"
#include "layout.h"
#include "idioma.h"
#include <stdio.h>
#include <string.h>

// --- desenho ---------------------------------------------------------------------
#define AV_X        96.0f    // PLR_MARGEM: o recuo do conteudo do player de filme
#define AV_Y        56.0f
#define AV_LOGO_W  150.0f    // caixa da marca no alto (a marca cabe dentro dela)
#define AV_LOGO_H   84.0f
#define AV_LOGO_TOM 0.965f   // G_LOGO_CLARO do guia: marca recortada em branco
#define AV_TEXTO_W 1180.0f   // coluna do programa (meta, sinopse, a seguir)
#define AV_TITULO_W 1480.0f  // o titulo do programa pode ir um pouco alem
#define AV_TRILHO_W 560.0f   // a barra do programa: informa, nao e scrubber
#define AV_BTN_H   BOTAO_H_SECUNDARIO
#define AV_BASE     56.0f    // da fileira de botoes ate a borda de baixo

static void hhmm(char *b, size_t n, time_t t) {
  struct tm lt;
  localtime_r(&t, &lt);
  strftime(b, n, "%H:%M", &lt);
}

// "4K", "1080p" (ou "HD"), "720p" -> a marca; -1 sem marca (SD vai em selo).
static int marcaDaResolucao(const char *res) {
  if (!res || !res[0]) return -1;
  if (!strcmp(res, "4K")) return FMT_4K;
  if (!strcmp(res, "1080p") || !strcmp(res, "HD")) return FMT_1080;
  if (!strcmp(res, "720p")) return FMT_720;
  return -1;
}

static const char *icone(int b) {
  switch (b) {
    case AV_B_PAUSA: return NULL;   // play/pause conforme o estado
    case AV_B_GUIA: return "menu_guide";
    case AV_B_FAV: return "aj_star";
    case AV_B_AUDIO: return "audio";
    case AV_B_LEGENDA: return "legenda";
    case AV_B_INFO: return "aj_info";
    case AV_B_RECARREGAR: return "aj_rotate-ccw-clock";
    case AV_B_FONTE: return "fontes";
    default: return NULL;
  }
}

const char *aovivo_rotulo(int b) {
  switch (b) {
    case AV_B_PAUSA: return "Pausar";
    case AV_B_GUIA: return "Guia";
    case AV_B_ANT: return "Canal −";
    case AV_B_PROX: return "Canal +";
    case AV_B_FAV: return "Favorito";
    case AV_B_AUDIO: return "Áudio";
    case AV_B_LEGENDA: return "Legendas";
    case AV_B_INFO: return "Informações";
    case AV_B_RECARREGAR: return "Recarregar";
    case AV_B_FONTE: return "Fonte";
    default: return "";
  }
}

static const char *rotuloDe(const AoVivoOsd *o, int b) {
  if (b == AV_B_PAUSA) return o->pausado ? "Continuar" : "Pausar";
  if (b == AV_B_FAV && o->favorito) return "Nos favoritos";
  return aovivo_rotulo(b);
}
static const char *iconeDe(const AoVivoOsd *o, int b) {
  return b == AV_B_PAUSA ? (o->pausado ? "play" : "pause") : icone(b);
}

// A FILEIRA DE BOTOES. Pilulas secundarias da tabela unica (botoes.h): mesmo
// corpo de texto, icone, repouso, foco no acento com tinta por contraste e o
// vidro de sempre. A unica licenca e a FOLGA: sao ate dez botoes numa linha,
// e com a folga de 28 px do secundario a fileira em portugues ja passava da
// margem com "Nos favoritos" e o Pausar. Com 22 px e 12 px entre eles cabe.
// Se ainda assim nao couber (alemao, russo), os botoes com icone e FORA do
// foco viram pilula redonda so com o icone, e o rotulo fica no que tem foco,
// como no player de filme. Nunca passa da margem.
#define AV_BTN_PADX 22.0f
#define AV_BTN_GAP  12.0f
static float larguraBotao(const char *rot, const char *ic) {
  return botao_largura(rot, ic, 0) - 2.0f * (BOTAO_PAD_X2 - AV_BTN_PADX);
}
static void fileiraBotoes(const AoVivoOsd *o, float y, float a) {
  float larg[AV_B_N], total = 0.0f, x = AV_X, util = NV_TELA_W - 2.0f * AV_X;
  int i, compacto;
  for (i = 0; i < o->nBotoes; i++) {
    int b = o->botoes[i];
    larg[i] = larguraBotao(rotuloDe(o, b), iconeDe(o, b));
    total += larg[i] + (i ? AV_BTN_GAP : 0.0f);
  }
  compacto = total > util;
  for (i = 0; i < o->nBotoes; i++) {
    int b = o->botoes[i], foco = (o->foco == i);
    const char *ic = iconeDe(o, b);
    if (compacto && ic && !foco) {
      // A superficie de repouso da pilula vizinha (sem rotulo nem icone) e o
      // icone no corpo dela (26 px), centrado e no cinza 235 de repouso.
      GfxRect r = { x, y, AV_BTN_H, AV_BTN_H };
      float c = 235.0f / 255.0f;
      botao_pilula(r, "", NULL, 0.0f, 0, 0, a);
      gfx_icone((GfxRect){ x + (AV_BTN_H - BOTAO_ICONE) * 0.5f, y + (AV_BTN_H - BOTAO_ICONE) * 0.5f,
                           BOTAO_ICONE, BOTAO_ICONE }, ic, c, c, c, a);
      x += AV_BTN_H + AV_BTN_GAP;
    } else {
      botao_pilula((GfxRect){ x, y, larg[i], AV_BTN_H }, rotuloDe(o, b), ic,
                   foco ? 1.0f : 0.0f, 0, 0, a);
      x += larg[i] + AV_BTN_GAP;
    }
  }
}

void aovivo_osd_desenhar(const AoVivoOsd *o, float a) {
  float y;
  int i;
  if (a <= 0.004f || !o) return;

  // Dois degrades, como os controles de filme: o do alto sustenta a marca e o
  // relogio; o de baixo, a programacao e os botoes. Acompanham a entrada.
  gfx_rect((GfxRect){ 0, 0, NV_TELA_W, 320.0f }, 0, GFX_VEU_TOPO, 0, 0, 0, 0.0f, 0, 0, 0, 0.72f * a);
  gfx_rect((GfxRect){ 0, NV_TELA_H - 560.0f, NV_TELA_W, 560.0f }, 0, GFX_VEU_BAIXO, 0, 0, 0, 0.0f, 0, 0, 0, 0.90f * a);

  // --- ALTO ESQUERDO: a linha do canal do guia ------------------------------------
  // Marca solta (sem placa), numero em cinza, nome; embaixo a categoria como
  // etiqueta, a marca do formato e a estrela do favorito.
  { GfxRect lx = { AV_X, AV_Y, AV_LOGO_W, AV_LOGO_H };
    float tx = AV_X + AV_LOGO_W + 28.0f, yl = AV_Y + 2.0f, xs;
    TxtLinha nome, num;
    int fm = marcaDaResolucao(o->res);
    guia_logo_desenhar(o->logo, o->nome, lx, AV_LOGO_W, AV_LOGO_H - 8.0f, AV_LOGO_TOM, a);
    nome = txt_linha_corta(TXT_HEADLINE, o->nome && o->nome[0] ? o->nome : "Canal",
                           246, 247, 250, 255, 1000.0f);
    if (o->numero > 0) {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", o->numero);
      num = txt_linha(TXT_DET_META2, nb, 150, 153, 162, 255);
      txt_desenhar_alpha(num, tx, yl + (float)(nome.h - num.h) * 0.5f, a);
      txt_desenhar_alpha(nome, tx + (float)num.w + 16.0f, yl, a);
    } else {
      txt_desenhar_alpha(nome, tx, yl, a);
    }
    yl += (float)nome.h + 8.0f;
    xs = tx;
    if (o->categoria && o->categoria[0]) xs += guia_etiqueta(o->categoria, xs, yl, 420.0f, a) + 14.0f;
    if (fm >= 0) {
      xs += marca_formato((FormatoMarca)fm, xs, yl + 2.0f, 30.0f, 0.86f, 0.87f, 0.90f, a) + 14.0f;
    } else if (o->res[0]) {
      xs += badge_desenhar(xs, yl + 3.0f, o->res, BADGE_NEUTRO, a) + 14.0f;
    }
    if (o->favorito) {
      TxtLinha s = txt_linha(TXT_DET_META2, "\xe2\x98\x85", 255, 214, 90, 255);
      txt_desenhar_alpha(s, xs, yl + (34.0f - (float)s.h) * 0.5f, a);
    } }

  // --- ALTO DIREITO: relogio e estado do fluxo --------------------------------------
  { time_t agoraT = time(NULL);
    char hora[8];
    TxtLinha lh;
    float yr = AV_Y;
    hhmm(hora, sizeof hora, agoraT);
    lh = txt_linha(TXT_PG_RELOGIO, hora, 255, 255, 255, 255);
    txt_desenhar_alpha(lh, NV_TELA_W - AV_X - lh.w, yr, a * 0.96f);
    yr += lh.h + 6.0f;
    if (o->bufferando || o->pausado) {
      TxtLinha lb = txt_linha(TXT_DET_META2, o->bufferando ? "Carregando o fluxo…" : "Pausado",
                              196, 198, 206, 255);
      txt_desenhar_alpha(lb, NV_TELA_W - AV_X - lb.w, yr, a);
      yr += lb.h + 6.0f;
    }
    if (o->infoAberta && o->nInfo > 0) {
      float pw = 600.0f, passo = 38.0f, ph = 40.0f + o->nInfo * passo;
      GfxRect p = { NV_TELA_W - AV_X - pw, yr + 14.0f, pw, ph };
      // Raio em fracao do menor lado: 20 px, o canto dos paineis do app.
      if (ajustes_vidro()) gfx_vidro_painel(p, 20.0f / ph, 0.72f, a);
      else gfx_cor(p, 20.0f / ph, 0.055f, 0.058f, 0.068f, 0.90f * a);
      for (i = 0; i < o->nInfo; i++) {
        TxtLinha li = txt_linha_corta(TXT_DET_META2, o->info[i], 222, 224, 230, 255, pw - 56.0f);
        txt_desenhar_alpha(li, p.x + 28.0f, p.y + 20.0f + i * passo + (passo - (float)li.h) * 0.5f, a);
      }
    } }

  // --- BAIXO: o heroi do guia, empilhado de baixo para cima -----------------------
  // Ordem do heroi: titulo do programa, AO VIVO + horario + quanto falta, a
  // barra, a descricao, e "A SEGUIR" na base. Aqui a base e a fileira de botoes.
  y = NV_TELA_H - AV_BASE - AV_BTN_H;
  { float yb = y - 40.0f;
    const AoVivoEpg *e = &o->epg;
    time_t agoraT = time(NULL);
    if (e->temProx) {
      char hp[8];
      TxtLinha l = txt_linha(TXT_PG_ROTULO, "A SEGUIR", 140, 143, 152, 255), hr, tt;
      float lx = AV_X;
      hhmm(hp, sizeof hp, e->proxIni);
      yb -= 30.0f;
      txt_desenhar_alpha(l, lx, yb + (30.0f - (float)l.h) * 0.5f, a);
      lx += (float)l.w + 20.0f;
      hr = txt_linha(TXT_BODY, hp, 180, 183, 192, 255);
      txt_desenhar_alpha(hr, lx, yb + (30.0f - (float)hr.h) * 0.5f, a);
      lx += (float)hr.w + 14.0f;
      tt = txt_linha_corta(TXT_BODY, e->proxTit, 230, 231, 236, 255, AV_X + AV_TEXTO_W - lx);
      txt_desenhar_alpha(tt, lx, yb + (30.0f - (float)tt.h) * 0.5f, a);
      yb -= 22.0f;
    }
    if (o->desc && o->desc[0]) {
      float h = txt_bloco_corta(TXT_DET_SIN, o->desc, 172, 175, 184, -1.0f, 0.0f,
                                AV_TEXTO_W, 34.0f, 0.0f, 2);
      yb -= h;
      txt_bloco_corta(TXT_DET_SIN, o->desc, 172, 175, 184, AV_X, yb, AV_TEXTO_W, 34.0f, a * 0.95f, 2);
      yb -= 22.0f;
    }
    if (e->temAgora) {
      GfxRect tr = { AV_X, yb - 4.0f, AV_TRILHO_W, 4.0f }, an;
      float ar, ag, ab, f = e->progresso < 0.0f ? 0.0f : (e->progresso > 1.0f ? 1.0f : e->progresso);
      ajustes_acento(&ar, &ag, &ab);
      an = tr; an.w = tr.w * f;
      gfx_cor(tr, 0.5f, 1, 1, 1, 0.14f * a);
      if (an.w > 0.5f) gfx_cor(an, 0.5f, ar, ag, ab, a);
      yb = tr.y - 18.0f;
    }
    // Linha de meta: o selo do guia e o horario, ou "sem grade" quando o canal
    // nao casa com nenhuma grade (a maioria, no FrostView).
    { char meta[160];
      float mx;
      yb -= 32.0f;
      mx = AV_X + guia_selo_ao_vivo(AV_X, yb, a) + 16.0f;
      if (e->temAgora) {
        char h1[8], h2[8], resto[64];
        int falta = (int)((e->agoraFim - agoraT + 59) / 60);
        if (falta < 0) falta = 0;
        hhmm(h1, sizeof h1, e->agoraIni); hhmm(h2, sizeof h2, e->agoraFim);
        snprintf(resto, sizeof resto, i18n("%d min restantes"), falta);
        snprintf(meta, sizeof meta, "%s \xe2\x80\x93 %s  \xc2\xb7  %s", h1, h2, resto);
      } else {
        snprintf(meta, sizeof meta, "%s", i18n("Sem grade de programação"));
      }
      { TxtLinha t = txt_linha_corta(TXT_DET_META, meta, 196, 198, 206, 255, AV_X + AV_TEXTO_W - mx);
        txt_desenhar_alpha(t, mx, yb + (32.0f - (float)t.h) * 0.5f, a); }
      yb -= 12.0f; }
    if (e->temAgora) {
      TxtLinha lt = txt_linha_corta(TXT_TITULO2, e->agoraTit, 246, 247, 250, 255, AV_TITULO_W);
      yb -= (float)lt.h;
      txt_desenhar_alpha(lt, AV_X, yb, a);
    } }

  fileiraBotoes(o, y, a);
}

// Superficie dos cartoes flutuantes (banner, erro): o painel do app, um degrau
// acima do fundo, ou o vidro. `raioPx` em pixels (a API quer fracao da altura).
static void cartao(GfxRect r, float raioPx, float a) {
  if (ajustes_vidro()) gfx_vidro_painel(r, raioPx / r.h, 0.72f, a);
  else gfx_cor(r, raioPx / r.h, 0.055f, 0.058f, 0.068f, 0.92f * a);
}

void aovivo_banner_desenhar(const AoVivoBanner *b, float a) {
  float w = 900.0f, h = 132.0f, x = AV_X, y = NV_TELA_H - 96.0f - h;
  float tx0 = x + 24.0f + AV_LOGO_W + 24.0f, tx = tx0, dir = x + w - 28.0f, yl = y + 22.0f;
  GfxRect r = { x, y, w, h };
  TxtLinha nome, st, tn;
  char s[64];
  if (a <= 0.004f || !b) return;
  gfx_rect((GfxRect){ 0, NV_TELA_H - 420.0f, NV_TELA_W, 420.0f }, 0, GFX_VEU_BAIXO, 0, 0, 0, 0.0f, 0, 0, 0, 0.80f * a);
  cartao(r, 20.0f, a);
  guia_logo_desenhar(b->logo, b->nome, (GfxRect){ x + 24.0f, y + 24.0f, AV_LOGO_W, h - 48.0f },
                     AV_LOGO_W, h - 56.0f, AV_LOGO_TOM, a);
  // Estado a direita, na altura do nome: quantos canais ja somou, ou so que a
  // troca esta a caminho.
  if (b->salto > 1 || b->salto < -1) snprintf(s, sizeof s, i18n("%+d canais"), b->salto);
  else snprintf(s, sizeof s, "%s", i18n("Trocando de canal…"));
  st = txt_linha(TXT_DET_META2, s, 150, 153, 162, 255);
  if (b->numero > 0) {
    char nb[24];
    snprintf(nb, sizeof nb, "%d", b->numero);
    tn = txt_linha(TXT_DET_META2, nb, 150, 153, 162, 255);
    tx += (float)tn.w + 16.0f;
  }
  nome = txt_linha_corta(TXT_HEADLINE, b->nome && b->nome[0] ? b->nome : "Canal", 246, 247, 250, 255,
                         dir - (float)st.w - 24.0f - tx);
  if (b->numero > 0) txt_desenhar_alpha(tn, tx0, yl + (float)(nome.h - tn.h) * 0.5f, a);
  txt_desenhar_alpha(nome, tx, yl, a);
  txt_desenhar_alpha(st, dir - (float)st.w, yl + (float)(nome.h - st.h) * 0.5f, a);
  if (b->agoraTit && b->agoraTit[0]) {
    TxtLinha ag = txt_linha_corta(TXT_BODY, b->agoraTit, 196, 198, 206, 255, dir - tx0);
    txt_desenhar_alpha(ag, tx0, yl + (float)nome.h + 10.0f, a);
  }
}

// O CARTAO DE ERRO. So o que a pessoa precisa: de qual canal, o que houve e o
// que fazer. A marca e o AO VIVO ja estao no alto do OSD; repetidos aqui eram
// ruido (e um selo vermelho num cartao de erro le como alarme). O canal entra
// como sobrelinha em cinza, para o cartao se explicar sozinho quando o OSD
// recolhe.
void aovivo_erro_desenhar(const char *nome, const char *logo, const char *titulo,
                          const char *dica, float a) {
  const float w = 1040.0f, pad = 48.0f, tw = w - 2.0f * pad;
  const char *tit = titulo && titulo[0] ? titulo : "Não foi possível abrir a fonte";
  const char *dc = dica && dica[0] ? dica : "Abra Fontes para escolher outra opção ou recarregar.";
  const char *acao = "Use Fonte ou Recarregar aqui embaixo, ou CH+ e CH− para trocar de canal.";
  float x = (NV_TELA_W - w) * 0.5f, y, h, hTit, hDica, yy;
  TxtLinha k, ac;
  (void)logo;
  if (a <= 0.004f) return;
  k = txt_linha_corta(TXT_DET_META2, nome && nome[0] ? nome : "Canal", 150, 153, 162, 255, tw);
  hTit = txt_bloco_corta(TXT_HEADLINE, tit, 246, 247, 250, -1.0f, 0.0f, tw, 46.0f, 0.0f, 2);
  hDica = txt_bloco_corta(TXT_DET_META, dc, 196, 198, 206, -1.0f, 0.0f, tw, 34.0f, 0.0f, 2);
  ac = txt_linha_corta(TXT_DET_META2, acao, 150, 153, 162, 255, tw);
  h = pad + (float)k.h + 10.0f + hTit + 12.0f + hDica + 32.0f + (float)ac.h + pad - 8.0f;
  // Centrado no vao entre a linha do canal (alto) e a programacao (baixo).
  y = 420.0f - h * 0.5f;
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0, 0.02f, 0.02f, 0.025f, 0.55f * a);
  cartao((GfxRect){ x, y, w, h }, 24.0f, a);
  yy = y + pad - 4.0f;
  txt_desenhar_alpha(k, x + pad, yy, a);
  yy += (float)k.h + 10.0f;
  txt_bloco_corta(TXT_HEADLINE, tit, 246, 247, 250, x + pad, yy, tw, 46.0f, a, 2);
  yy += hTit + 12.0f;
  txt_bloco_corta(TXT_DET_META, dc, 196, 198, 206, x + pad, yy, tw, 34.0f, a, 2);
  yy += hDica + 32.0f;
  txt_desenhar_alpha(ac, x + pad, yy, a);
}
