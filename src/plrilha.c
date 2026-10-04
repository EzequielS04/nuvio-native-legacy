// A ilha do relogio dentro do player — ver plrilha.h.
//
// CUSTO, a mesma regra da ilha.c: uma sombra do tamanho da ilha, o miolo, a
// luz de canto, o texto do cabecalho e o corpo de quem pediu. Nada de tela
// cheia (o veu de cima do aviso sozinho e o unico, e so sem o OSD).
#include "plrilha.h"
#include "plrui.h"
#include "ajustes.h"
#include "anim.h"
#include "idioma.h"
#include "layout.h"
#include "text.h"
#include "relogiofim.h"
#include "desempenho.h"
#define NV_ESCALA_TELA   // o arquivo inteiro mede pela tela virtual (escala.h)
#include "escala.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// A mola da ilha (ilha.c, ILHA_MOLA_W/Z; o Spotlight usa a mesma, SP_MOLA).
#define MOLA_W 10.0f
#define MOLA_Z 0.72f
#define X_ESQ   96.0f      // PLR_MARGEM: a coluna do OSD
#define Y_TOPO  48.0f      // PLR_PAD_Y
#define CAB_H   64.0f      // cabecalho do corpo = a pilula aberta
#define PIL_H   56.0f
#define RAIO_CORPO 36.0f
// A frase do aviso no player: mais larga que a da ilha de fora (760), porque
// os avisos de fonte do player sao frases inteiras e nao ha nada ao lado.
#define PLR_TEXTO_MAX 1300.0f

static PlrIlhaPedido ped, ult;      // o deste quadro e o ultimo com corpo
static int temPed, temUlt, escondida;
static char pedTexto[200], pedDir[80], pedIcone[32];
static char ultTexto[200], ultDir[80], ultIcone[32];
static float relA;                  // opacidade do OSD neste quadro
static double falta = -1.0;
static float W, vW, H, vH, A, corpoA, textoA = 1.0f;
static GfxRect ultRect;
static int ultOk;
static Uint32 ultQuadro;
static int dir;
// MEDIDOR DE DESEMPENHO (desempenho.h): so na pilula da HORA sozinha (sem
// pedido nem corpo saindo). dsP = a forma deste quadro; dsA = o corpo dele.
static int dsP;
static float dsA;
#ifdef NV_SHOT_HOOKS
static time_t horaFixa;
void plrilha_shot_hora(time_t t) { horaFixa = t; }
#endif

static float mola(float *v, float x, float alvo, float dt) {
  int k;
  if (anim_politica_reduzida || ajustes_animacoes_reduzidas()) { *v = 0.0f; return alvo; }
  if (dt > 0.05f) dt = 0.05f;
  for (k = 0; k < 4; k++) {
    float h = dt * 0.25f, ac = MOLA_W * MOLA_W * (alvo - x) - 2.0f * MOLA_Z * MOLA_W * (*v);
    *v += ac * h;
    x += *v * h;
  }
  return x;
}

static void copiar(PlrIlhaPedido *d, const PlrIlhaPedido *s, char *t, char *dr, char *ic) {
  *d = *s;
  snprintf(t, 200, "%s", s->texto ? s->texto : "");
  snprintf(dr, 80, "%s", s->direita ? s->direita : "");
  snprintf(ic, 32, "%s", s->icone ? s->icone : "");
  d->texto = t[0] ? t : NULL;
  d->direita = dr[0] ? dr : NULL;
  d->icone = ic[0] ? ic : NULL;
}

// CARTAO CENTRAL (dono, 03/10: "ao abrir o filme pode deixar o componente de
// opening source no meio da tela, centralizado, mesmo sem o relogio"): o
// pedido com `centro` nao passa pela ilha do canto. Vira um cartao no mesmo
// material, so com o corpo, no centro, que entra e sai por alfa.
static PlrIlhaPedido cen, cenUlt;
static char cenT[200], cenD[80], cenI[32], cenUT[200], cenUD[80], cenUI[32];
static int temCen;
static float cenA;
static Uint32 cenQuadro;

void plrilha_pedir(const PlrIlhaPedido *p) {
  if (p->centro && p->w > 0.0f && p->h > 0.0f) {
    copiar(&cen, p, cenT, cenD, cenI);
    temCen = 1;
    return;
  }
  // BAIXA PRIORIDADE (guia parental): nunca derruba um pedido que ja esta no
  // quadro, e qualquer outro pedido a derruba, venha antes ou depois dela.
  if (p->baixa && temPed) return;
  if (temPed && ped.baixa) temPed = 0;
  // Quem tem corpo vence quem e so aviso: a lista aberta nao vira toast.
  if (temPed && ped.w > 0.0f && p->w <= 0.0f) return;
  copiar(&ped, p, pedTexto, pedDir, pedIcone);
  temPed = 1;
}

void plrilha_relogio(float a, double f) {
  if (a > relA) relA = a;
  falta = f;
}

void plrilha_esconder(void) { escondida = 1; }

int plrilha_direita(void) {
  return 1;   // sempre a direita (dono, 03/10)
}

int plrilha_rect(GfxRect *r) { if (ultOk && r) *r = ultRect; return ultOk; }
float plrilha_corpo_alfa(void) { return temUlt ? corpoA : 0.0f; }

// --- a linha da pilula ------------------------------------------------------------
typedef struct {
  TxtLinha txt, hora, fim, dir;
  float w;                 // largura do conteudo (sem o recuo)
  float ds;                // o trecho do medidor no fim da linha (0 = nada)
  int temIcone, temTxt, temFim;
} Linha;

static void horaAgora(char *h, size_t n, char *fim, size_t nf) {
  time_t t = time(NULL);
  struct tm lt;
#ifdef NV_SHOT_HOOKS
  if (horaFixa) t = horaFixa;
#endif
  localtime_r(&t, &lt);
  strftime(h, n, "%H:%M", &lt);
  fim[0] = 0;
  if (falta >= 0.0) relogio_fim_ilha(fim, nf, t, falta);
}

static float montar(const PlrIlhaPedido *p, Linha *L) {
  char h[16], fim[RELOGIO_FIM_MAX];
  float w = 0.0f;
  memset(L, 0, sizeof *L);
  horaAgora(h, sizeof h, fim, sizeof fim);
  L->temIcone = p && p->icone;
  L->temTxt = p && p->texto;
  L->temFim = (!p || !p->semFim) && fim[0];
  L->hora = txt_linha(TXT_ILHA_NOME, h, 243, 242, 239, 255);
  if (L->temIcone) w += 24.0f + 12.0f;
  if (p && p->respira) w += 10.0f + 14.0f;
  if (L->temTxt) {
    L->txt = txt_linha_corta(TXT_ILHA_NOME, p->texto, 243, 242, 239, 255, PLR_TEXTO_MAX);
    w += (float)L->txt.w + 12.0f;
  }
  if (p && p->pontos > 0) w += 4.0f + p->pontos * 8.0f + (p->pontos - 1) * 7.0f + 12.0f;
  if (L->temTxt || (p && p->pontos > 0)) w += 1.0f + 12.0f;   // separador
  w += (float)L->hora.w;
  if (L->temFim) {
    L->fim = txt_linha(TXT_G19M, fim, 243, 242, 239, 153);
    w += 12.0f + 1.0f + 12.0f + (float)L->fim.w;
  }
  if (p && p->direita) L->dir = txt_linha(TXT_G19M, p->direita, 243, 242, 239, 140);
  L->ds = dsP ? desempenho_linha_w(dsP) : 0.0f;
  w += L->ds;
  L->w = w;
  return w;
}

static void desenharLinha(const PlrIlhaPedido *p, const Linha *L, float x, float yc, float a) {
  float cr = 1, cg = 1, cb = 1;
  if (a < 0.01f) return;
  if (L->temIcone) {
    if (p->corIcone == 1) { cr = 0.941f; cg = 0.725f; cb = 0.290f; }
    else if (p->corIcone == 2) ajustes_acento(&cr, &cg, &cb);
    gfx_icone((GfxRect){ x, yc - 12.0f, 24.0f, 24.0f }, p->icone, cr, cg, cb, a);
    x += 24.0f + 12.0f;
  }
  if (p && p->respira) {
    plrui_respira(x + 5.0f, yc, 10.0f, SDL_GetTicks(), a);
    x += 10.0f + 14.0f;
  }
  if (L->temTxt) {
    txt_desenhar_alpha(L->txt, x, yc - (float)L->txt.h * 0.5f, a);
    x += (float)L->txt.w + 12.0f;
  }
  if (p && p->pontos > 0) {
    float ar, ag, ab;
    int i;
    ajustes_acento(&ar, &ag, &ab);
    x += 4.0f;
    for (i = 0; i < p->pontos; i++) {
      GfxRect d = { x, yc - 4.0f, 8.0f, 8.0f };
      if (i == p->ponto) gfx_cor(d, 0.5f, ar, ag, ab, a);
      else gfx_cor(d, 0.5f, 1, 1, 1, 0.25f * a);
      x += 8.0f + 7.0f;
    }
    x += 12.0f - 7.0f;
  }
  if (L->temTxt || (p && p->pontos > 0)) { plrui_sep(x, yc, a); x += 1.0f + 12.0f; }
  txt_desenhar_alpha(L->hora, x, yc - (float)L->hora.h * 0.5f, a);
  x += (float)L->hora.w;
  if (L->temFim) {
    x += 12.0f;
    plrui_sep(x, yc, a);
    x += 1.0f + 12.0f;
    txt_desenhar_alpha(L->fim, x, yc - (float)L->fim.h * 0.5f + 1.0f, a);
    x += (float)L->fim.w;
  }
  if (L->ds > 0.0f) desempenho_linha(dsP, x, yc, a);
}

static void plrilha_desenharCorpo_(Uint32 agora);
// Camada ampliada (escala.h): o corpo desenha na tela virtual.
static void cartaoCentro(Uint32 agora) {
  float dt = cenQuadro ? (float)(agora - cenQuadro) / 1000.0f : 1.0f / 60.0f;
  cenQuadro = agora;
  if (dt > 0.1f) dt = 0.1f;
  if (temCen) copiar(&cenUlt, &cen, cenUT, cenUD, cenUI);
  if (anim_politica_reduzida || ajustes_animacoes_reduzidas()) cenA = temCen ? 1.0f : 0.0f;
  else cenA = anim_mola(cenA, temCen ? 1.0f : 0.0f, dt, temCen ? 10.0f : 12.0f);
  temCen = 0;
  if (cenA < 0.01f) { cenA = 0.0f; return; }
  { const PlrIlhaPedido *c = &cenUlt;
    GfxRect R = { (NV_TELA_W - c->w) * 0.5f, (NV_TELA_H - c->h) * 0.5f, c->w, c->h };
    plrui_material(R, RAIO_CORPO, c->modal, cenA);
    gfx_recorte(R.x, R.y, R.w, R.h);
    if (c->corpo) c->corpo(R, cenA, c->u);
    gfx_sem_recorte(); }
}

void plrilha_desenhar(Uint32 agora) {
  ESCALA_INI();
  plrilha_desenharCorpo_(agora);
  cartaoCentro(agora);
  ESCALA_FIM();
}
static void plrilha_desenharCorpo_(Uint32 agora) {
  float dt = ultQuadro ? (float)(agora - ultQuadro) / 1000.0f : 1.0f / 60.0f;
  const PlrIlhaPedido *p;
  PlrIlhaPedido vazio;
  Linha L;
  float alvoW, alvoH, pilH, pad, x, conteudoW, cresce;
  int corpo, quer;
  ultQuadro = agora;
  if (dt > 0.1f) dt = 0.1f;
  memset(&vazio, 0, sizeof vazio);
  dir = plrilha_direita();

  if (temPed && ped.w > 0.0f) { copiar(&ult, &ped, ultTexto, ultDir, ultIcone); temUlt = 1; }
  p = temPed ? &ped : &vazio;
  corpo = temPed && ped.w > 0.0f;
  quer = !escondida && (temPed || relA > 0.01f);

  // O que a linha da pilula mostra: o pedido, ou (encolhendo depois de um
  // corpo) a linha dele ate o corpo apagar — a hora volta com a forma.
  if (!temPed && temUlt && corpoA > 0.02f) p = &ult;
  // O medidor entra so na hora sozinha: aviso, lista ou carregamento na ilha
  // o tiram (e ele volta quando ela fica so com a hora).
  dsP = p == &vazio && quer ? desempenho_forma() : DS_DESLIGADO;
  pilH = p->aberta || corpo ? CAB_H : PIL_H;
  pad = pilH > PIL_H + 0.5f ? 28.0f : 24.0f;
  conteudoW = montar(p, &L);
  alvoW = corpo ? ped.w : conteudoW + pad * 2.0f;
  alvoH = corpo ? CAB_H + ped.h : pilH;
  { float bw = 0.0f, bh = 0.0f;
    desempenho_corpo_tam(dsP, &bw, &bh);
    if (bh > 0.0f) { if (bw > alvoW) alvoW = bw; alvoH = CAB_H + bh; }
    dsA = (anim_politica_reduzida || ajustes_animacoes_reduzidas()) ? (bh > 0.0f ? 1.0f : 0.0f)
        : anim_mola(dsA, bh > 0.0f && fabsf(H - alvoH) < 0.2f * alvoH ? 1.0f : 0.0f, dt, 12.0f); }
  if (!quer) { alvoW = PIL_H * 0.6f; alvoH = PIL_H * 0.6f; }
  if (W <= 0.0f) { W = alvoW; H = alvoH; }
  W = mola(&vW, W, alvoW, dt);
  H = mola(&vH, H, alvoH, dt);
  A = anim_mola(A, quer ? 1.0f : 0.0f, dt, quer ? 9.0f : 12.0f);
  // Corpo: entra quando a forma chegou perto do tamanho final, sai antes de ela
  // encolher (a lista nunca e espremida dentro da pilula).
  { float alvoC = 0.0f;
    if (corpo && fabsf(H - alvoH) < 0.35f * alvoH && fabsf(W - alvoW) < 0.35f * alvoW) alvoC = 1.0f;
    corpoA = (anim_politica_reduzida || ajustes_animacoes_reduzidas())
           ? alvoC : anim_mola(corpoA, alvoC, dt, alvoC > corpoA ? 10.0f : 26.0f);
    if (!corpo && corpoA < 0.02f) { corpoA = 0.0f; temUlt = 0; } }
  textoA = anim_mola(textoA, 1.0f, dt, 12.0f);

  temPed = 0; relA = 0.0f; escondida = 0;
  if (A < 0.01f) { ultOk = 0; W = H = 0.0f; vW = vH = 0.0f; return; }

  x = dir ? NV_TELA_W - X_ESQ : X_ESQ;
  { float w = (W < H && H <= CAB_H) ? H : W, h = H < 8.0f ? 8.0f : H;
    GfxRect R = { dir ? x - w : x, Y_TOPO, w, h };
    float raioPx = h * 0.5f;
    cresce = anim_clamp((h - pilH) / 120.0f, 0.0f, 1.0f);
    if (raioPx > RAIO_CORPO * cresce + h * 0.5f * (1.0f - cresce)) raioPx = RAIO_CORPO * cresce + h * 0.5f * (1.0f - cresce);
    if (raioPx > h * 0.5f) raioPx = h * 0.5f;
    ultRect = R; ultOk = 1;
    plrui_material(R, raioPx, (temUlt ? ult.modal : 0), A);
    gfx_recorte(R.x, R.y, R.w, R.h);
    { float cabH = h < CAB_H ? h : CAB_H;
      float yc = R.y + cabH * 0.5f;
      // A linha: centrada na pilula; com corpo, encostada a esquerda do
      // cabecalho (o recuo de 28).
      float lx = R.x + (R.w - L.w) * 0.5f;
      float ex = R.x + 28.0f;
      float k = anim_clamp((R.w - (L.w + pad * 2.0f)) / 40.0f, 0.0f, 1.0f);
      lx = lx + (ex - lx) * k;
      desenharLinha(p, &L, lx, yc, A * textoA);
      if (p->direita && k > 0.0f)
        txt_desenhar_alpha(L.dir, R.x + R.w - 28.0f - (float)L.dir.w, yc - (float)L.dir.h * 0.5f + 1.0f, A * k);
      // Fio de 1 px sob o cabecalho, so com o corpo de pe.
      if (h > CAB_H + 4.0f && dsP == DS_DESLIGADO)
        gfx_cor((GfxRect){ R.x, R.y + CAB_H, R.w, 1.0f }, 0.0f, 1, 1, 1, 0.07f * A * cresce); }
    if (temUlt && corpoA > 0.01f && ult.corpo && h > CAB_H + 4.0f) {
      GfxRect c = { R.x, R.y + CAB_H, R.w, h - CAB_H };
      ult.corpo(c, A * corpoA, ult.u);
    }
    if (dsP != DS_DESLIGADO && dsA > 0.01f && h > CAB_H + 4.0f)
      desempenho_corpo(dsP, (GfxRect){ R.x, R.y + CAB_H, R.w, h - CAB_H }, A * dsA);
    gfx_sem_recorte(); }
}
