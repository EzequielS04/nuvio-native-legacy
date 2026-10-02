#include "seekr.h"
#include "seekrvtt.h"
#include "rede.h"
#include "gfx.h"
#include "jpegrapido.h"
#include <SDL2/SDL.h>
#ifndef __EMSCRIPTEN__
#include <SDL2/SDL_image.h>
#endif
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static char     chave[96];
static unsigned geracao;
static int      estado = SEEKR_DESLIGADO;
static SeekrVtt vtt;
// O que ja foi pedido, para a mesma combinacao nao gastar outra consulta da
// cota (o player re-chama a cada troca de estado).
static char pedImdb[64]; static int pedT, pedE; static long pedDur;

// FOLHA DECODIFICADA: uma so, a ultima. Uma folha serve dezenas de cues
// seguidas, e procurar no filme anda quase sempre dentro dela; decodificar de
// novo a cada cue seria um JPEG inteiro por tecla.
static int           folhaIdx = -1;
static unsigned      folhaG;
static SDL_Surface  *folhaSup;

// Recorte pronto, esperando o upload no fio de desenho.
static unsigned char *prontoPx; static int prontoW, prontoH, prontoCue;
static unsigned       prontoG;
static int            emVoo;       // 1 = um fio de recorte trabalhando

// Lado do desenho (so o fio de desenho mexe).
static GLuint   tex; static int texCue = -1; static unsigned texG;

void seekr_definir_chave(const char *c) {
  pthread_mutex_lock(&trava);
  snprintf(chave, sizeof chave, "%s", c ? c : "");
  // Chave nova: o que foi recusado ou achado com a anterior nao vale mais.
  geracao++; estado = SEEKR_DESLIGADO; pedImdb[0] = 0;
  pthread_mutex_unlock(&trava);
}
int seekr_tem_chave(void) {
  int r; pthread_mutex_lock(&trava); r = chave[0] != 0; pthread_mutex_unlock(&trava);
  return r;
}
int seekr_estado(void) {
  int r; pthread_mutex_lock(&trava); r = estado; pthread_mutex_unlock(&trava);
  return r;
}

static void soltarFolha(void) {
  if (folhaSup) SDL_FreeSurface(folhaSup);
  folhaSup = NULL; folhaIdx = -1;
}

// Chamar com a trava.
static void limparTudo(void) {
  seekr_vtt_liberar(&vtt);
  soltarFolha();
  free(prontoPx); prontoPx = NULL;
  pedImdb[0] = 0;
}

void seekr_desligar(void) {
  pthread_mutex_lock(&trava);
  geracao++; estado = SEEKR_DESLIGADO;
  limparTudo();
  pthread_mutex_unlock(&trava);
}

typedef struct { char url[512]; char cab[160]; unsigned g; } Pedido;

static void *consultar(void *u) {
  Pedido *p = u;
  const char *cabs[2] = { p->cab, NULL };
  int st = 0, n = 0, novo = SEEKR_SEM_PREVIA;
  char vttUrl[2300], *corpo, *txt = NULL;
  SeekrVtt v; memset(&v, 0, sizeof v);
  corpo = rede_baixar_st(p->url, 12, cabs, &st);
  if (st == 401 || st == 403) novo = SEEKR_CHAVE_RECUSADA;
  else if (st == 200 && corpo && seekr_ler_lookup(corpo, vttUrl, sizeof vttUrl - 8)) {
    // st=1: o servico ja entrega os tempos na linha do tempo do cliente (aplica
    // o `scale` de conversao de quadros), como o SDK oficial pede.
    strcat(vttUrl, strchr(vttUrl, '?') ? "&st=1" : "?st=1");
    txt = rede_baixar(vttUrl, 15);    // sem chave: o VTT e assinado
    n = txt ? seekr_vtt_ler(txt, &v) : 0;
    if (n > 0) novo = SEEKR_PRONTO;
  }
  // Nunca a URL inteira no log: o VTT e as folhas levam assinatura.
  printf("[seekr] /sprites http=%d cues=%d folhas=%d\n", st, n, v.nFolhas);
  fflush(stdout);
  free(corpo); free(txt);
  pthread_mutex_lock(&trava);
  if (p->g == geracao) {
    seekr_vtt_liberar(&vtt); soltarFolha();
    vtt = v; memset(&v, 0, sizeof v);
    estado = novo;
  }
  pthread_mutex_unlock(&trava);
  seekr_vtt_liberar(&v);
  free(p);
  return NULL;
}

void seekr_pedir(const char *imdb, int t, int e, long durMs) {
  Pedido *p; pthread_t fio;
  char url[512];
  if (!seekr_url_lookup(url, sizeof url, imdb, t, e, durMs)) { seekr_desligar(); return; }
  pthread_mutex_lock(&trava);
  if (!chave[0]) { pthread_mutex_unlock(&trava); return; }
  // Mesma combinacao: nada a fazer. A duracao entra com folga de 2 s — o
  // pipeline refina o numero nos primeiros segundos e cada refino seria uma
  // consulta a mais da cota.
  if (!strcmp(pedImdb, imdb) && pedT == t && pedE == e &&
      labs(pedDur - durMs) < 2000) { pthread_mutex_unlock(&trava); return; }
  p = calloc(1, sizeof *p);
  if (!p) { pthread_mutex_unlock(&trava); return; }
  snprintf(pedImdb, sizeof pedImdb, "%s", imdb); pedT = t; pedE = e; pedDur = durMs;
  geracao++; estado = SEEKR_BUSCANDO;
  seekr_vtt_liberar(&vtt); soltarFolha();
  free(prontoPx); prontoPx = NULL;
  snprintf(p->url, sizeof p->url, "%s", url);
  snprintf(p->cab, sizeof p->cab, "X-API-Key: %s", chave);
  p->g = geracao;
  pthread_mutex_unlock(&trava);
  if (pthread_create(&fio, NULL, consultar, p) == 0) pthread_detach(fio);
  else free(p);
}

// --- recorte -----------------------------------------------------------------

static SDL_Surface *decodificar(const unsigned char *b, long n) {
  SDL_Surface *s, *c;
  int w0 = 0, h0 = 0;
  // Largura pedida enorme = sem reducao de DCT: o recorte precisa do pixel
  // inteiro (o quadro ja e pequeno, 320x180).
  s = jpeg_rapido_carregar_mem(b, (size_t)n, 1 << 20, &w0, &h0);
#ifndef __EMSCRIPTEN__
  if (!s) { SDL_RWops *rw = SDL_RWFromConstMem(b, (int)n); s = rw ? IMG_Load_RW(rw, 1) : NULL; }
#endif
  if (!s) return NULL;
  if (s->format->format == SDL_PIXELFORMAT_ABGR8888) return s;
  c = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ABGR8888, 0);
  SDL_FreeSurface(s);
  return c;
}

typedef struct { char url[2300]; int folha, cue, x, y, w, h; unsigned g; } Recorte;

static void *recortar(void *u) {
  Recorte *r = u;
  SDL_Surface *s = NULL;
  unsigned char *px = NULL;
  int precisa;
  pthread_mutex_lock(&trava);
  precisa = !(folhaSup && folhaIdx == r->folha && folhaG == r->g);
  if (!precisa) s = folhaSup;
  pthread_mutex_unlock(&trava);
  if (precisa) {
    long n = 0;
    char *b = rede_baixar_bin(r->url, 15, &n);   // assinada, sem chave
    s = b && n > 0 ? decodificar((unsigned char *)b, n) : NULL;
    free(b);
    printf("[seekr] folha %d: %dx%d\n", r->folha, s ? s->w : 0, s ? s->h : 0);
    fflush(stdout);
    pthread_mutex_lock(&trava);
    if (s && r->g == geracao) { soltarFolha(); folhaSup = s; folhaIdx = r->folha; folhaG = r->g; }
    else if (s) { SDL_FreeSurface(s); s = NULL; }
    pthread_mutex_unlock(&trava);
  }
  pthread_mutex_lock(&trava);
  // Recorta DENTRO da trava: a folha pode ser trocada por outro pedido.
  if (s && s == folhaSup && r->g == geracao && r->x + r->w <= s->w && r->y + r->h <= s->h &&
      (px = malloc((size_t)r->w * (size_t)r->h * 4u)) != NULL) {
    int y;
    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);
    for (y = 0; y < r->h; y++)
      memcpy(px + (size_t)y * (size_t)r->w * 4u,
             (const unsigned char *)s->pixels + (size_t)(r->y + y) * (size_t)s->pitch + (size_t)r->x * 4u,
             (size_t)r->w * 4u);
    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);
    free(prontoPx);
    prontoPx = px; prontoW = r->w; prontoH = r->h; prontoCue = r->cue; prontoG = r->g;
  }
  emVoo = 0;
  pthread_mutex_unlock(&trava);
  free(r);
  return NULL;
}

GLuint seekr_quadro(double posSeg, double *cueSeg) {
  int i;
  Recorte *pedir = NULL;
  unsigned char *px = NULL; int pw = 0, ph = 0, pc = -1; unsigned pg = 0;
  unsigned g;
  pthread_mutex_lock(&trava);
  g = geracao;
  if (estado != SEEKR_PRONTO) { pthread_mutex_unlock(&trava); return 0; }
  i = seekr_vtt_cue(&vtt, (long)(posSeg * 1000.0));
  if (i < 0) { pthread_mutex_unlock(&trava); return 0; }
  if (cueSeg) *cueSeg = vtt.cues[i].ini / 1000.0;
  if (prontoPx) { px = prontoPx; pw = prontoW; ph = prontoH; pc = prontoCue; pg = prontoG; prontoPx = NULL; }
  if (!emVoo && !(texG == g && texCue == i) && !(px && pg == g && pc == i)) {
    const SeekrCue *c = &vtt.cues[i];
    pedir = calloc(1, sizeof *pedir);
    if (pedir) {
      snprintf(pedir->url, sizeof pedir->url, "%s", vtt.folhas[c->folha]);
      pedir->folha = c->folha; pedir->cue = i;
      pedir->x = c->x; pedir->y = c->y; pedir->w = c->w; pedir->h = c->h;
      pedir->g = g; emVoo = 1;
    }
  }
  pthread_mutex_unlock(&trava);
  if (pedir) {
    pthread_t fio;
    if (pthread_create(&fio, NULL, recortar, pedir) == 0) pthread_detach(fio);
    else { free(pedir); pthread_mutex_lock(&trava); emVoo = 0; pthread_mutex_unlock(&trava); }
  }
  if (px) {
    if (pg == g) {
      if (!tex) glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_2D, tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pw, ph, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      gfx_tex_esquecer(0);
      texCue = pc; texG = pg;
    }
    free(px);
  }
  // A anterior enquanto a nova nao chega — mas so do MESMO titulo.
  return (tex && texG == g) ? tex : 0;
}

int seekr_validar(const char *c) {
  char cab[160]; const char *cabs[2] = { cab, NULL };
  int st = 0, r;
  char *corpo;
  if (!c || !*c) return 0;
  snprintf(cab, sizeof cab, "X-API-Key: %s", c);
  corpo = rede_baixar_st("https://api.seekr.tv/v1/keys/validate", 10, cabs, &st);
  if (!corpo || st == 0) r = -1;
  else r = strstr(corpo, "\"valid\":true") ? 1 : 0;
  printf("[seekr] validar http=%d -> %d\n", st, r); fflush(stdout);
  free(corpo);
  return r;
}
