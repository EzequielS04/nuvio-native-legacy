#include "pausao.h"
#include "catalogo.h"
#include "ajustes.h"
#include "gfx.h"
#include "text.h"
#include "anim.h"
#include "layout.h"
#include "idioma.h"
#include "relogiofim.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

// Os cinco segundos do web (playerScreen.js:567). Nao e um numero de gosto: e o
// que separa "parei um instante" de "parei para ler". Encurtar faz o painel
// pular na cara de quem so ajustou o volume.
#define PAUSAO_ESPERA_MS  5000u

// CAMADA DE TELA CHEIA. O painel foi uma faixa discreta ancorada no rodape (veu
// so do topo do texto para baixo, texto em 1160 de largura) e o dono relatou
// que "as infos que mostra com o overlay nao pegam a tela inteira". Agora o veu
// vai de ponta a ponta e o conteudo se distribui pelo 1920x1080 com as margens
// do app. O quadro continua legivel por tras: o veu geral e leve, e o que
// segura o texto sao os degrades de cima e de baixo.
#define PAUSAO_X          96.0f    // mesmo recuo do conteudo do player (PLR_MARGEM)
#define PAUSAO_Y          64.0f    // topo do selo e do relogio
#define PAUSAO_BASE      124.0f    // margem inferior da ficha (acima da barra)
#define PAUSAO_LARG     1480.0f    // largura util do texto da ficha
// Passo entre linhas da sinopse. E PASSO, nao vao: txt_bloco desenha a linha i
// em y + i*leading. O mesmo numero que a pagina de titulo usa neste estilo.
#define PAUSAO_LD_SIN     40.0f
#define PAUSAO_SIN_LINHAS     3
// Veu: geral, mais o degrade de cima (selo/relogio) e o de baixo (a ficha).
#define PAUSAO_VEU_GERAL   0.34f
#define PAUSAO_VEU_TOPO    0.62f
#define PAUSAO_VEU_BAIXO   0.86f
#define PAUSAO_TOPO_H     280.0f
#define PAUSAO_BAIXO_H    640.0f
#define PAUSAO_CHIP_H     44.0f
#define PAUSAO_CHIP_PAD   18.0f
#define PAUSAO_CHIP_GAP   10.0f
// A BARRA (29/09/2026): era um fio de 6 px colado na borda de baixo, de ponta a
// ponta. Na borda ele cai na area de overscan de parte das TVs e some; e
// sozinho la embaixo nao conversava com nada. Agora e o trilho de 4 px do heroi
// do guia, na margem do conteudo, com o tempo na ponta direita da mesma linha.
#define PAUSAO_TRILHO_H    4.0f
#define PAUSAO_TRILHO_Y   (NV_TELA_H - 64.0f)   // centro da linha barra + tempo

// Quantos nomes de elenco cabem. O web para em oito (:568); aqui o teto e o do
// dado, nao o do layout: CatItem guarda seis.
#define PAUSAO_ELENCO_MAX 6

static int    visivel;
static float  anim;            // 0..1, a entrada por mola
static Uint32 desdeQuando;     // quando a condicao passou a valer; 0 = nao vale
static int    idxItem = -1;
static char   idItem[64];      // o titulo de idxItem (#190; ver pausao.h)
static char   epLinha[220];

void pausao_fechar(void) {
  visivel = 0;
  anim = 0.0f;
  desdeQuando = 0;
  idxItem = -1;
  idItem[0] = 0;
  epLinha[0] = 0;
}

int pausao_indice(void) { return cat_indice_vivo(idxItem, idItem); }

void pausao_atualizar(float dt, Uint32 agora, int podeSubir, int idx,
                      const char *imdb, const char *linhaEp) {
  idxItem = idx;
  snprintf(idItem, sizeof idItem, "%s", imdb ? imdb : "");
  snprintf(epLinha, sizeof epLinha, "%s", linhaEp ? linhaEp : "");

  // O ajuste e consultado AQUI e nao na abertura: desligar a opcao com o painel
  // de pe tem de derrubar o painel, e nao valer so no filme seguinte.
  if (!podeSubir || !ajustes_pausa_overlay()) {
    // schedulePauseOverlay/syncPauseOverlayState (:7397): condicao que cai
    // derruba o painel e ZERA o relogio. Rearmar de onde parou faria uma
    // sequencia de pausas curtas somar cinco segundos e o painel subir sozinho
    // no meio de uma cena.
    visivel = 0;
    desdeQuando = 0;
  } else {
    if (!desdeQuando) desdeQuando = agora;
    if (!visivel && agora - desdeQuando >= PAUSAO_ESPERA_MS) visivel = 1;
  }

  anim = anim_mola(anim, visivel ? 1.0f : 0.0f, dt,
                   visivel ? NV_MOLA_FOCO : NV_MOLA_DESFOCO);
  if (!visivel && anim < 0.004f) anim = 0.0f;
}

// Enquanto o painel ainda esta saindo ele continua desenhado, mas ja NAO e
// visivel para quem pergunta: se fosse, o player manteria os controles
// recolhidos durante a saida e a barra so voltaria depois do fade.
int pausao_visivel(void) { return visivel; }

int pausao_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!visivel || !e || e->type != SDL_KEYDOWN) return PAUSAO_LIVRE;
  k = e->key.keysym.sym;

  // O Back NAO e tratado aqui, e essa e uma divergencia deliberada do web. La
  // (:22138) o Back derruba o painel e ainda segue para a regra seguinte;
  // aqui o Back e a unica saida da reproducao, e roubar o primeiro toque para
  // fechar um painel informativo faria a pessoa apertar duas vezes para sair de
  // um filme. Quem quer sair, sai.
  if (k == SDLK_ESCAPE || k == SDLK_AC_BACK || k == SDLK_BACKSPACE ||
      k == SDLK_DELETE) return PAUSAO_LIVRE;

  visivel = 0;
  desdeQuando = 0;

  // playerScreen.js:22212 — OK/Play com o painel de pe derruba o painel E
  // retoma. E o gesto obvio: quem esta olhando a ficha e aperta o centro quer
  // voltar ao filme, nao so fechar uma caixa.
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE)
    return PAUSAO_RETOMAR;

  // Qualquer outra tecla (:22218): derruba o painel e devolve os controles. O
  // relogio dos 5s recomeca sozinho no proximo `pausao_atualizar`, porque
  // `desdeQuando` foi zerado — que e o `schedulePauseOverlay()` do web.
  return PAUSAO_CONSUMIU;
}

// O SELO "Pausado" (pilula de 56 px com o icone). Publico desde 29/09/2026: o
// OSD do canal ao vivo pausado usa este mesmo selo, e nao um parecido.
float pausao_selo(float x, float y, int direita, float a) {
  TxtLinha lp = txt_linha(TXT_PLR_CORPO, "Pausado", 246, 247, 250, 255);
  float d = PAUSAO_SELO_H, pw = d + 18.0f + (float)lp.w + 26.0f;
  GfxRect pil, ic;
  if (direita) x -= pw;
  pil = (GfxRect){ x, y, pw, d };
  ic  = (GfxRect){ x + 8.0f, y + 8.0f, d - 16.0f, d - 16.0f };
  if (ajustes_vidro()) gfx_vidro_painel(pil, 0.5f, 0.55f, a);
  else                 gfx_cor(pil, 0.5f, 1, 1, 1, 0.16f * a);
  gfx_icone(ic, "pause", 0.96f, 0.96f, 0.96f, 0.94f * a);
  txt_desenhar_alpha(lp, x + d + 6.0f, y + (d - (float)lp.h) * 0.5f, a);
  return pw;
}

static void fmtT(char *b, size_t n, float seg) {
  int t = (int)(seg < 0 ? 0 : seg + 0.5f);
  if (t >= 3600) snprintf(b, n, "%d:%02d:%02d", t / 3600, (t / 60) % 60, t % 60);
  else           snprintf(b, n, "%d:%02d", t / 60, t % 60);
}

void pausao_desenhar(Uint32 agora, const PausaoCena *cena) {
  const CatItem *c;
  float a = anim, y, sobe, alt = 0.0f, hSin = 0.0f, larg = PAUSAO_LARG;
  char meta[192];
  const char *sepEp = NULL;
  TxtLinha lKick, lTit, lMeta, lEp, lCast;
  int temMeta = 0, temEp = 0, temCast, i, vidro;
  (void)agora;

  if (a <= 0.004f) return;
  // PELO TITULO, e nao so pelo indice (#190): uma troca de bloco entre o
  // pausao_atualizar e este desenho poe outro titulo na mesma posicao.
  { int i = cat_indice_vivo(idxItem, idItem);
    if (i < 0) return;
    idxItem = i; }
  c = cat_item(idxItem);
  if (!c) return;
  vidro = ajustes_vidro();

  // --- VEU DE PONTA A PONTA ---------------------------------------------------
  // Tres camadas, todas do tamanho da tela em unidades de layout (NV_TELA_W x
  // NV_TELA_H; o gfx mapeia para o drawable, entao 4K cobre igual): um escurecido
  // geral leve, o degrade do topo (para o selo e o relogio terem contra o que
  // se apoiar) e o do rodape (a ficha).
  { GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
    GfxRect topo = { 0, 0, NV_TELA_W, PAUSAO_TOPO_H };
    GfxRect base = { 0, NV_TELA_H - PAUSAO_BAIXO_H, NV_TELA_W, PAUSAO_BAIXO_H };
    gfx_cor(tela, 0.0f, 0, 0, 0, PAUSAO_VEU_GERAL * a);
    gfx_rect(topo, 0, GFX_VEU_TOPO, 0, 0, 0, 0.0f, 0, 0, 0, PAUSAO_VEU_TOPO * a);
    gfx_rect(base, 0, GFX_VEU_BAIXO, 0, 0, 0, 0.0f, 0, 0, 0, PAUSAO_VEU_BAIXO * a); }

  // --- ALTO: selo "Pausado" a esquerda, relogio (e fim) a direita ----------------
  pausao_selo(PAUSAO_X, PAUSAO_Y, 0, a);
  { time_t agoraT = time(NULL);
    struct tm lt;
    char hora[8];
    TxtLinha lh;
    float yR = PAUSAO_Y;
    localtime_r(&agoraT, &lt);
    strftime(hora, sizeof hora, "%H:%M", &lt);
    lh = txt_linha(TXT_PG_RELOGIO, hora, 255, 255, 255, 255);
    txt_desenhar_alpha(lh, NV_TELA_W - PAUSAO_X - lh.w, yR, a * 0.96f);
    yR += lh.h + 2.0f;
    if (cena && cena->dur > 0.0f) {
      char fim[RELOGIO_FIM_MAX];
      TxtLinha lfim;
      relogio_fim(fim, sizeof fim, agoraT, cena->dur - cena->pos);
      lfim = txt_linha(TXT_PG_FIM, fim, 255, 255, 255, 255);
      txt_desenhar_alpha(lfim, NV_TELA_W - PAUSAO_X - lfim.w, yR, a * 0.78f);
    } }

  // --- FICHA: medida antes, ancorada na base -------------------------------------
  // A altura tem de ser conhecida para saber onde a primeira linha comeca.
  lKick = txt_linha(TXT_PLR_CORPO, "Você está assistindo", 214, 216, 222, 255);
  lTit  = txt_linha_corta(TXT_TITULO2, c->titulo, 255, 255, 255, 255, larg);
  alt = (float)lKick.h + 10.0f + (float)lTit.h + 8.0f;

  meta[0] = 0;
  if (c->meta[0]) snprintf(meta, sizeof meta, "%s", c->meta);
  if (epLinha[0]) {
    const char *sep = strstr(epLinha, " · ");
    size_t n = sep ? (size_t)(sep - epLinha) : strlen(epLinha);
    sepEp = sep;
    if (n > 0 && n < 32) {
      char cod[32];
      snprintf(cod, sizeof cod, "%.*s", (int)n, epLinha);
      if (meta[0]) {
        char junto[192];
        snprintf(junto, sizeof junto, "%s · %s", meta, cod);
        snprintf(meta, sizeof meta, "%s", junto);
      } else {
        snprintf(meta, sizeof meta, "%s", cod);
      }
    }
  }
  if (meta[0]) {
    // Meta no corpo da linha de meta do guia (25 px): em 20 px, a um metro e
    // meio da tela, "2h 07min" nao se lia.
    lMeta = txt_linha_corta(TXT_DET_META, meta, 214, 216, 222, 255, larg);
    temMeta = 1;
    alt += (float)lMeta.h + 6.0f;
  }
  if (sepEp && sepEp[3]) {
    lEp = txt_linha_corta(TXT_PLR_CORPO, sepEp + 3, 240, 241, 246, 255, larg);
    temEp = 1;
    alt += (float)lEp.h + 14.0f;
  }
  // x = -1 mede sem desenhar. E o mesmo recurso que detail.c usa para saber a
  // altura da sinopse antes de decidir o resto da coluna.
  if (c->sinopse[0]) {
    hSin = txt_bloco_corta(TXT_DET_SIN, c->sinopse, 214, 216, 222, -1.0f, 0.0f,
                           larg, PAUSAO_LD_SIN, 0.0f, PAUSAO_SIN_LINHAS);
    alt += hSin + 20.0f;
  }
  temCast = c->nElenco > 0;
  if (temCast) {
    // Rotulo de secao no estilo do "A SEGUIR" do guia (22/500 em cinza), e nao
    // o TXT_MINI de 15 px, que e o corpo de selo de classificacao.
    lCast = txt_linha(TXT_PG_ROTULO, "Elenco", 150, 153, 162, 255);
    alt += (float)lCast.h + 8.0f + PAUSAO_CHIP_H;
  }

  // Sobe 24px entrando. E o unico movimento do painel.
  sobe = (1.0f - a) * 24.0f;
  y = NV_TELA_H - PAUSAO_BASE - alt + sobe;
  if (y < PAUSAO_Y + 96.0f) y = PAUSAO_Y + 96.0f;   // nunca sobe sobre o selo

  txt_desenhar_alpha(lKick, PAUSAO_X, y, a * 0.62f);
  y += lKick.h + 10.0f;
  txt_desenhar_alpha(lTit, PAUSAO_X, y, a);
  y += lTit.h + 8.0f;
  if (temMeta) { txt_desenhar_alpha(lMeta, PAUSAO_X, y, a * 0.90f); y += lMeta.h + 6.0f; }
  if (temEp)   { txt_desenhar_alpha(lEp, PAUSAO_X, y, a * 0.90f);   y += lEp.h + 14.0f; }
  if (hSin > 0.0f) {
    // PASSO ENTRE LINHAS, nao vao entre elas (o "texto embolado" da foto, quando
    // era 8). 40 e o passo da pagina de titulo para este estilo. Com reticencias
    // quando a sinopse passa das linhas: cortar no meio da frase parecia erro.
    txt_bloco_corta(TXT_DET_SIN, c->sinopse, 214, 216, 222, PAUSAO_X, y, larg,
                    PAUSAO_LD_SIN, a * 0.84f, PAUSAO_SIN_LINHAS);
    y += hSin + 20.0f;
  }

  // ELENCO. Pastilhas so com o NOME, como o .player-pause-cast-chip do web
  // (:7480). No vidro a pastilha e o proprio vidro fosco do app.
  if (temCast) {
    float x = PAUSAO_X;
    txt_desenhar_alpha(lCast, PAUSAO_X, y, a);
    y += lCast.h + 8.0f;
    for (i = 0; i < c->nElenco && i < PAUSAO_ELENCO_MAX; i++) {
      TxtLinha l = txt_linha(TXT_CAPTION, c->elenco[i].nome, 236, 237, 242, 255);
      float w = (float)l.w + PAUSAO_CHIP_PAD * 2.0f;
      GfxRect chip;
      if (x + w > NV_TELA_W - PAUSAO_X) break;   // uma fileira so
      chip.x = x; chip.y = y; chip.w = w; chip.h = PAUSAO_CHIP_H;
      // Raio e FRACAO do menor lado nesta API (ver gfx.h): 0.5 e a pilula.
      if (vidro) gfx_vidro_painel(chip, 0.5f, 0.55f, a);
      else       gfx_cor(chip, 0.5f, 1, 1, 1, 0.14f * a);
      txt_desenhar_alpha(l, x + PAUSAO_CHIP_PAD,
                         y + (PAUSAO_CHIP_H - (float)l.h) * 0.5f, a * 0.92f);
      x += w + PAUSAO_CHIP_GAP;
    }
  }

  // --- BARRA: de onde o filme parou, na margem do conteudo, com o tempo ---------
  if (cena && cena->dur > 0.0f) {
    float f = cena->pos / cena->dur, xFim;
    GfxRect trilho, feito;
    char t1[24], t2[24], tudo[52];
    TxtLinha lt;
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    fmtT(t1, sizeof t1, cena->pos);
    fmtT(t2, sizeof t2, cena->dur);
    snprintf(tudo, sizeof tudo, "%s / %s", t1, t2);
    lt = txt_linha(TXT_PLR_CORPO, tudo, 255, 255, 255, 230);
    xFim = NV_TELA_W - PAUSAO_X - (float)lt.w - 28.0f;
    trilho = (GfxRect){ PAUSAO_X, PAUSAO_TRILHO_Y - PAUSAO_TRILHO_H * 0.5f,
                        xFim - PAUSAO_X, PAUSAO_TRILHO_H };
    gfx_cor(trilho, 0.5f, 1, 1, 1, 0.18f * a);
    feito = trilho; feito.w = trilho.w * f;
    if (feito.w > 0.5f) gfx_cor(feito, 0.5f, cena->fr, cena->fg, cena->fb, a);
    txt_desenhar_alpha(lt, NV_TELA_W - PAUSAO_X - lt.w,
                       PAUSAO_TRILHO_Y - (float)lt.h * 0.5f, a * 0.88f);
  }
}
