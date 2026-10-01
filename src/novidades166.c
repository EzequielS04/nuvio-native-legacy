// Cartao de NOVIDADES DA 1.6.6: o layout Apple TV e a Live TV.
//
// O MOLDE E O DA 1.4.8 e o da 1.6.0 (o dono: "ficou lindo"): a esquerda uma
// PREVIA VIVA, desenhada com os componentes de verdade do app (a pilula e o
// painel da barra da Dinamica nas medidas de menu.c, a janela GFX_JANELA do
// carrossel de detail.c, a ilha do relogio, botoes.h, badges.h, a marca do
// guia e os paineis do Diagnostico da Live TV); a direita o titulo e a lista
// AGRUPADA, cada linha com icone, nome e UMA linha apagada dizendo o que mudou
// e onde achar; no rodape as tres pilulas da tabela unica. Tres cenas trocam
// sozinhas, com passagem suave.
//
// DADOS, NAO CODIGO. As cenas (CENAS) e as linhas (ITENS) sao tabelas: cada
// recurso e UMA entrada, e apagar uma linha reorganiza o cartao sozinho (a
// lista mede e distribui o espaco; a cena acende a linha pelo id).
//
// CUSTO (LG C9): texto so por txt_linha/txt_bloco, que guardam a textura por
// (estilo, cor, texto) — cor de texto NUNCA muda por quadro, so o alfa; o foco
// que anda troca entre DUAS linhas prontas (clara e escura). A lista abre pelo
// portao de texto (textogate.h): inteira ou nada. A cena SEGUINTE e desenhada
// uma vez numa tesoura de 1 px, com alfa quase nulo, so para as linhas dela ja
// existirem quando ela entrar. A arte e pedida numa largura so (N166_ARTE_W),
// para cada arquivo ocupar uma textura no cache.
//
// A MARCA E "novidades-166-ui.txt" (N166_ARQ). O numero da versao mora so em
// N166_VERSAO (novidades166.h).
#include "novidades166.h"
#include "ajustes.h"
#include "anim.h"
#include "badges.h"
#include "botoes.h"
#include "dados.h"
#include "gfx.h"
#include "guia.h"
#include "idioma.h"
#include "idiomacod.h"
#include "layout.h"
#include "menu.h"
#include "ponteiro.h"
#include "tex_cache.h"
#include "text.h"
#include "textogate.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N166_ARQ          "novidades-166-ui.txt"
#define N166_W            1720.0f
#define N166_H            1000.0f
#define N166_X            ((NV_TELA_W - N166_W) * 0.5f)
#define N166_Y            ((NV_TELA_H - N166_H) * 0.5f)
#define N166_PAD            52.0f
#define N166_RAIO           32.0f
// A previa: a coluna da esquerda inteira, acima do rodape.
#define N166_PV_W          760.0f
#define N166_PV_H          (N166_H - 2.0f * N166_PAD - BOTAO_H_PRIMARIO - 28.0f)
#define N166_PV_RAIO        26.0f
#define N166_COL_GAP        64.0f
#define N166_TXT_X        (N166_X + N166_PAD + N166_PV_W + N166_COL_GAP)
#define N166_TXT_W        (N166_X + N166_W - N166_PAD - N166_TXT_X)
#define N166_ICONE          40.0f
#define N166_ABRIR_MS      280.0f
#define N166_FECHAR_MS     160.0f
#define N166_TRANSICAO_S     0.55f
// Margem da cena: o selo com o nome e os tracos do ciclo ocupam o alto.
#define N166_M              48.0f
// Largura em que TODA arte de fundo e pedida: a previa mostra um recorte
// "cover" de um quadro 16:9 da altura dela (~1415 px), entao 1280 e o menor
// pedido que nao deixa a arte borrada, e um so tamanho = uma textura por arquivo.
#define N166_ARTE_W       1280.0f

enum { B_DEPOIS = 0, B_GUIA = 1, B_LAYOUT = 2, B_N };

// De que assunto cada cena e cada linha falam: a linha da cena na tela ganha o
// disco no realce. Um id e nao um indice, para apagar uma entrada sem desalinhar.
enum { ID_NADA = 0, ID_ATV, ID_LIVE, ID_DIAG, ID_FILEIRA, ID_ILHA, ID_FONTES,
       ID_COMUNIDADE, ID_LOGO, ID_ATUALIZA, ID_CONSERTOS };

static int   aberto, decidido, foco = B_LAYOUT, naPrevia, pedido;
static int   cena, cenaAntiga;
static float entrada, transicao = 1.0f, relogioCena, tempoAntiga;
static char  dirArte[512] = "deploy/app/art";
static TextoGate gateLista;
static unsigned aquecida;   // bit por cena: as linhas dela ja estao no cache

// ------------------------------------------------------------------ a arte
//
// Tudo do pacote (deploy/app/art), sem rede: fundos NN.jpg, cartazes
// poster/NN.jpg e logos logo/NN.png (o mesmo NN = o mesmo titulo).
enum { F_HOME, F_R1, F_R2, F_R3, F_CE, F_C0, F_C1, F_C2, F_VIVO, F_N };
// One Battle After Another; a fileira; Outcome (vizinho), Devil Wears Prada 2,
// Project Hail Mary e The Martian no carrossel; 3 Body Problem atras da Live TV.
static const int FUNDO_N[F_N] = { 3, 13, 12, 7, 6, 7, 13, 12, 15 };
#define N166_NP 2
static const int POSTER_N[N166_NP] = { 15, 21 };   // as pastas de Streaming
enum { L_HOME, L_C0, L_C1, L_C2, L_N };
static const int LOGO_N[L_N] = { 3, 7, 13, 12 };

static char fundo[F_N][600], cartaz[N166_NP][600], logo[L_N][600];

static void montarCaminhos(void) {
  int i;
  for (i = 0; i < F_N; i++) snprintf(fundo[i], sizeof fundo[i], "%s/%02d.jpg", dirArte, FUNDO_N[i]);
  for (i = 0; i < N166_NP; i++)
    snprintf(cartaz[i], sizeof cartaz[i], "%s/%s/%02d.jpg", dirArte, "poster", POSTER_N[i]);
  for (i = 0; i < L_N; i++) snprintf(logo[i], sizeof logo[i], "%s/logo/%02d.png", dirArte, LOGO_N[i]);
}

// Raio em pixels -> fracao do menor lado (a unidade de gfx_rect).
static float rr(float px, GfxRect r) { float m = r.w < r.h ? r.w : r.h; return m > 0.0f ? px / m : 0.0f; }
static GLuint texFundo(int f) { return tex_obter_larg(fundo[f], N166_ARTE_W); }

// Arte de fundo em "cover", com o veu de leitura (esquerda e base) assado na
// mesma passada: GFX_VITRINE, o modo dos destaques da home.
static void arte(int f, GfxRect r, float raioPx, float veu, float a) {
  GLuint t;
  if (a <= 0.003f) return;
  t = texFundo(f);
  if (!t) { gfx_cor(r, rr(raioPx, r), 0.10f, 0.11f, 0.13f, a); return; }
  gfx_tex_aspect_atual = tex_aspecto(fundo[f]);
  if (gfx_tex_aspect_atual <= 0.0f) gfx_tex_aspect_atual = 16.0f / 9.0f;
  gfx_rect(r, t, GFX_VITRINE, veu, 0.35f, 0.0f, rr(raioPx, r), 0, 0, 0, a);
  gfx_tex_aspect_atual = 0.0f;
}

// O logo do titulo com a base em `yBase`, cabendo em maxW x maxH.
static void logoTitulo(int l, float x, float yBase, float maxW, float maxH, float a) {
  GLuint t;
  float ap, w, h;
  if (a <= 0.003f) return;
  t = tex_obter_larg(logo[l], 330.0f);
  if (!t) return;
  ap = tex_aspecto(logo[l]);
  if (ap <= 0.0f) ap = 3.0f;
  w = maxW; h = w / ap;
  if (h > maxH) { h = maxH; w = h * ap; }
  gfx_tex_aspect_atual = 0.0f;
  gfx_rect((GfxRect){ x, yBase - h, w, h }, t,
           tex_marca_escura(logo[l]) ? GFX_MARCA : GFX_TEXTO, 0, 0, 0, 0.0f, 1, 1, 1, a);
}

static void pedirArtes(void) {
  int i;
  for (i = 0; i < F_N; i++) texFundo(i);
  for (i = 0; i < N166_NP; i++) tex_obter_larg(cartaz[i], 90.0f);
  for (i = 0; i < L_N; i++) tex_obter_larg(logo[i], 330.0f);
}

// Tesoura da cena atual (a previa, ou a interseccao com ela).
static GfxRect clipCena;
static void recorte(GfxRect r) { gfx_recorte(r.x, r.y, r.w, r.h); }
static void recorteDentro(GfxRect r) {
  float x0 = fmaxf(r.x, clipCena.x), y0 = fmaxf(r.y, clipCena.y);
  float x1 = fminf(r.x + r.w, clipCena.x + clipCena.w), y1 = fminf(r.y + r.h, clipCena.y + clipCena.h);
  gfx_recorte(x0, y0, fmaxf(0.0f, x1 - x0), fmaxf(0.0f, y1 - y0));
}
static void recorteVolta(void) { recorte(clipCena); }

// Passagem de 0 a 1 com saida suave a partir de `ini`, durando `dur` segundos.
static float passo(float t, float ini, float dur) {
  return anim_suave(anim_clamp((t - ini) / dur, 0.0f, 1.0f));
}
// A MOLA da barra e da ilha, como funcao do tempo (a previa e determinista:
// a captura pula para qualquer instante): passa ~6 % do alvo e assenta.
static float mola(float t, float ini, float dur) {
  float u = (t - ini) / dur;
  if (u <= 0.0f) return 0.0f;
  if (u >= 1.0f) return 1.0f;
  return 1.0f - expf(-7.0f * u) * cosf(8.0f * u);
}
static GfxRect misturaRect(GfxRect a, GfxRect b, float s) {
  GfxRect r = { anim_mistura(a.x, b.x, s), anim_mistura(a.y, b.y, s),
                anim_mistura(a.w, b.w, s), anim_mistura(a.h, b.h, s) };
  return r;
}
static void segundosDec(char *d, size_t n, int decimos) {
  int ponto = idioma_ponto_decimal(ajustes_idioma());
  snprintf(d, n, "%d%c%d s", decimos / 10, ponto ? '.' : ',', decimos % 10);
}

// ====================================================== CENA 0: A BARRA NOVA
//
// A home da Dinamica com a pilula "‹ (casa) Inicio" no canto. Ela CRESCE ate
// o painel flutuante (cabecalho, itens, Streaming com as pastas), o foco
// branco desce pelos itens, o painel volta a ser pilula, e no fim a ilha do
// relogio, no outro canto, se abre para um aviso. Medidas de menu.c a ~75 %.
#define N166_P_W      300.0f
#define N166_P_CAB     70.0f
#define N166_P_LIN     56.0f
#define N166_P_PIL     50.0f
#define N166_P_ROT     34.0f
#define N166_P_COLX    26.0f    // centro da coluna de icones, a partir da pilula
#define N166_P_ROTX    54.0f    // rotulo, a partir da pilula
#define N166_PIL_H     50.0f
#define N166_PIL_CIRC  38.0f
#define N166_SETA      22.0f
#define N166_ABRE_T     0.75f
#define N166_ANDA_T     1.45f
#define N166_ANDA_S     0.34f
#define N166_FECHA_T    4.05f
#define N166_ILHA_T     4.55f

static const int BARRA_DEST[6] = { MENU_INICIO, MENU_BUSCAR, MENU_EXPLORAR, MENU_GUIA,
                                   MENU_BIBLIOTECA, MENU_AJUSTES };
static const char *const BARRA_ICONE[6] = { "menu_home", "menu_search", "portal", "menu_guide",
                                            "menu_library", "menu_settings" };
static const char *const PASTA_NOME[N166_NP] = { "Sci-Fi", "Drama" };
// Ano e duracao do destaque: numeros, iguais em todo idioma.
static const char *const META_HOME = "2025  ·  2h 42min";
#define N166_FOCOS 8   // seis itens e as duas pastas

// Topo de cada foco dentro do painel (0 = topo do painel).
static float yFoco(int i) {
  if (i < 6) return N166_P_CAB + (float)i * N166_P_LIN;
  return N166_P_CAB + 6.0f * N166_P_LIN + N166_P_ROT + (float)(i - 6) * N166_P_LIN;
}
static float painelAltura(void) { return yFoco(N166_FOCOS - 1) + N166_P_LIN + 10.0f; }

// Circulo de pasta de Streaming: a capa recortada em circulo (cover).
static void circuloPasta(int p, float cx, float cy, float d, float a) {
  GfxRect c = { cx - d * 0.5f, cy - d * 0.5f, d, d };
  GLuint t = tex_obter_larg(cartaz[p], 90.0f);
  if (!t) { gfx_cor(c, 0.5f, 0.30f, 0.30f, 0.34f, a); return; }
  gfx_tex_aspect_atual = tex_aspecto(cartaz[p]);
  if (gfx_tex_aspect_atual <= 0.0f) gfx_tex_aspect_atual = 2.0f / 3.0f;
  gfx_card_forcar_cover_atual = 1.0f;
  gfx_rect(c, t, GFX_CARD, 0, 0, 0, 0.5f, 0, 0, 0, a);
  gfx_card_forcar_cover_atual = 0.0f;
  gfx_tex_aspect_atual = 0.0f;
}

// A ilha do relogio ancorada pela DIREITA em (xd, y): relogio em repouso, e a
// partir de `abre` (0..1) o aviso do lembrete com o sino no realce.
static void ilhaMini(float xd, float y, float abre, float tAviso, float maxTexto, float a) {
  float ar, ag, ab;
  TxtLinha hora = txt_linha(TXT_CALLOUT, "21:40", 244, 245, 248, 255);
  TxtLinha av = txt_linha_corta(TXT_CALLOUT, i18n("Lost começa agora no Drama HD"), 240, 242, 246, 255, maxTexto);
  float wR = 36.0f + (float)hora.w, wA = 40.0f + 24.0f + 10.0f + (float)av.w;
  float w = anim_mistura(wR, wA, abre), h = anim_mistura(46.0f, 54.0f, abre);
  GfxRect r = { xd - w, y, w, h };
  float aR = a * (1.0f - anim_clamp(abre * 4.0f, 0.0f, 1.0f));
  float aA = a * passo(tAviso, 0.22f, 0.2f);
  if (a <= 0.003f) return;
  ajustes_acento(&ar, &ag, &ab);
  gfx_rect((GfxRect){ r.x - 14.0f, r.y - 5.0f, r.w + 28.0f, r.h + 28.0f }, 0, GFX_SOMBRA,
           1.0f, 0, 0, 0.5f, 0, 0, 0, 0.34f * a);
  if (ajustes_vidro()) gfx_vidro_painel(r, 0.5f, 0.80f, a);
  else gfx_cor(r, 0.5f, 0.055f, 0.058f, 0.068f, 0.86f * a);
  // A luz do aviso respira a ~1 Hz, como na ilha de verdade.
  { float luz = 0.07f + abre * (0.07f + 0.10f * (0.5f + 0.5f * sinf(tAviso * 6.2831853f / 1.1f)));
    float cr = anim_mistura(1.0f, ar, abre), cg = anim_mistura(1.0f, ag, abre), cb = anim_mistura(1.0f, ab, abre);
    gfx_luz_canto(r, 0.5f, r.w * 0.25f, -r.h * 0.9f, r.w * 0.85f, cr, cg, cb, luz * a); }
  recorteDentro((GfxRect){ r.x + 6.0f, r.y, r.w - 12.0f, r.h });
  txt_desenhar_alpha(hora, r.x + (r.w - (float)hora.w) * 0.5f, r.y + (r.h - (float)hora.h) * 0.5f, aR);
  if (aA > 0.003f) {
    float x0 = r.x + (r.w - (wA - 40.0f)) * 0.5f, yc = r.y + r.h * 0.5f;
    gfx_icone((GfxRect){ x0, yc - 12.0f, 24.0f, 24.0f }, "sino", ar, ag, ab, aA);
    txt_desenhar_alpha(av, x0 + 34.0f, yc - (float)av.h * 0.5f, aA);
  }
  recorteVolta();
}

static void cenaBarra(float x, float y, float t, float a) {
  const float W = N166_PV_W, H = N166_PV_H;
  GfxRect pv = { x, y, W, H };
  float s = mola(t, N166_ABRE_T, 0.62f) * (1.0f - passo(t, N166_FECHA_T, 0.34f));
  float px = x + 24.0f, py = y + 88.0f;
  const char *rotIni = menu_rotulo(MENU_INICIO);
  TxtLinha lIni = txt_linha(TXT_CALLOUT, rotIni, 245, 245, 248, 255);
  GfxRect P = { px + N166_SETA, py, 7.0f + N166_PIL_CIRC + 12.0f + (float)lIni.w + 22.0f, N166_PIL_H };
  GfxRect Q = { px, py, N166_P_W, painelAltura() };
  GfxRect R = misturaRect(P, Q, s);
  float raio = anim_mistura(P.h * 0.5f, 30.0f, s);
  float ar, ag, ab;
  int i;
  ajustes_acento(&ar, &ag, &ab);

  // ---- a home da Dinamica atras: destaque, botoes e a fileira na base.
  arte(F_HOME, pv, N166_PV_RAIO, 0.9f, a);
  { float yb = y + H - 24.0f - 113.0f;          // topo da fileira
    float yl = yb - 152.0f;                     // base do logo
    float fo = 1.0f - anim_clamp(s * 2.0f, 0.0f, 1.0f);
    // Aberta, o painel fica por cima do bloco do destaque: ele recua para o
    // painel nao ler como texto por cima de texto.
    float ah = a * (1.0f - 0.85f * anim_clamp(s, 0.0f, 1.0f));
    const char *rp = i18n("Reproduzir");
    float wp = botao_largura(rp, "play", 1);
    TxtLinha meta = txt_linha(TXT_DET_META2, META_HOME, 214, 218, 226, 255);
    logoTitulo(L_HOME, x + N166_M, yl, 300.0f, 104.0f, ah);
    { float wi = badge_imdb(x + N166_M, yl + 16.0f, 77, 0, ah);
      txt_desenhar_alpha(meta, x + N166_M + wi + 16.0f, yl + 16.0f + (BADGE_H - (float)meta.h) * 0.5f, ah); }
    botao_pilula((GfxRect){ x + N166_M, yl + 62.0f, wp, BOTAO_H_SECUNDARIO }, rp, "play", fo, 1, 0, ah);
    botao_disco((GfxRect){ x + N166_M + wp + 14.0f, yl + 62.0f, BOTAO_H_SECUNDARIO, BOTAO_H_SECUNDARIO },
                "mais", 0.0f, ah);
    for (i = 0; i < 3; i++) {
      GfxRect c = { x + N166_M + (float)i * 222.0f, yb, 200.0f, 113.0f };
      arte(F_R1 + i, c, 14.0f, 0.25f, a);
    } }

  // ---- o veu so do lado esquerdo, como tvSombra: faixas de cor chapada.
  if (s > 0.01f) {
    float sx = x + 360.0f;
    gfx_cor((GfxRect){ x, y, 360.0f, H }, 0.0f, 0, 0, 0, 0.26f * s * a);
    for (i = 1; i <= 12; i++, sx += 20.0f)
      gfx_cor((GfxRect){ sx, y, 20.0f, H }, 0.0f, 0, 0, 0, 0.26f * s * a * (1.0f - (float)i / 13.0f));
  }

  // ---- a superficie: escura translucida, tingida de leve pelo realce.
  { float vr = 0.100f + ar * 0.05f, vg = 0.104f + ag * 0.05f, vb = 0.118f + ab * 0.05f;
    gfx_cor(R, raio / R.h, vr, vg, vb, 0.90f * a);
    gfx_anel(R, raio / R.h, 1.2f, 1.0f, 1.0f, 1.0f, 0.12f * a); }

  // ---- fechada: a seta solta sobre a arte, o circulo e a secao.
  { float ap = a * (1.0f - anim_clamp(s * 3.0f, 0.0f, 1.0f));
    if (ap > 0.01f) {
      TxtLinha seta = txt_linha(TXT_HEADLINE, "\xE2\x80\xB9", 245, 245, 248, 255);
      TxtLinha sombra = txt_linha(TXT_HEADLINE, "\xE2\x80\xB9", 0, 0, 0, 255);
      float cy = P.y + P.h * 0.5f, sx = px + (N166_SETA - (float)seta.w) * 0.5f - 3.0f;
      float sy = cy - (float)seta.h * 0.5f - 2.0f;
      GfxRect c = { P.x + 6.0f, cy - N166_PIL_CIRC * 0.5f, N166_PIL_CIRC, N166_PIL_CIRC };
      txt_desenhar_alpha(sombra, sx + 1.0f, sy + 2.0f, ap * 0.35f);
      txt_desenhar_alpha(seta, sx, sy, ap * 0.9f);
      gfx_cor(c, 0.5f, 1, 1, 1, 0.22f * ap);
      gfx_icone((GfxRect){ c.x + 8.0f, c.y + 8.0f, 22.0f, 22.0f }, "menu_home", 0.97f, 0.97f, 0.98f, ap);
      txt_desenhar_alpha(lIni, c.x + c.w + 12.0f, cy - (float)lIni.h * 0.5f, ap);
    } }

  // ---- aberta: cabecalho, itens e as pastas, presos ao retangulo que cresce.
  { float ac = a * anim_clamp((s - 0.22f) / 0.7f, 0.0f, 1.0f);
    if (ac > 0.01f) {
      // O foco anda de item em item; antes de andar, fica no Inicio.
      float pos = anim_clamp((t - N166_ANDA_T) / N166_ANDA_S, 0.0f, (float)(N166_FOCOS - 1));
      int k = (int)pos;
      float m = passo(pos - (float)k, 0.0f, 0.55f);
      float fy = anim_mistura(yFoco(k), yFoco(k + 1 < N166_FOCOS ? k + 1 : k), m);
      const int TINTA = 22;
      recorteDentro(R);
      // Cabecalho: avatar, nome e o relogio.
      { float cy = py + N166_P_CAB * 0.5f + 6.0f;
        GfxRect av = { px + 10.0f + N166_P_COLX - 18.0f, cy - 18.0f, 36.0f, 36.0f };
        TxtLinha ini = txt_linha(TXT_CAPTION2, "N", 255, 255, 255, 255);
        TxtLinha nome = txt_linha(TXT_BODY, i18n("Sua conta"), 240, 240, 240, 255);
        TxtLinha rel = txt_linha(TXT_CAPTION2, "21:40", 200, 200, 200, 255);
        gfx_cor(av, 0.5f, 0.20f, 0.48f, 0.95f, ac);
        txt_desenhar_alpha(ini, av.x + (av.w - (float)ini.w) * 0.5f, av.y + (av.h - (float)ini.h) * 0.5f, ac);
        txt_desenhar_alpha(nome, px + 10.0f + N166_P_ROTX, cy - (float)nome.h * 0.5f, ac);
        txt_desenhar_alpha(rel, px + N166_P_W - 20.0f - (float)rel.w, cy - (float)rel.h * 0.5f, ac); }
      // A pilula BRANCA do foco, deslizando (texto e icone escuros nela).
      { GfxRect pill = { px + 8.0f, py + fy + (N166_P_LIN - N166_P_PIL) * 0.5f, N166_P_W - 16.0f, N166_P_PIL };
        // O item ATUAL (Inicio) sem foco: so um veu claro leve.
        if (fy > yFoco(0) + 1.0f)
          gfx_cor((GfxRect){ pill.x, py + yFoco(0) + (N166_P_LIN - N166_P_PIL) * 0.5f, pill.w, N166_P_PIL },
                  0.5f, 1, 1, 1, 0.09f * ac);
        gfx_cor(pill, 0.5f, 0.95f, 0.95f, 0.96f, ac); }
      { TxtLinha st = txt_linha(TXT_CAPTION2, "Streaming", 150, 152, 160, 255);
        txt_desenhar_alpha(st, px + 10.0f + N166_P_COLX - 12.0f,
                           py + yFoco(6) - (float)st.h - 8.0f, ac); }
      for (i = 0; i < N166_FOCOS; i++) {
        float ly = py + yFoco(i), cy = ly + N166_P_LIN * 0.5f;
        // Quanto da pilula branca esta nesta linha: o texto e o icone passam
        // do claro ao escuro junto com ela (duas linhas prontas, so o alfa muda).
        float f = anim_clamp(1.0f - fabsf(fy - yFoco(i)) / (N166_P_PIL * 0.8f), 0.0f, 1.0f);
        const char *rot = i < 6 ? menu_rotulo(BARRA_DEST[i]) : PASTA_NOME[i - 6];
        TxtLinha l = txt_linha(TXT_CALLOUT, rot, 235, 235, 235, 255);
        TxtLinha le = txt_linha(TXT_CALLOUT, rot, TINTA, TINTA, TINTA, 255);
        float ccx = px + 8.0f + N166_P_COLX, lx = px + 8.0f + N166_P_ROTX;
        if (i < 6) {
          GfxRect ic = { ccx - 11.0f, cy - 11.0f, 22.0f, 22.0f };
          if (f < 1.0f) gfx_icone(ic, BARRA_ICONE[i], 1.0f, 1.0f, 1.0f, ac * 0.85f * (1.0f - f));
          if (f > 0.0f) gfx_icone(ic, BARRA_ICONE[i], 0.08f, 0.08f, 0.08f, ac * f);
        } else circuloPasta(i - 6, ccx, cy, 36.0f, ac);
        if (f < 1.0f) txt_desenhar_alpha(l, lx, cy - (float)l.h * 0.5f, ac * (1.0f - f));
        if (f > 0.0f) txt_desenhar_alpha(le, lx, cy - (float)le.h * 0.5f, ac * f);
      }
      recorteVolta();
    } }

  // ---- a ilha do relogio, no canto de cima a direita (onde a Dinamica a poe).
  { float abre = mola(t, N166_ILHA_T, 0.55f);
    // O aviso nunca passa por cima da pilula da barra, no outro canto.
    ilhaMini(x + W - 24.0f, py + 2.0f, abre, t - N166_ILHA_T, (x + W - 24.0f) - (P.x + P.w + 20.0f) - 74.0f, a); }
}

// ================================================= CENA 1: O CARROSSEL
//
// A fileira da home; o cartao em foco ABRE no cartao grande do carrossel, com
// os vizinhos espiando dos lados. O trailer comeca a tocar dentro do cartao
// (a arte avanca devagar, o selo Trailer acende) e a tira anda para o
// seguinte. O cartao e a GFX_JANELA de detail.c: a arte em "cover" num quadro
// 16:9 da altura da previa, vista por uma janela arredondada.
#define N166_CAR_ABRE   0.55f
#define N166_CAR_ABRE_S 0.80f
#define N166_TRAILER_T  1.85f
#define N166_ANDA_CAR   4.10f
#define N166_CAR_VAO    26.0f

typedef struct { int fundo, logo; const char *meta; int imdb; } TituloCar;
static const TituloCar CAR[4] = {
  { F_CE, -1,   "",                  0 },
  { F_C0, L_C0, "2026  ·  1h 59min", 75 },
  { F_C1, L_C1, "2026  ·  2h 37min", 0 },
  { F_C2, L_C2, "2015  ·  2h 21min", 80 },
};

static void janela(int f, GfxRect r, GfxRect quadro, float zoom, float raioPx, float a) {
  GLuint t = texFundo(f);
  float cx = r.x + r.w * 0.5f, cy = r.y + r.h * 0.5f;
  if (a <= 0.003f) return;
  if (!t) { gfx_cor(r, rr(raioPx, r), 0.16f, 0.17f, 0.19f, a); return; }
  // ZOOM do trailer: a janela cobre um pedaco menor do mesmo quadro.
  gfx_janela_atual[2] = r.w / quadro.w / zoom;
  gfx_janela_atual[3] = r.h / quadro.h / zoom;
  gfx_janela_atual[0] = (cx - quadro.x) / quadro.w - gfx_janela_atual[2] * 0.5f;
  gfx_janela_atual[1] = (cy - quadro.y) / quadro.h - gfx_janela_atual[3] * 0.5f;
  gfx_tex_aspect_atual = tex_aspecto(fundo[f]);
  if (gfx_tex_aspect_atual <= 0.0f) gfx_tex_aspect_atual = 16.0f / 9.0f;
  gfx_rect(r, t, GFX_JANELA, 0.86f, 0.0f, 0.0f, raioPx / r.h, 1, 1, 1, a);
  gfx_tex_aspect_atual = 0.0f;
  gfx_janela_atual[0] = gfx_janela_atual[1] = 0.0f;
  gfx_janela_atual[2] = gfx_janela_atual[3] = 1.0f;
}

// Logo, linha do filme e os botoes no canto de baixo do cartao.
static void conteudoCartao(const TituloCar *c, GfxRect r, float a) {
  const char *rp = i18n("Reproduzir");
  float wp = botao_largura(rp, "play", 1), by = r.y + r.h - 40.0f - 58.0f, my, x0 = r.x + 40.0f;
  static const char *const IC[3] = { "mais", "visto", "trailer" };
  int i;
  if (a <= 0.003f) return;
  my = by - 22.0f - BADGE_H;
  logoTitulo(c->logo, x0, my - 18.0f, 290.0f, 120.0f, a);
  { float xm = x0;
    TxtLinha l = txt_linha(TXT_DET_META2, c->meta, 214, 218, 226, 255);
    if (c->imdb) xm += badge_imdb(xm, my, c->imdb, 0, a) + 14.0f;
    txt_desenhar_alpha(l, xm, my + (BADGE_H - (float)l.h) * 0.5f, a); }
  botao_pilula((GfxRect){ x0, by, wp, 58.0f }, rp, "play", 1.0f, 1, 0, a);
  for (i = 0; i < 3; i++)
    botao_disco((GfxRect){ x0 + wp + 14.0f + (float)i * 70.0f, by, 58.0f, 58.0f }, IC[i], 0.0f, a);
}

static void cenaCarrossel(float x, float y, float t, float a) {
  const float W = N166_PV_W, H = N166_PV_H;
  GfxRect pv = { x, y, W, H };
  GfxRect quadro = { x + (W - H * 16.0f / 9.0f) * 0.5f, y, H * 16.0f / 9.0f, H };
  GfxRect C = { x + 58.0f, y + 92.0f, W - 116.0f, H - 92.0f - 30.0f };
  float o = passo(t, N166_CAR_ABRE, N166_CAR_ABRE_S);
  float off = passo(t, N166_ANDA_CAR, 0.62f);
  float ar, ag, ab;
  int k;
  // A fileira da home de onde ele abre: tres cartoes 16:9, o do meio em foco.
  GfxRect fila[3];
  ajustes_acento(&ar, &ag, &ab);
  for (k = 0; k < 3; k++)
    fila[k] = (GfxRect){ x + (W - 300.0f) * 0.5f + (float)(k - 1) * 324.0f, y + 330.0f, 300.0f, 169.0f };

  // A home atras: a arte do titulo em foco, escurecida (o fundo da Dinamica).
  if (o < 0.999f) {
    float ah = a * (1.0f - anim_clamp(o * 3.0f, 0.0f, 1.0f));
    arte(F_C0, pv, N166_PV_RAIO, 1.0f, ah);
    gfx_cor(pv, rr(N166_PV_RAIO, pv), 0.02f, 0.02f, 0.03f, 0.55f * ah);
    for (k = 0; k < 3; k++) {
      int f = k == 0 ? F_CE : k == 1 ? F_C0 : F_C1;
      arte(f, fila[k], 14.0f, 0.2f, ah);
    }
    gfx_anel_fora(fila[1], rr(14.0f, fila[1]), 3.0f, 3.0f, 0.97f, 0.97f, 0.98f, ah);
  }
  // A folha: opaca em um terco da abertura (a home deixa de ser desenhada).
  gfx_cor(pv, rr(N166_PV_RAIO, pv), 0.105f, 0.110f, 0.125f, a * anim_clamp(o * 3.0f, 0.0f, 1.0f));

  // A tira: o cartao cresce da fileira ate o cartao grande.
  { GfxRect h = misturaRect(fila[1], C, o);
    float raio = anim_mistura(14.0f, 30.0f, o), passoX = h.w + N166_CAR_VAO;
    float zoom = 1.0f + 0.10f * anim_clamp((t - N166_TRAILER_T) / 2.6f, 0.0f, 1.0f);
    for (k = 0; k < 4; k++) {
      GfxRect r = { h.x + ((float)(k - 1) - off) * passoX, h.y, h.w, h.h };
      float ak = k == 1 ? a * anim_clamp(o * 2.5f, 0.0f, 1.0f)
                        : a * anim_clamp((o - 0.55f) * 2.2f, 0.0f, 1.0f);
      if (r.x >= x + W || r.x + r.w <= x) continue;
      // O QUADRO da arte sai do proprio cartaz da fileira (ali a arte aparece
      // inteira) e cresce ate o quadro 16:9 da previa: a abertura e um zoom
      // continuo, sem o salto de "arte inteira" para "recorte".
      { GfxRect qa = { r.x, r.y, r.h * 16.0f / 9.0f, r.h };
        qa.x = r.x + (r.w - qa.w) * 0.5f;
        janela(CAR[k].fundo, r, misturaRect(qa, (GfxRect){ quadro.x + (r.x - h.x), quadro.y, quadro.w, quadro.h }, o),
               k == 1 ? zoom : 1.0f, raio, ak); }
    }
    // Conteudo: so do cartao em cena; na troca, o que sai some antes.
    { float aIn = a * passo(o, 0.75f, 0.25f);
      GfxRect r1 = { h.x - off * passoX, h.y, h.w, h.h }, r2 = { r1.x + passoX, h.y, h.w, h.h };
      if (off < 0.5f) conteudoCartao(&CAR[1], r1, aIn * (1.0f - anim_clamp(off * 3.0f, 0.0f, 1.0f)));
      if (off > 0.4f) conteudoCartao(&CAR[2], r2, a * passo(off, 0.55f, 0.45f)); }
    // O selo do trailer tocando no cartao: icone e a palavra, em pilula escura.
    { float at = a * passo(t, N166_TRAILER_T, 0.3f) * (1.0f - anim_clamp(off * 4.0f, 0.0f, 1.0f));
      if (at > 0.003f) {
        TxtLinha l = txt_linha(TXT_CAPTION2, i18n("Trailer"), 236, 238, 244, 255);
        GfxRect p = { h.x + h.w - 28.0f - 36.0f - 44.0f - (float)l.w + 14.0f, h.y + 26.0f,
                      44.0f + (float)l.w + 22.0f, 40.0f };
        gfx_cor(p, 0.5f, 0.02f, 0.02f, 0.03f, 0.62f * at);
        gfx_icone((GfxRect){ p.x + 14.0f, p.y + 9.0f, 22.0f, 22.0f }, "trailer", ar, ag, ab, at);
        txt_desenhar_alpha(l, p.x + 44.0f, p.y + (p.h - (float)l.h) * 0.5f, at);
        // O progresso do trailer, fino, na base do selo.
        { float pr = anim_clamp((t - N166_TRAILER_T) / 5.0f, 0.0f, 1.0f);
          gfx_cor((GfxRect){ p.x + 14.0f, p.y + p.h - 6.0f, p.w - 28.0f, 2.0f }, 0.5f, 1, 1, 1, 0.18f * at);
          gfx_cor((GfxRect){ p.x + 14.0f, p.y + p.h - 6.0f, (p.w - 28.0f) * pr, 2.0f }, 0.5f, ar, ag, ab, at); }
      } } }
}

// ======================================================== CENA 2: A LIVE TV
//
// O Diagnostico da Live TV com os paineis da tela de verdade (livetvdiag.c):
// a rede ate o provedor, os canais testados um de cada vez (Abrindo no
// player… e o resultado), e a recomendacao do proxy de TS no fim.
// ok: 1 tocou, 2 tocou pelo proxy de TS (o modo P do player), 0 nao decodifica.
typedef struct { const char *nome; int decimos; const char *fmt; int altura; const char *codec;
                 int mbps10, ok; } CanalDiag;
static const CanalDiag DIAG[4] = {
  { "Sci-Fi 24", 12, "HLS", 1080, "H.264", 62, 1 },
  { "Drama HD",  21, "HLS", 1080, "H.264", 54, 2 },
  { "Mystery+",   9, "TS",   720, "H.264", 31, 1 },
  { "Cinema 4K",  0, "",    2160, "HEVC",   0, 0 },
};
// A medida da rede, com a virgula ou o ponto do idioma (numeros, sem traducao).
static const char *const REDE[2] = { "48,6 Mbps  ·  38 ms", "48.6 Mbps  ·  38 ms" };
#define N166_DIAG_T0  0.9f
#define N166_DIAG_S   0.95f
#define N166_LIN_DIAG 64.0f

static void painelDiag(GfxRect r, float ar, float ag, float ab, float a) {
  gfx_cor(r, rr(22.0f, r), 0.055f, 0.065f, 0.085f, 0.97f * a);
  gfx_luz_canto(r, rr(26.0f, r), 150.0f, -70.0f, 500.0f, ar, ag, ab, 0.11f * a);
  gfx_cor((GfxRect){ r.x + 18.0f, r.y, r.w - 36.0f, 2.0f }, 1.0f, ar, ag, ab, 0.48f * a);
}

static void cenaLiveTV(float x, float y, float t, float a) {
  const float W = N166_PV_W, H = N166_PV_H;
  float ar, ag, ab, yy;
  int i, ponto = idioma_ponto_decimal(ajustes_idioma());
  ajustes_acento(&ar, &ag, &ab);
  arte(F_VIVO, (GfxRect){ x, y, W, H }, N166_PV_RAIO, 1.0f, 0.30f * a);
  // Titulo e o que a tela faz.
  { float hT = txt_bloco(TXT_TITULO3, i18n("Diagnóstico da Live TV"), 248, 249, 252, x + N166_M,
                         y + 96.0f, W - 2.0f * N166_M, 50.0f, a, 1);
    // Uma linha so: em duas, a segunda cairia em cima do painel da rede.
    TxtLinha sub = txt_linha_corta(TXT_CAPTION2, i18n("Mede a rede até o provedor e testa canais de verdade nesta TV."),
                                   184, 190, 202, 255, W - 2.0f * N166_M);
    txt_desenhar_alpha(sub, x + N166_M, y + 96.0f + hT + 6.0f, a); }
  // A rede: medindo, e o numero quando chega.
  { GfxRect r = { x + 32.0f, y + 200.0f, W - 64.0f, 72.0f };
    float med = passo(t, 0.55f, 0.25f);
    TxtLinha rot = txt_linha(TXT_BODY, i18n("Rede até o provedor"), 238, 242, 248, 255);
    TxtLinha mm = txt_linha(TXT_CAPTION, i18n("Medindo…"), 170, 178, 190, 255);
    TxtLinha v = txt_linha(TXT_BODY, REDE[ponto ? 1 : 0], 238, 242, 248, 255);
    painelDiag(r, ar, ag, ab, a);
    txt_desenhar_alpha(rot, r.x + 28.0f, r.y + (r.h - (float)rot.h) * 0.5f, a);
    txt_desenhar_alpha(mm, r.x + r.w - 28.0f - (float)mm.w, r.y + (r.h - (float)mm.h) * 0.5f, a * (1.0f - med));
    txt_desenhar_alpha(v, r.x + r.w - 28.0f - (float)v.w, r.y + (r.h - (float)v.h) * 0.5f, a * med); }
  // Os canais, um de cada vez.
  { GfxRect r = { x + 32.0f, y + 288.0f, W - 64.0f, 92.0f + 4.0f * N166_LIN_DIAG };
    char sub[160];
    painelDiag(r, ar, ag, ab, a);
    snprintf(sub, sizeof sub, i18n("%d canais de “%s”, um de cada vez"), 4, "Sci-Fi");
    { TxtLinha tt = txt_linha(TXT_BODY, i18n("Canais testados"), 238, 242, 248, 255);
      TxtLinha ts = txt_linha_corta(TXT_CAPTION, sub, 154, 164, 178, 255, r.w - 56.0f);
      txt_desenhar_alpha(tt, r.x + 28.0f, r.y + 20.0f, a);
      txt_desenhar_alpha(ts, r.x + 28.0f, r.y + 52.0f, a); }
    for (i = 0; i < 4; i++) {
      const CanalDiag *c = &DIAG[i];
      float ly = r.y + 92.0f + (float)i * N166_LIN_DIAG, t0 = N166_DIAG_T0 + N166_DIAG_S * (float)i;
      float vez = (t >= t0 && t < t0 + N166_DIAG_S) ? 1.0f : 0.0f;
      float pronto = passo(t, t0 + 0.62f, 0.2f);
      GfxRect lg = { r.x + 24.0f, ly - 2.0f, 70.0f, 42.0f };
      char res[160], tmp[32], fm[48];
      TxtLinha nm, la, lr, ld;
      int cr, cg, cb;
      if (vez > 0.0f)
        gfx_cor((GfxRect){ r.x + 14.0f, ly - 8.0f, r.w - 28.0f, N166_LIN_DIAG - 2.0f },
                rr(12.0f, (GfxRect){ 0, 0, r.w, N166_LIN_DIAG - 4.0f }), ar, ag, ab, 0.10f * a);
      guia_logo_desenhar("", c->nome, lg, 70.0f, 42.0f, 0.965f, a);
      nm = txt_linha_corta(TXT_BODY, c->nome, 232, 236, 244, 255, r.w * 0.34f);
      txt_desenhar_alpha(nm, lg.x + lg.w + 16.0f, ly + (lg.h - (float)nm.h) * 0.5f, a);
      if (c->ok) {
        segundosDec(tmp, sizeof tmp, c->decimos);
        if (c->ok == 2) snprintf(fm, sizeof fm, i18n("%s · modo %s"), c->fmt, "P");
        else snprintf(fm, sizeof fm, "%s", c->fmt);
        snprintf(res, sizeof res, i18n("Tocou em %s (%s)"), tmp, fm);
        cr = 150; cg = 222; cb = 170;
      } else {
        snprintf(res, sizeof res, "%s", i18n("Os dados chegam, mas a TV não decodifica"));
        cr = 244; cg = 170; cb = 140;
      }
      // Na fila -> Abrindo no player… -> o resultado (com o detalhe embaixo).
      { const char *estado = t < t0 ? i18n("Na fila") : i18n("Abrindo no player…");
        float wMax = r.x + r.w - 24.0f - (lg.x + lg.w + 16.0f + (float)nm.w + 18.0f);
        la = txt_linha_corta(TXT_CAPTION, estado, 190, 196, 206, 255, wMax);
        lr = txt_linha_corta(TXT_CAPTION, res, cr, cg, cb, 255, wMax);
        txt_desenhar_alpha(la, r.x + r.w - 24.0f - (float)la.w, ly + (lg.h - (float)la.h) * 0.5f,
                           a * (1.0f - pronto));
        txt_desenhar_alpha(lr, r.x + r.w - 24.0f - (float)lr.w, ly + (lg.h - (float)lr.h) * 0.5f,
                           a * pronto); }
      // O detalhe como o diagnostico escreve: resolucao, codec e vazao.
      { char d[64];
        if (c->ok == 0) snprintf(d, sizeof d, "%dp  ·  %s  ·  %s", c->altura, c->codec, i18n("10 bits"));
        else snprintf(d, sizeof d, "%dp  ·  %s  ·  %d%c%d Mbps", c->altura, c->codec, c->mbps10 / 10,
                      ponto ? '.' : ',', c->mbps10 % 10);
        ld = txt_linha(TXT_CAPTION2, d, 150, 160, 174, 255); }
      txt_desenhar_alpha(ld, r.x + r.w - 24.0f - (float)ld.w, ly + lg.h - 6.0f, a * pronto);
    }
    yy = r.y + r.h + 16.0f; }
  // A recomendacao que liga o proxy, e o Aplicar.
  { float ra = a * passo(t, N166_DIAG_T0 + 4.0f * N166_DIAG_S + 0.1f, 0.35f);
    GfxRect r = { x + 32.0f, yy, W - 64.0f, y + H - 30.0f - yy };
    const char *ap = i18n("Aplicar");
    float wb = botao_largura(ap, NULL, 1);
    if (ra > 0.003f) {
      painelDiag(r, ar, ag, ab, ra);
      txt_bloco(TXT_CAPTION, i18n("Os canais HLS só abriram pelo proxy de TS: ele fica ligado para a Live TV."),
                (int)(ar * 90.0f + 160.0f), (int)(ag * 90.0f + 160.0f), (int)(ab * 90.0f + 160.0f),
                r.x + 28.0f, r.y + 20.0f, r.w - 56.0f - wb - 24.0f, 28.0f, ra, 3);
      botao_pilula((GfxRect){ r.x + r.w - 24.0f - wb, r.y + (r.h - BOTAO_H_SECUNDARIO) * 0.5f, wb,
                              BOTAO_H_SECUNDARIO }, ap, NULL, 1.0f, 1, 0, ra);
    } }
}

// ================================================================ as tabelas
typedef struct {
  const char *nome;                              // selo no alto da previa
  void (*desenhar)(float x, float y, float t, float a);
  float duracao, estatico;                       // s; o quadro das animacoes reduzidas
  int id, id2;                                   // linhas da lista que ela ilustra
} Cena;

static const Cena CENAS[] = {
  { "Barra lateral nova",    cenaBarra,     6.4f, 2.75f, ID_ATV, ID_ILHA },
  { "Abertura em carrossel", cenaCarrossel, 6.0f, 3.0f, ID_ATV,  ID_NADA },
  { "Live TV",               cenaLiveTV,    6.6f, 6.6f, ID_DIAG, ID_LIVE },
};
#define N166_NC ((int)(sizeof CENAS / sizeof *CENAS))

typedef struct { int id; const char *icone, *nome, *linha; } Item;

// A LISTA, agrupada por area: nome e UMA linha, com o caminho quando ha um.
static const Item ITENS[] = {
  { ID_ATV,        "aj_layout-dashboard", "Layout Apple TV",
    "Barra em pílula com o Streaming e títulos em carrossel. Ajustes › Layout." },
  { ID_FILEIRA,    "aj_rows-3",           "Estilo da fileira",
    "Nove formas, tamanhos P, M e G e ranking. Segure OK num pôster." },
  { ID_ILHA,       "sino",                "Ilha do relógio",
    "O relógio no alto da tela se abre para mostrar os avisos." },
  { ID_LIVE,       "menu_guide",          "Live TV",
    "Canais HLS tocam na LG, busca no guia e botão Proporção no canal." },
  { ID_DIAG,       "aj_scan",             "Diagnóstico da Live TV",
    "Testa canais nesta TV e acerta resolução e formato. Fica no guia." },
  { ID_FONTES,     "fontes",              "Fontes",
    "A folha de fontes ficou mais leve." },
  { ID_LOGO,       "aj_images",           "Logo do título",
    "O logo no lugar do nome nos menus do título e da temporada." },
  { ID_COMUNIDADE, "recomendar",          "Comunidade Nuvio Native",
    "Os perfis públicos de todo mundo, em Social › Encontrar pessoas." },
  { ID_ATUALIZA,   "aj_rotate-ccw-clock", "Atualizações",
    "Procurar atualização em Ajustes › Sobre. O app confere a cada 6 h." },
  { ID_CONSERTOS,  "check",               "Consertos",
    "Continuar assistindo, pôsteres no idioma, legendas, Magic Remote e trailer." },
};
#define N166_NI ((int)(sizeof ITENS / sizeof *ITENS))

// ------------------------------------------------------------------- estado
int novidades166_itens(void) { return N166_NI; }
int novidades166_item_largura(int i, int *limite, const char **nome) {
  float tx = N166_TXT_X + N166_ICONE + 20.0f;
  if (i < 0 || i >= N166_NI) return 0;
  if (limite) *limite = (int)(N166_TXT_X + N166_TXT_W - tx);
  if (nome) *nome = ITENS[i].nome;
  return txt_largura(TXT_CAPTION, i18n(ITENS[i].linha));
}

int novidades166_cenas(void) { return N166_NC; }
int novidades166_cena(void) { return cena; }
int novidades166_aberto(void) { return aberto; }
int novidades166_foco_na_previa(void) { return naPrevia; }

void novidades166_dir(const char *d) {
  if (d && d[0]) snprintf(dirArte, sizeof dirArte, "%s", d);
  montarCaminhos();
}

int novidades166_pedido(void) {
  int p = pedido;
  pedido = N166_PEDIU_NADA;
  return p;
}

static void mudarCena(int nova) {
  if (nova < 0) nova = N166_NC - 1;
  if (nova >= N166_NC) nova = 0;
  if (nova == cena) return;
  tempoAntiga = relogioCena;
  cenaAntiga = cena;
  cena = nova;
  transicao = ajustes_animacoes_reduzidas() ? 1.0f : 0.0f;
  relogioCena = 0.0f;
}

void novidades166_ir(int c, float t) {
  cena = cenaAntiga = (c % N166_NC + N166_NC) % N166_NC;
  transicao = 1.0f;
  relogioCena = t;
}

static void comecar(float e) {
  aberto = decidido = 1;
  foco = B_LAYOUT;
  naPrevia = 0;
  cena = cenaAntiga = 0;
  entrada = e;
  transicao = 1.0f;
  relogioCena = tempoAntiga = 0.0f;
  aquecida = 0;
  textogate_reiniciar(&gateLista);
  if (!fundo[0][0]) montarCaminhos();
  pedirArtes();
}

void novidades166_abrir(void) { comecar(1.0f); }

void novidades166_primeira_vez(void) {
  char *s;
  if (decidido) return;
  decidido = 1;
  s = dados_ler(N166_ARQ);
  if (s) { free(s); return; }
  comecar(0.0f);
}

static void fechar(int oQue) {
  aberto = 0;
  dados_gravar(N166_ARQ, "1\n");
  pedido = oQue;
}

// D-PAD. Duas fileiras de foco: os botoes (de fabrica, no primario) e a previa
// (cima). Na previa, esquerda/direita trocam a cena; nos botoes, andam entre eles.
void novidades166_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto || !e || e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (k == SDLK_UP)   { naPrevia = 1; return; }
  if (k == SDLK_DOWN) { naPrevia = 0; return; }
  if (k == SDLK_LEFT) {
    if (naPrevia) mudarCena(cena - 1);
    else if (foco > 0) foco--;
    return;
  }
  if (k == SDLK_RIGHT) {
    if (naPrevia) mudarCena(cena + 1);
    else if (foco < B_N - 1) foco++;
    return;
  }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) {
    if (naPrevia) { mudarCena(cena + 1); return; }
    fechar(foco == B_LAYOUT ? N166_PEDIU_LAYOUT : foco == B_GUIA ? N166_PEDIU_GUIA
                                                                 : N166_PEDIU_NADA);
    return;
  }
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      k == SDLK_DELETE || e->key.keysym.scancode == NV_SCANCODE_BACK)
    fechar(N166_PEDIU_NADA);
}

void novidades166_atualizar(float dt, Uint32 agora) {
  (void)agora;
  if (!aberto && entrada < 0.002f) { entrada = 0.0f; return; }
  if (aberto) pedirArtes();
  if (ajustes_animacoes_reduzidas()) {
    // Sem passagem, sem movimento dentro da cena; a troca continua, seca.
    entrada = aberto ? 1.0f : 0.0f;
    transicao = 1.0f;
    if (aberto) {
      relogioCena += dt;
      if (relogioCena >= CENAS[cena].duracao) mudarCena(cena + 1);
    }
    return;
  }
  entrada = anim_rampa(entrada, aberto ? 1.0f : 0.0f, dt, aberto ? N166_ABRIR_MS : N166_FECHAR_MS);
  if (aberto) {
    if (transicao < 1.0f) {
      transicao += dt / N166_TRANSICAO_S;
      if (transicao > 1.0f) transicao = 1.0f;
      tempoAntiga += dt;
    }
    relogioCena += dt;
    if (relogioCena >= CENAS[cena].duracao) mudarCena(cena + 1);
  }
}

// ------------------------------------------------------------------ desenho
static float tempoDe(int c, float t) {
  return ajustes_animacoes_reduzidas() ? CENAS[c].estatico : t;
}

static void desenhaCena(int c, float x, float y, float t, float a) {
  if (a <= 0.003f) return;
  CENAS[c].desenhar(x, y, tempoDe(c, t), a);
}

// O selo com o nome da cena e os tracos do ciclo, no alto da previa. Com o
// foco na previa, o selo vira a pilula de foco dos botoes.
static GfxRect seloRect;
static void selo(float x, float y, float a) {
  float fr, fg, fb, ti = botao_cor_foco(&fr, &fg, &fb);
  int tinta = (int)(ti * 255.0f + 0.5f), i;
  const char *nome = i18n(CENAS[cena].nome);
  TxtLinha l = txt_linha(TXT_CAPTION, nome, 236, 238, 244, 255);
  TxtLinha lf = txt_linha(TXT_CAPTION, nome, tinta, tinta, tinta, 255);
  GfxRect s = { x + 24.0f, y + 24.0f, (float)l.w + 36.0f, 44.0f };
  float tA = anim_suave(transicao);
  gfx_cor(s, 0.5f, 0.02f, 0.02f, 0.03f, 0.58f * a);
  if (naPrevia) { botao_luz(s, 1.0f, a); gfx_cor(s, 0.5f, fr, fg, fb, a); }
  txt_desenhar_alpha(naPrevia ? lf : l, s.x + 18.0f, s.y + (s.h - (float)l.h) * 0.5f, a * tA);
  seloRect = s;
  { float tw = 26.0f, gap = 8.0f, lw = (float)N166_NC * (tw + gap) - gap;
    GfxRect p = { x + N166_PV_W - 24.0f - lw - 16.0f, y + 32.0f, lw + 32.0f, 28.0f };
    gfx_cor(p, 0.5f, 0.02f, 0.02f, 0.03f, 0.50f * a); }
  for (i = 0; i < N166_NC; i++) {
    float tw = 26.0f, gap = 8.0f;
    float tx = x + N166_PV_W - 24.0f - (float)(N166_NC - i) * (tw + gap) + gap;
    GfxRect tr = { tx, y + 44.0f, tw, 4.0f };
    gfx_cor(tr, 0.5f, 1, 1, 1, 0.26f * a);
    if (i < cena) gfx_cor(tr, 0.5f, 1, 1, 1, 0.62f * a);
    if (i == cena) {
      float p = anim_clamp(relogioCena / CENAS[cena].duracao, 0.0f, 1.0f);
      tr.w *= p;
      if (tr.w > 1.0f) gfx_cor(tr, 0.5f, 1, 1, 1, 0.94f * a);
    }
  }
}

// Uma linha da lista. `vivo` 0..1: a cena na previa fala desta linha.
static float linhaH[32];
static void desenhaItem(int i, float y, float vivo, int maxL, float a) {
  float fr, fg, fb, ti = botao_cor_foco(&fr, &fg, &fb);
  float tx = N166_TXT_X + N166_ICONE + 20.0f, tw = N166_TXT_X + N166_TXT_W - tx;
  GfxRect d = { N166_TXT_X, y + 4.0f, N166_ICONE, N166_ICONE };
  GfxRect ic = { d.x + 9.0f, d.y + 9.0f, 22.0f, 22.0f };
  float c = 0.90f + (ti - 0.90f) * vivo;
  gfx_cor(d, 0.5f, 1, 1, 1, 0.08f * a);
  if (vivo > 0.0f) gfx_cor(d, 0.5f, fr, fg, fb, vivo * a);
  gfx_icone(ic, ITENS[i].icone, c, c, c + (vivo > 0.5f ? 0.0f : 0.02f), a);
  { TxtLinha n = txt_linha_corta(TXT_BODY, ITENS[i].nome, 244, 246, 250, 255, tw);
    txt_desenhar_alpha(n, tx, y, a); }
  txt_bloco_corta(TXT_CAPTION, ITENS[i].linha, 176, 182, 196, tx, y + 33.0f, tw, 27.0f, a, maxL);
}

// Mede a lista (sem rasterizar) e distribui o espaco. Cada descricao tem UMA
// linha; a que nao cabe ganha a segunda enquanto houver altura (na ordem da
// lista), e as que sobrarem cortam com reticencias. O espaco entre as linhas
// e o que restar, entre 8 e 26 px.
static int linhaMax[32];
static void medirLista(float alto, float *gap) {
  float tx = N166_TXT_X + N166_ICONE + 20.0f, tw = N166_TXT_X + N166_TXT_W - tx;
  float soma = 60.0f * (float)N166_NI, folga;
  int i;
  folga = alto - soma - 8.0f * (float)(N166_NI - 1);
  for (i = 0; i < N166_NI; i++) {
    int larga = txt_largura(TXT_CAPTION, i18n(ITENS[i].linha)) > (int)tw;
    linhaMax[i] = 1;
    linhaH[i] = 60.0f;
    if (larga && folga >= 27.0f) {
      linhaMax[i] = 2; linhaH[i] += 27.0f; soma += 27.0f; folga -= 27.0f;
    }
  }
  *gap = N166_NI > 1 ? anim_clamp((alto - soma) / (float)(N166_NI - 1), 8.0f, 26.0f) : 0.0f;
}

static void ponteiroFoco(int b, int nada) { (void)nada; foco = b; naPrevia = 0; }
static void focarSelo(int nada, int nada2) { (void)nada; (void)nada2; naPrevia = 1; }
static void ponteiroSelo(int nada, int nada2) { (void)nada; (void)nada2; naPrevia = 1; mudarCena(cena + 1); }

void novidades166_desenhar(Uint32 agora) {
  float a = anim_suave(entrada), dy, y0, ar, ag, ab;
  GfxRect card;
  (void)agora;
  if (entrada < 0.002f) return;
  if (aberto) ponteiro_camada();
  ajustes_acento(&ar, &ag, &ab);
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0, 0, 0, 0.78f * entrada);
  dy = (1.0f - a) * 34.0f;
  y0 = N166_Y + dy;
  card = (GfxRect){ N166_X, y0, N166_W, N166_H };
  gfx_cor(card, N166_RAIO / N166_H, 0.050f, 0.053f, 0.062f, 0.96f * a);
  gfx_rect(card, 0, GFX_ANEL, 0, 1.2f / N166_H, 0, N166_RAIO / N166_H, 1, 1, 1, 0.06f * a);
  gfx_luz_canto(card, N166_RAIO / N166_H, N166_PAD + N166_PV_W * 0.5f, -120.0f, 820.0f,
                ar, ag, ab, 0.10f * a);
  recorte(card);

  // ----- a previa: cena que sai e cena que entra, cada uma na sua tesoura.
  { float px = N166_X + N166_PAD, py = y0 + N166_PAD;
    float t = anim_suave(transicao);
    GfxRect pv = { px, py, N166_PV_W, N166_PV_H };
    clipCena = pv;
    recorte(pv);
    // O chao da previa e um so; as cenas passam EM SEQUENCIA por cima dele (a
    // que sai some na primeira metade, a que entra aparece depois).
    gfx_cor(pv, rr(N166_PV_RAIO, pv), 0.062f, 0.066f, 0.080f, a);
    if (transicao < 1.0f && cenaAntiga != cena)
      desenhaCena(cenaAntiga, px - t * 24.0f, py, tempoAntiga, a * (1.0f - anim_clamp(t / 0.5f, 0.0f, 1.0f)));
    desenhaCena(cena, px + (1.0f - t) * 24.0f, py, relogioCena,
                a * (cenaAntiga != cena ? anim_clamp((t - 0.38f) / 0.62f, 0.0f, 1.0f) : t));
    // Aquecer a cena seguinte: numa tesoura de 1 px e alfa quase nulo.
    { int prox = (cena + 1) % N166_NC;
      if (!(aquecida & (1u << prox)) && transicao >= 1.0f) {
        int pend0 = txt_pendentes;
        clipCena = (GfxRect){ px, py, 1.0f, 1.0f };
        recorte(clipCena);
        desenhaCena(prox, px, py, CENAS[prox].estatico, NV_TXTGATE_AQUECER);
        if (txt_pendentes == pend0) aquecida |= 1u << prox;
        clipCena = pv;
        recorte(pv);
      }
      if (txt_pendentes == 0) aquecida |= 1u << cena; }
    selo(px, py, a);
    if (aberto)
      ponteiro_alvo(seloRect.x, seloRect.y, seloRect.w, seloRect.h, focarSelo, ponteiroSelo, 0, 0);
    recorte(card); }

  // ----- a coluna da direita: a linha de cima, o titulo e a lista.
  { char tit[64];
    float ya = y0 + N166_PAD, ta;
    int pend0 = txt_pendentes, i;
    float gap, alto, yy;
    float fechado = textogate_aberto(&gateLista) ? 0.0f : 1.0f;
    ta = fechado > 0.0f ? NV_TXTGATE_AQUECER : a * textogate_passo(&gateLista, 0, SDL_GetTicks());
    snprintf(tit, sizeof tit, i18n("Novidades da %s"), N166_VERSAO);
    { TxtLinha k = txt_linha(TXT_CAPTION2, i18n("LAYOUT APPLE TV E LIVE TV"),
                             (int)(ar * 90.0f + 150.0f), (int)(ag * 90.0f + 150.0f),
                             (int)(ab * 90.0f + 150.0f), 255);
      txt_desenhar_alpha(k, N166_TXT_X, ya, ta); }
    txt_bloco(TXT_TITULO2, tit, 248, 249, 252, N166_TXT_X, ya + 30.0f, N166_TXT_W, 62.0f, ta, 1);
    yy = ya + 30.0f + 88.0f;
    alto = (y0 + N166_PAD + N166_PV_H) - yy;
    medirLista(alto, &gap);
    for (i = 0; i < N166_NI; i++) {
      float vivo = 0.0f, tA = anim_suave(transicao);
      float local = ajustes_animacoes_reduzidas() || fechado > 0.0f
                  ? 1.0f : anim_clamp((a - 0.04f * (float)i) * 3.0f, 0.0f, 1.0f);
      if (ITENS[i].id == CENAS[cena].id || ITENS[i].id == CENAS[cena].id2) vivo += tA;
      if (transicao < 1.0f && (ITENS[i].id == CENAS[cenaAntiga].id || ITENS[i].id == CENAS[cenaAntiga].id2))
        vivo += 1.0f - tA;
      if (vivo > 1.0f) vivo = 1.0f;
      desenhaItem(i, yy + (1.0f - local) * 12.0f, vivo, linhaMax[i], ta * local);
      yy += linhaH[i] + gap;
    }
    if (fechado > 0.0f) textogate_passo(&gateLista, txt_pendentes - pend0, SDL_GetTicks()); }

  // ----- o rodape: a dica do D-pad a esquerda, os tres botoes a direita.
  { float yBase = y0 + N166_H - N166_PAD;
    const char *rotL = i18n("Ver o layout Apple TV");
    const char *rotG = i18n("Abrir o guia");
    const char *rotD = i18n("Agora não");
    float wL = botao_largura(rotL, NULL, 1), wG = botao_largura(rotG, NULL, 0);
    float wD = botao_largura(rotD, NULL, 0);
    GfxRect bL = { N166_X + N166_W - N166_PAD - wL, yBase - BOTAO_H_PRIMARIO, wL, BOTAO_H_PRIMARIO };
    GfxRect bG = { bL.x - BOTAO_GAP - wG, yBase - BOTAO_H_PRIMARIO * 0.5f - BOTAO_H_SECUNDARIO * 0.5f,
                   wG, BOTAO_H_SECUNDARIO };
    GfxRect bD = { bG.x - BOTAO_GAP - wD, bG.y, wD, BOTAO_H_SECUNDARIO };
    { TxtLinha h = txt_linha_corta(TXT_CAPTION2,
                     i18n(naPrevia ? "← → Trocar a prévia  ·  ↓ Botões" : "↑ Escolher a prévia"),
                     150, 156, 170, 255, bD.x - 32.0f - (N166_X + N166_PAD));
      txt_desenhar_alpha(h, N166_X + N166_PAD + 4.0f, yBase - BOTAO_H_PRIMARIO * 0.5f - (float)h.h * 0.5f,
                         a * 0.9f); }
    botao_pilula(bD, rotD, NULL, !naPrevia && foco == B_DEPOIS ? 1.0f : 0.0f, 0, 0, a);
    botao_pilula(bG, rotG, NULL, !naPrevia && foco == B_GUIA ? 1.0f : 0.0f, 0, 0, a);
    botao_pilula(bL, rotL, NULL, !naPrevia && foco == B_LAYOUT ? 1.0f : 0.0f, 1, 0, a);
    if (aberto) {
      ponteiro_alvo(bD.x, bD.y, bD.w, bD.h, ponteiroFoco, NULL, B_DEPOIS, 0);
      ponteiro_alvo(bG.x, bG.y, bG.w, bG.h, ponteiroFoco, NULL, B_GUIA, 0);
      ponteiro_alvo(bL.x, bL.y, bL.w, bL.h, ponteiroFoco, NULL, B_LAYOUT, 0);
    } }
  gfx_sem_recorte();
}
