// Ver central.h.
#define NV_ESCALA_TELA
#include "central.h"
#include "centrallista.h"
#include "chsegura.h"
#include "ajustes.h"
#include "anim.h"
#include "catalogo.h"
#include "dados.h"
#include "escala.h"
#include "gfx.h"
#include "horafmt.h"
#include "idioma.h"
#include "layout.h"
#include "perfiltv.h"
#include "perfis.h"
#include "player.h"
#include "ponteiro.h"
#include "redesaude.h"
#include "tex_cache.h"
#include "text.h"
#include "video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef NV_VERSAO
#define NV_VERSAO "dev"
#endif

// A ILHA: coluna na borda direita, da altura da tela, como a da Apple TV.
#define CC_W      780.0f
#define CC_MARGEM  40.0f
#define CC_PAD     36.0f
#define CC_RAIO    36.0f
#define CC_COLS     3
#define CC_GAP     14.0f
#define CC_BOTAO_H 128.0f
#define CC_BOTAO_R  24.0f
#define CC_LINHA_H  60.0f   // linha da lista de edicao
#define CC_LINHA_GAP 8.0f
#define CC_ICONE   28.0f

static int aberta, editando;
static float anim;              // 0 fora, 1 na tela
static int foco;                // botao em foco; == nBotoes e "Editar atalhos"
static int focoEd;              // item do catalogo em foco na edicao
static float rolaEd;            // rolagem da lista de edicao (px)
static CentralLista lista;
static int perfilLido = -1;
static char aviso[96];
static Uint32 avisoAte;

static ChSegura chs;
static SDL_Event chDown;        // o KEYDOWN guardado para reentregar
static int chLogado;

// --- A LISTA DO PERFIL -------------------------------------------------------
static void nomeArquivo(char *d, size_t n) { snprintf(d, n, "central-p%d.txt", perfis_ativo()); }
static void carregar(void) {
  char nome[40], *t;
  if (perfis_ativo() == perfilLido) return;
  perfilLido = perfis_ativo();
  nomeArquivo(nome, sizeof nome);
  t = dados_ler(nome);
  centrallista_ler(&lista, t);
  free(t);
}
static void gravar(void) {
  char nome[40], buf[CENTRAL_MAX * 48 + 8];
  nomeArquivo(nome, sizeof nome);
  centrallista_escrever(&lista, buf, sizeof buf);
  dados_gravar(nome, buf);
}

// Os botoes que existem neste build (a chave pode nao estar na tela daqui).
static int botoes(int *op, int *item) {
  int i, n = 0;
  for (i = 0; i < lista.n; i++) {
    const CentralItem *c = central_catalogo(lista.item[i]);
    int o = c ? ajustes_rapido_op(c->chave) : -1;
    if (o < 0) continue;
    if (op) op[n] = o;
    if (item) item[n] = lista.item[i];
    n++;
  }
  return n;
}
// O catalogo oferecido na edicao: so o que este build tem.
static int ofertas(int *item) {
  int i, n = 0;
  for (i = 0; i < central_catalogo_n(); i++)
    if (ajustes_rapido_op(central_catalogo(i)->chave) >= 0) item[n++] = i;
  return n;
}

void central_abrir(void) {
  carregar();
  if (aberta) return;
  aberta = 1;
  editando = 0;
  foco = 0;
  aviso[0] = 0;
  printf("[central] aberta (%d atalhos)\n", botoes(NULL, NULL));
  fflush(stdout);
}
void central_fechar(void) { aberta = 0; editando = 0; }
int  central_aberta(void) { return aberta; }

static void avisar(const char *s) {
  snprintf(aviso, sizeof aviso, "%s", s);
  avisoAte = SDL_GetTicks() + 2500u;
}

// --- TECLADO -----------------------------------------------------------------
static int ehVoltar(SDL_Keycode k) {
  return k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE;
}
// O CH+ depois do remapeamento (Salvos na Samsung/Android, scancode no LG) e a
// AZUL: a mesma tecla que abriu fecha.
static int ehCh(const SDL_Event *e) {
  SDL_Keycode k = e->key.keysym.sym;
  int sc = e->key.keysym.scancode;
  return k == SDLK_s || k == SDLK_PAGEUP || sc == NV_SCANCODE_BLUE || sc == NV_SCANCODE_CH_UP;
}

static void ativarBotao(int i) {
  int op[CENTRAL_MAX], n = botoes(op, NULL);
  if (i == n) {
    int it[64], m = ofertas(it), j;
    editando = 1; focoEd = 0; rolaEd = 0;
    for (j = 0; j < m; j++) if (centrallista_tem(&lista, it[j])) { focoEd = j; break; }
    return;
  }
  if (i < 0 || i >= n) return;
  if (ajustes_rapido_passo(op[i], 1))
    printf("[central] %s -> %s\n", ajustes_rapido_rotulo(op[i]), ajustes_rapido_valor(op[i]));
  else avisar("Não foi possível salvar. O valor anterior foi mantido.");
  fflush(stdout);
}

static void alternarOferta(int j) {
  int it[64], m = ofertas(it), r;
  if (j < 0 || j >= m) return;
  r = centrallista_alternar(&lista, it[j]);
  if (r < 0) {
    char t[96];
    snprintf(t, sizeof t, i18n("No máximo %d atalhos"), CENTRAL_MAX);
    avisar(t);
    return;
  }
  gravar();
}

void central_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberta || e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (ehCh(e)) { central_fechar(); return; }
  if (editando) {
    int it[64], m = ofertas(it);
    if (ehVoltar(k)) { editando = 0; return; }
    if (k == SDLK_UP && focoEd > 0) focoEd--;
    else if (k == SDLK_DOWN && focoEd + 1 < m) focoEd++;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) alternarOferta(focoEd);
    return;
  }
  {
    int n = botoes(NULL, NULL) + 1;   // + "Editar atalhos"
    if (foco >= n) foco = n - 1;
    if (ehVoltar(k)) { central_fechar(); return; }
    if (k == SDLK_LEFT && foco % CC_COLS > 0) foco--;
    else if (k == SDLK_RIGHT && foco % CC_COLS < CC_COLS - 1 && foco + 1 < n) foco++;
    else if (k == SDLK_UP && foco >= CC_COLS) foco -= CC_COLS;
    else if (k == SDLK_DOWN && foco + CC_COLS < n) foco += CC_COLS;
    else if (k == SDLK_DOWN && foco / CC_COLS < (n - 1) / CC_COLS) foco = n - 1;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) ativarBotao(foco);
  }
}

// Ponteiro (Magic Remote, toque): mesmo foco e mesmo OK das setas.
static void ptFoco(int i, int ed) { if (ed) focoEd = i; else foco = i; }
static void ptOk(int i, int ed) { ptFoco(i, ed); if (ed) alternarOferta(i); else ativarBotao(i); }
static void ptFora(int a, int b) { (void)a; (void)b; central_fechar(); }

void central_atualizar(float dt, Uint32 agora) {
  (void)agora;
  anim = anim_mola(anim, aberta ? 1.0f : 0.0f, dt, 16.0f);
  if (!aberta && anim < 0.01f) anim = 0.0f;
  if (aberta && anim > 0.995f) anim = 1.0f;
}

// --- A TECLA CH+ ---------------------------------------------------------------
int central_tecla_ocupada(void) { return chs_ocupado(&chs); }

int central_tecla(const SDL_Event *e, int pode) {
  Uint32 t;
  int r = CHS_NADA;
  if (e->type != SDL_KEYDOWN && e->type != SDL_KEYUP) return 0;
  if (!chs_ocupado(&chs) && (!pode || e->type == SDL_KEYUP)) return 0;
  t = e->key.timestamp ? e->key.timestamp : SDL_GetTicks();
  if (e->type == SDL_KEYDOWN) {
    if (!chs_ocupado(&chs)) chDown = *e;
    r = chs_desce(&chs, t);
  } else {
    // Uma linha so por sessao: quanto o CH+ deste controle leva entre descer e
    // subir. E o que diz, num log de TV, se o KEYUP vale como "soltou".
    if (!chLogado) {
      chLogado = 1;
      printf("[central] CH+ subiu %u ms depois de descer\n", (unsigned)(t - chs.tUlt));
      fflush(stdout);
    }
    chs_sobe(&chs, t);
  }
  if (r == CHS_LONGO) central_abrir();
  return 1;
}

void central_tecla_quadro(Uint32 agora, void (*entregar)(SDL_Event *e)) {
  int r;
  if (!chs_ocupado(&chs)) return;
  r = chs_quadro(&chs, agora);
  if (r == CHS_LONGO) central_abrir();
  else if (r == CHS_CURTO && entregar) {
    SDL_Event d = chDown, u = chDown;
    u.type = SDL_KEYUP;
    u.key.state = SDL_RELEASED;
    entregar(&d);
    entregar(&u);
  }
}

// --- DESENHO -----------------------------------------------------------------
static void kicker(const char *s, float x, float y) { ajustes_ui_kicker(i18n(s), x, y, 1.0f); }

// Rotulo a esquerda, valor a direita, sobre um fio: a linha de informacao.
static float linhaInfo(const char *k, const char *v, float x, float y, float w) {
  TxtLinha lk = txt_linha(TXT_ILHA_META, k, 243, 242, 239, 255);
  TxtLinha lv = txt_linha_corta(TXT_ILHA_META, v, 243, 242, 239, 255, w - (float)lk.w - 24.0f);
  gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.07f);
  txt_desenhar_alpha(lk, x, y + 12.0f, 0.5f);
  txt_desenhar_alpha(lv, x + w - (float)lv.w, y + 12.0f, 0.9f);
  return 12.0f + (float)lk.h + 12.0f;
}

static const char *nomePlataforma(void) {
  switch (ptv_plataforma()) {
    case PTV_LG: return "LG webOS";
    case PTV_TIZEN: return "Samsung Tizen";
    case PTV_TPK: return "Samsung Tizen";
    case PTV_ANDROID: return "Android TV";
    default: return "";
  }
}

// O VIDEO QUE ESTA TOCANDO: so o que o pipeline disse (video.h).
static float blocoTocando(float x, float y, float w) {
  const CatItem *c;
  char meta[160];
  const char *hdr;
  size_t u = 0;
  float y0 = y;
  if (!player_aberto() || !player_com_video()) return 0;
  c = cat_item(player_indice());
  kicker("Tocando agora", x, y);
  y += 30.0f;
  if (c && c->titulo[0]) {
    TxtLinha t = txt_linha_corta(TXT_ILHA_NOME, c->titulo, 243, 242, 239, 255, w);
    txt_desenhar_alpha(t, x, y, 1.0f);
    y += (float)t.h + 6.0f;
  }
  meta[0] = 0;
  { const char *ep = player_linha_episodio();
    if (ep && ep[0]) u += (size_t)snprintf(meta + u, sizeof meta - u, "%s", ep); }
  if (video_largura() > 0 && video_altura() > 0 && u < sizeof meta)
    u += (size_t)snprintf(meta + u, sizeof meta - u, "%s%d×%d", u ? " · " : "", video_largura(), video_altura());
  hdr = video_hdr();
  if (hdr && hdr[0] && strcmp(hdr, "none") && strcmp(hdr, "SDR") && u < sizeof meta)
    u += (size_t)snprintf(meta + u, sizeof meta - u, "%s%s", u ? " · " : "",
                          !strcmp(hdr, "DolbyVision") ? "Dolby Vision" : hdr);
  if (video_tem_atmos() && u < sizeof meta)
    u += (size_t)snprintf(meta + u, sizeof meta - u, "%sDolby Atmos", u ? " · " : "");
  if (meta[0]) {
    TxtLinha m = txt_linha_corta(TXT_ILHA_META, meta, 243, 242, 239, 255, w);
    txt_desenhar_alpha(m, x, y, 0.62f);
    y += (float)m.h;
  }
  return y - y0 + 22.0f;
}

static void superficie(GfxRect r, float raioPx, float f) {
  if (ajustes_vidro()) gfx_cor(r, raioPx / r.h, 1, 1, 1, 0.06f + 0.10f * f);
  else gfx_cor(r, raioPx / r.h, 0.125f + 0.06f * f, 0.13f + 0.06f * f, 0.153f + 0.06f * f, 1);
}

static void botao(GfxRect r, const char *icone, const char *rot, const char *val, int ligado, int f) {
  float ar, ag, ab;
  int aceso = ligado == 1;
  int tinta = aceso ? ajustes_tinta_foco() : 243;
  float ti = (float)tinta / 255.0f;
  ajustes_acento(&ar, &ag, &ab);
  // Interruptor LIGADO = botao cheio no acento, como os da Apple TV; o resto
  // e superficie neutra. Foco = superficie mais clara e o anel.
  if (aceso) gfx_cor(r, CC_BOTAO_R / r.h, ar, ag, ab, 1);
  else superficie(r, CC_BOTAO_R, f ? 1.0f : 0.0f);
  if (f) gfx_anel_fora(r, CC_BOTAO_R / r.h, 4.0f, 3.0f, 1, 1, 1, 0.95f);
  gfx_icone((GfxRect){ r.x + 20.0f, r.y + 20.0f, CC_ICONE, CC_ICONE }, icone, ti, ti, ti, aceso ? 1.0f : 0.85f);
  { TxtLinha v = txt_linha_corta(TXT_ILHA_ITEM, val, tinta, tinta, tinta, 255, r.w - 40.0f);
    TxtLinha l = txt_linha_corta(TXT_ILHA_GENERO, rot, tinta, tinta, tinta, 255, r.w - 40.0f);
    txt_desenhar_alpha(v, r.x + 20.0f, r.y + r.h - 18.0f - (float)v.h, 1.0f);
    txt_desenhar_alpha(l, r.x + 20.0f, r.y + r.h - 18.0f - (float)v.h - 2.0f - (float)l.h, aceso ? 0.8f : 0.6f); }
}

static void desenhaBotoes(float x, float y, float w) {
  int op[CENTRAL_MAX], item[CENTRAL_MAX], n = botoes(op, item), i;
  float bw = (w - CC_GAP * (CC_COLS - 1)) / CC_COLS;
  if (foco > n) foco = n;
  for (i = 0; i <= n; i++) {
    GfxRect r = { x + (float)(i % CC_COLS) * (bw + CC_GAP), y + (float)(i / CC_COLS) * (CC_BOTAO_H + CC_GAP),
                  bw, CC_BOTAO_H };
    if (i < n) {
      const CentralItem *c = central_catalogo(item[i]);
      botao(r, c->icone, ajustes_rapido_rotulo(op[i]), ajustes_rapido_valor(op[i]),
            ajustes_rapido_ligado(op[i]), i == foco);
    } else botao(r, "aj_sliders-horizontal", "Atalhos", "Editar", 0, i == foco);
    if (anim > 0.5f && ponteiro_ativo()) ponteiro_alvo(r.x, r.y, r.w, r.h, ptFoco, ptOk, i, 0);
  }
}

static void desenhaEdicao(float x, float y, float w, float h) {
  int it[64], m = ofertas(it), j;
  float passo = CC_LINHA_H + CC_LINHA_GAP;
  float alvo = (float)focoEd * passo - (h - CC_LINHA_H) * 0.5f;
  float maxR = (float)m * passo - CC_LINHA_GAP - h;
  if (alvo > maxR) alvo = maxR;
  if (alvo < 0) alvo = 0;
  rolaEd = alvo;
  gfx_recorte(x - 8.0f, y, w + 16.0f, h);
  for (j = 0; j < m; j++) {
    const CentralItem *c = central_catalogo(it[j]);
    int op = ajustes_rapido_op(c->chave), tem = centrallista_tem(&lista, it[j]);
    GfxRect r = { x, y + (float)j * passo - rolaEd, w, CC_LINHA_H };
    if (r.y + r.h < y || r.y > y + h) continue;
    if (j == focoEd) superficie(r, r.h * 0.5f, 1.0f);
    gfx_icone((GfxRect){ r.x + 20.0f, r.y + (r.h - 24.0f) * 0.5f, 24, 24 }, c->icone,
              0.953f, 0.949f, 0.937f, j == focoEd ? 1.0f : 0.72f);
    { TxtLinha t = txt_linha_corta(TXT_PG_ROTULO, ajustes_rapido_rotulo(op), 243, 242, 239, 255,
                                   r.w - 20.0f - 24.0f - 18.0f - 72.0f);
      txt_desenhar_alpha(t, r.x + 62.0f, r.y + (r.h - (float)t.h) * 0.5f, j == focoEd ? 1.0f : 0.72f); }
    if (tem) {
      float ar, ag, ab;
      ajustes_acento_marca(&ar, &ag, &ab);
      gfx_icone((GfxRect){ r.x + r.w - 20.0f - 26.0f, r.y + (r.h - 26.0f) * 0.5f, 26, 26 }, "aj_circle-check",
                ar, ag, ab, 1.0f);
    } else
      gfx_icone((GfxRect){ r.x + r.w - 20.0f - 26.0f, r.y + (r.h - 26.0f) * 0.5f, 26, 26 }, "aj_plus",
                0.953f, 0.949f, 0.937f, 0.45f);
    if (anim > 0.5f && ponteiro_ativo()) ponteiro_alvo(r.x, r.y, r.w, r.h, ptFoco, ptOk, j, 1);
  }
  gfx_sem_recorte();
}

void central_desenhar(Uint32 agora) {
  float x, y, w, h, k, cx, cw;
  ESCALA_INI();
  if (anim <= 0.0f) { ESCALA_FIM(); return; }
  k = anim;
  carregar();
  w = CC_W;
  h = NV_TELA_H - 2.0f * CC_MARGEM;
  x = NV_TELA_W - CC_MARGEM - w + (1.0f - k) * (w + CC_MARGEM + 40.0f);
  y = CC_MARGEM;
  // Veu so do lado esquerdo da ilha: a tela segue legivel, como na Apple TV.
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0, 0, 0, 0, 0.38f * k);
  if (anim > 0.5f && ponteiro_ativo()) {
    ponteiro_camada();
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ptFora, 0, 0);
    ponteiro_alvo(x, y, w, h, NULL, NULL, 0, 0);
  }
  ajustes_ui_ilha((GfxRect){ x, y, w, h }, CC_RAIO, 1);
  cx = x + CC_PAD; cw = w - 2.0f * CC_PAD;
  y += CC_PAD;

  if (editando) {
    char t[64];
    kicker("Editar atalhos", cx, y);
    snprintf(t, sizeof t, "%d / %d", lista.n, CENTRAL_MAX);
    { TxtLinha l = txt_linha(TXT_ILHA_META, t, 243, 242, 239, 255);
      txt_desenhar_alpha(l, cx + cw - (float)l.w, y - 2.0f, 0.55f); }
    y += 44.0f;
    desenhaEdicao(cx, y, cw, CC_MARGEM + h - CC_PAD - 64.0f - y);
  } else {
    kicker("Central de controle", cx, y);
    y += 30.0f;
    // HORA e PERFIL na mesma linha.
    { time_t tt = time(NULL);
      struct tm lt;
      char hora[24];
      const ContaPerfil *p = perfis_item_ativo();
      TxtLinha lh;
      localtime_r(&tt, &lt);
      hora_tela(hora, sizeof hora, &lt);
      lh = txt_linha(TXT_G52B, hora, 243, 242, 239, 255);
      txt_desenhar_alpha(lh, cx, y, 1.0f);
      if (p && p->nome[0]) {
        TxtLinha ln = txt_linha_corta(TXT_ILHA_NOME, p->nome, 243, 242, 239, 255, cw * 0.5f);
        float pr = 0.12f, pg = 0.53f, pb = 0.90f;
        unsigned v;
        if (p->corHex[0] == '#' && sscanf(p->corHex + 1, "%6x", &v) == 1) {
          pr = (float)((v >> 16) & 255) / 255.0f; pg = (float)((v >> 8) & 255) / 255.0f; pb = (float)(v & 255) / 255.0f;
        }
        { float nx = cx + cw - (float)ln.w, ny = y + ((float)lh.h - (float)ln.h) * 0.5f;
          gfx_cor((GfxRect){ nx - 28.0f, ny + ((float)ln.h - 16.0f) * 0.5f, 16, 16 }, 0.5f, pr, pg, pb, 1);
          txt_desenhar_alpha(ln, nx, ny, 0.9f); }
      }
      y += (float)lh.h + 18.0f;
    }
    // O QUE O APP SABE AGORA.
    y += linhaInfo("Internet", rede_saude_offline() ? "Sem internet" : "Conectado", cx, y, cw);
    { long usado = 0, teto = tex_orcamento_bytes();
      char t[64];
      tex_estatisticas(NULL, NULL, &usado, NULL, NULL);
      if (teto > 0) {
        snprintf(t, sizeof t, i18n("%d de %d MB"), (int)(usado >> 20), (int)(teto >> 20));
        y += linhaInfo("Imagens", t, cx, y, cw);
      }
    }
    { char t[96];
      const char *pl = nomePlataforma();
      snprintf(t, sizeof t, "%s%s%s", NV_VERSAO, pl[0] ? " · " : "", pl);
      y += linhaInfo("Versão", t, cx, y, cw);
    }
    gfx_cor((GfxRect){ cx, y, cw, 1 }, 0, 1, 1, 1, 0.07f);
    y += 26.0f;
    y += blocoTocando(cx, y, cw);
    kicker("Atalhos", cx, y);
    y += 34.0f;
    desenhaBotoes(cx, y, cw);
  }

  // AVISO curto (lista cheia, falha ao gravar) acima das dicas.
  if (aviso[0] && (Sint32)(avisoAte - agora) > 0) {
    TxtLinha l = txt_linha_corta(TXT_ILHA_META, aviso, 240, 190, 130, 255, cw);
    txt_desenhar_alpha(l, cx, CC_MARGEM + h - CC_PAD - 30.0f - 16.0f - (float)l.h, 1.0f);
  }
  { static const char *const tk[] = { "OK", "Voltar" };
    const char *rot[2];
    rot[0] = editando ? "Pôr ou tirar" : "Mudar";
    rot[1] = editando ? "Pronto" : "Fechar";
    ajustes_ui_dicas(tk, rot, 2, cx, CC_MARGEM + h - CC_PAD - 30.0f, 1); }
  ESCALA_FIM();
}

#ifdef CENTRAL_TESTE
void central_teste_foco(int f, int editar) {
  central_abrir();
  anim = 1.0f;
  foco = f < 0 ? botoes(NULL, NULL) : f;
  if (editar) ativarBotao(botoes(NULL, NULL));
}
#endif
