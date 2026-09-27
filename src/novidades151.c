// Cartao de novidades da 1.5.1. Tres cenas com amostras reais de tipografia
// e desenhos simples dos novos paines de Ajustes.
#include "novidades151.h"
#include "ajustes.h"
#include "anim.h"
#include "botoes.h"
#include "dados.h"
#include "gfx.h"
#include "idioma.h"
#include "layout.h"
#include "player.h"
#include "ponteiro.h"
#include "text.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define N151_ARQ          "novidades-151-ui.txt"
#define N151_W            1600.0f
#define N151_H             880.0f
#define N151_X            ((NV_TELA_W - N151_W) * 0.5f)
#define N151_Y            ((NV_TELA_H - N151_H) * 0.5f)
#define N151_PAD            48.0f
#define N151_RAIO           30.0f
#define N151_PREV_W        700.0f
#define N151_PREV_H        590.0f
#define N151_GAP            58.0f
#define N151_TXT_X        (N151_X + N151_PAD + N151_PREV_W + N151_GAP)
#define N151_TXT_W        (N151_X + N151_W - N151_PAD - N151_TXT_X)
#define N151_ABRIR_MS      260.0f
#define N151_FECHAR_MS     150.0f
#define N151_TRANSICAO_S     0.42f
#define N151_CENA_S          6.2f
#define N151_CENAS              3

enum { N151_AGORA = 0, N151_EXPLORAR = 1 };

static int aberto, decidido, focoBotao = N151_EXPLORAR;
static int cena, cenaAntiga, pedido;
static float entrada, transicao = 1.0f, relogioCena;

typedef struct { const char *titulo, *descricao; } CenaTexto;
static const CenaTexto TEXTOS[N151_CENAS] = {
  { "Fontes para a interface e legendas",
    "Inter, Montserrat, Roboto e Atkinson Hyperlegible Next para menus. A legenda mantém sua escolha no player." },
  { "Prévia por categoria e opção",
    "Veja como cada ajuste afeta pôsteres, detalhes e reprodução." },
  { "Mais clareza em Ajustes",
    "A ajuda visual acompanha o item em foco; sinopses longas terminam com reticências." }
};

static void mudarCena(int nova) {
  if (nova < 0) nova = N151_CENAS - 1;
  if (nova >= N151_CENAS) nova = 0;
  if (nova == cena) return;
  cenaAntiga = cena;
  cena = nova;
  transicao = 0.0f;
  relogioCena = 0.0f;
}

static void fechar(int explorar) {
  aberto = 0;
  dados_gravar(N151_ARQ, "1\n");
  pedido = explorar ? N151_PEDIU_AJUSTES : N151_PEDIU_NADA;
}

void novidades151_primeira_vez(void) {
  char *s;
  if (decidido) return;
  decidido = 1;
  s = dados_ler(N151_ARQ);
  if (s) { free(s); return; }
  aberto = 1;
  focoBotao = N151_EXPLORAR;
  cena = cenaAntiga = 0;
  entrada = 0.0f;
  transicao = 1.0f;
  relogioCena = 0.0f;
}

int novidades151_aberto(void) { return aberto; }

void novidades151_abrir(void) {
  aberto = decidido = 1;
  focoBotao = N151_EXPLORAR;
  cena = cenaAntiga = 0;
  entrada = 1.0f;
  transicao = 1.0f;
  relogioCena = 0.0f;
}

int novidades151_pedido(void) {
  int p = pedido;
  pedido = N151_PEDIU_NADA;
  return p;
}

static void ponteiroCena(int c, int nada) { (void)nada; mudarCena(c); }
static void ponteiroBotao(int b, int nada) { (void)nada; focoBotao = b; }

void novidades151_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto || !e || e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (k == SDLK_LEFT) { mudarCena(cena - 1); return; }
  if (k == SDLK_RIGHT) { mudarCena(cena + 1); return; }
  if (k == SDLK_UP || k == SDLK_DOWN) {
    focoBotao = focoBotao == N151_EXPLORAR ? N151_AGORA : N151_EXPLORAR;
    return;
  }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) {
    fechar(focoBotao == N151_EXPLORAR);
    return;
  }
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      k == SDLK_DELETE || e->key.keysym.scancode == NV_SCANCODE_BACK)
    fechar(0);
}

void novidades151_atualizar(float dt, Uint32 agora) {
  (void)agora;
  if (!aberto && entrada < 0.002f) { entrada = 0.0f; return; }
  if (ajustes_animacoes_reduzidas()) {
    entrada = aberto ? 1.0f : 0.0f;
    transicao = 1.0f;
    relogioCena = 0.0f; // sem movimento automatico com animacoes reduzidas
    return;
  }
  entrada = anim_rampa(entrada, aberto ? 1.0f : 0.0f, dt,
                       aberto ? N151_ABRIR_MS : N151_FECHAR_MS);
  if (aberto) {
    if (transicao < 1.0f) {
      transicao += dt / N151_TRANSICAO_S;
      if (transicao > 1.0f) transicao = 1.0f;
    }
    relogioCena += dt;
    if (relogioCena >= N151_CENA_S) mudarCena(cena + 1);
  }
}

static void caixa(GfxRect r, float raio, float a) {
  gfx_cor(r, raio / r.h, 0.090f, 0.095f, 0.11f, a);
}

static void contornoAcao(GfxRect r, float raio, float a) {
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  gfx_anel(r, raio / r.h, 2.0f, ar, ag, ab, a);
}

static void desenhoFontes(float x, float y, float a) {
  static const TxtFamilia F[] = {
    TXT_FAMILIA_INTER, TXT_FAMILIA_MONTSERRAT,
    TXT_FAMILIA_ROBOTO, TXT_FAMILIA_ATKINSON
  };
  const TxtFamilia interfaceAtual = txt_fonte_interface();
  VideoLegendaEstilo *leg = player_leg_estilo();
  TxtFamilia familiaLeg = leg && leg->familia >= 0 && leg->familia < TXT_FAMILIA_N
                        ? (TxtFamilia)leg->familia : TXT_FAMILIA_INTER;
  int i;
  const int nFontes = (int)(sizeof F / sizeof *F);
  /* Ping-pong entre as quatro linhas: nunca invade o painel de legendas. */
  float ciclo = fmodf(relogioCena / 1.55f, (float)(2 * (nFontes - 1)));
  float focoDemo = ciclo <= (float)(nFontes - 1)
                 ? ciclo : (float)(2 * (nFontes - 1)) - ciclo;
  int focoLinha = (int)focoDemo;
  int proxLinha = focoLinha < nFontes - 1 ? focoLinha + 1 : focoLinha - 1;
  float focoMix = anim_suave(focoDemo - (float)focoLinha);
  for (i = 0; i < (int)(sizeof F / sizeof *F); i++) {
    float yy = y + 22.0f + (float)i * 75.0f;
    GfxRect r = { x + 18.0f, yy, N151_PREV_W - 36.0f, 72.0f };
    caixa(r, 12.0f, a);
    if (F[i] == interfaceAtual) contornoAcao(r, 12.0f, a * 0.94f);
    { TxtLinha nome = txt_linha(TXT_CAPTION2, TXT_FAMILIAS_PT[F[i]],
                                170, 176, 188, 255);
      txt_desenhar_alpha(nome, r.x + 16.0f, r.y + 3.0f, a); }
    { TxtLinha amostra = txt_linha_familia(TXT_HEADLINE,
                              i18n("Aa  Nuvio  0123"), 241, 243, 247, 255, F[i]);
      txt_desenhar_alpha(amostra, r.x + 16.0f, r.y + 27.0f, a); }
  }
  // A borda branca e o foco de demonstração que passeia entre as amostras;
  // o anel do acento identifica separadamente a fonte realmente escolhida.
  { GfxRect demoA = { x + 18.0f, y + 22.0f + (float)focoLinha * 75.0f,
                      N151_PREV_W - 36.0f, 72.0f };
    GfxRect demoB = { x + 18.0f, y + 22.0f + (float)proxLinha * 75.0f,
                      N151_PREV_W - 36.0f, 72.0f };
    gfx_anel(demoA, 12.0f / demoA.h, 1.5f, 0.90f, 0.92f, 0.97f,
             0.52f * a * (1.0f - focoMix));
    gfx_anel(demoB, 12.0f / demoB.h, 1.5f, 0.90f, 0.92f, 0.97f,
             0.52f * a * focoMix); }
  { float sy = y + 334.0f;
    GfxRect r = { x + 18.0f, sy, N151_PREV_W - 36.0f, 176.0f };
    TxtLinha label = txt_linha(TXT_CAPTION2, i18n("Legendas"),
                               170, 176, 188, 255);
    TxtLinha nome = txt_linha(TXT_CAPTION2, TXT_FAMILIAS_PT[familiaLeg],
                              170, 176, 188, 255);
    caixa(r, 14.0f, a);
    txt_desenhar_alpha(label, r.x + 18.0f, r.y + 14.0f, a);
    txt_desenhar_alpha(nome, r.x + r.w - (float)nome.w - 18.0f,
                       r.y + 14.0f, a);
    { TxtLinha amostra = txt_linha_familia(TXT_LEG_100,
                          i18n("Aa  Nuvio  0123"), 242, 242, 244, 255, familiaLeg);
      txt_desenhar_alpha(amostra, r.x + 18.0f, r.y + 82.0f, a); }
  }
}

static void miniPoster(float x, float y, float w, float h, int foco,
                       float ar, float ag, float ab, float a) {
  GfxRect r = { x, y, w, h };
  gfx_cor(r, 10.0f / h, 0.22f + foco * 0.05f, 0.24f + foco * 0.05f,
          0.30f + foco * 0.06f, a);
  gfx_cor((GfxRect){x + 12.0f, y + h * 0.62f, w * 0.68f, 4.0f},
          0.5f, 0.77f, 0.79f, 0.84f, 0.75f * a);
  gfx_cor((GfxRect){x + 12.0f, y + h * 0.74f, w * 0.42f, 3.0f},
          0.5f, 0.55f, 0.58f, 0.63f, 0.54f * a);
  if (foco) gfx_anel(r, 10.0f / h, 3.0f, ar, ag, ab, a);
}

static void desenhoOpcoes(float x, float y, float a) {
  float ar, ag, ab;
  float focoCategoria = fmodf(relogioCena / 1.8f, 3.0f);
  int catA = (int)focoCategoria, catB = (catA + 1) % 3;
  float catMix = anim_suave(focoCategoria - (float)catA);
  float focoCartao = fmodf(relogioCena / 1.25f, 3.0f);
  int cartaoA = (int)focoCartao, cartaoB = (cartaoA + 1) % 3;
  float cartaoMix = anim_suave(focoCartao - (float)cartaoA);
  GfxRect tela = { x + 18.0f, y + 18.0f, N151_PREV_W - 36.0f, N151_PREV_H - 36.0f };
  ajustes_acento(&ar, &ag, &ab);
  caixa(tela, 16.0f, a);
  // Três categorias: a linha de foco liga o nome ao efeito da opção.
  { static const char *const C[] = { "Aparência", "Layout", "Reprodução" };
    int i;
    for (i = 0; i < 3; i++) {
      float yy = tela.y + 28.0f + (float)i * 74.0f;
      GfxRect r = { tela.x + 18.0f, yy, 220.0f, 60.0f };
      caixa(r, 12.0f, a);
      if (i == catA) contornoAcao(r, 12.0f, a * (1.0f - catMix));
      if (i == catB) contornoAcao(r, 12.0f, a * catMix);
      { TxtLinha l = txt_linha(TXT_CAPTION2, i18n(C[i]),
                               222, 225, 231, 255);
        txt_desenhar_alpha(l, r.x + 14.0f,
                           r.y + (r.h - (float)l.h) * 0.5f, a); }
    } }
  // O item destacado se converte num exemplo concreto de cartões na Home.
  { GfxRect detalhe = { tela.x + 264.0f, tela.y + 28.0f, tela.w - 292.0f, 254.0f };
    caixa(detalhe, 14.0f, a);
    gfx_cor((GfxRect){detalhe.x + 18.0f, detalhe.y + 18.0f,
                      detalhe.w - 36.0f, 76.0f}, 10.0f / 76.0f,
            0.18f, 0.20f, 0.25f, a);
    gfx_cor((GfxRect){detalhe.x + 36.0f, detalhe.y + 42.0f, detalhe.w * 0.44f, 5.0f},
            0.5f, 0.76f, 0.78f, 0.83f, 0.85f * a);
    { int i; float pw = (detalhe.w - 54.0f) / 3.0f;
      for (i = 0; i < 3; i++) {
        float foco = (i == cartaoA ? 1.0f - cartaoMix : 0.0f) +
                     (i == cartaoB ? cartaoMix : 0.0f);
        miniPoster(detalhe.x + 18.0f + (float)i * (pw + 9.0f),
                   detalhe.y + 118.0f, pw, 116.0f, foco > 0.5f, ar, ag, ab, a);
        if (foco > 0.0f && foco <= 0.5f) {
          GfxRect r = { detalhe.x + 18.0f + (float)i * (pw + 9.0f),
                        detalhe.y + 118.0f, pw, 116.0f };
          gfx_anel(r, 10.0f / 116.0f, 3.0f, ar, ag, ab, a * foco);
        }
      }
  }
  }
  // Sugere tambem o player, sem desenhar valores de conta, métricas ou rede.
  { GfxRect player = { tela.x + 18.0f, tela.y + 330.0f, tela.w - 36.0f, 122.0f };
    caixa(player, 14.0f, a);
    gfx_cor((GfxRect){player.x + 20.0f, player.y + 20.0f, 128.0f, 82.0f},
            10.0f / 82.0f, 0.19f, 0.21f, 0.26f, a);
    gfx_cor((GfxRect){player.x + 175.0f, player.y + 32.0f, player.w - 205.0f, 5.0f},
            0.5f, 0.63f, 0.65f, 0.70f, a);
    gfx_cor((GfxRect){player.x + 175.0f, player.y + 58.0f, player.w * 0.52f, 4.0f},
            0.5f, ar, ag, ab, 0.88f * a);
    gfx_anel((GfxRect){player.x + 162.0f, player.y + 80.0f,
                       player.w - 186.0f, 24.0f}, 7.0f / 24.0f,
             2.0f, ar, ag, ab, a);
  }
}

static void desenhoClareza(float x, float y, float a) {
  float ar, ag, ab;
  GfxRect pagina = { x + 18.0f, y + 18.0f, 390.0f, N151_PREV_H - 36.0f };
  GfxRect ajuda = { x + 426.0f, y + 18.0f, N151_PREV_W - 444.0f, N151_PREV_H - 36.0f };
  ajustes_acento(&ar, &ag, &ab);
  caixa(pagina, 16.0f, a);
  caixa(ajuda, 16.0f, a);
  { TxtLinha t = txt_linha(TXT_CAPTION2, i18n("Título e sinopse"),
                           202, 206, 215, 255);
    txt_desenhar_alpha(t, pagina.x + 22.0f, pagina.y + 20.0f, a); }
  miniPoster(pagina.x + 22.0f, pagina.y + 68.0f, 136.0f, 204.0f,
             0, ar, ag, ab, a);
  { TxtLinha tit = txt_linha_corta(TXT_CALLOUT, i18n("Título · T1E3"),
                                    232, 235, 241, 255, pagina.w - 182.0f);
    txt_desenhar_alpha(tit, pagina.x + 174.0f, pagina.y + 86.0f, a);
    { TxtLinha ep = txt_linha(TXT_CAPTION2, i18n("Episódio 3"),
                              160, 166, 178, 255);
      txt_desenhar_alpha(ep, pagina.x + 174.0f, pagina.y + 132.0f, a); } }
  { const char *sinopse = i18n("Uma pista inesperada aproxima a equipe da verdade por trás do mistério e muda o rumo de toda a temporada.");
    txt_bloco_corta(TXT_DET_SIN, sinopse, 211, 215, 223, pagina.x + 22.0f,
                    pagina.y + 302.0f, pagina.w - 44.0f, 35.0f, a, 3); }
  { txt_bloco(TXT_CAPTION2, i18n("Prévia por categoria e opção"),
              202, 206, 215, ajuda.x + 20.0f, ajuda.y + 18.0f,
              ajuda.w - 40.0f, 25.0f, a, 2); }
  { GfxRect foco = { ajuda.x + 18.0f, ajuda.y + 82.0f, ajuda.w - 36.0f, 86.0f };
    caixa(foco, 12.0f, a);
    gfx_cor((GfxRect){foco.x + 16.0f, foco.y + 20.0f, foco.w * 0.60f, 5.0f},
            0.5f, 0.71f, 0.74f, 0.80f, 0.95f * a);
    contornoAcao(foco, 12.0f, a); }
  { int i;
    for (i = 0; i < 3; i++) {
      float yy = ajuda.y + 196.0f + (float)i * 82.0f;
      GfxRect r = { ajuda.x + 18.0f, yy, ajuda.w - 36.0f, 64.0f };
      caixa(r, 11.0f, a * (0.85f - 0.12f * i));
      gfx_cor((GfxRect){r.x + 16.0f, r.y + 21.0f,
                        r.w * (0.72f - 0.12f * i), 4.0f}, 0.5f,
              0.52f, 0.55f, 0.61f, a * (0.8f - 0.1f * i));
    } }
}

static void desenhoCena(int c, float x, float y, float a) {
  GfxRect fundo = { x, y, N151_PREV_W, N151_PREV_H };
  gfx_cor(fundo, 18.0f / fundo.h, 0.052f, 0.058f, 0.072f, a);
  if (c == 0) desenhoFontes(x, y, a);
  else if (c == 1) desenhoOpcoes(x, y, a);
  else desenhoClareza(x, y, a);
}

void novidades151_desenhar(Uint32 agora) {
  float a = anim_suave(entrada), dy, top;
  float ar, ag, ab;
  const char *btnAgora = i18n("Agora não");
  const char *btnExplorar = i18n("Explorar Ajustes");
  (void)agora;
  if (entrada < 0.002f) return;
  if (aberto) ponteiro_camada();
  ajustes_acento(&ar, &ag, &ab);
  gfx_cor((GfxRect){0, 0, NV_TELA_W, NV_TELA_H}, 0.0f, 0, 0, 0, 0.76f * entrada);
  dy = (1.0f - a) * 28.0f;
  top = N151_Y + dy;
  { GfxRect cartao = {N151_X, top, N151_W, N151_H};
    gfx_cor(cartao, N151_RAIO / cartao.h, 0.050f, 0.054f, 0.064f, 0.97f * a);
    gfx_luz_canto(cartao, N151_RAIO / cartao.h, 160.0f, -70.0f,
                  620.0f, ar, ag, ab, 0.16f * a);
    gfx_recorte(cartao.x, cartao.y, cartao.w, cartao.h); }

  { TxtLinha tag = txt_linha(TXT_CAPTION2, i18n("NOVO NO NUVIO"),
                             150, 155, 169, 255);
    TxtLinha tit = txt_linha(TXT_TITULO2, i18n("Novidades da 1.5.1"),
                             246, 247, 250, 255);
    txt_desenhar_alpha(tag, N151_X + N151_PAD, top + 42.0f, a * 0.9f);
    txt_desenhar_alpha(tit, N151_X + N151_PAD, top + 72.0f, a); }

  // As cenas cruzam suavemente; com animacoes reduzidas o estado e estatico.
  { float px = N151_X + N151_PAD, py = top + 158.0f;
    float t = anim_suave(transicao);
    if (transicao < 1.0f && cenaAntiga != cena)
      desenhoCena(cenaAntiga, px - (1.0f - t) * 24.0f, py,
                  a * (1.0f - t));
    desenhoCena(cena, px + (1.0f - t) * 24.0f, py, a * t);
  }

  { float tx = N151_TXT_X, ty = top + 204.0f, alturaTitulo;
    float ta = a * anim_suave(transicao);
    alturaTitulo = txt_bloco(TXT_TITULO3, i18n(TEXTOS[cena].titulo),
                             244, 246, 250, tx, ty, N151_TXT_W, 58.0f, ta, 3);
    txt_bloco(TXT_BODY, i18n(TEXTOS[cena].descricao),
              185, 190, 201, tx, ty + alturaTitulo + 20.0f,
              N151_TXT_W, 34.0f, ta, 4);
    { TxtLinha hint = txt_linha(TXT_CAPTION2,
            i18n("← → Prévia     ↑ ↓ Ações     OK Confirmar"),
            145, 151, 162, 255);
      txt_desenhar_alpha(hint, tx, top + 552.0f, a * 0.78f); }
  }

  // Paginacao e controles vivem abaixo da miniatura, sem competir com o CTA.
  { float cx = N151_X + N151_PAD + N151_PREV_W * 0.5f;
    float cy = top + 158.0f + N151_PREV_H + 38.0f;
    int i;
    for (i = 0; i < N151_CENAS; i++) {
      float d = i == cena ? 14.0f : 9.0f;
      float dx = cx + (float)(i - 1) * 28.0f;
      GfxRect dot = {dx - 20.0f, cy - 20.0f, 40.0f, 40.0f};
      gfx_cor((GfxRect){dx - d * 0.5f, cy - d * 0.5f, d, d}, 0.5f,
              i == cena ? ar : 0.48f, i == cena ? ag : 0.50f,
              i == cena ? ab : 0.55f, a * (i == cena ? 0.98f : 0.68f));
      if (aberto) ponteiro_alvo(dot.x, dot.y, dot.w, dot.h, NULL,
                               ponteiroCena, i, 0);
    }
  }

  // A escolha inicial favorece explorar. Cima/baixo alterna entre os dois
  // botoes; as setas horizontais ficam livres para navegar nas cenas.
  { float wd = botao_largura(btnAgora, NULL, 0);
    float we = botao_largura(btnExplorar, NULL, 1);
    float stackW = fmaxf(wd, we);
    float bx = N151_TXT_X + (N151_TXT_W - stackW) * 0.5f;
    float by = top + 610.0f;
    GfxRect bE = {bx + (stackW - we) * 0.5f, by, we, BOTAO_H_PRIMARIO};
    GfxRect bD = {bx + (stackW - wd) * 0.5f,
                  by + BOTAO_H_PRIMARIO + 14.0f, wd, BOTAO_H_SECUNDARIO};
    botao_pilula(bD, btnAgora, NULL, focoBotao == N151_AGORA ? 1.0f : 0.0f,
                 0, 0, a);
    botao_pilula(bE, btnExplorar, NULL, focoBotao == N151_EXPLORAR ? 1.0f : 0.0f,
                 1, 0, a);
    if (aberto) {
      ponteiro_alvo(bD.x, bD.y, bD.w, bD.h, ponteiroBotao, NULL, N151_AGORA, 0);
      ponteiro_alvo(bE.x, bE.y, bE.w, bE.h, ponteiroBotao, NULL, N151_EXPLORAR, 0);
    }
  }
  gfx_sem_recorte();
}
