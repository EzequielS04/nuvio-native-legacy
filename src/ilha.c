// A ilha do relogio — ver ilha.h.
//
// CUSTO, porque a C9 e o teto: por quadro sao no maximo uma sombra do tamanho
// da pilula (+ folga), a pilula, uma luz de canto do tamanho dela, um icone e
// duas linhas de texto (as do relogio so durante os 320 ms da virada do
// minuto). Nada de tela cheia, nada de FBO, nenhuma textura nova alem das de
// texto — que o cache de text.c ja guarda por string.
#include "ilha.h"
#include "ilha_voo.h"
#include "ajustes.h"
#include "anim.h"
#include "botoes.h"
#include "ponteiro.h"
#include "tex_cache.h"
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

enum { M_RELOGIO = 0, M_AVISO, M_ATIVIDADE, M_CARTAO };

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

// Cartoes persistentes (ilha.h) e o que a pilula mostra deles.
static IlhaCartao cartoes[ILHA_N_CARTOES];
static int temCartao[ILHA_N_CARTOES];
static int cartaoVez = -1;              // o da vez neste quadro (-1 = nenhum)
static IlhaCartao mostraC;              // o DESENHADO (pode atrasar o da vez)
static int mostraQual;
static int coberta;
static GfxRect ultRect;
static int ultRectOk;

// O modal: a pilula cresce ate ele (modalT 0 -> 1, a mesma mola da forma).
static int modalAberto, modalQual, modalFoco;
static IlhaCartao modalC;
static float modalT, modalV, modalFocoA[3];
static Uint32 modalDesde;
static int pedido, pedidoQual;
static IlhaCartao pedidoC;

// MINIMIZAR NA ILHA (ilha_minimizar): o quadro do video encolhe ate a mini capa.
static int voo;                  // 1 = em voo
static float vooT;              // 0 -> 1, sem repique
static Uint32 vooDesde;
static GfxRect vooAlvo;
static int vooAlvoOk;
static char vooArte[1024], vooCapa[1024];
// DISSOLVER (Android, sessao retida): o video parado continua no plano de
// baixo, entao o primeiro quadro do voo e ELE (a tela inteira transparente) e
// a arte + home entram por cima em VOO_DISSOLVE_MS, ja encolhendo. Sem isso o
// voo nascia com o still em tela cheia: um corte do quadro do filme para o
// fundo do titulo. (Copiar o quadro real com PixelCopy levou 603-724 ms na
// TCL, 02/10: lento demais para a saida.)
#define VOO_DISSOLVE_MS 150u
static int vooDissolve;
static Uint32 pousouEm;          // o pulso da pilula conta daqui
static Uint32 altBase;           // a alternancia dos cartoes conta daqui

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

void ilha_cartao(int qual, const IlhaCartao *c) {
  if (qual < 0 || qual >= ILHA_N_CARTOES) return;
  if (!c) { temCartao[qual] = 0; return; }
  cartoes[qual] = *c;
  temCartao[qual] = 1;
}

void ilha_cartao_invalidar(int qual) {
  if (qual < 0 || qual >= ILHA_N_CARTOES) return;
  temCartao[qual] = 0;
  memset(&cartoes[qual], 0, sizeof cartoes[qual]);
  if (cartaoVez == qual) cartaoVez = -1;
  // A pilula conserva uma copia durante a troca de conteudo. Ela tambem
  // pertence a quem saiu e nao pode dissolver sobre a tela da outra pessoa.
  if (mostraQual == qual) {
    if (mostra == M_CARTAO) { mostra = -1; conteudoA = 0.0f; mostraChave[0] = 0; }
    memset(&mostraC, 0, sizeof mostraC);
  }
  if (modalQual == qual) {
    ilha_modal_fechar(1);
    memset(&modalC, 0, sizeof modalC);
  }
  if (pedido && pedidoQual == qual) {
    pedido = 0;
    memset(&pedidoC, 0, sizeof pedidoC);
  }
  if (qual == ILHA_VIVO) {
    voo = vooDissolve = vooAlvoOk = 0; pousouEm = 0;
    vooArte[0] = vooCapa[0] = 0;
  }
}

// QUAL CARTAO E O DA VEZ. Com os dois, alternam a cada ILHA_ALTERNA_MS: a
// sessao interrompida e a estreia sao as duas "o que eu faco agora", e nenhuma
// deve esconder a outra para sempre. Um so: ele.
static int cartaoDaVez(Uint32 agora) {
  int q[ILHA_N_CARTOES], n = 0, i;
  for (i = 0; i < ILHA_N_CARTOES; i++) if (temCartao[i]) q[n++] = i;
  if (!n) return -1;
  return q[((agora - altBase) / ILHA_ALTERNA_MS) % (unsigned)n];
}

int ilha_cartao_na_tela(void) { return relogioQuer && cartaoVez >= 0; }

void ilha_coberta(int c) { coberta = c; }

int ilha_rect(float *x, float *y, float *w, float *h) {
  if (!ultRectOk) return 0;
  *x = ultRect.x; *y = ultRect.y; *w = ultRect.w; *h = ultRect.h;
  return 1;
}

// --- o modal ---------------------------------------------------------------------
static int nBotoes(void) { return 3; }
static const char *rotuloBotao(int i) {
  if (i == 0) return modalQual == ILHA_ESTREIA ? "Assistir" : "Retomar";
  if (i == 1) return "Detalhes";
  return modalQual == ILHA_ESTREIA ? "Marcar como visto" : "Fechar";
}
static const char *iconeBotao(int i) {
  if (i == 0) return "play";
  if (i == 1) return "aj_info";
  return modalQual == ILHA_ESTREIA ? "visto" : NULL;
}

int ilha_modal_abrir(void) {
  if (modalAberto || cartaoVez < 0 || !relogioQuer) return 0;
  modalAberto = 1;
  modalQual = cartaoVez;
  modalC = cartoes[cartaoVez];
  modalFoco = 0;
  modalDesde = SDL_GetTicks();
  memset(modalFocoA, 0, sizeof modalFocoA);
  return 1;
}

void ilha_modal_fechar(int seco) {
  modalAberto = 0;
  if (seco) { modalT = 0.0f; modalV = 0.0f; }
}

int ilha_modal_aberto(void) { return modalAberto; }
int ilha_modal_visivel(void) { return modalAberto || modalT > 0.01f; }

static void pedir(int o) {
  pedido = o; pedidoC = modalC; pedidoQual = modalQual;
}

int ilha_pediu(IlhaCartao *c, int *qual) {
  int o = pedido;
  if (!o) return 0;
  pedido = 0;
  if (c) *c = pedidoC;
  if (qual) *qual = pedidoQual;
  return o;
}

static void acionar(int i) {
  if (i == 0) { pedir(ILHA_PEDIU_TOCAR); ilha_modal_fechar(0); }
  else if (i == 1) { pedir(ILHA_PEDIU_DETALHES); ilha_modal_fechar(0); }
  else {
    // "Fechar" na atividade ao vivo e "Marcar como visto" na estreia tiram o
    // cartao: o modal recolhe para uma pilula que ja nao o tem.
    pedir(ILHA_PEDIU_DISPENSAR);
    temCartao[modalQual] = 0;
    ilha_modal_fechar(0);
  }
}

int ilha_evento(const SDL_Event *e) {
  SDL_Keycode k;
  int sc;
  if (!modalAberto) return 0;
  if (e->type != SDL_KEYDOWN) return e->type == SDL_KEYUP;
  k = e->key.keysym.sym; sc = e->key.keysym.scancode;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE || sc == NV_SCANCODE_BACK) {
    ilha_modal_fechar(0);
    return 1;
  }
  if (e->key.repeat && k != SDLK_LEFT && k != SDLK_RIGHT) return 1;
  if (k == SDLK_LEFT) { if (modalFoco > 0) modalFoco--; return 1; }
  // A DIREITA DO ULTIMO BOTAO (ou a AZUL de novo) e o painel de Salvos, que
  // nasce do proprio modal. A seta para o lado e o gesto natural: o painel
  // mora a direita da tela.
  if (k == SDLK_RIGHT) {
    if (modalFoco + 1 < nBotoes()) modalFoco++;
    else pedir(ILHA_PEDIU_SALVOS);
    return 1;
  }
  // A MESMA JANELA DE 400 ms do atalho em app.c: o controle manda a AZUL
  // segurada como KEYDOWNs separados, e o segundo levaria direto ao painel.
  if (k == SDLK_s || sc == NV_SCANCODE_BLUE) {
    if (SDL_GetTicks() - modalDesde >= 400u) pedir(ILHA_PEDIU_SALVOS);
    return 1;
  }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) { acionar(modalFoco); return 1; }
  return 1;
}

// Magic Remote: passar por cima foca, o clique chega como OK (ponteiro.h).
static void pontFoco(int i, int b) { (void)b; modalFoco = i; }
static void pontFora(int a, int b) { (void)a; (void)b; ilha_modal_fechar(0); }
static void pontSalvos(int a, int b) { (void)a; (void)b; if (modalAberto) pedir(ILHA_PEDIU_SALVOS); }
static void pontPilula(int a, int b) { (void)a; (void)b; ilha_modal_abrir(); }

void ilha_ancorar(float x, float y, int daDireita) {
  ancDef = 1; ancX = x; ancY = y; ancDir = daDireita;
}

// Canto escolhido em Ajustes > Aparencia > Posicao do relogio. O Guia tem o
// titulo a esquerda e fica sempre a direita. Fora dele: 0 = Automatica e 2 =
// Direita vao a DIREITA em qualquer layout (padrao do dono desde a 1.7.2; era
// esquerda, e direita so na Dinamica); 1 = Esquerda, que no layout Dinamica
// vai AO LADO da pilula da barra (o canto dela).
void ilha_posicionar(int guia) {
  int pos = ajustes_relogio_pos();
  if (guia || pos != 1) ilha_ancorar(NV_TELA_W - NV_ILHA_MARGEM_D, NV_ILHA_Y, 1);
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
// w mais baixo = mais devagar. Pilula ~0,45 s ate assentar; modal ~0,7 s
// (o dono achou 15 rad/s rapido demais: "ta abrindo muito rapido a ilha").
#define ILHA_MOLA_W   10.0f
#define ILHA_MOLA_Z   0.72f
#define MODAL_MOLA_W  7.5f
#define MODAL_MOLA_Z  0.80f
static float molaIlhaWZ(float *v, float x, float alvo, float dt, float w, float z) {
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
static float molaIlha(float *v, float x, float alvo, float dt) {
  return molaIlhaWZ(v, x, alvo, dt, ILHA_MOLA_W, ILHA_MOLA_Z);
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

// --- o cartao na pilula --------------------------------------------------------
#define CT_TIT_MAX 300.0f
#define CT_CAPA_W   30.0f
#define CT_CAPA_H   44.0f
#define CT_VAO      14.0f
typedef struct { TxtLinha hora, tit, meta; } LinhasCartao;

// "T1E3 · 32 min restantes" / "T2E5 · hoje". O FORMATO passa por i18n inteiro
// quando existe; na estreia sao duas partes ja traduzidas juntadas por " · ",
// que nao e palavra.
static void metaCartao(const IlhaCartao *c, int qual, char *b, size_t n) {
  if (qual == ILHA_ESTREIA) {
    char te[32] = "";
    if (c->serie && c->t > 0 && c->e > 0) snprintf(te, sizeof te, i18n("T%dE%d"), c->t, c->e);
    if (te[0] && c->quando[0]) snprintf(b, n, "%s · %s", te, c->quando);
    else snprintf(b, n, "%s", te[0] ? te : c->quando);
    return;
  }
  { int m = c->restanteMin < 1 ? 1 : c->restanteMin;
    if (c->serie && c->t > 0 && c->e > 0) snprintf(b, n, i18n("T%dE%d · %d min restantes"), c->t, c->e, m);
    else snprintf(b, n, i18n("%d min restantes"), m); }
}

static float larguraCartao(const IlhaCartao *c, int qual, LinhasCartao *L) {
  char meta[96];
  float w;
  L->hora = txt_linha(TXT_PG_RELOGIO, hora, 244, 245, 248, 255);
  L->tit = txt_linha_corta(TXT_BODY, c->titulo, 240, 242, 246, 255, CT_TIT_MAX);
  metaCartao(c, qual, meta, sizeof meta);
  L->meta = txt_linha(TXT_CAPTION2, meta, 176, 180, 190, 255);
  w = (float)L->hora.w + CT_VAO * 2.0f + 1.5f + (qual == ILHA_VIVO ? CT_CAPA_W : ICONE) + VAO +
      (float)L->tit.w + 10.0f + (float)L->meta.w;
  if (qual == ILHA_ESTREIA) w += 12.0f + 10.0f;   // o ponto de nao lido
  return w;
}

// Onde a mini capa fica numa pilula de retangulo r: o mesmo passo a passo de
// desenharCartao, e o alvo do voo (ilha_minimizar).
static GfxRect capaNaPilula(const IlhaCartao *c, GfxRect r) {
  LinhasCartao L;
  float cw = larguraCartao(c, ILHA_VIVO, &L);
  float x = r.x + (r.w - cw) * 0.5f + (float)L.hora.w + CT_VAO + 1.5f + CT_VAO;
  return (GfxRect){ x, r.y + r.h * 0.5f - CT_CAPA_H * 0.5f, CT_CAPA_W, CT_CAPA_H };
}

static void desenharCartao(const IlhaCartao *c, int qual, GfxRect r, float a) {
  LinhasCartao L;
  float cw = larguraCartao(c, qual, &L);
  float x = r.x + (r.w - cw) * 0.5f, yc = r.y + r.h * 0.5f, cr, cg, cb, xTexto;
  int barra = qual == ILHA_VIVO && c->progresso >= 0.0f;
  float yt = barra ? yc - 3.0f : yc;
  if (a < 0.01f) return;
  ajustes_acento(&cr, &cg, &cb);
  txt_desenhar_alpha(L.hora, x, yc - (float)L.hora.h * 0.5f, a);
  x += (float)L.hora.w + CT_VAO;
  // Fio entre o relogio e o cartao: e a mesma pilula, com duas coisas dentro.
  gfx_cor((GfxRect){ x, yc - 13.0f, 1.5f, 26.0f }, 0.0f, 1.0f, 1.0f, 1.0f, 0.22f * a);
  x += 1.5f + CT_VAO;
  if (qual == ILHA_VIVO) {
    GfxRect capa = { x, yc - CT_CAPA_H * 0.5f, CT_CAPA_W, CT_CAPA_H };
    GLuint tex = c->poster[0] ? tex_obter_larg(c->poster, CT_CAPA_W) : 0;
    if (voo) {
      // Em voo a capa e o quadro que esta pousando: desenhar as duas dobraria.
    } else if (tex) {
      gfx_tex_aspect_atual = tex_aspecto(c->poster);
      gfx_rect(capa, tex, GFX_CARD, 0.0f, 0.0f, 0.0f, 6.0f / CT_CAPA_H, 0, 0, 0, a);
      gfx_tex_aspect_atual = 0.0f;
    } else gfx_cor(capa, 6.0f / CT_CAPA_H, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, a);
    x += CT_CAPA_W + VAO;
  } else {
    gfx_icone((GfxRect){ x, yc - ICONE * 0.5f, ICONE, ICONE }, "lembrete", cr, cg, cb, a);
    x += ICONE + VAO;
  }
  xTexto = x;
  txt_desenhar_alpha(L.tit, x, yt - (float)L.tit.h * 0.5f, a);
  x += (float)L.tit.w + 10.0f;
  txt_desenhar_alpha(L.meta, x, yt - (float)L.meta.h * 0.5f, a);
  x += (float)L.meta.w;
  if (qual == ILHA_ESTREIA)
    gfx_cor((GfxRect){ x + 12.0f, yc - 5.0f, 10.0f, 10.0f }, 0.5f, cr, cg, cb, a);
  if (barra) {
    // A BARRA FINA vai sob o texto (nao sob a capa): ela mede o titulo.
    float pr = c->progresso > 1.0f ? 1.0f : c->progresso;
    GfxRect trilho = { xTexto, r.y + r.h - 12.0f, x - xTexto, 3.0f };
    gfx_cor(trilho, 0.5f, 1.0f, 1.0f, 1.0f, 0.14f * a);
    if (trilho.w * pr > 3.0f)
      gfx_cor((GfxRect){ trilho.x, trilho.y, trilho.w * pr, 3.0f }, 0.5f, cr, cg, cb, a);
  }
}

// --- o modal -----------------------------------------------------------------------
// Um cartao de 1120 x 414: a arte do episodio (16:9) a esquerda, logo ou
// titulo, T/E e nome, sinopse curta e o tempo a direita, os botoes embaixo.
// Mesmo canto da pilula: ancorada a esquerda cresce para a direita e para
// baixo, ancorada a direita cresce para a esquerda.
#define MD_W      1120.0f
#define MD_H       414.0f
#define MD_PAD      32.0f
#define MD_ARTE_W  480.0f
#define MD_ARTE_H  270.0f
#define MD_RAIO     30.0f
#define MD_LOGO_W  380.0f
#define MD_LOGO_H   80.0f

static GfxRect modalAlvo(GfxRect p, int dir) {
  GfxRect m = { dir ? p.x + p.w - MD_W : p.x, p.y, MD_W, MD_H };
  if (m.x + m.w > NV_TELA_W - 40.0f) m.x = NV_TELA_W - 40.0f - m.w;
  if (m.x < 40.0f) m.x = 40.0f;
  return m;
}

static void desenharModal(GfxRect m, float a) {
  const IlhaCartao *c = &modalC;
  float ax = m.x + MD_PAD, ay = m.y + MD_PAD;
  float cx = ax + MD_ARTE_W + 32.0f, cw = m.x + m.w - MD_PAD - cx;
  float y = ay, by = ay + MD_ARTE_H + 24.0f, sy, cr, cg, cb;
  int i;
  if (a < 0.01f) return;
  ajustes_acento(&cr, &cg, &cb);
  // A arte: o still do episodio quando ha, senao o fundo do titulo, senao o cartaz.
  { const char *arte = c->arte[0] ? c->arte : c->poster;
    GfxRect ra = { ax, ay, MD_ARTE_W, MD_ARTE_H };
    GLuint tex = arte[0] ? tex_obter_larg(arte, MD_ARTE_W) : 0;
    if (tex) {
      gfx_tex_aspect_atual = tex_aspecto(arte);
      gfx_rect(ra, tex, GFX_CARD, 0.0f, 0.0f, 0.0f, 18.0f / MD_ARTE_H, 0, 0, 0, a);
      gfx_tex_aspect_atual = 0.0f;
    } else gfx_cor(ra, 18.0f / MD_ARTE_H, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, a); }
  // Logo do titulo (como a pagina do titulo), ou o nome.
  { GLuint lt = c->logo[0] ? tex_obter_larg_qualquer(c->logo, MD_LOGO_W) : 0;
    float asp = lt ? tex_aspecto(c->logo) : 0.0f;
    if (lt && asp > 0.0f) {
      float h = MD_LOGO_H, w = h * asp;
      GfxModo md = tex_marca_escura(c->logo) ? GFX_MARCA : GFX_TEXTO;
      if (w > MD_LOGO_W) { w = MD_LOGO_W; h = w / asp; }
      gfx_tex_aspect_atual = 0.0f;
      gfx_rect((GfxRect){ cx, y + MD_LOGO_H - h, w, h }, lt, md, 0, 0, 0, 0.0f, 1, 1, 1, a);
      y += MD_LOGO_H;
    } else {
      TxtLinha t = txt_linha_corta(TXT_TITULO3, c->titulo, 246, 247, 252, 255, cw);
      txt_desenhar_alpha(t, cx, y, a);
      y += (float)t.h;
    } }
  y += 14.0f;
  if (c->serie && c->t > 0 && c->e > 0) {
    char b[200];
    TxtLinha t;
    if (c->epNome[0]) snprintf(b, sizeof b, i18n("T%dE%d · %s"), c->t, c->e, c->epNome);
    else snprintf(b, sizeof b, i18n("T%dE%d"), c->t, c->e);
    t = txt_linha_corta(TXT_CALLOUT, b, 226, 228, 234, 255, cw);
    txt_desenhar_alpha(t, cx, y, a);
    y += (float)t.h + 10.0f;
  }
  // A linha de estado fica na base da arte; a sinopse usa o que sobra entre.
  { char b[120];
    TxtLinha t;
    if (modalQual == ILHA_VIVO) {
      int mi = c->restanteMin < 1 ? 1 : c->restanteMin;
      snprintf(b, sizeof b, i18n("%d min restantes"), mi);
    } else if (c->quando[0]) snprintf(b, sizeof b, "%s · %s", i18n("Episódio novo"), c->quando);
    else snprintf(b, sizeof b, "%s", i18n("Episódio novo"));
    t = txt_linha(TXT_CAPTION, b, 200, 204, 212, 255);
    sy = ay + MD_ARTE_H - (float)t.h;
    if (modalQual == ILHA_ESTREIA) {
      gfx_cor((GfxRect){ cx, sy + (float)t.h * 0.5f - 5.0f, 10.0f, 10.0f }, 0.5f, cr, cg, cb, a);
      txt_desenhar_alpha(t, cx + 20.0f, sy, a);
    } else {
      float bx = cx + (float)t.w + 18.0f, bw = cx + cw - bx;
      float pr = c->progresso < 0.0f ? 0.0f : c->progresso > 1.0f ? 1.0f : c->progresso;
      txt_desenhar_alpha(t, cx, sy, a);
      if (bw > 40.0f) {
        GfxRect tr = { bx, sy + (float)t.h * 0.5f - 2.0f, bw, 4.0f };
        gfx_cor(tr, 0.5f, 1.0f, 1.0f, 1.0f, 0.14f * a);
        if (tr.w * pr > 4.0f) gfx_cor((GfxRect){ tr.x, tr.y, tr.w * pr, 4.0f }, 0.5f, cr, cg, cb, a);
      }
    } }
  if (c->sinopse[0]) {
    int linhas = (int)((sy - 12.0f - y) / 30.0f);
    if (linhas > 3) linhas = 3;
    if (linhas >= 1)
      txt_bloco_corta(TXT_CAPTION, c->sinopse, 176, 180, 190, cx, y, cw, 30.0f, a, linhas);
  }
  // Botoes. O ponteiro: o fundo inteiro fecha, o modal absorve, cada botao foca.
  if (a > 0.3f) {
    ponteiro_camada();
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, pontFora, 0, 0);
    ponteiro_alvo(m.x, m.y, m.w, m.h, NULL, NULL, 0, 0);
  }
  { float x = ax;
    for (i = 0; i < nBotoes(); i++) {
      const char *rot = rotuloBotao(i), *ic = iconeBotao(i);
      float w = botao_largura(rot, ic, i == 0);
      GfxRect r = { x, by, w, BOTAO_H_SECUNDARIO };
      botao_pilula(r, rot, ic, modalFocoA[i], i == 0, 0, a);
      if (a > 0.3f) ponteiro_alvo(r.x, r.y, r.w, r.h, pontFoco, NULL, i, 0);
      x += w + BOTAO_GAP;
    } }
  // "Salvos ›" na ponta: o lado para onde a seta leva.
  { TxtLinha t = txt_linha(TXT_CAPTION2, "Salvos", 176, 180, 190, 255);
    TxtLinha v = txt_linha(TXT_CALLOUT, "›", 176, 180, 190, 255);
    float xr = m.x + m.w - MD_PAD, yc = by + BOTAO_H_SECUNDARIO * 0.5f;
    float x0 = xr - (float)v.w - 8.0f - (float)t.w;
    txt_desenhar_alpha(t, x0, yc - (float)t.h * 0.5f, a * 0.9f);
    txt_desenhar_alpha(v, xr - (float)v.w, yc - (float)v.h * 0.5f - 2.0f, a * 0.9f);
    if (a > 0.3f) ponteiro_alvo(x0 - 10.0f, by, xr - x0 + 20.0f, BOTAO_H_SECUNDARIO, NULL, pontSalvos, 0, 0); }
}

// --- minimizar na ilha ----------------------------------------------------------
// Pedido do dono (02/10): "quando sair do filme, minimizasse para a ilha do
// relogio e voltasse para a home". O plano de video e hardware e nao se le de
// volta (LG), entao a transicao usa a arte do modal (still do
// episodio ou fundo do titulo): nasce em tela cheia e encolhe numa mola de 560 ms (ilha_voo.h)
// ate o retangulo exato da mini capa da pilula, onde troca para o
// cartaz que a capa mostra. A home aparece por tras com o veu preto apagando.
//
// CUSTO: a arte (1 quad), o cartaz no fim (1 quad, so no cruzamento), uma
// sombra do tamanho dela e o veu em ate 4 faixas AO REDOR da arte — nunca por
// baixo dela, entao em pixel nao ha uma tela cheia a mais sobre a home.
int ilha_minimizar(const char *fundoReserva) {
  const IlhaCartao *c = &cartoes[ILHA_VIVO];
  // relogioQuer ainda e o do ultimo quadro desenhado (o da pagina, antes do
  // player): quem vale e o do proximo, conferido em ilha_desenhar.
  if (!temCartao[ILHA_VIVO] || !ajustes_relogio_ligado()) return 0;
  altBase = SDL_GetTicks();
  if (anim_politica_reduzida || ajustes_animacoes_reduzidas()) { voo = 0; return 1; }
  snprintf(vooArte, sizeof vooArte, "%s", c->arte[0] ? c->arte : fundoReserva ? fundoReserva : "");
  // A arte do episodio pode nunca ter sido decodificada nesta sessao (serie
  // aberta pelo Continuar): o fundo do titulo, que a pagina acabou de mostrar.
  if (vooArte[0] && !tex_obter_larg_qualquer(vooArte, 960.0f) && fundoReserva && fundoReserva[0])
    snprintf(vooArte, sizeof vooArte, "%s", fundoReserva);
  snprintf(vooCapa, sizeof vooCapa, "%s", c->poster);
  vooDissolve = 0;
  // vooDesde = 0: o relogio do voo comeca no primeiro quadro DESENHADO. O
  // ultimo quadro da ilha foi antes do player, e um dt de minutos daria o
  // primeiro passo inteiro de uma vez.
  voo = 1; vooT = 0.0f; vooDesde = 0; vooAlvoOk = 0; pousouEm = 0;
  printf("[ilha] minimizar: %s -> mini capa\n", c->imdb);
  return 1;
}

int ilha_minimizando(void) { return voo; }

void ilha_minimizar_dissolver(int sim) {
  if (voo) vooDissolve = sim ? 1 : 0;
  if (voo && sim) printf("[ilha] minimizar: dissolve a partir do video parado\n");
}

static void vooFim(const char *por, Uint32 agora) {
  if (!voo) return;
  voo = 0;
  altBase = agora;
  pousouEm = strcmp(por, "pousou") ? 0 : agora ? agora : 1;
  printf("[ilha] minimizar: fim (%s, %u ms)\n", por, vooDesde ? (unsigned)(agora - vooDesde) : 0u);
}

static float suave01(float a, float b, float x) {
  float t = (x - a) / (b - a);
  t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
  return t * t * (3.0f - 2.0f * t);
}

// O retangulo do quadro no instante t: tamanho e centro na MESMA fracao, em
// linha reta. A curva acelera e desacelera sem atravessar o destino: tamanho
// e centro chegam juntos, sem um cartaz grande sobre o texto da pilula.
static GfxRect vooRect(GfxRect alvo, float t, float *f) {
  return ilha_voo_rect(alvo, t, NV_TELA_W, NV_TELA_H, f);
}

// Veu: o preto do player apagando, so ao redor da arte.
static void vooVeu(GfxRect q, float a) {
  float W0 = (float)NV_TELA_W, H0 = (float)NV_TELA_H;
  float x0 = q.x < 0.0f ? 0.0f : q.x, x1 = q.x + q.w > W0 ? W0 : q.x + q.w;
  float y0 = q.y < 0.0f ? 0.0f : q.y, y1 = q.y + q.h > H0 ? H0 : q.y + q.h;
  if (a < 0.01f) return;
  if (y0 > 0.0f) gfx_cor((GfxRect){ 0, 0, W0, y0 }, 0.0f, 0, 0, 0, a);
  if (y1 < H0)   gfx_cor((GfxRect){ 0, y1, W0, H0 - y1 }, 0.0f, 0, 0, 0, a);
  if (x0 > 0.0f) gfx_cor((GfxRect){ 0, y0, x0, y1 - y0 }, 0.0f, 0, 0, 0, a);
  if (x1 < W0)   gfx_cor((GfxRect){ x1, y0, W0 - x1, y1 - y0 }, 0.0f, 0, 0, 0, a);
}

// COVER FORCADO: o retangulo passa de 16:9 para a proporcao da capa no fim,
// e o GFX_CARD trocava cover por contain (com faixas cinza) assim que a
// moldura fugia 25% da arte (issue #89) — no meio do voo a imagem pulava para
// uma tarja. Aqui ela recorta sempre; nunca estica nem ganha faixa.
static void vooTexEm(GLuint tex, float asp, GfxRect q, float raio, float a) {
  float antes = gfx_card_forcar_cover_atual;
  gfx_tex_aspect_atual = asp;
  gfx_card_forcar_cover_atual = 1.0f;
  gfx_rect(q, tex, GFX_CARD, 0.0f, 0.0f, 0.0f, raio, 0, 0, 0, a);
  gfx_card_forcar_cover_atual = antes;
  gfx_tex_aspect_atual = 0.0f;
}
static void vooArteEm(const char *url, GfxRect q, float raio, float a) {
  GLuint tex = url[0] ? tex_obter_larg_qualquer(url, 960.0f) : 0;
  if (a < 0.01f) return;
  if (tex) vooTexEm(tex, tex_aspecto(url), q, raio, a);
  else gfx_cor(q, raio, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, a);
}

// fase 0 = o veu (vai por baixo da pilula), 1 = o quadro (por cima dela).
static void desenharVoo(GfxRect pilulaFinal, int fase) {
  GfxRect q;
  float f, t = vooT, raioPx, cruza;
  if (!vooAlvoOk) {
    vooAlvo = capaNaPilula(&cartoes[ILHA_VIVO], pilulaFinal);
    vooAlvoOk = 1;
  }
  q = vooRect(vooAlvo, t, &f);
  // Veu: no primeiro quadro e o preto exato em volta do video (a tarja do
  // player); escurece a home de leve e se desfaz antes do pouso.
  if (fase == 0) { float v = 1.0f - suave01(0.0f, 0.7f, t); vooVeu(q, v * v); return; }
  // Canto: reto no primeiro quadro (igual ao player), arredonda cedo e
  // assenta no da capa (6 px), em pixels e nunca acima de meia altura.
  raioPx = 6.0f * f + 30.0f * suave01(0.0f, 0.25f, f) * (1.0f - f);
  if (raioPx > q.h * 0.5f) raioPx = q.h * 0.5f;
  if (f > 0.02f)
    gfx_rect((GfxRect){ q.x - 18.0f, q.y - 8.0f, q.w + 36.0f, q.h + 40.0f }, 0, GFX_SOMBRA,
             1.0f, 0, 0, 0.5f, 0, 0, 0, 0.40f * suave01(0.0f, 0.3f, f));
  // O cartaz entra enquanto o quadro assume a forma da capa: no pouso ja e ele.
  cruza = vooCapa[0] ? suave01(0.72f, 0.98f, t) : 0.0f;
  if (cruza < 0.99f) vooArteEm(vooArte, q, raioPx / q.h, 1.0f);
  if (cruza > 0.0f) vooArteEm(vooCapa, q, raioPx / q.h, cruza);
  // Por ultimo: tudo o que ja esta no quadro (home, veu, arte) entra em
  // fracao `d`, e o resto e o video parado no plano de baixo.
  if (vooDissolve && vooDesde) {
    float d = (float)(SDL_GetTicks() - vooDesde) / (float)VOO_DISSOLVE_MS;
    if (d >= 1.0f) vooDissolve = 0;
    else gfx_dissolver_tela(d * d * (3.0f - 2.0f * d));
  }
}

static void vooPasso(Uint32 agora) {
  if (!voo) return;
  if (!vooDesde) { vooDesde = agora ? agora : 1; return; }
  vooT = ilha_voo_fracao(agora - vooDesde);
  if (vooT >= 1.0f) vooFim("pousou", agora);
}

void ilha_desenhar(Uint32 agora) {
  float dt = ultQuadro ? (float)(agora - ultQuadro) / 1000.0f : 1.0f / 60.0f;
  int alvo, vis, dir;
  float alvoW, alvoH, x, y;
  TxtLinha t1, t2;
  GfxRect vooPf;
  ultQuadro = agora;
  if (dt > 0.1f) dt = 0.1f;

  // Aviso vencido sai; o prazo so comeca a contar quando ele aparece.
  if (temCur && curAte && (Sint32)(agora - curAte) >= 0) proximo();
  if (temCur && !curAte) curAte = agora + cur.ms;
  atualizarHora();
  if (horaT < 1.0f) { horaT += dt / 0.32f; if (horaT > 1.0f) horaT = 1.0f; }

  cartaoVez = cartaoDaVez(agora);
  // EM VOO o cartao e o da sessao que acabou de sair; se ela sumiu, a pilula
  // saiu da tela ou o modal abriu por cima, o voo acaba seco.
  if (voo) {
    if (!temCartao[ILHA_VIVO]) vooFim("cartao", agora);
    else if (!relogioQuer && vooDesde && agora - vooDesde > 200u) vooFim("relogio", agora);
    else if (modalAberto) vooFim("modal", agora);
    else cartaoVez = ILHA_VIVO;
  }
  // O MODAL SO EXISTE COM O RELOGIO NA TELA. Saiu dela (o detalhe abriu pelo
  // "Retomar", outra camada entrou): some seco, sem recolher por cima dela.
  if (!relogioQuer && (modalAberto || modalT > 0.0f)) ilha_modal_fechar(1);
  alvo = temCur ? M_AVISO : atividadeViva(agora) ? M_ATIVIDADE
       : (relogioQuer && cartaoVez >= 0) ? M_CARTAO : M_RELOGIO;
  vis = alvo != M_RELOGIO || relogioQuer;
  // O que esta desenhado so troca quando o conteudo velho ja apagou: a pilula
  // muda de forma com o texto antigo saindo, e o novo entra com ela perto do
  // tamanho final — a troca nunca acontece com o texto cheio na tela.
  if (mostra < 0) { mostra = alvo; conteudoA = 0.0f; }
  { const char *ch = alvo == M_AVISO ? cur.chave : alvo == M_CARTAO ? cartoes[cartaoVez].chave : "";
    if (mostra != alvo || strcmp(mostraChave, ch)) {
      conteudoA = ajustes_animacoes_reduzidas() ? 0.0f : anim_mola(conteudoA, 0.0f, dt, 26.0f);
      if (conteudoA < 0.06f) {
        mostra = alvo; conteudoA = 0.0f;
        snprintf(mostraChave, sizeof mostraChave, "%s", ch);
        if (alvo == M_CARTAO) { mostraC = cartoes[cartaoVez]; mostraQual = cartaoVez; }
      }
    } else if (alvo == M_CARTAO) mostraC = cartoes[cartaoVez];   // tempo e barra ao vivo
  }

  alvoH = alvo == M_RELOGIO ? NV_ILHA_H : NV_ILHA_H_ABERTA;
  if (alvo == M_CARTAO) { LinhasCartao L; alvoW = PAD_E + PAD_D + larguraCartao(&cartoes[cartaoVez], cartaoVez, &L); }
  else alvoW = PAD_E + PAD_D + larguraConteudo(alvo, &t1, &t2);
  // Sumindo, ela encolhe para uma gota antes de apagar (e nasce dela).
  if (!vis) alvoW = alvoH = NV_ILHA_H * 0.6f;
  if (W <= 0.0f) { W = NV_ILHA_H * 0.6f; H = W; }
  W = molaIlha(&vW, W, alvoW, dt);
  H = molaIlha(&vH, H, alvoH, dt);
  A = anim_mola(A, vis ? 1.0f : 0.0f, dt, vis ? 9.0f : 12.0f);
  if (mostra == alvo && vis) {
    float perto = fabsf(W - alvoW) < 0.18f * alvoW ? 1.0f : 0.0f;
    conteudoA = anim_mola(conteudoA, perto, dt, 10.0f);
  }
  modalT = molaIlhaWZ(&modalV, modalT, modalAberto ? 1.0f : 0.0f, dt, MODAL_MOLA_W, MODAL_MOLA_Z);
  if (!modalAberto && modalT < 0.01f) { modalT = 0.0f; modalV = 0.0f; }
  for (int i = 0; i < 3; i++)
    modalFocoA[i] = anim_mola(modalFocoA[i], modalAberto && i == modalFoco ? 1.0f : 0.0f, dt, NV_MOLA_FOCO);
  // POSICAO, num ponto so (ilha_ancorar ou o padrao, que e a direita).
  if (ancDef) { x = ancX; y = ancY; dir = ancDir; }
  else { x = NV_TELA_W - NV_ILHA_MARGEM_D; y = NV_ILHA_Y; dir = 1; }
  // O alvo do voo e a pilula ASSENTADA (a mola da forma e mais rapida que a
  // do voo: quando o quadro pousa, ela ja esta la).
  { LinhasCartao Lv;
    float pw = voo ? PAD_E + PAD_D + larguraCartao(&cartoes[ILHA_VIVO], ILHA_VIVO, &Lv) : alvoW;
    GfxRect pf = { dir ? x - pw : x, y, pw, NV_ILHA_H_ABERTA };
    vooPasso(agora);
    if (A < 0.01f) {
      if (!vis) { mostra = alvo; conteudoA = 0.0f; W = H = NV_ILHA_H * 0.6f; vW = vH = 0.0f; }
      ancDef = 0; ultRectOk = 0; coberta = 0;
      if (voo) { desenharVoo(pf, 0); desenharVoo(pf, 1); }
      return;
    }
    if (voo) desenharVoo(pf, 0);
    vooPf = pf; }
  ancDef = 0;
  { float w = W < H ? H : W, h = H < 8.0f ? 8.0f : H;
    GfxRect r = { dir ? x - w : x, y, w, h }, R = r;
    float raio = 0.5f, cr, cg, cb, fundo = 0.80f, solido = 0.86f, aPil = 1.0f, aMod = 0.0f;
    // O MODAL E A MESMA PILULA CRESCIDA: um retangulo so que vai da forma da
    // pilula ate a do modal na mola subamortecida (o repique e o "pulo" da
    // Dynamic Island), com o raio em pixels indo de meia altura a MD_RAIO. O
    // texto da pilula apaga no comeco, o do modal entra no fim. Sao os mesmos
    // quads da pilula com outro tamanho: nenhuma camada de tela cheia.
    if (modalT > 0.0f) {
      GfxRect M = modalAlvo(r, dir);
      float t = modalT > 1.06f ? 1.06f : modalT, tr = t > 1.0f ? 1.0f : t, rpx;
      R.x = r.x + (M.x - r.x) * t; R.y = r.y + (M.y - r.y) * t;
      R.w = r.w + (M.w - r.w) * t; R.h = r.h + (M.h - r.h) * t;
      rpx = r.h * 0.5f + (MD_RAIO - r.h * 0.5f) * tr;
      // O raio e fracao da ALTURA do retangulo (o SDF de gfx.c mede por ela).
      raio = rpx / (R.h > 1.0f ? R.h : 1.0f);
      fundo = 0.80f + 0.08f * tr;
      solido = 0.86f + 0.08f * tr;
      aPil = 1.0f - modalT * 3.0f; if (aPil < 0.0f) aPil = 0.0f;
      aMod = (modalT - 0.55f) / 0.40f; aMod = aMod < 0.0f ? 0.0f : aMod > 1.0f ? 1.0f : aMod;
      if (anim_politica_reduzida || ajustes_animacoes_reduzidas()) { aPil = modalAberto ? 0.0f : 1.0f; aMod = modalAberto ? 1.0f : 0.0f; }
    }
    // A PILULA RECEBE O QUADRO: depois do pouso ela cresce ~6% e assenta
    // (ilha_voo_pulso), em volta do proprio centro. So o vidro; o conteudo
    // fica onde estava, para o texto nao tremer.
    if (pousouEm && modalT <= 0.0f && !anim_politica_reduzida && !ajustes_animacoes_reduzidas()) {
      unsigned d = agora - pousouEm;
      float k = ilha_voo_pulso(d);
      if (d >= NV_ILHA_PULSO_MS) pousouEm = 0;
      else { float cx = R.x + R.w * 0.5f, cy = R.y + R.h * 0.5f;
             R.w *= k; R.h *= k; R.x = cx - R.w * 0.5f; R.y = cy - R.h * 0.5f; }
    }
    ultRect = R; ultRectOk = 1;
    if (coberta) { coberta = 0; return; }
    // Sombra caida, curta: separa a pilula de arte clara sem virar halo. No
    // modal ela cresce junto (e o tamanho dele + folga, nunca a tela).
    { float k = modalT > 0.0f ? (modalT > 1.0f ? 1.0f : modalT) : 0.0f;
      gfx_rect((GfxRect){ R.x - 16.0f - 24.0f * k, R.y - 6.0f - 10.0f * k,
                          R.w + 32.0f + 48.0f * k, R.h + 34.0f + 50.0f * k }, 0, GFX_SOMBRA,
               1.0f, 0, 0, 0.5f, 0, 0, 0, (0.34f + 0.16f * k) * A); }
    if (ajustes_vidro()) gfx_vidro_painel(R, raio, fundo, A);
    else gfx_cor(R, raio, 0.055f, 0.058f, 0.068f, solido * A);
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
      gfx_luz_canto(R, raio, R.w * 0.25f, -R.h * 0.9f, R.w * 0.85f, cr, cg, cb, luz * A); }
    gfx_recorte(R.x + 6.0f, R.y, R.w - 12.0f, R.h);
    if (aPil > 0.0f) {
      if (mostra == M_CARTAO) desenharCartao(&mostraC, mostraQual, r, A * conteudoA * aPil);
      else desenharConteudo(mostra, r, A * conteudoA * aPil, agora);
    }
    if (aMod > 0.0f) desenharModal(modalAlvo(r, dir), A * aMod);
    gfx_sem_recorte();
    // Magic Remote: o clique na pilula com um cartao abre o modal.
    if (modalT <= 0.0f && mostra == M_CARTAO && A > 0.5f) ponteiro_alvo(r.x, r.y, r.w, r.h, NULL, pontPilula, 0, 0);
    if (voo) desenharVoo(vooPf, 1); }
}
