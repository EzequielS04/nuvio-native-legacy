// Perfil do amigo (tela C). Ver amigoperfil.h.
#include "amigoperfil.h"
#include "socialvis.h"
#include "svdesenho.h"
#include "ajustes.h"
#include "anim.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "layout.h"
#include "idioma.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Tres fileiras de cartazes 2:3. 150x225 cabe as tres com titulo e legenda
// entre y=96 e y=1040 (medido na captura, amigoperfil no socialui_shot).
#define AP_PW        150.0f
#define AP_PH        225.0f
#define AP_PGAP       26.0f
#define AP_FILA_TOPO  92.0f
#define AP_FILA_PASSO 318.0f
enum { AP_ASSISTINDO = 0, AP_GOSTOU, AP_MANDOU, AP_NFILAS };

static char pessoa[96];
static SvPerfil perf;
static int temPerfil;
static unsigned revPerf = ~0u;
static int fila, col;
static float fFoco[AP_NFILAS][SV_FILA_MAX];
static float entrada;
static int sair, temPedido;
static char pedido[24];

static int nFila(int f) {
  if (!temPerfil) return 0;
  return f == AP_ASSISTINDO ? perf.nAssistindo : f == AP_GOSTOU ? perf.nGostou : perf.nMandou;
}

static void recarregar(void) {
  temPerfil = socialvis_perfil(pessoa, &perf);
  revPerf = socialvis_revisao();
  if (fila < 0 || fila >= AP_NFILAS || nFila(fila) == 0) {
    int f;
    fila = -1;
    for (f = 0; f < AP_NFILAS; f++) if (nFila(f) > 0) { fila = f; break; }
    col = 0;
  }
  if (fila >= 0 && col >= nFila(fila)) col = nFila(fila) - 1;
}

void amigoperfil_abrir(const char *id) {
  snprintf(pessoa, sizeof pessoa, "%s", id ? id : "");
  fila = -1; col = 0;
  sair = 0; temPedido = 0;
  entrada = 0.0f;
  memset(fFoco, 0, sizeof fFoco);
  socialvis_atualizar();
  recarregar();
  socialvis_marcar_visto(pessoa);
}

int amigoperfil_quer_sair(void) { int v = sair; sair = 0; return v; }
int amigoperfil_pediu_titulo(char *imdb, size_t tam) {
  if (!temPedido) return 0;
  temPedido = 0;
  if (imdb && tam) snprintf(imdb, tam, "%s", pedido);
  return 1;
}

static const char *imdbDe(int f, int c) {
  if (f == AP_ASSISTINDO && c < perf.nAssistindo) return perf.assistindo[c].imdb;
  if (f == AP_GOSTOU && c < perf.nGostou) return perf.gostou[c].imdb;
  if (f == AP_MANDOU && c < perf.nMandou) return perf.mandou[c].imdb;
  return "";
}

void amigoperfil_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!e || e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      e->key.keysym.scancode == NV_SCANCODE_BACK) { sair = 1; return; }
  if (fila < 0) return;
  if (k == SDLK_LEFT && col > 0) col--;
  else if (k == SDLK_RIGHT && col + 1 < nFila(fila)) col++;
  else if (k == SDLK_UP || k == SDLK_DOWN) {
    int d = k == SDLK_UP ? -1 : 1, f;
    for (f = fila + d; f >= 0 && f < AP_NFILAS; f += d)
      if (nFila(f) > 0) { fila = f; if (col >= nFila(f)) col = nFila(f) - 1; break; }
  } else if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) {
    const char *id = imdbDe(fila, col);
    if (id[0]) { snprintf(pedido, sizeof pedido, "%s", id); temPedido = 1; }
  }
}

void amigoperfil_atualizar(float dt, Uint32 agora) {
  int f, c, red = ajustes_animacoes_reduzidas();
  (void)agora;
  socialvis_atualizar();
  if (revPerf != socialvis_revisao()) recarregar();
  entrada = red ? 1.0f : anim_mola(entrada, 1.0f, dt, NV_MOLA_TELA);
  for (f = 0; f < AP_NFILAS; f++)
    for (c = 0; c < SV_FILA_MAX; c++) {
      float alvo = (f == fila && c == col) ? 1.0f : 0.0f;
      fFoco[f][c] = red ? alvo : anim_mola(fFoco[f][c], alvo, dt, NV_MOLA_FOCO);
    }
}

// Cartaz com o aro de foco da home por fora.
static void cartaz(GfxRect r, const char *url, float f, float a) {
  float raio = NV_RAIO_CARD;
  if (f > 0.01f) {
    float ar, ag, ab, menor = r.w;
    GfxRect b = { r.x - NV_ANEL_FOCO, r.y - NV_ANEL_FOCO, r.w + 2 * NV_ANEL_FOCO, r.h + 2 * NV_ANEL_FOCO };
    ajustes_acento(&ar, &ag, &ab);
    if (ajustes_vidro()) gfx_vidro_cartao(r, raio * menor / r.h, f, a);
    else gfx_cor(b, (raio * menor + NV_ANEL_FOCO) / (menor + 2 * NV_ANEL_FOCO), ar, ag, ab, f * a);
  }
  svd_poster(r, url, raio * r.w / r.h, a);
}

static void numero(GfxRect r, const char *valor, const char *rotulo, float a) {
  TxtLinha v = txt_linha(TXT_TITULO3, valor, 246, 246, 250, 255);
  TxtLinha l = txt_linha_corta(TXT_CAPTION, rotulo, 167, 164, 178, 255, r.w - 36.0f);
  gfx_cor(r, 18.0f / r.h, 0.118f, 0.114f, 0.141f, 0.94f * a);
  txt_desenhar_alpha(v, r.x + 20.0f, r.y + 16.0f, a);
  txt_desenhar_alpha(l, r.x + 20.0f, r.y + r.h - 16.0f - (float)l.h, a * 0.95f);
}

static void tituloFila(float x, float y, const char *t, float a) {
  TxtLinha l = txt_linha(TXT_CALLOUT, t, 207, 205, 216, 255);
  txt_desenhar_alpha(l, x, y, a);
}

static void vazioFila(float x, float y, const char *t, float a) {
  TxtLinha l = txt_linha(TXT_CAPTION, t, 140, 138, 150, 255);
  gfx_cor((GfxRect){ x, y, 4.0f * (AP_PW + AP_PGAP) - AP_PGAP, 90.0f }, 18.0f / 90.0f,
          1.0f, 1.0f, 1.0f, 0.04f * a);
  txt_desenhar_alpha(l, x + 24.0f, y + (90.0f - (float)l.h) * 0.5f, a);
}

static void legenda(float x, float y, const char *t, int r, int g, int b, float a) {
  TxtLinha l = txt_linha_corta(TXT_MINI, t, r, g, b, 255, AP_PW + AP_PGAP - 8.0f);
  txt_desenhar_alpha(l, x, y, a);
}

void amigoperfil_desenhar(Uint32 agora) {
  float a = anim_suave(entrada);
  float R = ajustes_rail_largura_fixa();
  float lx = 96.0f + R, lw = 500.0f, rx = lx + lw + 80.0f;
  char buf[200];
  int f, c;
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0, NV_COR_FUNDO_R, NV_COR_FUNDO_G,
          NV_COR_FUNDO_B, 1.0f);
  if (!temPerfil) {
    TxtLinha t = txt_linha(TXT_TITULO3, "Perfil indisponível", 240, 240, 245, 255);
    txt_desenhar_alpha(t, lx, 200.0f, a);
    return;
  }
  // A ARTE DO QUE ELE ESTA VENDO (ou viu por ultimo) atras, bem baixa: e o
  // "ambiente" da pessoa. Uma textura so, ja decodificada na fileira da home.
  { const SvEvento *e = perf.a.nTit > 0 ? &perf.a.tit[0] : NULL;
    const char *arte = e ? (e->arte[0] ? e->arte : e->poster) : NULL;
    GLuint t = arte ? tex_obter_larg(arte, 960.0f) : 0;
    if (t) {
      gfx_tex_aspect_atual = tex_aspecto(arte);
      gfx_rect((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H * 0.62f }, t, GFX_HERO, 0, 0, 0, 0,
               0, 0, 0, 0.22f * a);
      gfx_tex_aspect_atual = 0.0f;
    } }

  // --- coluna da pessoa ---
  { GfxRect av = { lx + 6.0f, 96.0f, 200.0f, 200.0f };
    float y;
    svd_rosto(av, &perf.a, 0.0f, a, agora);
    y = av.y + av.h + 40.0f;
    { TxtLinha t = txt_linha_corta(TXT_TITULO2, perf.a.nome, 246, 246, 250, 255, lw);
      txt_desenhar_alpha(t, lx, y, a); y += (float)t.h + 6.0f; }
    // "Amigos ha 2 meses · pelo codigo" — a FONTE pequena, so aqui.
    { char q[80] = "", por[80];
      if (perf.desde > 0) {
        long long d = ((long long)time(NULL) - perf.desde) / 86400;
        if (d >= 60) snprintf(q, sizeof q, i18n("Amigos há %d meses"), (int)(d / 30));
        else if (d >= 2) snprintf(q, sizeof q, i18n("Amigos há %d dias"), (int)d);
      }
      if (perf.porOnde == SV_FONTE_NUVIO) snprintf(por, sizeof por, "%s", i18n("pelo código"));
      else snprintf(por, sizeof por, i18n("pelo %s"), socialvis_fonte_nome(perf.porOnde));
      if (q[0]) snprintf(buf, sizeof buf, "%s \xc2\xb7 %s", q, por);
      else snprintf(buf, sizeof buf, "%s", por);
      { TxtLinha t = txt_linha_corta(TXT_CAPTION, buf, 168, 166, 178, 255, lw);
        txt_desenhar_alpha(t, lx, y, a); y += (float)t.h + 28.0f; } }
    if (perf.a.agora && perf.a.nTit > 0) {
      char st[160];
      socialvis_status(&perf.a.tit[0], st, sizeof st);
      snprintf(buf, sizeof buf, "%s \xc2\xb7 %s", st, perf.a.tit[0].titulo);
      svd_ponto_vivo(lx + 8.0f, y + 15.0f, 14.0f, 0.0f, a, agora);
      { TxtLinha t = txt_linha_corta(TXT_CAPTION, buf, 255, 214, 214, 255, lw - 26.0f);
        txt_desenhar_alpha(t, lx + 26.0f, y, a); y += (float)t.h + 22.0f; }
    }
    // GOSTO PARECIDO: so com dado; sem ele a linha nao existe (um "0 %" seria
    // uma afirmacao falsa sobre duas pessoas).
    if (perf.gostoPct >= 0) {
      TxtLinha l = txt_linha(TXT_CALLOUT, "Gosto parecido", 216, 213, 224, 255);
      char pc[16];
      TxtLinha v;
      snprintf(pc, sizeof pc, "%d%%", perf.gostoPct);
      v = txt_linha(TXT_CALLOUT, pc, 247, 192, 138, 255);
      txt_desenhar_alpha(l, lx, y, a);
      txt_desenhar_alpha(v, lx + (float)l.w + 14.0f, y, a);
      y += (float)l.h + 4.0f;
      if (perf.emComum > 0) {
        snprintf(buf, sizeof buf, i18n("vocês gostaram de %d títulos iguais"), perf.emComum);
        { TxtLinha t = txt_linha_corta(TXT_CAPTION, buf, 168, 166, 178, 255, lw);
          txt_desenhar_alpha(t, lx, y, a); y += (float)t.h; }
      }
      y += 26.0f;
    }
    // OS QUATRO NUMEROS DO MES, numa grade 2x2. "—" quando nao ha dado.
    { char v[4][32];
      const char *rot[4] = { "assistidas neste mês", "filmes vistos", "séries em curso",
                             "recomendações vistas" };
      float bw = (lw - 16.0f) * 0.5f, bh = 116.0f;
      int i;
      if (perf.minutosMes >= 0) snprintf(v[0], sizeof v[0], i18n("%d h"), perf.minutosMes / 60);
      else snprintf(v[0], sizeof v[0], "\xe2\x80\x94");
      if (perf.filmesMes >= 0) snprintf(v[1], sizeof v[1], "%d", perf.filmesMes);
      else snprintf(v[1], sizeof v[1], "\xe2\x80\x94");
      if (perf.seriesCurso >= 0) snprintf(v[2], sizeof v[2], "%d", perf.seriesCurso);
      else snprintf(v[2], sizeof v[2], "\xe2\x80\x94");
      if (perf.recsVistas >= 0 && perf.recsTotal > 0)
        snprintf(v[3], sizeof v[3], i18n("%d de %d"), perf.recsVistas, perf.recsTotal);
      else snprintf(v[3], sizeof v[3], "\xe2\x80\x94");
      if (y < 640.0f) y = 640.0f;
      for (i = 0; i < 4; i++) {
        GfxRect r = { lx + (float)(i % 2) * (bw + 16.0f), y + (float)(i / 2) * (bh + 16.0f), bw, bh };
        numero(r, v[i], rot[i], a);
      } } }

  // --- as tres fileiras ---
  for (f = 0; f < AP_NFILAS; f++) {
    float y = AP_FILA_TOPO + (float)f * AP_FILA_PASSO;
    const char *tit = f == AP_ASSISTINDO ? "Assistindo" : f == AP_GOSTOU ? "Gostou recentemente"
                                                                       : "Você mandou";
    int n = nFila(f);
    tituloFila(rx, y, tit, a);
    y += 46.0f;
    if (n == 0) {
      vazioFila(rx, y, f == AP_MANDOU ? "Você ainda não mandou nada para essa pessoa."
                                      : "Nada por aqui ainda.", a);
      continue;
    }
    for (c = 0; c < n && c < SV_FILA_MAX; c++) {
      float x = rx + (float)c * (AP_PW + AP_PGAP);
      GfxRect r = { x, y, AP_PW, AP_PH };
      float fc = fFoco[f][c];
      if (x + AP_PW > NV_TELA_W - 40.0f) break;
      if (f == AP_MANDOU) {
        const SvEnviada *m = &perf.mandou[c];
        cartaz(r, m->poster, fc, a);
        // O ESTADO, que e o motivo desta fileira existir.
        { const char *st = m->estado == SV_REC_VIU
                             ? (m->reacao == SV_REAC_GOSTOU ? "viu · gostou" : "viu")
                         : m->estado == SV_REC_ABRIU ? "abriu" : "ainda não viu";
          int ok = m->estado == SV_REC_VIU;
          legenda(x, y + AP_PH + 10.0f, st,
                  ok ? 111 : 160, ok ? 207 : 158, ok ? 151 : 170, a); }
      } else {
        const SvEvento *e = f == AP_ASSISTINDO ? &perf.assistindo[c] : &perf.gostou[c];
        char ep[24];
        cartaz(r, e->poster[0] ? e->poster : e->arte, fc, a);
        if (e->pct >= 0 && f == AP_ASSISTINDO)
          svd_barra((GfxRect){ x + 10.0f, y + AP_PH - 14.0f, AP_PW - 20.0f, 5.0f }, e->pct, a);
        socialvis_ep(e, ep, sizeof ep);
        if (f == AP_ASSISTINDO && e->acao == SV_AGORA) {
          snprintf(buf, sizeof buf, "%s%s%s", i18n("Agora"), ep[0] ? " \xc2\xb7 " : "", ep);
          legenda(x, y + AP_PH + 10.0f, buf, 255, 190, 190, a);
        } else if (f == AP_ASSISTINDO) {
          legenda(x, y + AP_PH + 10.0f, ep[0] ? ep : e->titulo, 190, 188, 200, a);
        } else {
          legenda(x, y + AP_PH + 10.0f, e->titulo, 190, 188, 200, a);
        }
      }
    }
  }
  { TxtLinha t = txt_linha(TXT_MINI, "OK · abrir o título   ·   Voltar · fechar", 150, 148, 160, 255);
    txt_desenhar_alpha(t, lx, NV_TELA_H - 44.0f, a * 0.9f); }
}
