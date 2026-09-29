// Desenho do OSD do canal ao vivo, do banner do zapping e do cartao de erro.
// Ver aovivo.h; separado da conta para ela ser testada sem GL.
#include "aovivo.h"
#include "ajustes.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "layout.h"
#include "idioma.h"
#include <stdio.h>
#include <string.h>

// --- desenho ---------------------------------------------------------------------
#define AV_X        96.0f
#define AV_Y        56.0f
#define AV_LOGO_W  188.0f
#define AV_LOGO_H  108.0f
#define AV_BTN_H    68.0f
#define AV_BTN_GAP  12.0f

static void hhmm(char *b, size_t n, time_t t) {
  struct tm lt;
  localtime_r(&t, &lt);
  strftime(b, n, "%H:%M", &lt);
}

// A marca do canal dentro de um cartao: escura e translucida atras, a marca
// centrada por dentro, sem esticar. Sem marca, o nome numa pilula — o canal
// nunca fica sem rosto.
static void cartaoLogo(float x, float y, float w, float h, const char *logo,
                       const char *nome, float a) {
  GfxRect r = { x, y, w, h };
  GLuint tex = (logo && logo[0]) ? tex_obter_larg_qualquer(logo, w) : 0;
  if (ajustes_vidro()) gfx_vidro_painel(r, 0.16f, 0.62f, a);
  else                 gfx_cor(r, 0.16f, 0.06f, 0.07f, 0.09f, 0.72f * a);
  if (tex) {
    float ar = tex_aspecto(logo), lw = w - 32.0f, lh = h - 28.0f;
    if (ar > 0.0f) {
      if (lw / ar > lh) lw = lh * ar; else lh = lw / ar;
    }
    gfx_rect((GfxRect){ x + (w - lw) * 0.5f, y + (h - lh) * 0.5f, lw, lh }, tex,
             tex_marca_escura(logo) ? GFX_MARCA : GFX_TEXTO, 0, 0, 0, 0,
             .95f, .95f, .97f, a);
  } else {
    TxtLinha t = txt_linha_corta(TXT_PLR_CORPO, nome && nome[0] ? nome : "TV", 240, 241, 245, 255, w - 24.0f);
    txt_desenhar_alpha(t, x + (w - t.w) * 0.5f, y + (h - (float)t.h) * 0.5f, a);
  }
}

// O selo AO VIVO: pilula vermelha com um ponto. Vermelho e a cor do assunto em
// toda TV do mundo, e o unico lugar do player onde o acento do tema nao manda.
static float seloAoVivo(float x, float y, float a) {
  TxtLinha t = txt_linha(TXT_MINI, "AO VIVO", 255, 255, 255, 255);
  float h = 34.0f, w = 20.0f + 12.0f + (float)t.w + 18.0f;
  GfxRect r = { x, y, w, h };
  gfx_cor(r, 0.5f, 0.86f, 0.13f, 0.16f, 0.94f * a);
  gfx_cor((GfxRect){ x + 14.0f, y + h * 0.5f - 5.0f, 10.0f, 10.0f }, 0.5f, 1, 1, 1, 0.96f * a);
  txt_desenhar_alpha(t, x + 32.0f, y + (h - (float)t.h) * 0.5f, a);
  return w;
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

static void pilulaBotao(const AoVivoOsd *o, int i, float *x, float y, float a) {
  int b = o->botoes[i], foco = (o->foco == i);
  const char *rot = b == AV_B_PAUSA ? (o->pausado ? "Continuar" : "Pausar")
                  : (b == AV_B_FAV && o->favorito) ? "Nos favoritos" : aovivo_rotulo(b);
  const char *ic = b == AV_B_PAUSA ? (o->pausado ? "play" : "pause") : icone(b);
  TxtLinha t;
  float w, ix, tint;
  GfxRect r;
  int tinta;
  if (ajustes_vidro()) tinta = gfx_vidro_tinta(foco ? 1.0f : 0.0f);
  else tinta = foco ? ajustes_tinta_foco() : 240;
  t = txt_linha(TXT_PG_FIM, rot, tinta, tinta, tinta, 255);
  w = (ic ? 30.0f + 12.0f : 0.0f) + (float)t.w + 44.0f;
  r = (GfxRect){ *x, y, w, AV_BTN_H };
  if (ajustes_vidro()) {
    gfx_vidro_painel(r, 0.5f, 0.55f, a);
    if (foco) gfx_vidro_pilula_cheia(r, 0.5f, 1.0f, a);
  } else if (foco) {
    gfx_cor(r, 0.5f, o->fr, o->fg, o->fb, 0.96f * a);
  } else {
    gfx_cor(r, 0.5f, 1, 1, 1, 0.14f * a);
  }
  tint = tinta / 255.0f;
  ix = *x + 22.0f;
  if (ic) {
    gfx_icone((GfxRect){ ix, y + (AV_BTN_H - 30.0f) * 0.5f, 30.0f, 30.0f }, ic,
              tint, tint, tint, 0.96f * a);
    ix += 42.0f;
  }
  txt_desenhar_alpha(t, ix, y + (AV_BTN_H - (float)t.h) * 0.5f, a);
  *x += w + AV_BTN_GAP;
}

void aovivo_osd_desenhar(const AoVivoOsd *o, float a) {
  float x, y, yTopo = AV_Y;
  int i;
  if (a <= 0.004f || !o) return;

  // Dois degrades, como os controles de filme: o do alto sustenta a marca e o
  // relogio; o de baixo, a programacao e os botoes. Acompanham a entrada.
  gfx_rect((GfxRect){ 0, 0, NV_TELA_W, 320.0f }, 0, GFX_VEU_TOPO, 0, 0, 0, 0.0f, 0, 0, 0, 0.72f * a);
  gfx_rect((GfxRect){ 0, NV_TELA_H - 560.0f, NV_TELA_W, 560.0f }, 0, GFX_VEU_BAIXO, 0, 0, 0, 0.0f, 0, 0, 0, 0.90f * a);

  // --- ALTO ESQUERDO: marca, numero, nome, categoria, AO VIVO ---------------------
  cartaoLogo(AV_X, yTopo, AV_LOGO_W, AV_LOGO_H, o->logo, o->nome, a);
  x = AV_X + AV_LOGO_W + 28.0f;
  { float yl = yTopo + 2.0f;
    TxtLinha nome = txt_linha_corta(TXT_PLR_TITULO, o->nome && o->nome[0] ? o->nome : "Canal",
                                    255, 255, 255, 255, 1000.0f);
    float xn = x;
    if (o->numero > 0) {
      char nb[16];
      TxtLinha tn;
      GfxRect chip;
      snprintf(nb, sizeof nb, "%d", o->numero);
      tn = txt_linha(TXT_PLR_CORPO, nb, 255, 255, 255, 255);
      chip = (GfxRect){ x, yl + ((float)nome.h - 48.0f) * 0.5f, (float)tn.w + 34.0f, 48.0f };
      if (ajustes_vidro()) gfx_vidro_painel(chip, 0.5f, 0.55f, a);
      else gfx_cor(chip, 0.5f, 1, 1, 1, 0.20f * a);
      txt_desenhar_alpha(tn, chip.x + 17.0f, chip.y + (48.0f - (float)tn.h) * 0.5f, a);
      xn = chip.x + chip.w + 16.0f;
    }
    txt_desenhar_alpha(nome, xn, yl, a);
    yl += nome.h + 12.0f;
    { float xs = x, wsel = seloAoVivo(xs, yl, a);
      xs += wsel + 16.0f;
      if (o->categoria && o->categoria[0]) {
        TxtLinha tc = txt_linha_corta(TXT_PG_FIM, o->categoria, 214, 216, 222, 255, 700.0f);
        txt_desenhar_alpha(tc, xs, yl + (34.0f - (float)tc.h) * 0.5f, a * 0.85f);
        xs += tc.w + 20.0f;
      }
      if (o->res[0]) {
        TxtLinha tr = txt_linha(TXT_MINI, o->res, 236, 237, 242, 255);
        GfxRect cr = { xs, yl, (float)tr.w + 24.0f, 34.0f };
        gfx_cor(cr, 0.3f, 1, 1, 1, 0.16f * a);
        txt_desenhar_alpha(tr, xs + 12.0f, yl + (34.0f - (float)tr.h) * 0.5f, a * 0.9f);
      } } }

  // --- ALTO DIREITO: relogio e estado do fluxo --------------------------------------
  { time_t agoraT = time(NULL);
    char hora[8];
    TxtLinha lh;
    float yr = AV_Y;
    hhmm(hora, sizeof hora, agoraT);
    lh = txt_linha(TXT_PG_RELOGIO, hora, 255, 255, 255, 255);
    txt_desenhar_alpha(lh, NV_TELA_W - AV_X - lh.w, yr, a * 0.96f);
    yr += lh.h + 10.0f;
    if (o->bufferando) {
      TxtLinha lb = txt_linha(TXT_PG_FIM, "Carregando o fluxo…", 255, 214, 120, 255);
      txt_desenhar_alpha(lb, NV_TELA_W - AV_X - lb.w, yr, a * 0.95f);
      yr += lb.h + 8.0f;
    } else if (o->pausado) {
      TxtLinha lb = txt_linha(TXT_PG_FIM, "Pausado", 236, 237, 242, 255);
      txt_desenhar_alpha(lb, NV_TELA_W - AV_X - lb.w, yr, a * 0.9f);
      yr += lb.h + 8.0f;
    }
    if (o->infoAberta && o->nInfo > 0) {
      float pw = 520.0f, ph = 28.0f + o->nInfo * 40.0f;
      GfxRect p = { NV_TELA_W - AV_X - pw, yr + 8.0f, pw, ph };
      if (ajustes_vidro()) gfx_vidro_painel(p, 0.06f, 0.72f, a);
      else gfx_cor(p, 0.06f, 0.05f, 0.06f, 0.08f, 0.82f * a);
      for (i = 0; i < o->nInfo; i++) {
        TxtLinha li = txt_linha_corta(TXT_PG_FIM, o->info[i], 232, 234, 240, 255, pw - 44.0f);
        txt_desenhar_alpha(li, p.x + 22.0f, p.y + 16.0f + i * 40.0f, a * 0.92f);
      }
    } }

  // --- BAIXO: programacao (agora / a seguir) e a fileira de botoes ---------------------
  y = NV_TELA_H - 56.0f - AV_BTN_H;   // topo da fileira de botoes
  { float yb = y - 30.0f;             // base do bloco da programacao
    const AoVivoEpg *e = &o->epg;
    float larg = 1180.0f;
    if (o->desc && o->desc[0] && e->temAgora) {
      float h = txt_bloco_corta(TXT_PG_FIM, o->desc, 200, 202, 208, -1.0f, 0.0f, larg, 32.0f, 0.0f, 2);
      yb -= h;
      txt_bloco_corta(TXT_PG_FIM, o->desc, 200, 202, 208, AV_X, yb, larg, 32.0f, a * 0.75f, 2);
      yb -= 18.0f;
    }
    if (e->temAgora) {
      char h1[8], h2[8], faixa[40], falta[48];
      TxtLinha lt, lf, lk;
      GfxRect tr, fe;
      int min = (int)((e->agoraFim - time(NULL)) / 60);
      hhmm(h1, sizeof h1, e->agoraIni); hhmm(h2, sizeof h2, e->agoraFim);
      snprintf(faixa, sizeof faixa, "%s\xe2\x80\x93%s", h1, h2);
      if (min < 0) min = 0;
      if (min >= 60) snprintf(falta, sizeof falta, i18n("faltam %dh%02d"), min / 60, min % 60);
      else snprintf(falta, sizeof falta, i18n("faltam %d min"), min);
      // Barra do programa (nao do fluxo: ao vivo nao tem fim), na largura do bloco.
      tr = (GfxRect){ AV_X, yb - 8.0f, larg, 8.0f };
      yb = tr.y - 14.0f;
      gfx_cor(tr, 0.5f, 1, 1, 1, 0.24f * a);
      fe = tr; fe.w = larg * e->progresso;
      if (fe.w > 1.0f) gfx_cor(fe, 0.5f, o->fr, o->fg, o->fb, a);
      lf = txt_linha(TXT_PG_FIM, faixa, 214, 216, 222, 255);
      yb -= lf.h;
      txt_desenhar_alpha(lf, AV_X, yb, a * 0.85f);
      lk = txt_linha(TXT_PG_FIM, falta, 168, 170, 178, 255);
      txt_desenhar_alpha(lk, AV_X + lf.w + 24.0f, yb, a * 0.8f);
      yb -= 8.0f;
      lt = txt_linha_corta(TXT_TITULO3, e->agoraTit, 255, 255, 255, 255, larg);
      yb -= lt.h;
      txt_desenhar_alpha(lt, AV_X, yb, a);
      yb -= 6.0f;
      { TxtLinha ka = txt_linha(TXT_MINI, "AGORA", 214, 216, 222, 255);
        yb -= ka.h;
        txt_desenhar_alpha(ka, AV_X, yb, a * 0.62f); }
    } else {
      TxtLinha t = txt_linha(TXT_PLR_CORPO, "Sem programação disponível para este canal", 200, 202, 208, 255);
      txt_desenhar_alpha(t, AV_X, yb - t.h, a * 0.75f);
    }
    if (e->temProx) {
      char hp[8], lin[220];
      TxtLinha lp, ks;
      float xr, w = 520.0f, yp = y - 30.0f;
      hhmm(hp, sizeof hp, e->proxIni);
      snprintf(lin, sizeof lin, "%s  \xc2\xb7  %s", hp, e->proxTit);
      lp = txt_linha_corta(TXT_PLR_CORPO, lin, 236, 237, 242, 255, w);
      ks = txt_linha(TXT_MINI, "A SEGUIR", 214, 216, 222, 255);
      xr = NV_TELA_W - AV_X - w;
      txt_desenhar_alpha(lp, xr, yp - lp.h, a * 0.9f);
      txt_desenhar_alpha(ks, xr, yp - lp.h - 6.0f - ks.h, a * 0.62f);
    } }

  x = AV_X;
  for (i = 0; i < o->nBotoes; i++) pilulaBotao(o, i, &x, y, a);
}

void aovivo_banner_desenhar(const AoVivoBanner *b, float a) {
  float w = 940.0f, h = 148.0f, x = AV_X, y = NV_TELA_H - 96.0f - h;
  GfxRect r = { x, y, w, h };
  char nb[24];
  TxtLinha nome, ag, sal;
  if (a <= 0.004f || !b) return;
  gfx_rect((GfxRect){ 0, NV_TELA_H - 420.0f, NV_TELA_W, 420.0f }, 0, GFX_VEU_BAIXO, 0, 0, 0, 0.0f, 0, 0, 0, 0.80f * a);
  if (ajustes_vidro()) gfx_vidro_painel(r, 0.14f, 0.72f, a);
  else gfx_cor(r, 0.14f, 0.05f, 0.06f, 0.08f, 0.86f * a);
  cartaoLogo(x + 16.0f, y + 16.0f, 180.0f, h - 32.0f, b->logo, b->nome, a);
  snprintf(nb, sizeof nb, "%d", b->numero);
  nome = txt_linha_corta(TXT_PLR_TITULO, b->nome && b->nome[0] ? b->nome : "Canal", 255, 255, 255, 255, w - 480.0f);
  { float xt = x + 220.0f, yt = y + 22.0f;
    if (b->numero > 0) {
      TxtLinha tn = txt_linha(TXT_PLR_CORPO, nb, 255, 255, 255, 255);
      GfxRect chip = { xt, yt + ((float)nome.h - 44.0f) * 0.5f, (float)tn.w + 30.0f, 44.0f };
      gfx_cor(chip, 0.5f, 1, 1, 1, 0.20f * a);
      txt_desenhar_alpha(tn, chip.x + 15.0f, chip.y + (44.0f - (float)tn.h) * 0.5f, a);
      xt = chip.x + chip.w + 14.0f;
    }
    txt_desenhar_alpha(nome, xt, yt, a); }
  if (b->agoraTit && b->agoraTit[0]) {
    ag = txt_linha_corta(TXT_PLR_CORPO, b->agoraTit, 220, 222, 228, 255, w - 250.0f);
    txt_desenhar_alpha(ag, x + 220.0f, y + 22.0f + nome.h + 10.0f, a * 0.85f);
  }
  { char s[64];
    if (b->salto > 1 || b->salto < -1) snprintf(s, sizeof s, i18n("%+d canais"), b->salto);
    else snprintf(s, sizeof s, "%s", i18n("Trocando de canal…"));
    sal = txt_linha(TXT_MINI, s, 200, 202, 208, 255);
    txt_desenhar_alpha(sal, x + w - 28.0f - sal.w, y + 30.0f, a * 0.7f); }
}

void aovivo_erro_desenhar(const char *nome, const char *logo, const char *titulo,
                          const char *dica, float a) {
  float w = 1180.0f, h = 320.0f, x = (NV_TELA_W - w) * 0.5f, y = 230.0f;
  GfxRect r = { x, y, w, h };
  TxtLinha t, d, dc;
  if (a <= 0.004f) return;
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0, 0.02f, 0.02f, 0.025f, 0.55f * a);
  if (ajustes_vidro()) gfx_vidro_painel(r, 0.08f, 0.78f, a);
  else gfx_cor(r, 0.08f, 0.07f, 0.08f, 0.10f, 0.90f * a);
  cartaoLogo(x + 44.0f, y + 40.0f, 188.0f, 108.0f, logo, nome, a);
  seloAoVivo(x + 44.0f, y + 164.0f, a);
  t = txt_linha_corta(TXT_CALLOUT, titulo && titulo[0] ? titulo : "Não foi possível abrir a fonte",
                      244, 245, 247, 255, w - 340.0f);
  txt_desenhar_alpha(t, x + 268.0f, y + 44.0f, a);
  d = txt_linha_corta(TXT_PG_FIM, dica && dica[0] ? dica : "Abra Fontes para escolher outra opção ou recarregar.",
                      200, 202, 208, 255, w - 340.0f);
  txt_desenhar_alpha(d, x + 268.0f, y + 44.0f + t.h + 14.0f, a * 0.9f);
  dc = txt_linha_corta(TXT_PG_FIM, "Use Fonte ou Recarregar aqui embaixo, ou CH+ e CH− para trocar de canal.",
                       170, 172, 180, 255, w - 88.0f);
  txt_desenhar_alpha(dc, x + 44.0f, y + h - 30.0f - dc.h, a * 0.8f);
}
