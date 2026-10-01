// A ilha do relogio — ver ilha.h.
//
// CUSTO, porque a C9 e o teto: por quadro sao no maximo uma sombra do tamanho
// da pilula (+ folga), a pilula, uma luz de canto do tamanho dela, um icone e
// duas linhas de texto (as do relogio so durante os 320 ms da virada do
// minuto). Nada de tela cheia, nada de FBO, nenhuma textura nova alem das de
// texto — que o cache de text.c ja guarda por string.
#include "ilha.h"
#include "ajustes.h"
#include "anim.h"
#include "gfx.h"
#include "layout.h"
#include "menu.h"
#include "salvosintro.h"
#include "text.h"
#include "idioma.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum { M_RELOGIO = 0, M_AVISO, M_ATIVIDADE };

typedef struct {
  char chave[32], icone[32], texto[160];
  int tipo, tecla;
  unsigned ms;
} Aviso;

#define FILA 4
static Aviso fila[FILA];
static int nFila;
static Aviso cur;               // o da tela (valido se temCur)
static int temCur;
static Uint32 curAte;           // 0 = ainda nao apareceu (o prazo conta do 1o quadro)

static char atvTexto[160];
static float atvProg = -1.0f;
static Uint32 atvVisto;         // SDL_GetTicks da ultima renovacao

static int relogioQuer;
static int ancDef, ancDir;
static float ancX, ancY;

// Mola da forma e do conteudo.
static float W, vW, H, vH, A;
static float conteudoA;
static int   mostra = -1;               // o que esta DESENHADO (pode atrasar o alvo)
static char  mostraChave[32];
static Uint32 ultQuadro;

// Relogio e a virada do minuto.
static char hora[8], horaAnt[8];
static float horaT = 1.0f;
static time_t horaSeg;

void ilha_avisar(const char *chave, int tipo, const char *icone,
                 const char *texto, unsigned ms, int tecla) {
  static unsigned seq;
  Aviso a;
  int i;
  if (!texto || !texto[0]) return;
  memset(&a, 0, sizeof a);
  // Sem chave, uma propria: dois avisos avulsos nunca se fundem.
  if (chave && chave[0]) snprintf(a.chave, sizeof a.chave, "%s", chave);
  else snprintf(a.chave, sizeof a.chave, "#%u", ++seq);
  snprintf(a.icone, sizeof a.icone, "%s", icone ? icone : "");
  snprintf(a.texto, sizeof a.texto, "%s", texto);
  a.tipo = tipo; a.tecla = tecla; a.ms = ms ? ms : 4000u;
  // Mesma chave na tela: troca no lugar e renova o prazo.
  if (temCur && !strcmp(cur.chave, a.chave)) {
    cur = a;
    if (curAte) curAte = SDL_GetTicks() + a.ms;
    return;
  }
  for (i = 0; i < nFila; i++)
    if (!strcmp(fila[i].chave, a.chave)) { fila[i] = a; return; }
  if (!temCur) { cur = a; temCur = 1; curAte = 0; return; }
  // Fila cheia: o mais antigo da fila cede o lugar.
  if (nFila == FILA) { memmove(fila, fila + 1, sizeof fila[0] * (FILA - 1)); nFila--; }
  fila[nFila++] = a;
}

static void proximo(void) {
  temCur = 0;
  if (nFila > 0) {
    cur = fila[0];
    memmove(fila, fila + 1, sizeof fila[0] * (size_t)(nFila - 1));
    nFila--;
    temCur = 1; curAte = 0;
  }
}

void ilha_retirar(const char *chave) {
  int i, j;
  if (!chave || !chave[0]) return;
  for (i = j = 0; i < nFila; i++)
    if (strcmp(fila[i].chave, chave)) fila[j++] = fila[i];
  nFila = j;
  if (temCur && !strcmp(cur.chave, chave)) proximo();
}

int ilha_tem(const char *chave) {
  int i;
  if (!chave) return 0;
  if (temCur && !strcmp(cur.chave, chave)) return 1;
  for (i = 0; i < nFila; i++) if (!strcmp(fila[i].chave, chave)) return 1;
  return 0;
}

void ilha_atividade(const char *texto, float progresso) {
  snprintf(atvTexto, sizeof atvTexto, "%s", texto ? texto : "");
  atvProg = progresso;
  atvVisto = SDL_GetTicks();
  if (!atvVisto) atvVisto = 1;
}

void ilha_relogio_visivel(int visivel) { relogioQuer = visivel; }

void ilha_ancorar(float x, float y, int daDireita) {
  ancDef = 1; ancX = x; ancY = y; ancDir = daDireita;
}

// Canto escolhido em Ajustes > Aparencia > Posicao do relogio. O Guia tem o
// titulo a esquerda e fica sempre a direita. Fora dele: 2 = Direita; 1 =
// Esquerda, que no layout Dinamica vai AO LADO da pilula da barra (o canto
// dela); 0 = Automatica, o padrao de sempre (ver ilha_desenhar).
void ilha_posicionar(int guia) {
  int pos = ajustes_relogio_pos();
  if (guia || pos == 2) ilha_ancorar(NV_TELA_W - NV_ILHA_MARGEM_D, NV_ILHA_Y, 1);
  else if (pos == 1 && ajustes_home_layout() == HOME_LAYOUT_DINAMICA) {
    float px, py, pw, ph;
    if (menu_pilula_rect(&px, &py, &pw, &ph))
      ilha_ancorar(px + pw + NV_MENU_PILULA_VAO, py + (ph - NV_ILHA_H) * 0.5f, 0);
    else ilha_ancorar(ajustes_conteudo_x(), NV_ILHA_Y, 0);   // sem pilula na tela: canto livre
  } else if (pos == 1) ilha_ancorar(ajustes_conteudo_x(), NV_ILHA_Y, 0);
}

static int atividadeViva(Uint32 agora) {
  return atvVisto && atvTexto[0] && agora - atvVisto < 400u;
}

int ilha_ocupada(void) { return temCur || atividadeViva(SDL_GetTicks()); }

// MOLA SUBAMORTECIDA (zeta 0,68): passa um pouco do alvo e volta, que e o
// "pulo" da Dynamic Island. As molas de anim.h sao criticas de proposito (sem
// repique) — aqui o repique E o efeito, e so na forma, nunca no texto.
static float molaIlha(float *v, float x, float alvo, float dt) {
  const float w = 15.0f, z = 0.68f;
  int k;
  if (anim_politica_reduzida || ajustes_animacoes_reduzidas()) { *v = 0.0f; return alvo; }
  if (dt > 0.05f) dt = 0.05f;
  for (k = 0; k < 4; k++) {
    float h = dt * 0.25f, ac = w * w * (alvo - x) - 2.0f * z * w * (*v);
    *v += ac * h;
    x += *v * h;
  }
  return x;
}

static void corDoTipo(int tipo, float *r, float *g, float *b) {
  if (tipo == ILHA_ERRO) { *r = 1.0f; *g = 0.45f; *b = 0.42f; return; }
  if (tipo == ILHA_OK)   { *r = 0.40f; *g = 0.86f; *b = 0.56f; return; }
  ajustes_acento(r, g, b);
}

static const char *iconeDo(const Aviso *a) {
  if (a->icone[0]) return a->icone;
  if (a->tipo == ILHA_OK) return "check";
  if (a->tipo == ILHA_ERRO) return "aj_info";
  return "sino";
}

static void atualizarHora(void) {
  time_t t = time(NULL);
  struct tm lt;
  char h[8];
  if (t == horaSeg) return;
  horaSeg = t;
  if (!localtime_r(&t, &lt)) return;
  strftime(h, sizeof h, "%H:%M", &lt);
  if (strcmp(h, hora)) {
    if (hora[0] && !ajustes_animacoes_reduzidas()) {
      memcpy(horaAnt, hora, sizeof horaAnt);
      horaT = 0.0f;
    }
    memcpy(hora, h, sizeof hora);
  }
}

#define PAD_E   22.0f
#define PAD_D   24.0f
#define ICONE   28.0f
#define VAO     12.0f
#define TECLA   38.0f

// Largura do conteudo de cada modo (sem o recuo), e as linhas dele.
static float larguraConteudo(int m, TxtLinha *t1, TxtLinha *t2) {
  float w;
  t1->w = t1->h = 0; t2->w = t2->h = 0;
  if (m == M_RELOGIO) {
    *t1 = txt_linha(TXT_PG_RELOGIO, hora, 244, 245, 248, 255);
    return (float)t1->w;
  }
  if (m == M_AVISO) {
    *t1 = txt_linha_corta(TXT_BODY, cur.texto, 240, 242, 246, 255, NV_ILHA_TEXTO_MAX);
    w = ICONE + VAO + (float)t1->w;
    if (cur.tecla) {
      *t2 = txt_linha(TXT_CAPTION2, i18n("abre"), 176, 180, 190, 255);
      w += 18.0f + TECLA + 8.0f + (float)t2->w;
    }
    return w;
  }
  *t1 = txt_linha_corta(TXT_BODY, atvTexto, 236, 238, 244, 255, NV_ILHA_TEXTO_MAX);
  w = 12.0f + VAO + (float)t1->w;
  if (atvProg >= 0.0f) {
    char n[8];
    snprintf(n, sizeof n, "%d%%", (int)(atvProg * 100.0f + 0.5f));
    *t2 = txt_linha(TXT_CAPTION2, n, 176, 180, 190, 255);
    w += 14.0f + (float)t2->w;
  }
  return w;
}

static void desenharConteudo(int m, GfxRect r, float a, Uint32 agora) {
  TxtLinha t1, t2;
  float cw = larguraConteudo(m, &t1, &t2);
  // Centrado na pilula: durante a mola o conteudo nao fica grudado num lado.
  float x = r.x + (r.w - cw) * 0.5f, yc = r.y + r.h * 0.5f;
  if (a < 0.01f) return;
  if (m == M_RELOGIO) {
    if (horaT < 1.0f && horaAnt[0]) {
      // VIRADA DO MINUTO: o numero velho sobe e some, o novo vem de baixo.
      float e = anim_suave(horaT), d = 14.0f;
      TxtLinha v = txt_linha(TXT_PG_RELOGIO, horaAnt, 244, 245, 248, 255);
      txt_desenhar_alpha(v, r.x + (r.w - (float)v.w) * 0.5f, yc - (float)v.h * 0.5f - d * e, a * (1.0f - e));
      txt_desenhar_alpha(t1, x, yc - (float)t1.h * 0.5f + d * (1.0f - e), a * e);
    } else txt_desenhar_alpha(t1, x, yc - (float)t1.h * 0.5f, a);
    return;
  }
  if (m == M_AVISO) {
    float cr, cg, cb;
    corDoTipo(cur.tipo, &cr, &cg, &cb);
    gfx_icone((GfxRect){ x, yc - ICONE * 0.5f, ICONE, ICONE }, iconeDo(&cur), cr, cg, cb, a);
    x += ICONE + VAO;
    txt_desenhar_alpha(t1, x, yc - (float)t1.h * 0.5f, a);
    if (cur.tecla) {
      x += (float)t1.w + 18.0f;
      sintro_tecla_atalho(x, yc - TECLA * 0.5f, TECLA, a);
      txt_desenhar_alpha(t2, x + TECLA + 8.0f, yc - (float)t2.h * 0.5f, a);
    }
    return;
  }
  { float cr, cg, cb, p;
    ajustes_acento(&cr, &cg, &cb);
    // Ponto que respira: "esta acontecendo", sem girar nada.
    p = ajustes_animacoes_reduzidas() ? 1.0f
        : 0.55f + 0.45f * sinf((float)agora * (2.0f * 3.14159265f / 1200.0f));
    gfx_cor((GfxRect){ x, yc - 6.0f, 12.0f, 12.0f }, 0.5f, cr, cg, cb, a * p);
    x += 12.0f + VAO;
    txt_desenhar_alpha(t1, x, yc - (float)t1.h * 0.5f, a);
    if (atvProg >= 0.0f) {
      float pr = atvProg > 1.0f ? 1.0f : atvProg;
      GfxRect trilho = { r.x + PAD_E, r.y + r.h - 9.0f, r.w - PAD_E - PAD_D, 3.0f };
      txt_desenhar_alpha(t2, x + (float)t1.w + 14.0f, yc - (float)t2.h * 0.5f, a);
      gfx_cor(trilho, 0.5f, 1.0f, 1.0f, 1.0f, 0.12f * a);
      if (trilho.w * pr > 3.0f)
        gfx_cor((GfxRect){ trilho.x, trilho.y, trilho.w * pr, 3.0f }, 0.5f, cr, cg, cb, a);
    } }
}

void ilha_desenhar(Uint32 agora) {
  float dt = ultQuadro ? (float)(agora - ultQuadro) / 1000.0f : 1.0f / 60.0f;
  int alvo, vis, dir;
  float alvoW, alvoH, x, y;
  TxtLinha t1, t2;
  ultQuadro = agora;
  if (dt > 0.1f) dt = 0.1f;

  // Aviso vencido sai; o prazo so comeca a contar quando ele aparece.
  if (temCur && curAte && (Sint32)(agora - curAte) >= 0) proximo();
  if (temCur && !curAte) curAte = agora + cur.ms;
  atualizarHora();
  if (horaT < 1.0f) { horaT += dt / 0.32f; if (horaT > 1.0f) horaT = 1.0f; }

  alvo = temCur ? M_AVISO : atividadeViva(agora) ? M_ATIVIDADE : M_RELOGIO;
  vis = alvo != M_RELOGIO || relogioQuer;
  // O que esta desenhado so troca quando o conteudo velho ja apagou: a pilula
  // muda de forma com o texto antigo saindo, e o novo entra com ela perto do
  // tamanho final — a troca nunca acontece com o texto cheio na tela.
  if (mostra < 0) { mostra = alvo; conteudoA = 0.0f; }
  { const char *ch = alvo == M_AVISO ? cur.chave : "";
    if (mostra != alvo || strcmp(mostraChave, ch)) {
      conteudoA = ajustes_animacoes_reduzidas() ? 0.0f : anim_mola(conteudoA, 0.0f, dt, 26.0f);
      if (conteudoA < 0.06f) {
        mostra = alvo; conteudoA = 0.0f;
        snprintf(mostraChave, sizeof mostraChave, "%s", ch);
      }
    } }

  alvoH = alvo == M_RELOGIO ? NV_ILHA_H : NV_ILHA_H_ABERTA;
  alvoW = PAD_E + PAD_D + larguraConteudo(alvo, &t1, &t2);
  // Sumindo, ela encolhe para uma gota antes de apagar (e nasce dela).
  if (!vis) alvoW = alvoH = NV_ILHA_H * 0.6f;
  if (W <= 0.0f) { W = NV_ILHA_H * 0.6f; H = W; }
  W = molaIlha(&vW, W, alvoW, dt);
  H = molaIlha(&vH, H, alvoH, dt);
  A = anim_mola(A, vis ? 1.0f : 0.0f, dt, vis ? 14.0f : 18.0f);
  if (mostra == alvo && vis) {
    float perto = fabsf(W - alvoW) < 0.18f * alvoW ? 1.0f : 0.0f;
    conteudoA = anim_mola(conteudoA, perto, dt, 16.0f);
  }
  if (A < 0.01f) {
    if (!vis) { mostra = alvo; conteudoA = 0.0f; W = H = NV_ILHA_H * 0.6f; vW = vH = 0.0f; }
    ancDef = 0;
    return;
  }

  // POSICAO, num ponto so (ilha_ancorar ou o padrao).
  if (ancDef) { x = ancX; y = ancY; dir = ancDir; }
  else if (ajustes_home_layout() == HOME_LAYOUT_DINAMICA) { x = NV_TELA_W - NV_ILHA_MARGEM_D; y = NV_ILHA_Y; dir = 1; }
  else { x = ajustes_conteudo_x(); y = NV_ILHA_Y; dir = 0; }
  ancDef = 0;
  { float w = W < H ? H : W, h = H < 8.0f ? 8.0f : H;
    GfxRect r = { dir ? x - w : x, y, w, h };
    float raio = 0.5f, cr, cg, cb;
    // Sombra caida, curta: separa a pilula de arte clara sem virar halo.
    gfx_rect((GfxRect){ r.x - 16.0f, r.y - 6.0f, r.w + 32.0f, r.h + 34.0f }, 0, GFX_SOMBRA,
             1.0f, 0, 0, 0.5f, 0, 0, 0, 0.34f * A);
    if (ajustes_vidro()) gfx_vidro_painel(r, raio, 0.80f, A);
    else gfx_cor(r, raio, 0.055f, 0.058f, 0.068f, 0.86f * A);
    // O "vidro": um brilho largo e fraco por cima, branco no relogio e na cor
    // do aviso quando ele abre.
    if (mostra == M_AVISO) corDoTipo(cur.tipo, &cr, &cg, &cb);
    else { cr = cg = cb = 1.0f; }
    // Aberta, a luz RESPIRA a ~1 Hz (a mesma chamada do toast antigo): "tem
    // algo aqui" sem piscar, que num canto de TV le como defeito.
    { float luz = 0.07f;
      if (mostra == M_AVISO)
        luz = ajustes_animacoes_reduzidas() ? 0.20f
              : 0.14f + 0.10f * (0.5f + 0.5f * sinf((float)agora * (2.0f * 3.14159265f / 1100.0f)));
      gfx_luz_canto(r, raio, r.w * 0.25f, -r.h * 0.9f, r.w * 0.85f, cr, cg, cb, luz * A); }
    gfx_recorte(r.x + 6.0f, r.y, r.w - 12.0f, r.h);
    desenharConteudo(mostra, r, A * conteudoA, agora);
    gfx_sem_recorte(); }
}
