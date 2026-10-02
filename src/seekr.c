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
//
// MEDIDO em 02/10/2026 com a API real: a folha e 3200x1800 (10x10 quadros), o
// que decodificado da ~22 MB. Por isso ela so vive ENQUANTO a pessoa procura:
// seekr_ocioso() a solta quando o avanco termina, e fica so o JPEG (bem menor)
// para a proxima busca nao baixar de novo.
static int            folhaIdx = -1;
static unsigned       folhaG;
static SDL_Surface   *folhaSup;
static char          *jpgBytes; static long jpgN; static int jpgIdx = -1;
static unsigned       jpgG;

// Recortes prontos, esperando o upload no fio de desenho.
#define SK_SLOTS 4
typedef struct { unsigned char *px; int w, h, cue; unsigned g; } Pronto;
static Pronto prontos[SK_SLOTS];
static int    emVoo;       // 1 = um fio de recorte trabalhando
// Ajuste de sincronia em ms, somado a posicao antes da escolha da cue (o
// "preview sync" da documentacao).
static long   ajusteMs;

// Lado do desenho (so o fio de desenho mexe): texturas por cue, para a fita
// (anterior, atual, seguinte) nao refazer upload a cada quadro.
typedef struct { GLuint tex; int cue; unsigned g; Uint32 uso; } Slot;
static Slot slots[SK_SLOTS];
static Uint32 relogioUso;

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
static void soltarProntos(void) {
  int i;
  for (i = 0; i < SK_SLOTS; i++) { free(prontos[i].px); prontos[i].px = NULL; }
}
static void soltarJpg(void) { free(jpgBytes); jpgBytes = NULL; jpgN = 0; jpgIdx = -1; }

// Chamar com a trava.
static void limparTudo(void) {
  seekr_vtt_liberar(&vtt);
  soltarFolha(); soltarJpg(); soltarProntos();
  pedImdb[0] = 0;
}

void seekr_ocioso(void) {
  pthread_mutex_lock(&trava);
  if (!emVoo) soltarFolha();
  pthread_mutex_unlock(&trava);
}
void seekr_definir_ajuste_ms(long ms) {
  pthread_mutex_lock(&trava); ajusteMs = ms; pthread_mutex_unlock(&trava);
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
    seekr_vtt_liberar(&vtt); soltarFolha(); soltarJpg();
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
  seekr_vtt_liberar(&vtt); soltarFolha(); soltarJpg(); soltarProntos();
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
  pthread_mutex_unlock(&trava);
  if (precisa) {
    long n = 0;
    char *b = NULL;
    Uint32 t0 = SDL_GetTicks();
    int baixou = 0;
    // O JPEG desta folha ja esta guardado? Entao so decodifica.
    pthread_mutex_lock(&trava);
    if (jpgBytes && jpgIdx == r->folha && jpgG == r->g && (b = malloc((size_t)jpgN)) != NULL) {
      memcpy(b, jpgBytes, (size_t)jpgN); n = jpgN;
    }
    pthread_mutex_unlock(&trava);
    if (!b) { b = rede_baixar_bin(r->url, 15, &n); baixou = 1; }   // assinada, sem chave
    s = b && n > 0 ? decodificar((unsigned char *)b, n) : NULL;
    printf("[seekr] folha %d: %dx%d, %ld KB %s em %u ms\n", r->folha, s ? s->w : 0, s ? s->h : 0,
           n / 1024, baixou ? "baixada" : "do cache", SDL_GetTicks() - t0);
    fflush(stdout);
    pthread_mutex_lock(&trava);
    if (s && r->g == geracao) {
      soltarFolha(); folhaSup = s; folhaIdx = r->folha; folhaG = r->g;
      if (baixou) { soltarJpg(); jpgBytes = b; jpgN = n; jpgIdx = r->folha; jpgG = r->g; b = NULL; }
    } else if (s) { SDL_FreeSurface(s); s = NULL; }
    pthread_mutex_unlock(&trava);
    free(b);
  }
  pthread_mutex_lock(&trava);
  s = folhaSup;
  // Recorta DENTRO da trava: a folha pode ser trocada por outro pedido.
  if (s && folhaIdx == r->folha && r->g == geracao && r->x + r->w <= s->w && r->y + r->h <= s->h &&
      (px = malloc((size_t)r->w * (size_t)r->h * 4u)) != NULL) {
    int y, k, livre = 0;
    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);
    for (y = 0; y < r->h; y++)
      memcpy(px + (size_t)y * (size_t)r->w * 4u,
             (const unsigned char *)s->pixels + (size_t)(r->y + y) * (size_t)s->pitch + (size_t)r->x * 4u,
             (size_t)r->w * 4u);
    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);
    for (k = 0; k < SK_SLOTS; k++) if (!prontos[k].px) { livre = k; break; }
    if (k == SK_SLOTS) livre = 0;
    free(prontos[livre].px);
    prontos[livre] = (Pronto){ px, r->w, r->h, r->cue, r->g };
  }
  emVoo = 0;
  pthread_mutex_unlock(&trava);
  free(r);
  return NULL;
}

static int slotDe(int cue, unsigned g) {
  int i;
  for (i = 0; i < SK_SLOTS; i++) if (slots[i].tex && slots[i].cue == cue && slots[i].g == g) return i;
  return -1;
}
static int slotLivre(const int *proteger, int n) {
  int i, k, melhor = -1;
  for (i = 0; i < SK_SLOTS; i++) {
    int prot = 0;
    for (k = 0; k < n; k++) if (slots[i].cue == proteger[k]) prot = 1;
    if (prot && slots[i].tex) continue;
    if (melhor < 0 || slots[i].uso < slots[melhor].uso) melhor = i;
  }
  return melhor < 0 ? 0 : melhor;
}

int seekr_quadros(double posSeg, int n, GLuint *texs, double *cueSeg) {
  int quer[3], nq = 0, i, k, achou = 0;
  Recorte *pedir = NULL;
  Pronto pr[SK_SLOTS];
  unsigned g;
  if (n < 1) return 0;
  if (n > 3) n = 3;
  for (k = 0; k < n; k++) { texs[k] = 0; if (cueSeg) cueSeg[k] = -1.0; }
  pthread_mutex_lock(&trava);
  g = geracao;
  memcpy(pr, prontos, sizeof pr);
  memset(prontos, 0, sizeof prontos);
  if (estado != SEEKR_PRONTO) { pthread_mutex_unlock(&trava); for (k = 0; k < SK_SLOTS; k++) free(pr[k].px); return 0; }
  i = seekr_vtt_cue(&vtt, (long)(posSeg * 1000.0) + ajusteMs);
  if (i >= 0) {
    // Com fita: anterior, atual, seguinte (as que existem). A ATUAL vem
    // primeiro na fila de pedidos — e a que a pessoa esta olhando.
    if (n == 1) quer[nq++] = i;
    else {
      quer[nq++] = i;
      if (i > 0) quer[nq++] = i - 1;
      if (i + 1 < vtt.nCues) quer[nq++] = i + 1;
    }
    for (k = 0; k < nq; k++) {
      int c = quer[k], ja = slotDe(c, g), j, pend = 0;
      for (j = 0; j < SK_SLOTS; j++) if (pr[j].px && pr[j].g == g && pr[j].cue == c) pend = 1;
      if (ja < 0 && !pend && !emVoo && !pedir) {
        const SeekrCue *q = &vtt.cues[c];
        pedir = calloc(1, sizeof *pedir);
        if (pedir) {
          snprintf(pedir->url, sizeof pedir->url, "%s", vtt.folhas[q->folha]);
          pedir->folha = q->folha; pedir->cue = c;
          pedir->x = q->x; pedir->y = q->y; pedir->w = q->w; pedir->h = q->h;
          pedir->g = g; emVoo = 1;
        }
      }
    }
    if (cueSeg) {
      // Ordem de SAIDA: anterior, atual, seguinte (n == 3) ou so a atual.
      if (n == 1) cueSeg[0] = vtt.cues[i].ini / 1000.0;
      else {
        cueSeg[0] = i > 0 ? vtt.cues[i - 1].ini / 1000.0 : -1.0;
        cueSeg[1] = vtt.cues[i].ini / 1000.0;
        cueSeg[2] = i + 1 < vtt.nCues ? vtt.cues[i + 1].ini / 1000.0 : -1.0;
      }
    }
  }
  pthread_mutex_unlock(&trava);
  if (pedir) {
    pthread_t fio;
    if (pthread_create(&fio, NULL, recortar, pedir) == 0) pthread_detach(fio);
    else { free(pedir); pthread_mutex_lock(&trava); emVoo = 0; pthread_mutex_unlock(&trava); }
  }
  // Upload do que ficou pronto.
  for (k = 0; k < SK_SLOTS; k++) {
    if (!pr[k].px) continue;
    if (pr[k].g == g && slotDe(pr[k].cue, g) < 0) {
      int sl = slotLivre(quer, nq);
      if (!slots[sl].tex) glGenTextures(1, &slots[sl].tex);
      glBindTexture(GL_TEXTURE_2D, slots[sl].tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pr[k].w, pr[k].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pr[k].px);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      gfx_tex_esquecer(0);
      slots[sl].cue = pr[k].cue; slots[sl].g = pr[k].g;
    }
    free(pr[k].px);
  }
  if (i < 0) return 0;
  relogioUso++;
  {
    int ordem[3], no = 0;
    if (n == 1) ordem[no++] = i;
    else { ordem[no++] = i - 1; ordem[no++] = i; ordem[no++] = i + 1; }
    for (k = 0; k < no; k++) {
      int sl = ordem[k] >= 0 ? slotDe(ordem[k], g) : -1;
      if (sl >= 0) { texs[k] = slots[sl].tex; slots[sl].uso = relogioUso; if (ordem[k] == i) achou = 1; }
    }
    // A atual ainda nao chegou: a ultima usada do MESMO titulo no lugar dela
    // (melhor que piscar vazio entre duas cues).
    if (!achou) {
      int m = -1, c = n == 1 ? 0 : 1;
      for (k = 0; k < SK_SLOTS; k++)
        if (slots[k].tex && slots[k].g == g && (m < 0 || slots[k].uso > slots[m].uso)) m = k;
      if (m >= 0) { texs[c] = slots[m].tex; achou = 1; }
    }
  }
  return achou;
}

GLuint seekr_quadro(double posSeg, double *cueSeg) {
  GLuint t = 0;
  seekr_quadros(posSeg, 1, &t, cueSeg);
  return t;
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
