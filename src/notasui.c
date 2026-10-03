#include "notasui.h"
#include "ajustes.h"
#include "badges.h"
#include "idioma.h"
#include "layout.h"
#include "text.h"
#include "tex_cache.h"
#include "textogate.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// COMUM
// ---------------------------------------------------------------------------
static int virgula(void) { return !ajustes_idioma_ingles(); }

// Cor do texto sobre uma superficie colorida: escuro se ela for clara. O mesmo
// criterio de nf_cor_nota, para o numero nunca sumir dentro do quadrado.
static int tintaSobre(float r, float g, float b) {
  return (0.2126f * r + 0.7152f * g + 0.0722f * b) > 0.52f ? 20 : 250;
}

// Painel da secao: o mesmo degrau acima do fundo que as outras superficies da
// pagina de titulo (detail.c: moldura), ou o vidro quando a pessoa o ligou.
// GLASS UI (mockup "Detalhe", 03/10): o bloco .gl — branco a 5,5% no vidro,
// #15161A com a sombra curta no solido. Sem contorno.
static void painel(GfxRect r, float raio, float a) {
  float rf = r.h > 0.0f ? raio / r.h : 0.0f;
  float teto = r.h > 0.0f ? 0.5f * r.w / r.h : 0.5f;
  if (rf > 0.5f) rf = 0.5f;
  if (rf > teto) rf = teto;
  if (ajustes_vidro()) { gfx_cor(r, rf, 1, 1, 1, 0.055f * a); return; }
  gfx_rect((GfxRect){ r.x - 26.0f, r.y - 4.0f, r.w + 52.0f, r.h + 52.0f }, 0, GFX_SOMBRA,
           1.0f, 0, 0, 0.5f, 0, 0, 0, 0.45f * a);
  gfx_cor(r, rf, 0.082f, 0.086f, 0.102f, a);
}

static void pilula(float x, float yc, float w, float h, const char *txt,
                   float fr, float fg, float fb, float a) {
  GfxRect r = { x, yc - h * 0.5f, w, h };
  int c = tintaSobre(fr, fg, fb);
  TxtLinha l = txt_linha(TXT_MINI, txt, c, c, c, 255);
  gfx_cor(r, 0.32f, fr, fg, fb, a);
  txt_desenhar_alpha(l, x + (w - (float)l.w) * 0.5f, yc - (float)l.h * 0.5f, a);
}

// ---------------------------------------------------------------------------
// MARCAS
// ---------------------------------------------------------------------------
// Arquivo de marca (art/marcas/<nome>.png) de cada fonte, ou NULL quando a
// marca e desenhada aqui. O aspecto de reserva e o do PNG: a largura de um
// item da linha NAO PODE depender de a textura ja ter carregado, senao o
// encaixe decidiria diferente no primeiro quadro e no segundo.
typedef struct { const char *nome; float asp; } Png;

static Png pngDe(int f, int cru, int nalinha) {
  Png p = { NULL, 1.0f };
  switch (f) {
    // Tomate FRESCO de 60% para cima, RESPINGO abaixo — a convencao do proprio
    // site: o icone diz o veredito antes do numero.
    case EX_TOMATOES: p.nome = cru >= 600 ? "tomatoes_fresh" : "tomatoes_rotten";
                      p.asp = 0.99f; break;
    case EX_AUDIENCE: p.nome = "audience";   p.asp = 0.76f; break;
    case EX_TMDB:     p.nome = "tmdb";       p.asp = 1.40f; break;
    // Trakt: o WORDMARK na linha (o nome), o icone redondo nas linhas do
    // heatmap e nos cartoes.
    case EX_TRAKT:    p.nome = nalinha ? "trakt_wordmark" : "trakt";
                      p.asp = nalinha ? 2.62f : 1.0f; break;
    case EX_LETTERBOXD: p.nome = "letterboxd"; p.asp = 1.0f; break;
    case EX_IMDB:     p.nome = "imdb";       p.asp = 1.98f; break;
    case EX_METACRITIC: case EX_METAUSER:
                      p.nome = "metacritic"; p.asp = 1.0f; break;
    default: break;
  }
  return p;
}

// Desenha o PNG da marca em `r`. GFX_TEXTO preserva o RGB e usa o alfa: o
// GFX_SNAP ignora o alfa e o tomate saia num quadrado escuro. O wordmark do
// Trakt e escuro e vai por GFX_MARCA, que tinge o alfa.
static void desenhaPng(const char *nome, GfxRect r, float a) {
  const char *cam = extras_caminho_marca_nome(nome);
  GLuint t = tex_obter(cam);
  GfxModo m;
  if (!t) return;
  m = (!strcmp(nome, "trakt_wordmark") && tex_marca_escura(cam)) ? GFX_MARCA : GFX_TEXTO;
  gfx_rect(r, t, m, 0, 0, 0, 0, 0.93f, 0.94f, 0.96f, a);
}

// Largura real quando a textura existe, a reserva quando ainda nao.
static float larguraPng(const char *nome, float asp, float h) {
  float ap = tex_aspecto(extras_caminho_marca_nome(nome));
  float w = h * (ap > 0.0f ? ap : asp);
  return w > 110.0f ? 110.0f : w;
}

// Altura de cada marca na LINHA do titulo. O tomate e o popcorn sao icones
// altos; o wordmark do Trakt e o TMDB sao letreiros e ficam menores.
static float alturaLinha(int f) {
  switch (f) {
    case EX_TRAKT: return 22.0f;
    case EX_TMDB: return 34.0f;
    case EX_LETTERBOXD: return 28.0f;
    default: return 32.0f;
  }
}

// Marcas desenhadas (sem PNG): MyAnimeList, Roger Ebert, nota do MDBList.
static int marcaNativa(int f) { return f == EX_MAL || f == EX_EBERT || f == EX_MDBSCORE; }
static void corNativa(int f, float *r, float *g, float *b, const char **txt) {
  switch (f) {
    case EX_MAL:   *r = 0.180f; *g = 0.318f; *b = 0.635f; *txt = "MAL";   break;  // #2e51a2
    case EX_EBERT: *r = 0.950f; *g = 0.937f; *b = 0.902f; *txt = "EBERT"; break;  // papel
    default:       *r = 0.122f; *g = 0.710f; *b = 0.659f; *txt = "MDB";   break;  // #1fb5a8
  }
}
static float larguraNativa(int f) {
  const char *t; float r, g, b;
  corNativa(f, &r, &g, &b, &t);
  return (float)txt_largura(TXT_MINI, t) + 22.0f;
}

// ---------------------------------------------------------------------------
// LINHA DO TITULO
// ---------------------------------------------------------------------------
#define VAO_ITEM   24.0f     // entre uma nota e a seguinte (o de sempre)
#define GAP_MARCA  10.0f     // marca -> valor
#define ALT_QUADRO 34.0f     // quadrado do Metacritic
#define DIAM_USER  40.0f     // circulo do Metacritic de usuarios

static void textoLinha(int f, int cru, char *dst, size_t cap) {
  nf_texto(f, cru, virgula(), 0, dst, cap);
}

static float larguraItem(int f, int cru) {
  char v[24];
  int n100 = nf_norm100(f, cru);
  (void)n100;
  if (f == EX_IMDB) return badge_imdb_largura(cru);
  textoLinha(f, cru, v, sizeof v);
  if (f == EX_METACRITIC) {
    float w = (float)txt_largura(TXT_CAPTION2, v) + 14.0f;
    return w > ALT_QUADRO ? w : ALT_QUADRO;
  }
  if (f == EX_METAUSER) return DIAM_USER;
  { float lv = (float)txt_largura(TXT_DET_META2, v), lm;
    if (marcaNativa(f)) lm = larguraNativa(f);
    else { Png p = pngDe(f, cru, 1);
           lm = larguraPng(p.nome, p.asp, alturaLinha(f)); }
    return lm + GAP_MARCA + lv; }
}

void notasui_planejar(NotasPlano *p, const int cru[EX_NFONTES], float disp,
                      float leadPrimeiro, const unsigned char *querer) {
  int pos, i, n = 0;
  int fonte[EX_NFONTES], prio[EX_NFONTES];
  float larg[EX_NFONTES], comVao[EX_NFONTES];
  unsigned char manter[EX_NFONTES];
  memset(p, 0, sizeof *p);
  for (pos = 0; pos < EX_NFONTES; pos++) {
    int f = nf_na_posicao(pos);
    if (f < 0 || cru[f] <= 0) continue;
    if (querer ? !querer[f] : !ajustes_nota_titulo(f)) continue;
    fonte[n] = f;
    prio[n] = nf_prioridade(f);
    larg[n] = larguraItem(f, cru[f]);
    comVao[n] = larg[n] + VAO_ITEM;
    n++;
  }
  // Todo item leva o vao de 24; o primeiro leva `leadPrimeiro` no lugar dele.
  // Como quem e "primeiro" muda quando um cai, a diferenca vai para o espaco
  // livre de uma vez: (lead - 24) e o mesmo com qualquer um a frente.
  { float folga = leadPrimeiro - VAO_ITEM;
    nf_encaixar(comVao, prio, n, disp - (folga > 0.0f ? folga : 0.0f), manter); }
  for (i = 0; i < n; i++) {
    if (!manter[i]) continue;
    p->fonte[p->n] = fonte[i];
    p->cru[p->n]   = cru[fonte[i]];
    p->larg[p->n]  = larg[i];
    p->n++;
  }
}

static void desenhaItem(int f, int cru, float x, float yc, float a) {
  char v[24];
  textoLinha(f, cru, v, sizeof v);
  if (f == EX_IMDB) { badge_imdb(x, yc - BADGE_H * 0.5f, cru, 0, a); return; }
  if (f == EX_METACRITIC) {
    float r, g, b, w = larguraItem(f, cru);
    int c;
    TxtLinha l;
    nf_cor_metacritic(nf_norm100(f, cru), &r, &g, &b);
    c = tintaSobre(r, g, b);
    l = txt_linha(TXT_CAPTION2, v, c, c, c, 255);
    gfx_cor((GfxRect){ x, yc - ALT_QUADRO * 0.5f, w, ALT_QUADRO }, 0.18f, r, g, b, a);
    txt_desenhar_alpha(l, x + (w - (float)l.w) * 0.5f, yc - (float)l.h * 0.5f, a);
    return;
  }
  if (f == EX_METAUSER) {
    float r, g, b;
    int c;
    TxtLinha l;
    nf_cor_metacritic(nf_norm100(f, cru), &r, &g, &b);
    c = tintaSobre(r, g, b);
    l = txt_linha(TXT_MINI, v, c, c, c, 255);
    gfx_cor((GfxRect){ x, yc - DIAM_USER * 0.5f, DIAM_USER, DIAM_USER }, 0.5f, r, g, b, a);
    txt_desenhar_alpha(l, x + (DIAM_USER - (float)l.w) * 0.5f, yc - (float)l.h * 0.5f, a);
    return;
  }
  { float lm, h = alturaLinha(f);
    TxtLinha lv = txt_linha(TXT_DET_META2, v, 220, 220, 225, 255);
    if (marcaNativa(f)) {
      const char *t; float r, g, b;
      corNativa(f, &r, &g, &b, &t);
      lm = larguraNativa(f);
      pilula(x, yc, lm, 26.0f, t, r, g, b, a);
    } else {
      Png p = pngDe(f, cru, 1);
      lm = larguraPng(p.nome, p.asp, h);
      desenhaPng(p.nome, (GfxRect){ x, yc - h * 0.5f, lm, h }, a);
    }
    txt_desenhar_alpha(lv, x + lm + GAP_MARCA, yc - (float)lv.h * 0.5f, a);
  }
}

float notasui_desenhar_linha(const NotasPlano *p, float x, float yc, float a) {
  int i;
  for (i = 0; i < p->n; i++) {
    if (i) x += VAO_ITEM;
    desenhaItem(p->fonte[i], p->cru[i], x, yc, a);
    x += p->larg[i];
  }
  return x;
}

// ---------------------------------------------------------------------------
// CARTAO DA ABA (uma marca centralizada)
// ---------------------------------------------------------------------------
float notasui_marca_cartao(int f, int cru, float xc, float yc, float h, float a) {
  const char *t; float r, g, b, w;
  (void)cru; (void)h;
  if (!marcaNativa(f)) return 0.0f;
  corNativa(f, &r, &g, &b, &t);
  w = larguraNativa(f) + 8.0f;
  pilula(xc - w * 0.5f, yc, w, 30.0f, t, r, g, b, a);
  return w;
}

// ---------------------------------------------------------------------------
// SECAO "NOTAS"
// ---------------------------------------------------------------------------
#define SEC_W       1728.0f      // NV_TELA_W - 2 * NV_DETP_X
#define GRADE_ROT_W 64.0f
#define GRADE_MED_W 96.0f
#define GRADE_MAX_H 560.0f
// Os blocos .gl das notas (mockup "Detalhe"): grade 1fr 1fr com vao de 28,
// padding 34, anel de 170 (traco 15), linhas 150 | barra | 54 a 17 px.
#define GL_VAO       28.0f
#define GL_PAD       34.0f
#define GL_ANEL     170.0f
#define GL_TRACO     15.0f
#define GL_LINHA     37.0f
#define GL_ROT_W    150.0f
#define GL_VAL_W     54.0f

static TextoGate gateFontes, gateGrade;
void notasui_reiniciar(void) {
  textogate_reiniciar(&gateFontes);
  textogate_reiniciar(&gateGrade);
}

typedef struct {
  int nFontes;
  int fontes[EX_NFONTES];
  int norm[EX_NFONTES];
  int algumaNota;
  int comGrade;
  float cw, ch;               // celula da grade
  int nTempDesenho;           // temporadas que cabem (todas, salvo excesso)
  int maxEps;
  float hRows;                // bloco do heatmap por fonte + legenda
  float hA;                   // bloco A inteiro (max entre linhas e resumo)
  float hGrade;               // titulo + eixo + celulas + legenda
} Medidas;

#define GRADE_TIT_H 40.0f     // a frase de apoio (o titulo e o cabecalho da secao)
#define GRADE_EIXO_H 30.0f
#define GRADE_LEG_H 50.0f

static void medir(const NotasSecao *s, Medidas *m) {
  int pos, t;
  memset(m, 0, sizeof *m);
  for (pos = 0; pos < EX_NFONTES; pos++) {
    int f = nf_na_posicao(pos);
    if (f < 0 || s->cru[f] <= 0) continue;
    m->fontes[m->nFontes] = f;
    m->norm[m->nFontes] = nf_norm100(f, s->cru[f]);
    m->nFontes++;
  }
  m->algumaNota = m->nFontes > 0;
  if (m->nFontes) {
    // Os dois blocos do mockup lado a lado, na mesma altura: 34 de padding em
    // volta do anel de 170 ou das linhas (21 + 16 de vao cada), o que for maior.
    m->hRows = (float)m->nFontes * GL_LINHA - (GL_LINHA - 21.0f);
    m->hA = GL_PAD * 2.0f + (m->hRows > GL_ANEL ? m->hRows : GL_ANEL);
  }
  if (s->nTemp > 0 && s->nEps && s->epNota && s->tempNum && s->epNum) {
    int comNota = 0;
    m->maxEps = 0;
    for (t = 0; t < s->nTemp; t++) {
      int n = s->nEps(t), i;
      if (n > EX_EP_MAX) n = EX_EP_MAX;
      if (n > m->maxEps) m->maxEps = n;
      for (i = 0; i < n; i++) if (s->epNota(t, i) > 0) { comNota = 1; break; }
    }
    if (comNota && m->maxEps > 0) {
      float cw, ch;
      int nt = s->nTemp;
      if (!nf_grade_celula(nt, m->maxEps, SEC_W - GRADE_ROT_W - GRADE_MED_W,
                           GRADE_MAX_H, &cw, &ch)) {
        // Serie enorme (dezenas de temporadas): mostra as que cabem na altura
        // minima em vez de esconder a grade inteira.
        nt = (int)((GRADE_MAX_H + 4.0f) / 12.0f);
        if (!nf_grade_celula(nt, m->maxEps, SEC_W - GRADE_ROT_W - GRADE_MED_W,
                             GRADE_MAX_H, &cw, &ch)) { cw = 10.0f; ch = 8.0f; }
      }
      m->comGrade = 1;
      m->cw = cw; m->ch = ch;
      m->nTempDesenho = nt;
      m->hGrade = GRADE_TIT_H + GRADE_EIXO_H + (float)nt * (ch + 4.0f) - 4.0f
                + GRADE_LEG_H;
    }
  }
}

int notasui_fontes_tem(const NotasSecao *s) { Medidas m; medir(s, &m); return m.algumaNota; }
int notasui_grade_tem(const NotasSecao *s)  { Medidas m; medir(s, &m); return m.comGrade; }
float notasui_fontes_altura(const NotasSecao *s) { Medidas m; medir(s, &m); return m.hA; }
float notasui_grade_altura(const NotasSecao *s)  { Medidas m; medir(s, &m); return m.hGrade; }

static const char *rotulo(int f) {
  if (f == EX_METAUSER) return i18n("Metacritic (usuários)");
  if (f == EX_MDBSCORE) return i18n("Nota do MDBList");
  return nf_nome(f);
}

// Texto de uma linha, com a cor e a posicao pedidas.
static TxtLinha texto(TxtEstilo e, const char *s, int c) {
  return txt_linha(e, s, c, c, c, 255);
}

// Grade temporadas x episodios: uma celula por episodio, cor = nota.
static void desenhaGrade(const NotasSecao *s, const Medidas *m, float x, float y, float a) {
  int t, i;
  float gx = x + GRADE_ROT_W, gy = y + GRADE_TIT_H + GRADE_EIXO_H;
  float pas = m->ch + 4.0f, pasx = m->cw + 4.0f;
  int melhorT = -1, melhorI = -1, piorT = -1, piorI = -1, melhor = 0, pior = 1000;
  { TxtLinha l = texto(TXT_DET_META2, i18n("Cada quadrado é um episódio; a cor é a nota."), 150);
    txt_desenhar_alpha(l, x, y, a); }
  // Eixo: o numero do episodio. Todos se cabem; senao de 5 em 5.
  { int passoNum = m->cw >= 30.0f ? 1 : 5;
    for (i = 0; i < m->maxEps; i++) {
      char n[8];
      TxtLinha l;
      if (passoNum > 1 && (i + 1) % passoNum != 0 && i != 0) continue;
      snprintf(n, sizeof n, "%d", i + 1);
      l = texto(TXT_MINI, n, 150);
      txt_desenhar_alpha(l, gx + pasx * (float)i + (m->cw - (float)l.w) * 0.5f,
                         y + GRADE_TIT_H + 2.0f, a);
    } }
  for (t = 0; t < m->nTempDesenho; t++) {
    float ry = gy + pas * (float)t;
    int n = s->nEps(t), soma = 0, q = 0;
    if (n > EX_EP_MAX) n = EX_EP_MAX;
    // Fora da tela: nem o custo de percorrer os episodios.
    if (ry > NV_TELA_H || ry + m->ch < 0.0f) {
      for (i = 0; i < n; i++) {
        int d = s->epNota(t, i);
        if (d > 0) { soma += d; q++; if (d > melhor) { melhor = d; melhorT = t; melhorI = i; }
                     if (d < pior) { pior = d; piorT = t; piorI = i; } }
      }
      continue;
    }
    // Rotulo da temporada: todas quando ha altura; senao de 5 em 5.
    if (m->ch >= 18.0f || (t + 1) % 5 == 0 || t == 0) {
      char rot[12];
      TxtLinha l;
      snprintf(rot, sizeof rot, "T%d", s->tempNum(t));
      l = texto(m->ch >= 18.0f ? TXT_DET_META2 : TXT_MINI, rot, 190);
      txt_desenhar_alpha(l, x, ry + (m->ch - (float)l.h) * 0.5f, a);
    }
    for (i = 0; i < n; i++) {
      int d = s->epNota(t, i);
      float cr, cg, cb;
      GfxRect c = { gx + pasx * (float)i, ry, m->cw, m->ch };
      if (nf_cor_episodio(d, &cr, &cg, &cb)) {
        gfx_cor(c, 0.16f, cr, cg, cb, a);
        soma += d; q++;
        if (d > melhor) { melhor = d; melhorT = t; melhorI = i; }
        if (d < pior)   { pior = d; piorT = t; piorI = i; }
      } else {
        gfx_cor(c, 0.16f, 1, 1, 1, 0.06f * a);      // episodio sem nota
      }
    }
    if (q && m->ch >= 18.0f) {
      char v[8];
      TxtLinha l;
      { int mdec = (soma * 2 + q) / (q * 2);            // media em decimos, arredondada
        snprintf(v, sizeof v, virgula() ? "%d,%d" : "%d.%d", mdec / 10, mdec % 10); }
      l = texto(TXT_DET_META2, v, 200);
      txt_desenhar_alpha(l, gx + pasx * (float)m->maxEps + 16.0f,
                         ry + (m->ch - (float)l.h) * 0.5f, a);
    }
  }
  // Legenda da rampa (4,0 a 10) e melhor/pior episodio.
  { float ly = gy + pas * (float)m->nTempDesenho + 14.0f;
    int k, seg = 14;
    float sw = 280.0f / (float)seg;
    for (k = 0; k < seg; k++) {
      float cr, cg, cb;
      nf_cor_episodio(50 + (int)(50.0f * ((float)k + 0.5f) / (float)seg), &cr, &cg, &cb);
      gfx_cor((GfxRect){ gx + sw * (float)k, ly, sw + 0.5f, 10.0f }, 0, cr, cg, cb, a);
    }
    { TxtLinha l0 = texto(TXT_MINI, virgula() ? "5,0" : "5.0", 150), l1 = texto(TXT_MINI, "10", 150);
      txt_desenhar_alpha(l0, gx, ly + 14.0f, a);
      txt_desenhar_alpha(l1, gx + 280.0f - (float)l1.w, ly + 14.0f, a); }
    if (melhorT >= 0) {
      char b[96];
      float bx = gx + 340.0f;
      snprintf(b, sizeof b, i18n("Melhor: T%dE%d  ·  %.1f"), s->tempNum(melhorT),
               s->epNum(melhorT, melhorI), melhor / 10.0f);
      { TxtLinha l = texto(TXT_DET_META2, b, 225);
        txt_desenhar_alpha(l, bx, ly - 6.0f, a); bx += (float)l.w + 48.0f; }
      if (piorT >= 0 && (piorT != melhorT || piorI != melhorI)) {
        snprintf(b, sizeof b, i18n("Pior: T%dE%d  ·  %.1f"), s->tempNum(piorT),
                 s->epNum(piorT, piorI), pior / 10.0f);
        { TxtLinha l = texto(TXT_DET_META2, b, 225);
          txt_desenhar_alpha(l, bx, ly - 6.0f, a); }
      }
    } }
}

// Portao do texto (textogate.h) de um bloco: aparece INTEIRO ou nao aparece.
// Enquanto o rasterizador ainda deve linhas, desenha com opacidade quase nula —
// e isso que as rasteriza — e revela tudo junto num esvanecimento so. Fora da
// tela nao se consulta o portao: com nada pendente ele abriria de imediato, e o
// texto entraria em degraus quando a pagina rolasse ate aqui.
static float portao(TextoGate *g, float a, int *aberto) {
  *aberto = textogate_aberto(g);
  return *aberto ? a * textogate_passo(g, 0, SDL_GetTicks()) : NV_TXTGATE_AQUECER;
}

// Cor de marca da barra de cada fonte (o mockup pinta a barra na cor da casa,
// nao na rampa da nota): IMDb amarelo, Rotten vermelho-tomate, Metacritic
// verde, Trakt vermelho, Letterboxd azul. Sem marca propria: o acento.
static void corMarca(int f, float *r, float *g, float *b) {
  switch (f) {
    case EX_IMDB:       *r = 0.961f; *g = 0.773f; *b = 0.094f; return;   // #f5c518
    case EX_TOMATOES:   *r = 0.980f; *g = 0.353f; *b = 0.235f; return;   // #fa5a3c
    case EX_AUDIENCE:   *r = 0.980f; *g = 0.553f; *b = 0.235f; return;
    case EX_METACRITIC: case EX_METAUSER:
                        *r = 0.400f; *g = 0.800f; *b = 0.200f; return;   // #66cc33
    case EX_TRAKT:      *r = 0.929f; *g = 0.110f; *b = 0.141f; return;   // #ed1c24
    case EX_LETTERBOXD: *r = 0.251f; *g = 0.737f; *b = 0.957f; return;   // #40bcf4
    case EX_TMDB:       *r = 0.004f; *g = 0.706f; *b = 0.894f; return;   // #01b4e4
    case EX_MAL:        *r = 0.180f; *g = 0.318f; *b = 0.635f; return;
    case EX_MDBSCORE:   *r = 0.122f; *g = 0.710f; *b = 0.659f; return;
    default: ajustes_acento(r, g, b); return;
  }
}

// Anel de progresso: o trilho continuo (GFX_ANEL) e o arco no acento como
// discos encostados (nao ha modo de arco; 3 px de passo o deixa liso).
static void anelNota(float cx, float cy, float d, float traco, int pct, float a) {
  float ar, ag, ab, raio = (d - traco) * 0.5f;
  int k, n;
  gfx_rect((GfxRect){ cx - d * 0.5f, cy - d * 0.5f, d, d }, 0, GFX_ANEL, 0, traco / d, 0.0f,
           0.5f, 1, 1, 1, 0.08f * a);
  if (pct <= 0) return;
  if (pct > 100) pct = 100;
  ajustes_acento(&ar, &ag, &ab);
  n = (int)(2.0f * 3.14159265f * raio * (float)pct / 100.0f / 3.0f) + 1;
  for (k = 0; k <= n; k++) {
    float t = (float)pct / 100.0f * (float)k / (float)n;
    float ang = -1.57079633f + t * 6.28318531f;
    float px = cx + cosf(ang) * raio, py = cy + sinf(ang) * raio;
    gfx_cor((GfxRect){ px - traco * 0.5f, py - traco * 0.5f, traco, traco }, 0.5f, ar, ag, ab, a);
  }
}

// Bloco da esquerda: o anel com a media ("87 NUVIO") e as fontes em linhas
// de rotulo | barra na cor da marca | valor.
static void blocoFontes(const NotasSecao *s, const Medidas *m, GfxRect b, float a) {
  NfResumo r;
  int i;
  float cx = b.x + GL_PAD + GL_ANEL * 0.5f, cy = b.y + b.h * 0.5f;
  float lx = b.x + GL_PAD + GL_ANEL + 40.0f, lw = b.x + b.w - GL_PAD - lx;
  float ly = b.y + (b.h - m->hRows) * 0.5f;
  nf_resumo(m->fontes, m->norm, m->nFontes, &r);
  painel(b, 26.0f, a);
  anelNota(cx, cy, GL_ANEL, GL_TRACO, r.n > 0 ? r.media : m->norm[0], a);
  { char n[8]; TxtLinha big, rot;
    snprintf(n, sizeof n, "%d", r.n > 0 ? r.media : m->norm[0]);
    big = texto(TXT_G52B, n, 243);
    rot = txt_linha(TXT_AJ_CAPS13, "NUVIO", 243, 242, 239, 115);
    { float alt = (float)big.h + (float)rot.h - 8.0f, y0 = cy - alt * 0.5f;
      txt_desenhar_alpha(big, cx - (float)big.w * 0.5f, y0, a);
      txt_desenhar_alpha(rot, cx - (float)rot.w * 0.5f, y0 + (float)big.h - 8.0f, a); } }
  for (i = 0; i < m->nFontes; i++) {
    int f = m->fontes[i], n = m->norm[i];
    float yc = ly + (float)i * GL_LINHA + 10.5f, br, bg, bb;
    float tx = lx + GL_ROT_W + 14.0f, tw = lw - GL_ROT_W - 14.0f - 14.0f - GL_VAL_W;
    char v[24];
    TxtLinha lr = txt_linha_corta(TXT_ILHA_GENERO, rotulo(f), 243, 242, 239, 153, GL_ROT_W);
    txt_desenhar_alpha(lr, lx, yc - (float)lr.h * 0.5f, a);
    gfx_cor((GfxRect){ tx, yc - 4.0f, tw, 8.0f }, 0.5f, 1, 1, 1, 0.08f * a);
    corMarca(f, &br, &bg, &bb);
    { float fw = tw * (float)n / 100.0f;
      if (fw < 8.0f) fw = 8.0f;
      gfx_cor((GfxRect){ tx, yc - 4.0f, fw, 8.0f }, 0.5f, br, bg, bb, a); }
    nf_texto(f, s->cru[f], virgula(), 0, v, sizeof v);
    { TxtLinha lv = txt_linha(TXT_LOG_18B, v, 243, 242, 239, 255);
      txt_desenhar_alpha(lv, lx + lw - (float)lv.w, yc - (float)lv.h * 0.5f, a); }
  }
}

// Bloco da direita: o resumo das fontes no mesmo material — a media e de
// quantas, a crítica contra o publico, a faixa menor..maior com o ponto da
// media. (No mockup este lugar e "Entre os amigos"; sem notas de amigos, o
// app mostra o que sabe das fontes, com o mesmo cabecalho de bloco.)
static void blocoResumo(const Medidas *m, GfxRect b, float a) {
  NfResumo r;
  char buf[96];
  float px = b.x + GL_PAD, pw = b.w - GL_PAD * 2.0f, py = b.y + GL_PAD;
  nf_resumo(m->fontes, m->norm, m->nFontes, &r);
  painel(b, 26.0f, a);
  if (r.n == 0) {
    TxtLinha l = texto(TXT_ILHA_GENERO, i18n("Só a nota agregada chegou; não há o que comparar."), 179);
    txt_desenhar_alpha(l, px, py, a);
    return;
  }
  if (r.n > 1) snprintf(buf, sizeof buf, i18n("Média de %d fontes"), r.n);
  else         snprintf(buf, sizeof buf, "%s", i18n("Só uma fonte tem nota"));
  { TxtLinha lt = txt_linha_corta(TXT_G26B, buf, 243, 242, 239, 255, pw * 0.6f);
    txt_desenhar_alpha(lt, px, py, a);
    if (r.temDiff) {
      snprintf(buf, sizeof buf, i18n("Crítica %d  ·  Público %d"), r.criticos, r.publico);
      { TxtLinha ld = txt_linha(TXT_ILHA_APOIO, buf, 243, 242, 239, 115);
        txt_desenhar_alpha(ld, px + pw - (float)ld.w, py + (float)lt.h - (float)ld.h - 3.0f, a); }
    }
    py += (float)lt.h + 26.0f; }
  if (r.n > 1) {
    float ar, ag, ab, x0 = px + pw * (float)r.min / 100.0f, x1 = px + pw * (float)r.max / 100.0f;
    ajustes_acento(&ar, &ag, &ab);
    gfx_cor((GfxRect){ px, py, pw, 8.0f }, 0.5f, 1, 1, 1, 0.08f * a);
    gfx_cor((GfxRect){ x0, py, x1 - x0 > 8.0f ? x1 - x0 : 8.0f, 8.0f }, 0.5f, 1, 1, 1, 0.30f * a);
    gfx_cor((GfxRect){ px + pw * (float)r.media / 100.0f - 8.0f, py - 4.0f, 16.0f, 16.0f },
            0.5f, ar, ag, ab, a);
    py += 8.0f + 22.0f;
    snprintf(buf, sizeof buf, i18n("Menor: %s %d  ·  Maior: %s %d"),
             r.fonteMin == EX_METAUSER ? "Metacritic" : nf_nome(r.fonteMin), r.min,
             r.fonteMax == EX_METAUSER ? "Metacritic" : nf_nome(r.fonteMax), r.max);
    { TxtLinha l = txt_linha_corta(TXT_ILHA_GENERO, buf, 243, 242, 239, 153, pw);
      txt_desenhar_alpha(l, px, py, a); py += (float)l.h + 18.0f; }
  }
  if (r.temDiff) {
    int d = r.diff < 0 ? -r.diff : r.diff;
    const char *veredito;
    if (d <= 3) veredito = i18n("Crítica e público estão de acordo");
    else if (r.diff > 0) { snprintf(buf, sizeof buf, i18n("O público dá %d pontos a mais que a crítica"), d); veredito = buf; }
    else { snprintf(buf, sizeof buf, i18n("A crítica dá %d pontos a mais que o público"), d); veredito = buf; }
    { TxtLinha l = txt_linha_corta(TXT_G20B, veredito, 243, 242, 239, 255, pw);
      txt_desenhar_alpha(l, px, py, a); }
  }
}

float notasui_fontes_desenhar(const NotasSecao *s, float x, float y, float a) {
  Medidas m;
  int pend0 = txt_pendentes, aberto;
  float aa, w = (SEC_W - GL_VAO) * 0.5f;
  medir(s, &m);
  if (!m.algumaNota) return 0.0f;
  if (y >= NV_TELA_H || y + m.hA <= 0.0f) return m.hA;
  aa = portao(&gateFontes, a, &aberto);
  blocoFontes(s, &m, (GfxRect){ x, y, w, m.hA }, aa);
  blocoResumo(&m, (GfxRect){ x + w + GL_VAO, y, w, m.hA }, aa);
  if (!aberto) textogate_passo(&gateFontes, txt_pendentes - pend0, SDL_GetTicks());
  return m.hA;
}

float notasui_grade_desenhar(const NotasSecao *s, float x, float y, float a) {
  Medidas m;
  int pend0 = txt_pendentes, aberto;
  float aa;
  medir(s, &m);
  if (!m.comGrade) return 0.0f;
  if (y >= NV_TELA_H || y + m.hGrade <= 0.0f) return m.hGrade;
  aa = portao(&gateGrade, a, &aberto);
  desenhaGrade(s, &m, x, y, aa);
  if (!aberto) textogate_passo(&gateGrade, txt_pendentes - pend0, SDL_GetTicks());
  return m.hGrade;
}
