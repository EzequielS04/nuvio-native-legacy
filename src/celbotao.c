// Ver celbotao.h para o porque.
#include "celbotao.h"
#include "celular.h"
#include "text.h"
#include "anim.h"
#include "layout.h"
#include "ajustes.h"
#include "idioma.h"
#include "qr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// O CARTAO: 380 de largura cabe o QR de 248 com folga para o endereco numa
// linha em TXT_CAPTION ("192.168.100.200:65535/abcdefgh" mede ~330).
#define CB_W      380.0f
#define CB_PAD     30.0f
#define CB_QR     248.0f
#define CB_MOLDURA 12.0f
#define CB_VAO     14.0f   // entre o botao e o cartao

static int     dono, aberto;
static float   animCartao;            // 0..1 (mola), entrando
static Uint32  ultimoQuadro;
static GfxRect ancora[CELB_N];        // ultimo botao desenhado de cada dono
static float   animBotao[CELB_N];
static Uint32  ultimoBotao[CELB_N];
static char    titulo[96];
static GfxRect cartao;

int celb_disponivel(void) { return celular_disponivel(); }
int celb_aberto(void) { return aberto; }
int celb_dono(void) { return aberto ? dono : CELB_NENHUM; }
GfxRect celb_cartao_rect(void) { return aberto ? cartao : (GfxRect){ 0, 0, 0, 0 }; }

static float passo(Uint32 *ultimo) {
  Uint32 t = SDL_GetTicks();
  float dt = *ultimo ? (float)(t - *ultimo) / 1000.0f : 1.0f / 60.0f;
  *ultimo = t;
  return dt > 0.1f ? 0.1f : dt;
}

void celb_botao(int d, GfxRect r, int focado, PonteiroFn focar, int a, int b, float alpha) {
  float ar, ag, ab, k, esc, dt;
  GfxRect c;
  int t;
  if (d <= CELB_NENHUM || d >= CELB_N || !celular_disponivel()) return;
  ancora[d] = r;
  dt = passo(&ultimoBotao[d]);
  // Com o cartao aberto o botao fica aceso: e dele que o cartao saiu.
  if (aberto && dono == d) focado = 1;
  animBotao[d] = ajustes_animacoes_reduzidas() ? (focado ? 1.0f : 0.0f)
               : anim_mola(animBotao[d], focado ? 1.0f : 0.0f, dt, NV_MOLA_FOCO);
  k = animBotao[d];
  if (ponteiro_ativo()) ponteiro_alvo(r.x - 4.0f, r.y - 4.0f, r.w + 8.0f, r.h + 8.0f, focar, NULL, a, b);
  ajustes_acento(&ar, &ag, &ab);
  esc = 1.0f + 0.06f * k;
  c = (GfxRect){ r.x - r.w * (esc - 1.0f) * 0.5f, r.y - r.h * (esc - 1.0f) * 0.5f, r.w * esc, r.h * esc };
  // A MESMA FAMILIA DO FALAR (spotlight.c, busca.c, teclado.c): disco de
  // 0,2 de cinza em repouso, cheio na cor do realce no foco, com a mancha
  // difusa curta. O fio claro em repouso e o que diz "botao" sem foco.
  if (k > 0.01f)
    gfx_rect((GfxRect){ c.x - 12, c.y - 12, c.w + 24, c.h + 24 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
             ar, ag, ab, 0.30f * k * alpha);
  gfx_cor(c, 0.5f, anim_mistura(0.18f, ar, k), anim_mistura(0.185f, ag, k),
          anim_mistura(0.205f, ab, k), alpha);
  gfx_vidro_aro(c, 0.5f, 1.5f, 1.0f, 1.0f, 1.0f, 0.14f * (1.0f - k) * alpha);
  t = k > 0.5f ? ajustes_tinta_foco() : 224;
  gfx_icone((GfxRect){ c.x + c.w * 0.26f, c.y + c.h * 0.26f, c.w * 0.48f, c.h * 0.48f }, "aj_smartphone",
            t / 255.0f, t / 255.0f, t / 255.0f, alpha);
}

int celb_abrir(int d, const char *t) {
  if (d <= CELB_NENHUM || d >= CELB_N || !celular_disponivel()) return 0;
  snprintf(titulo, sizeof titulo, "%s", t ? t : "");
  dono = d;
  aberto = 1;
  animCartao = 0.0f;
  ultimoQuadro = 0;
  // Sem rede o cartao abre assim mesmo e diz por que nao ha codigo: um OK que
  // nao faz nada pareceria botao quebrado.
  celular_abrir(titulo);
  printf("[celular] cartao aberto (dono %d)\n", d);
  return 1;
}

void celb_fechar(void) {
  if (!aberto) return;
  aberto = 0;
  celular_fechar();
}
void celb_fechar_dono(int d) { if (aberto && dono == d) celb_fechar(); }

int celb_pegar(int d, char *dst, size_t n) {
  if (!aberto || dono != d) return 0;
  if (!celular_pegar(dst, n)) return 0;
  celb_fechar();
  return 1;
}

static void regerar(void) {
  if (aberto) celular_abrir(titulo);
}
static void fecharPonteiro(int a, int b) { (void)a; (void)b; celb_fechar(); }
static void regerarPonteiro(int a, int b) { (void)a; (void)b; regerar(); }

int celb_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto) return 0;
  if (e->type == SDL_TEXTINPUT || e->type == SDL_TEXTEDITING) return 1;
  if (e->type == SDL_KEYUP) return 1;
  if (e->type != SDL_KEYDOWN) return 0;
  k = e->key.keysym.sym;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE || k == SDLK_DELETE ||
      e->key.keysym.scancode == NV_SCANCODE_BACK) {
    celb_fechar();
    return 1;
  }
  if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && !e->key.repeat) {
    // OK com o codigo na tela nao faz nada: quem aperta OK ali esta
    // "confirmando", e fechar levaria o codigo embora antes do envio.
    if (celular_estado() != CEL_ESPERANDO) regerar();
    return 1;
  }
  return 1;
}

// --- desenho ------------------------------------------------------------------
// QR como TEXTURA (como login.c): a versao 3-4 tem ~1000 modulos, e um
// retangulo por modulo por quadro custaria mais que o cartao inteiro. NEAREST:
// modulo borrado e o jeito mais rapido de a camera nao ler.
static GLuint texQr;
static char   texQrDe[96];
static void qrTextura(const char *u) {
  Qr q;
  int lado, x, y;
  unsigned char *px;
  if (!u[0] || (texQr && !strcmp(texQrDe, u))) return;
  if (!qr_gerar(&q, u)) return;
  lado = q.lado + 4;
  px = (unsigned char *)malloc((size_t)lado * lado * 3);
  if (!px) return;
  memset(px, 255, (size_t)lado * lado * 3);
  for (y = 0; y < q.lado; y++)
    for (x = 0; x < q.lado; x++)
      if (qr_modulo(&q, x, y)) {
        size_t i = ((size_t)(y + 2) * lado + (x + 2)) * 3;
        px[i] = px[i + 1] = px[i + 2] = 0;
      }
  if (!texQr) glGenTextures(1, &texQr);
  glBindTexture(GL_TEXTURE_2D, texQr);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, lado, lado, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  free(px);
  snprintf(texQrDe, sizeof texQrDe, "%s", u);
}

// Altura do miolo, pela mesma conta que o desenho usa.
#define CB_TITULO_H   40.0f
#define CB_LINHA_H    26.0f
static float alturaCartao(int temQr) {
  if (!temQr) return CB_PAD + CB_TITULO_H + 16.0f + 3 * CB_LINHA_H + CB_PAD;
  return CB_PAD + CB_TITULO_H + 18.0f + CB_QR + 2 * CB_MOLDURA + 18.0f + 30.0f + 10.0f +
         2 * CB_LINHA_H + CB_PAD - 6.0f;
}

void celb_desenhar(void) {
  float a, dt, x, y, w, h, ar, ag, ab;
  GfxRect r;
  int est, temQr;
  const char *u, *curta;
  if (!aberto) { animCartao = 0.0f; return; }
  dt = passo(&ultimoQuadro);
  animCartao = ajustes_animacoes_reduzidas() ? 1.0f : anim_mola(animCartao, 1.0f, dt, NV_MOLA_TELA);
  a = anim_suave(animCartao);
  est = celular_estado();
  u = celular_url();
  temQr = est == CEL_ESPERANDO && u[0];
  r = ancora[dono];
  w = CB_W; h = alturaCartao(temQr);
  // ANCORADO NO BOTAO: abaixo dele, com a borda direita alinhada a dele; sem
  // espaco embaixo, em cima; nunca fora da tela.
  x = r.x + r.w - w;
  y = r.y + r.h + CB_VAO;
  if (y + h > NV_TELA_H - 24.0f) y = r.y - CB_VAO - h;
  if (y < 24.0f) y = 24.0f;
  if (x < 32.0f) x = 32.0f;
  if (x + w > NV_TELA_W - 32.0f) x = NV_TELA_W - 32.0f - w;
  y += (1.0f - a) * -10.0f;
  cartao = (GfxRect){ x, y, w, h };

  // CAMADA: clique fora fecha, clique no cartao vencido gera outro codigo.
  ponteiro_camada();
  if (ponteiro_ativo()) {
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, fecharPonteiro, 0, 0);
    ponteiro_alvo(x, y, w, h, NULL, temQr ? NULL : regerarPonteiro, 0, 0);
  }
  ajustes_acento(&ar, &ag, &ab);
  gfx_rect((GfxRect){ x - 40.0f, y - 20.0f, w + 80.0f, h + 70.0f }, 0, GFX_SOMBRA,
           1.0f, 0, 0, 0.5f, 0, 0, 0, 0.55f * a);
  { float raio = 26.0f / (w < h ? w : h);
    if (ajustes_vidro()) { gfx_cor(cartao, raio, 0.03f, 0.032f, 0.04f, 0.70f * a); gfx_vidro_folha(cartao, raio, a); }
    else gfx_cor(cartao, raio, 0.075f, 0.078f, 0.090f, 0.98f * a);
    gfx_vidro_aro(cartao, raio, 1.5f, 1.0f, 1.0f, 1.0f, 0.12f * a); }

  { TxtLinha t = txt_linha(TXT_HEADLINE, i18n("Digitar pelo celular"), 245, 248, 255, 255);
    gfx_icone((GfxRect){ x + CB_PAD, y + CB_PAD + (t.h - 30.0f) * 0.5f, 30.0f, 30.0f }, "aj_smartphone",
              ar, ag, ab, a);
    txt_desenhar_alpha(t, x + CB_PAD + 42.0f, y + CB_PAD, a); }
  y += CB_PAD + CB_TITULO_H;

  if (!temQr) {
    const char *m = est == CEL_FALHOU ? "Endereço bloqueado por tentativas erradas. Aperte OK para gerar outro."
                  : est == CEL_EXPIROU ? "O endereço expirou. Aperte OK para gerar outro."
                  : "Sem rede local. Conecte a TV ao Wi-Fi e aperte OK.";
    txt_bloco(TXT_BODY, i18n(m), 200, 204, 212, x + CB_PAD, y + 16.0f, w - 2 * CB_PAD, CB_LINHA_H + 6.0f, a, 3);
    return;
  }
  y += 18.0f;
  qrTextura(u);
  if (texQr) {
    float qx = x + (w - CB_QR) * 0.5f;
    gfx_cor((GfxRect){ qx - CB_MOLDURA, y, CB_QR + 2 * CB_MOLDURA, CB_QR + 2 * CB_MOLDURA }, 0.06f,
            1.0f, 1.0f, 1.0f, a);
    gfx_tex_aspect_atual = 0.0f;
    gfx_rect((GfxRect){ qx, y + CB_MOLDURA, CB_QR, CB_QR }, texQr, GFX_SNAP, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, a);
  }
  y += CB_QR + 2 * CB_MOLDURA + 18.0f;
  // ENDERECO CURTO, para quem nao tem camera: sem "http://" (o navegador do
  // celular completa sozinho), centrado.
  curta = !strncmp(u, "http://", 7) ? u + 7 : u;
  { TxtLinha t = txt_linha_corta(TXT_CAPTION, curta, 232, 236, 244, 255, w - 2 * CB_PAD);
    txt_desenhar_alpha(t, x + (w - t.w) * 0.5f, y, a); }
  y += 30.0f + 10.0f;
  txt_bloco(TXT_CAPTION2, i18n("Aponte a câmera do celular. Mesma rede Wi-Fi, vale por 5 minutos."),
            160, 164, 175, x + CB_PAD, y, w - 2 * CB_PAD, CB_LINHA_H, a * 0.9f, 2);
}
