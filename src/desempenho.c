// Ver desempenho.h.
#include "desempenho.h"
#include "ajustes.h"
#include "gfx.h"
#include "text.h"
#include "idioma.h"
#include "idiomacod.h"
#include "tex_cache.h"
#include "layout.h"
#include <stdio.h>
#include <string.h>

#define DS_N 36   // 36 amostras de 3 s: os ultimos 1 min 48 s

static float piorSerie[DS_N];
static int   nSerie;
static float uFps, uPior, uTelaMb, uRss;
static int   uJanks, uTela, uFila, uDesp, uDespTela, temAmostra;

void desempenho_amostra(float fps, float piorMs, int janks, int texTela, float texTelaMb,
                        int filaTex, int despejos, int despejosTela, float rssMb) {
  if (nSerie == DS_N) { memmove(piorSerie, piorSerie + 1, sizeof piorSerie - sizeof *piorSerie); nSerie--; }
  piorSerie[nSerie++] = piorMs;
  uFps = fps; uPior = piorMs; uJanks = janks; uTela = texTela; uTelaMb = texTelaMb;
  uFila = filaTex; uDesp = despejos; uDespTela = despejosTela; uRss = rssMb;
  temAmostra = 1;
}

#define DS_TX 243, 242, 239
#define DS_AMBAR 0.910f, 0.722f, 0.290f
static int vid(void) { return ajustes_vidro(); }
static void num(char *d, size_t t, double v, int casas) {
  char *p;
  snprintf(d, t, "%.*f", casas, v);
  if ((p = strchr(d, '.')) != NULL) *p = idioma_ponto_decimal(ajustes_idioma()) ? '.' : ',';
}
static void ilhaMat(GfxRect r, float raioPx) {
  float raio = raioPx / r.h;
  gfx_rect((GfxRect){ r.x - 20, r.y - 6, r.w + 40, r.h + 46 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
           0, 0, 0, vid() ? 0.36f : 0.45f);
  if (vid()) {
    gfx_cor(r, raio, 0.055f, 0.059f, 0.071f, 0.80f);
    gfx_luz_canto(r, raio, r.w * 0.22f, -r.h * 0.40f, r.h * 0.62f, 1, 1, 1, 0.10f);
  } else gfx_cor(r, raio, 0.082f, 0.086f, 0.102f, 1);
}
static TxtLinha t(TxtEstilo e, const char *s) { return txt_linha(e, s, DS_TX, 255); }
static float stat(const char *k, const char *v, float x, float y, float w) {
  TxtLinha lk = t(TXT_AJ_ESTADO, k), lv = t(TXT_AJ_ESTADO, v);
  gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.07f);
  txt_desenhar_alpha(lk, x, y + 12, 0.45f);
  txt_desenhar_alpha(lv, x + w - lv.w, y + 12, 0.88f);
  return 1 + 11 + 20.6f + 11;
}

static void pilula(void) {
  char a[32], b[48], c[32];
  int lento = uFps < 45.0f;
  TxtLinha la, lb, lc;
  float w, x, y = 36, h = 56;
  num(a, sizeof a, uFps, 0);
  { char q[40]; snprintf(q, sizeof q, "%s fps", a); snprintf(a, sizeof a, "%s", q); }
  { char p[16]; num(p, sizeof p, uPior, 0); snprintf(b, sizeof b, i18n("pior %s ms"), p); }
  snprintf(c, sizeof c, "%.0f MB", uRss);
  la = lento ? txt_linha(TXT_G23B, a, 232, 184, 74, 255) : t(TXT_G23B, a);
  lb = t(TXT_G22M, b); lc = t(TXT_G22M, c);
  w = 24 + 9 + 14 + la.w + 14 + 1 + 14 + lb.w + 14 + 1 + 14 + lc.w + 24;
  x = NV_TELA_W - 48 - w;
  ilhaMat((GfxRect){ x, y, w, h }, 28);
  x += 24;
  if (lento) gfx_cor((GfxRect){ x, y + 28 - 4.5f, 9, 9 }, 0.5f, DS_AMBAR, 1);
  else gfx_cor((GfxRect){ x, y + 28 - 4.5f, 9, 9 }, 0.5f, 0.298f, 0.765f, 0.541f, 1);
  x += 9 + 14;
  txt_desenhar(la, x, y + 28 - la.h * 0.5f); x += la.w + 14;
  gfx_cor((GfxRect){ x, y + 17, 1, 22 }, 0, 1, 1, 1, 0.18f); x += 1 + 14;
  txt_desenhar_alpha(lb, x, y + 28 - lb.h * 0.5f, 0.7f); x += lb.w + 14;
  gfx_cor((GfxRect){ x, y + 17, 1, 22 }, 0, 1, 1, 1, 0.18f); x += 1 + 14;
  txt_desenhar_alpha(lc, x, y + 28 - lc.h * 0.5f, 0.7f);
}

static void ilha(void) {
  float w = 520, x0 = NV_TELA_W - 48 - w, y0 = 36, x = x0 + 28, cw = w - 56, y, h;
  char v[64], p[32];
  int i, lento = uFps < 45.0f, itens, pend, quentes;
  long bytes, bytesQ, teto = tex_orcamento_bytes();
  tex_estatisticas(&itens, &pend, &bytes, &quentes, &bytesQ);
  h = 64 + 22 + 64 + 16 + 90 + 6 + 16 + 14 + 4 * 43.6f + 12 + 20 + 9 + 8 + 26;
  ilhaMat((GfxRect){ x0, y0, w, h }, 36);
  // cabecalho
  gfx_icone((GfxRect){ x, y0 + 20, 24, 24 }, "aj_activity", 0.953f, 0.949f, 0.937f, 0.85f);
  { TxtLinha l = t(TXT_ILHA_NOME, "Desempenho"); txt_desenhar(l, x + 36, y0 + 32 - l.h * 0.5f); }
  { TxtLinha l = t(TXT_G18M, "a cada 3 s"); txt_desenhar_alpha(l, x0 + w - 28 - l.w, y0 + 32 - l.h * 0.5f, 0.5f); }
  gfx_cor((GfxRect){ x0, y0 + 63, w, 1 }, 0, 1, 1, 1, 0.07f);
  y = y0 + 64 + 22;
  num(v, sizeof v, uFps, 1);
  { TxtLinha n = lento ? txt_linha(TXT_AJ_NUM64, v, 232, 184, 74, 255) : t(TXT_AJ_NUM64, v), f = t(TXT_AJ_TEXTO, "fps");
    float base = y + n.h * 0.8f;
    txt_desenhar(n, x, y);
    txt_desenhar_alpha(f, x + n.w + 12, base - f.h * 0.78f, 0.55f);
    // "pior 21 ms · 0 janks" a direita, numeros em branco forte
    { char a[16], b[16];
      TxtLinha l1, l2, l3, l4, l5;
      float xd = x + cw, esp = (float)(txt_largura(TXT_AJ_ESTADO, "a a") - txt_largura(TXT_AJ_ESTADO, "aa"));
      num(a, sizeof a, uPior, 0); snprintf(p, sizeof p, "%s ms", a);
      snprintf(b, sizeof b, "%d", uJanks);
      // A linha de texto perde os espacos das pontas quando tem acento ou
      // "·": os espacos entram a mao.
      l1 = t(TXT_AJ_ESTADO, i18n("pior")); l2 = t(TXT_LOG_18B, p); l3 = t(TXT_AJ_ESTADO, "·");
      l4 = t(TXT_LOG_18B, b); l5 = t(TXT_AJ_ESTADO, i18n("janks"));
      xd -= l5.w; txt_desenhar_alpha(l5, xd, base - l5.h * 0.78f, 0.6f); xd -= esp;
      xd -= l4.w; txt_desenhar(l4, xd, base - l4.h * 0.78f); xd -= esp;
      xd -= l3.w; txt_desenhar_alpha(l3, xd, base - l3.h * 0.78f, 0.6f); xd -= esp;
      xd -= l2.w; txt_desenhar(l2, xd, base - l2.h * 0.78f); xd -= esp;
      xd -= l1.w; txt_desenhar_alpha(l1, xd, base - l1.h * 0.78f, 0.6f); }
    y += 64 + 16; }
  // grafico do pior quadro: uma barra por amostra, ambar acima de 33 ms
  { float gx = x + 10, gy = y + 10, gw = cw - 20, gh = 74, pw = gw / DS_N, ly = gy + gh * (1 - 33.0f / 70.0f), dx;
    gfx_cor((GfxRect){ x, y, cw, 90 }, 12.0f / 90, 0, 0, 0, 0.18f);
    for (i = 0; i < nSerie; i++) {
      float vv = piorSerie[i] / 70.0f, hh, bx = gx + gw - (nSerie - i) * pw;
      if (vv > 1) vv = 1;
      hh = gh * vv; if (hh < 3) hh = 3;
      if (piorSerie[i] > 33) gfx_cor((GfxRect){ bx, gy + gh - hh, pw - 2, hh }, 0, DS_AMBAR, 0.6f);
      else gfx_cor((GfxRect){ bx, gy + gh - hh, pw - 2, hh }, 0, 0.953f, 0.949f, 0.937f, 0.18f);
    }
    for (dx = 0; dx < gw; dx += 10) gfx_cor((GfxRect){ gx + dx, ly, 4, 1 }, 0, 1, 1, 1, 0.22f);
    { TxtLinha l = t(TXT_AJ_MINI12, "33 ms"); txt_desenhar_alpha(l, gx + 4, ly - 6 - l.h, 0.45f); }
    y += 90 + 6; }
  { char ha[48];
    TxtLinha a, b;
    int seg = nSerie * 3;
    if (seg < 60) snprintf(ha, sizeof ha, i18n("pior quadro · há %d s"), seg);
    else snprintf(ha, sizeof ha, i18n("pior quadro · há %d min"), (seg + 30) / 60);
    a = t(TXT_AJ_MINI14, ha); b = t(TXT_AJ_MINI14, "agora");
    txt_desenhar_alpha(a, x, y, 0.4f); txt_desenhar_alpha(b, x + cw - b.w, y, 0.4f);
    y += 16 + 14; }
  snprintf(v, sizeof v, "%.0f MB", uRss);
  y += stat(i18n("Memória do app"), v, x, y, cw);
  snprintf(v, sizeof v, "%d · %.0f MB", uTela, uTelaMb);
  y += stat(i18n("Texturas na tela"), v, x, y, cw);
  snprintf(v, sizeof v, "%d", uFila);
  y += stat(i18n("Fila de texturas"), v, x, y, cw);
  snprintf(v, sizeof v, i18n("%d · %d da tela"), uDesp, uDespTela);
  y += stat(i18n("Despejadas"), v, x, y, cw);
  gfx_cor((GfxRect){ x, y, cw, 1 }, 0, 1, 1, 1, 0.07f);
  y += 12;
  { TxtLinha k = t(TXT_ILHA_GENERO, "Cache de imagens"), l;
    float fr = teto > 0 ? (float)bytes / (float)teto : 0;
    snprintf(v, sizeof v, "%ld / %ld MB", bytes >> 20, teto >> 20);
    l = t(TXT_ILHA_GENERO, v);
    txt_desenhar_alpha(k, x, y, 0.45f);
    txt_desenhar_alpha(l, x + cw - l.w, y, 0.88f);
    y += 20 + 9;
    if (fr > 1) fr = 1;
    gfx_cor((GfxRect){ x, y, cw, 8 }, 0.5f, 1, 1, 1, 0.07f);
    if (fr > 0.005f) gfx_cor((GfxRect){ x, y, cw * fr, 8 }, 0.5f, 0.298f, 0.765f, 0.541f, 0.8f); }
}

void desempenho_desenhar(Uint32 agora, int forma) {
  (void)agora;
  if (!temAmostra) return;
  gfx_sem_recorte();
  if (forma) pilula(); else ilha();
}

#ifdef DESEMPENHO_TESTE
void desempenho_teste_serie(const float *pior, int n) { int i; nSerie = 0; for (i = 0; i < n && i < DS_N; i++) piorSerie[nSerie++] = pior[i]; }
#endif
