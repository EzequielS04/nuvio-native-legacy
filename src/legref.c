// Ver legref.h. Coletor INDEPENDENTE de uma faixa de texto embutida no MKV,
// para servir de referencia ao AutoSync. Nao conversa com mkvass/overlay.
#include "legref.h"
#include "rede.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <errno.h>

#define LR_CABECA   (64L * 1024L)        // inicio do arquivo: EBML, SeekHead, Info, Tracks
#define LR_TRACKS_MAX (2L * 1024L * 1024L)
#define LR_CUES_MAX (8L * 1024L * 1024L)
#define LR_BLOCO    2048L                // janela de um BlockGroup de legenda
#define LR_BLOCO_MAX (64L * 1024L)
#define LR_VAO      (8L * 1024L)         // junta Ranges vizinhos a menos disto
#define LR_LEITURA_MAX (128L * 1024L)
#define LR_FAIXAS   64
#define LR_EVENTOS_MAX 7999              // acima disto o AutoSync recusa de qualquer jeito

// --- EBML -------------------------------------------------------------------
static int largura(unsigned char b) {
  int n = 1;
  if (!b) return 0;
  while (!(b & 0x80)) { b <<= 1; n++; }
  return n;
}
static int lerId(const unsigned char *p, long resta, unsigned long *id) {
  int n, i;
  if (resta < 1) return 0;
  n = largura(p[0]);
  if (n < 1 || n > 4 || n > resta) return 0;
  *id = 0;
  for (i = 0; i < n; i++) *id = (*id << 8) | p[i];
  return n;
}
// Tamanho com o marcador removido. -1 em *tam = desconhecido (todos os bits 1).
static int lerTam(const unsigned char *p, long resta, long long *tam) {
  int n, i, uns;
  unsigned long long v;
  if (resta < 1) return 0;
  n = largura(p[0]);
  if (n < 1 || n > 8 || n > resta) return 0;
  v = p[0] & (0xFFu >> n);
  uns = v == (0xFFu >> n);
  for (i = 1; i < n; i++) { v = (v << 8) | p[i]; if (p[i] != 0xFF) uns = 0; }
  *tam = uns ? -1 : (long long)v;
  return n;
}
static unsigned long long lerUint(const unsigned char *p, long n) {
  unsigned long long v = 0;
  long i;
  for (i = 0; i < n && i < 8; i++) v = (v << 8) | p[i];
  return v;
}
// Proximo filho dentro de [o, fim). Devolve 1 com id/dados/tam; 0 no fim ou
// em lixo. Filho que passa do fim e lixo (o pai mente ou o buffer foi cortado).
typedef struct { const unsigned char *p; long o, fim; } It;
static int prox(It *it, unsigned long *id, const unsigned char **d, long *tam) {
  int a, b;
  long long t;
  if (it->o >= it->fim) return 0;
  a = lerId(it->p + it->o, it->fim - it->o, id);
  if (!a) return 0;
  b = lerTam(it->p + it->o + a, it->fim - it->o - a, &t);
  if (!b || t < 0 || t > it->fim - it->o - a - b) return 0;
  *d = it->p + it->o + a + b; *tam = (long)t;
  it->o += a + b + (long)t;
  return 1;
}

// --- PARSERS PUROS ------------------------------------------------------------
typedef struct {
  int numero, tipo, forced, codificada;
  char codec[24], idioma[24], nome[96];
  const unsigned char *priv; long nPriv;
} Faixa;

static void copiaStr(char *dst, size_t tam, const unsigned char *d, long n) {
  size_t k = (size_t)n < tam - 1 ? (size_t)n : tam - 1;
  memcpy(dst, d, k); dst[k] = 0;
  while (k && !dst[k - 1]) dst[--k] = 0;
}

static int lerTracks(const unsigned char *p, long n, Faixa *v, int max) {
  It it = { p, 0, n }, sub;
  unsigned long id, id2;
  const unsigned char *d, *d2;
  long t, t2;
  int k = 0;
  while (prox(&it, &id, &d, &t)) {
    Faixa f;
    if (id != 0xAE || k >= max) continue;
    memset(&f, 0, sizeof f);
    snprintf(f.idioma, sizeof f.idioma, "eng");   // padrao do Matroska
    sub = (It){ d, 0, t };
    while (prox(&sub, &id2, &d2, &t2)) {
      if (id2 == 0xD7) f.numero = (int)lerUint(d2, t2);
      else if (id2 == 0x83) f.tipo = (int)lerUint(d2, t2);
      else if (id2 == 0x86) copiaStr(f.codec, sizeof f.codec, d2, t2);
      else if (id2 == 0x22B59C) copiaStr(f.idioma, sizeof f.idioma, d2, t2);
      else if (id2 == 0x22B59D && t2 > 0) copiaStr(f.idioma, sizeof f.idioma, d2, t2);
      else if (id2 == 0x536E) copiaStr(f.nome, sizeof f.nome, d2, t2);
      else if (id2 == 0x55AA) f.forced = lerUint(d2, t2) != 0;
      else if (id2 == 0x63A2) { f.priv = d2; f.nPriv = t2; }
      else if (id2 == 0x6D80) f.codificada = 1;   // compressao/cifra: nao lemos
    }
    v[k++] = f;
  }
  return k;
}

int legref_cues(const unsigned char *p, long n, int faixa, double escala,
                LegRefPonto **saida, int *semRel) {
  It it = { p, 0, n };
  unsigned long id;
  const unsigned char *d;
  long t;
  int k = 0, cap = 0;
  LegRefPonto *v = NULL;
  *saida = NULL; if (semRel) *semRel = 0;
  while (prox(&it, &id, &d, &t)) {
    It cp = { d, 0, t }, tp;
    unsigned long id2, id3;
    const unsigned char *d2, *d3;
    long t2, t3;
    double tempo = -1;
    if (id != 0xBB) continue;
    // CueTime vem antes das posicoes em todo muxer conhecido, mas a ordem nao
    // e garantida: le o tempo primeiro.
    while (prox(&cp, &id2, &d2, &t2)) if (id2 == 0xB3) tempo = (double)lerUint(d2, t2) * escala;
    if (tempo < 0) continue;
    cp.o = 0;
    while (prox(&cp, &id2, &d2, &t2)) {
      long long pos = -1; int rel = -1, tr = 0; double dur = -1;
      if (id2 != 0xB7) continue;
      tp = (It){ d2, 0, t2 };
      while (prox(&tp, &id3, &d3, &t3)) {
        if (id3 == 0xF7) tr = (int)lerUint(d3, t3);
        else if (id3 == 0xF1) pos = (long long)lerUint(d3, t3);
        else if (id3 == 0xF0) rel = (int)lerUint(d3, t3);
        else if (id3 == 0xB2) dur = (double)lerUint(d3, t3) * escala;
      }
      if (tr != faixa || pos < 0) continue;
      if (rel < 0) { if (semRel) (*semRel)++; continue; }
      if (k == cap) {
        LegRefPonto *nv;
        if (cap >= LR_EVENTOS_MAX + 1) { free(v); return -1; }
        cap = cap ? cap * 2 : 256;
        nv = realloc(v, (size_t)cap * sizeof *v);
        if (!nv) { free(v); return -1; }
        v = nv;
      }
      v[k++] = (LegRefPonto){ pos, rel, tempo, dur };
    }
  }
  *saida = v;
  return k;
}

// --- O MODULO -----------------------------------------------------------------
typedef struct {
  uint64_t id, sessao;
  char url[4096], idioma[24];
  int excl[16], nExcl;
  LegRefOrcamento orc;
} Pedido;

struct LegRef {
  pthread_mutex_t m;
  pthread_cond_t c;
  pthread_t fio;
  int fioVivo, sair, pausado;
  LegRefLer ler; void *lerU;
  Pedido ped;              // o pedido atual (id 0 = nenhum)
  int temPedido;           // ainda nao pego pelo fio
  uint64_t gerador;
  LegRefStatus st;
  LegendaDocumento *doc;   // pronto, do pedido st.pedido
};

typedef struct {
  LegRef *r; uint64_t id;
  Pedido p;
  char urlFinal[4096];
  long long bytes; int pedidos;
  struct timespec ultimo; int temUltimo;
  LegRefMotivo erro;
} Job;

static int parou(void *u) {
  Job *j = u; LegRef *r = j->r; int x;
  pthread_mutex_lock(&r->m); x = r->sair || r->ped.id != j->id; pthread_mutex_unlock(&r->m);
  return x;
}

static long msDesde(const struct timespec *a) {
  struct timespec b; clock_gettime(CLOCK_MONOTONIC, &b);
  return (long)((b.tv_sec - a->tv_sec) * 1000 + (b.tv_nsec - a->tv_nsec) / 1000000);
}

// Espera `ms` acordando no cancelamento. 1 = cancelado.
static int esperar(Job *j, long ms) {
  LegRef *r = j->r; struct timespec ate; int x;
  clock_gettime(CLOCK_REALTIME, &ate);
  ate.tv_sec += ms / 1000; ate.tv_nsec += (ms % 1000) * 1000000L;
  if (ate.tv_nsec >= 1000000000L) { ate.tv_sec++; ate.tv_nsec -= 1000000000L; }
  pthread_mutex_lock(&r->m);
  while (!(x = r->sair || r->ped.id != j->id))
    if (pthread_cond_timedwait(&r->c, &r->m, &ate) == ETIMEDOUT) break;
  x = r->sair || r->ped.id != j->id;
  pthread_mutex_unlock(&r->m);
  return x;
}

static void progresso(Job *j, int feitos, int total) {
  LegRef *r = j->r;
  pthread_mutex_lock(&r->m);
  if (r->st.pedido == j->id) {
    r->st.feitos = feitos; r->st.total = total;
    r->st.bytes = j->bytes; r->st.pedidos = j->pedidos;
  }
  pthread_mutex_unlock(&r->m);
}

// Um Range, respeitando pausa, ritmo e orcamento. NULL com j->erro preenchido.
static unsigned char *ler(Job *j, long long ini, long n, long *tam) {
  LegRef *r = j->r; unsigned char *b; int st = 0;
  long intervalo = j->p.orc.pedidosPorSeg > 0 ? 1000 / j->p.orc.pedidosPorSeg : 0;
  for (;;) {   // PAUSA: seek/buffer curto. Nao conta prazo nem orcamento.
    int pausa;
    pthread_mutex_lock(&r->m); pausa = r->pausado; pthread_mutex_unlock(&r->m);
    if (!pausa) break;
    if (esperar(j, 200)) { j->erro = LEGREF_PARADO; return NULL; }
  }
  if (j->temUltimo && intervalo > 0) {
    long passou = msDesde(&j->ultimo);
    if (passou < intervalo && esperar(j, intervalo - passou)) { j->erro = LEGREF_PARADO; return NULL; }
  }
  if (parou(j)) { j->erro = LEGREF_PARADO; return NULL; }
  if (j->pedidos >= j->p.orc.maxPedidos || j->bytes + n > j->p.orc.maxBytes) {
    j->erro = LEGREF_ORCAMENTO; return NULL;
  }
  j->pedidos++;
  clock_gettime(CLOCK_MONOTONIC, &j->ultimo); j->temUltimo = 1;
  *tam = 0;
  b = r->ler(r->lerU, j->urlFinal, ini, n, tam, &st, parou, j);
  if (b) j->bytes += *tam;
  if (parou(j)) { free(b); j->erro = LEGREF_PARADO; return NULL; }
  if (!b) { j->erro = st == 200 ? LEGREF_SEM_RANGE : LEGREF_REDE; return NULL; }
  if (st != 206) { free(b); j->erro = LEGREF_SEM_RANGE; return NULL; }
  return b;
}

// Elemento inteiro em `abs`, com teto. Le o cabecalho e depois o corpo.
static unsigned char *lerElemento(Job *j, long long abs, unsigned long esperado, long teto,
                                  long *dados, long *nDados) {
  long tam = 0, n2 = 0; unsigned long id; long long t; int a, b;
  unsigned char *h = ler(j, abs, 16, &tam), *corpo;
  if (!h) return NULL;
  a = lerId(h, tam, &id);
  b = a ? lerTam(h + a, tam - a, &t) : 0;
  free(h);
  if (!a || !b || id != esperado || t < 0) { j->erro = LEGREF_SEM_INDICE; return NULL; }
  if (t > teto) { j->erro = LEGREF_ORCAMENTO; return NULL; }
  corpo = ler(j, abs + a + b, (long)t, &n2);
  if (!corpo) return NULL;
  if (n2 < t) { free(corpo); j->erro = LEGREF_INCOMPLETO; return NULL; }
  *dados = 0; *nDados = (long)t;
  return corpo;
}

// --- BLOCOS ---------------------------------------------------------------------
typedef struct { double inicio, fim; char *texto; } Evento;

// Le um BlockGroup/SimpleBlock da faixa em p. 1 = ok; 0 = nao e este;
// -1 = precisa de *precisa bytes; -2 = lacing/sem duracao (faixa nao serve).
static int lerGrupo(const unsigned char *p, long n, int faixa, double escala, double durCue,
                    int *rel16, double *dur, const unsigned char **txt, long *nTxt, long *precisa) {
  unsigned long id; long long s; int a, b;
  a = lerId(p, n, &id);
  if (!a || (id != 0xA0 && id != 0xA3)) return 0;
  b = lerTam(p + a, n - a, &s);
  if (!b || s <= 4 || s > LR_BLOCO_MAX) return 0;
  if (a + b + s > n) { *precisa = a + b + (long)s; return -1; }
  {
    const unsigned char *bloco = NULL; long nb = 0; double d = -1;
    if (id == 0xA3) { bloco = p + a + b; nb = (long)s; }
    else {
      It it = { p + a + b, 0, (long)s }; unsigned long id2; const unsigned char *d2; long t2;
      int achou = 0;
      while (prox(&it, &id2, &d2, &t2)) {
        if (id2 == 0xA1) { bloco = d2; nb = t2; achou++; }
        else if (id2 == 0x9B) d = (double)lerUint(d2, t2) * escala;
      }
      // Os filhos tem de fechar EXATAMENTE no tamanho do grupo: e o que separa
      // um BlockGroup de verdade de bytes de video que comecam com 0xA0.
      if (it.o != s || achou != 1) return 0;
    }
    {
      int c = largura(bloco[0]), k; unsigned long tr = 0;
      if (c < 1 || c > 4 || c + 3 > nb) return 0;
      tr = bloco[0] & (0xFFu >> c);
      for (k = 1; k < c; k++) tr = (tr << 8) | bloco[k];
      if ((int)tr != faixa) return 0;
      *rel16 = (int)(short)((bloco[c] << 8) | bloco[c + 1]);
      if ((bloco[c + 2] >> 1) & 3) return -2;   // lacing em legenda: nao lemos
      if (d <= 0) d = durCue;
      if (d <= 0) return -2;
      *dur = d; *txt = bloco + c + 3; *nTxt = nb - c - 3;
    }
  }
  return 1;
}

// --- MONTAGEM DO DOCUMENTO -------------------------------------------------------
typedef struct { char *s; size_t n, cap; int falhou; } Str;
static void anexar(Str *b, const char *s, size_t n) {
  if (b->falhou) return;
  if (b->n + n + 1 > b->cap) {
    size_t cap = b->cap ? b->cap : 65536; char *nv;
    while (cap < b->n + n + 1) cap *= 2;
    if (cap > 16u * 1024u * 1024u) { b->falhou = 1; return; }
    nv = realloc(b->s, cap);
    if (!nv) { b->falhou = 1; return; }
    b->s = nv; b->cap = cap;
  }
  memcpy(b->s + b->n, s, n); b->n += n; b->s[b->n] = 0;
}
static void anexarf(Str *b, const char *fmt, double t, int ass) {
  char x[32]; long ms = (long)(t * 1000.0 + 0.5);
  if (ass) snprintf(x, sizeof x, fmt, ms / 3600000, ms / 60000 % 60, ms / 1000 % 60, ms % 1000 / 10);
  else snprintf(x, sizeof x, fmt, ms / 3600000, ms / 60000 % 60, ms / 1000 % 60, ms % 1000);
  anexar(b, x, strlen(x));
}

static LegendaDocumento *montar(const Faixa *f, Evento *ev, int n, uint64_t sessao, int ass) {
  Str b = { 0 }; int i; LegendaDocumento *doc; int nDoc = 0;
  LegendaDocumentoInfo info;
  memset(&info, 0, sizeof info);
  info.sessao = sessao;
  info.flags = LEGENDA_DOC_COMPLETO | (f->forced ? LEGENDA_DOC_FORCED : 0);
  snprintf(info.idioma, sizeof info.idioma, "%s", f->idioma);
  snprintf(info.origem, sizeof info.origem, "Embedded");
  // Identidade OPACA: o numero da faixa, nunca a URL.
  snprintf(info.identidade, sizeof info.identidade, "mkv-track:%d", f->numero);
  if (ass) {
    if (f->priv && f->nPriv > 0) anexar(&b, (const char *)f->priv, strnlen((const char *)f->priv, (size_t)f->nPriv));
    if (!b.s || !strstr(b.s, "[Events]"))
      anexar(&b, "\n[Events]\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n", 88);
    else anexar(&b, "\n", 1);
  }
  for (i = 0; i < n; i++) {
    const char *t = ev[i].texto;
    if (ass) {
      // Bloco ASS do Matroska: ReadOrder, Layer, Style, Name, MarginL, MarginR,
      // MarginV, Effect, Text. Vira Dialogue: Layer, Start, End, Style, ...
      const char *c1 = strchr(t, ','), *c2 = c1 ? strchr(c1 + 1, ',') : NULL;
      if (!c2) continue;
      anexar(&b, "Dialogue: ", 10);
      anexar(&b, c1 + 1, (size_t)(c2 - c1 - 1));
      anexarf(&b, ",%ld:%02ld:%02ld.%02ld", ev[i].inicio, 1);
      anexarf(&b, ",%ld:%02ld:%02ld.%02ld", ev[i].fim, 1);
      anexar(&b, c2, strlen(c2));
      anexar(&b, "\n", 1);
    } else {
      char num[16]; snprintf(num, sizeof num, "%d\n", i + 1);
      anexar(&b, num, strlen(num));
      anexarf(&b, "%02ld:%02ld:%02ld,%03ld", ev[i].inicio, 0);
      anexar(&b, " --> ", 5);
      anexarf(&b, "%02ld:%02ld:%02ld,%03ld", ev[i].fim, 0);
      anexar(&b, "\n", 1);
      anexar(&b, t, strlen(t));
      anexar(&b, "\n\n", 2);
    }
  }
  if (b.falhou || !b.s) { free(b.s); return NULL; }
  doc = legenda_documento_criar(b.s, &info);
  // O parser descartou alguma fala (texto vazio, tempo invalido): o documento
  // pode ate servir para desenho, mas nao e a faixa inteira.
  if (doc) { legenda_documento_dados(doc, &nDoc);
    if (nDoc != n && (legenda_documento_info(doc)->flags & LEGENDA_DOC_COMPLETO)) {
      legenda_documento_liberar(doc);
      info.flags &= ~LEGENDA_DOC_COMPLETO;
      doc = legenda_documento_criar(b.s, &info);
    } }
  free(b.s);
  return doc;
}

static int textoFaixa(const Faixa *f) {
  return f->tipo == 0x11 && !f->codificada &&
         (!strcmp(f->codec, "S_TEXT/UTF8") || !strcmp(f->codec, "S_TEXT/ASS") ||
          !strcmp(f->codec, "S_TEXT/SSA"));
}
// Letreiros/forced traduzem placas, nao o dialogo: nao servem de referencia.
static int letreiro(const Faixa *f) {
  static const char *const p[] = { "forced", "sign", "song", "letreiro", "forzad", "forcé" };
  size_t i;
  if (f->forced) return 1;
  for (i = 0; i < sizeof p / sizeof *p; i++) if (strcasestr(f->nome, p[i])) return 1;
  return 0;
}
static int mesmoIdioma(const char *a, const char *b) {
  size_t i;
  if (!a[0] || !b[0]) return 0;
  for (i = 0; i < 2; i++) if ((a[i] | 32) != (b[i] | 32)) return 0;
  return 1;
}
static int cmpPonto(const void *a, const void *b) {
  const LegRefPonto *x = a, *y = b;
  if (x->pos != y->pos) return x->pos < y->pos ? -1 : 1;
  return x->rel - y->rel;
}

static LegendaDocumento *coletar(Job *j, LegRefStatus *st) {
  long tam = 0, nTr = 0, oTr = 0, nCu = 0, oCu = 0;
  unsigned char *cab = NULL, *tracks = NULL, *cues = NULL, *bufTr;
  long long segData = -1, posTracks = -1, posCues = -1, posInfo = -1;
  double escala = 1e-6;   // TimestampScale padrao (1 ms) em segundos
  Faixa fx[LR_FAIXAS]; int nF = 0, escolhida = -1, i, k, semRel = 0, nP;
  LegRefPonto *pts = NULL; Evento *ev = NULL; int nEv = 0;
  LegendaDocumento *doc = NULL; int ass;
  // 1. Cabeca do arquivo. O primeiro pedido segue redirects (debrid) e guarda
  // o endereco final: Range e cabecalho do dono e nao atravessa origem num
  // redirect, entao os seguintes vao direto ao destino.
  cab = ler(j, 0, LR_CABECA, &tam);
  if (!cab && j->erro == LEGREF_SEM_RANGE && strcmp(j->urlFinal, j->p.url)) cab = ler(j, 0, LR_CABECA, &tam);
  if (!cab) goto fim;
  if (tam < 4 || cab[0] != 0x1A || cab[1] != 0x45 || cab[2] != 0xDF || cab[3] != 0xA3) { j->erro = LEGREF_NAO_MKV; goto fim; }
  {
    unsigned long id; long long t; long o = 0; int a, b;
    a = lerId(cab, tam, &id); b = a ? lerTam(cab + a, tam - a, &t) : 0;
    if (!a || !b || t < 0) { j->erro = LEGREF_NAO_MKV; goto fim; }
    o = a + b + (long)t;
    a = lerId(cab + o, tam - o, &id); b = a ? lerTam(cab + o + a, tam - o - a, &t) : 0;
    if (!a || !b || id != 0x18538067) { j->erro = LEGREF_NAO_MKV; goto fim; }
    segData = o + a + b;
    o = (long)segData;
    for (;;) {
      long long ts; long hdr, ini = o;
      a = lerId(cab + o, tam - o, &id); if (!a) break;
      b = lerTam(cab + o + a, tam - o - a, &ts); if (!b) break;
      hdr = a + b;
      if (id == 0x1F43B675 || ts < 0) break;   // primeiro Cluster: o resto vem pelo SeekHead
      if (ini + hdr + ts > tam) {              // elemento cortado pela cabeca
        if (id == 0x1654AE6B && posTracks < 0) posTracks = ini - segData;
        break;
      }
      if (id == 0x114D9B74) {
        It sk = { cab + ini + hdr, 0, (long)ts }, e; unsigned long id2, id3; const unsigned char *d2, *d3; long t2, t3;
        while (prox(&sk, &id2, &d2, &t2)) {
          unsigned long alvo = 0; long long pos = -1;
          if (id2 != 0x4DBB) continue;
          e = (It){ d2, 0, t2 };
          while (prox(&e, &id3, &d3, &t3)) {
            if (id3 == 0x53AB) alvo = (unsigned long)lerUint(d3, t3);
            else if (id3 == 0x53AC) pos = (long long)lerUint(d3, t3);
          }
          if (alvo == 0x1654AE6B && posTracks < 0) posTracks = pos;
          else if (alvo == 0x1C53BB6B && posCues < 0) posCues = pos;
          else if (alvo == 0x1549A966 && posInfo < 0) posInfo = pos;
        }
      } else if (id == 0x1549A966) {
        It in = { cab + ini + hdr, 0, (long)ts }; unsigned long id2; const unsigned char *d2; long t2;
        while (prox(&in, &id2, &d2, &t2)) if (id2 == 0x2AD7B1) { unsigned long long v = lerUint(d2, t2); if (v) escala = (double)v / 1e9; }
        posInfo = ini - segData;
      } else if (id == 0x1654AE6B) {
        nF = lerTracks(cab + ini + hdr, (long)ts, fx, LR_FAIXAS);
        bufTr = cab; (void)bufTr;
        posTracks = ini - segData;
      }
      o = (long)(ini + hdr + ts);
    }
  }
  if (!nF && posTracks >= 0) {
    tracks = lerElemento(j, segData + posTracks, 0x1654AE6B, LR_TRACKS_MAX, &oTr, &nTr);
    if (!tracks) goto fim;
    nF = lerTracks(tracks + oTr, nTr, fx, LR_FAIXAS);
  }
  (void)posInfo;
  // 2. A faixa: texto, nao letreiro, nao excluida; o idioma pedido primeiro.
  for (k = 0; k < 2 && escolhida < 0; k++)
    for (i = 0; i < nF; i++) {
      int e, fora = 0;
      if (!textoFaixa(&fx[i]) || letreiro(&fx[i])) continue;
      for (e = 0; e < j->p.nExcl; e++) if (j->p.excl[e] == fx[i].numero) fora = 1;
      if (fora || (k == 0 && !mesmoIdioma(fx[i].idioma, j->p.idioma))) continue;
      escolhida = i; break;
    }
  if (escolhida < 0) { j->erro = LEGREF_SEM_FAIXA; goto fim; }
  {
    const Faixa *f = &fx[escolhida];
    st->faixa = f->numero;
    snprintf(st->idioma, sizeof st->idioma, "%s", f->idioma);
    snprintf(st->codec, sizeof st->codec, "%s", f->codec);
    pthread_mutex_lock(&j->r->m);
    if (j->r->st.pedido == j->id) {
      j->r->st.faixa = f->numero;
      snprintf(j->r->st.idioma, sizeof j->r->st.idioma, "%s", f->idioma);
      snprintf(j->r->st.codec, sizeof j->r->st.codec, "%s", f->codec);
    }
    pthread_mutex_unlock(&j->r->m);
  }
  ass = strcmp(fx[escolhida].codec, "S_TEXT/UTF8") != 0;
  // 3. O indice.
  if (posCues < 0) { j->erro = LEGREF_SEM_INDICE; goto fim; }
  cues = lerElemento(j, segData + posCues, 0x1C53BB6B, LR_CUES_MAX, &oCu, &nCu);
  if (!cues) goto fim;
  nP = legref_cues(cues + oCu, nCu, fx[escolhida].numero, escala, &pts, &semRel);
  if (nP < 0) { j->erro = LEGREF_ORCAMENTO; goto fim; }
  // Um CuePoint sem posicao relativa e um bloco que nao sabemos buscar: a
  // faixa nao sai inteira, entao nao serve.
  if (semRel || nP < 1) { j->erro = LEGREF_SEM_INDICE; goto fim; }
  qsort(pts, (size_t)nP, sizeof *pts, cmpPonto);
  ev = calloc((size_t)nP, sizeof *ev);
  if (!ev) { j->erro = LEGREF_MEMORIA; goto fim; }
  progresso(j, 0, nP);
  // 4. Os blocos, em ordem de byte, juntando vizinhos.
  {
    long long clPos = -1; int clH = 0; double clBase = 0; int temBase = 0;
    for (i = 0; i < nP;) {
      long long ini = segData + pts[i].pos + 5 + pts[i].rel, fimL = segData + pts[i].pos + 12 + pts[i].rel + LR_BLOCO;
      int g = i + 1; unsigned char *buf; long n = 0;
      while (g < nP) {
        long long a2 = segData + pts[g].pos + 5 + pts[g].rel, b2 = a2 + 7 + LR_BLOCO;
        if (a2 > fimL + LR_VAO || b2 - ini > LR_LEITURA_MAX) break;
        if (b2 > fimL) fimL = b2;
        g++;
      }
      buf = ler(j, ini, (long)(fimL - ini), &n);
      if (!buf) goto fim;
      for (; i < g; i++) {
        long long cl = segData + pts[i].pos;
        int h, ok = 0, hIni = 5, hFim = 12;
        if (cl == clPos && clH) hIni = hFim = clH;
        for (h = hIni; h <= hFim && !ok; h++) {
          long long abs = cl + h + pts[i].rel; long off = (long)(abs - ini), precisa = 0, nTxt = 0;
          int rel16 = 0, r; double dur = 0; const unsigned char *txt = NULL;
          unsigned char *extra = NULL;
          if (off < 0 || off >= n) continue;
          r = lerGrupo(buf + off, n - off, fx[escolhida].numero, escala, pts[i].dur, &rel16, &dur, &txt, &nTxt, &precisa);
          if (r == -1) {   // grupo maior que a janela: le exato, uma vez
            long n2 = 0;
            extra = ler(j, abs, precisa, &n2);
            if (!extra) { free(buf); goto fim; }
            r = lerGrupo(extra, n2, fx[escolhida].numero, escala, pts[i].dur, &rel16, &dur, &txt, &nTxt, &precisa);
            if (r == -1) r = 0;
          }
          if (r == -2) { free(extra); free(buf); j->erro = LEGREF_SEM_DURACAO; goto fim; }
          if (r == 1) {
            // Mesmo Cluster, mesma base: tempo do indice - relativo do bloco.
            double base = pts[i].inicio - rel16 * escala;
            if (cl == clPos && temBase && (base - clBase > 0.002 || clBase - base > 0.002)) { free(extra); continue; }
            ev[nEv].inicio = pts[i].inicio; ev[nEv].fim = pts[i].inicio + dur;
            ev[nEv].texto = malloc((size_t)nTxt + 1);
            if (!ev[nEv].texto) { free(extra); free(buf); j->erro = LEGREF_MEMORIA; goto fim; }
            memcpy(ev[nEv].texto, txt, (size_t)nTxt); ev[nEv].texto[nTxt] = 0;
            { char *s = ev[nEv].texto; size_t m = strlen(s);   // NUL no meio corta; CR some
              while (m && (s[m - 1] == '\r' || s[m - 1] == '\n')) s[--m] = 0;
              if (ass) for (; *s; s++) if (*s == '\r' || *s == '\n') *s = ' '; }
            nEv++; ok = 1;
            if (cl != clPos) { clPos = cl; temBase = 1; clBase = base; }
            clH = h;
          }
          free(extra);
        }
        if (!ok) { free(buf); j->erro = LEGREF_INCOMPLETO; goto fim; }
        if (!(nEv % 16)) progresso(j, nEv, nP);
      }
      free(buf);
    }
  }
  progresso(j, nEv, nP);
  doc = montar(&fx[escolhida], ev, nEv, j->p.sessao, ass);
  if (!doc) j->erro = LEGREF_MEMORIA;
  else if (!(legenda_documento_info(doc)->flags & LEGENDA_DOC_COMPLETO)) {
    legenda_documento_liberar(doc); doc = NULL; j->erro = LEGREF_INCOMPLETO;
  }
fim:
  for (i = 0; i < nEv; i++) free(ev[i].texto);
  free(ev); free(pts); free(cues); free(tracks); free(cab);
  return doc;
}

static void *trabalhar(void *u) {
  LegRef *r = u;
  for (;;) {
    Job j; LegendaDocumento *doc; LegRefStatus st;
    pthread_mutex_lock(&r->m);
    while (!r->sair && !r->temPedido) pthread_cond_wait(&r->c, &r->m);
    if (r->sair) { pthread_mutex_unlock(&r->m); return NULL; }
    memset(&j, 0, sizeof j);
    j.r = r; j.id = r->ped.id; j.p = r->ped; r->temPedido = 0;
    snprintf(j.urlFinal, sizeof j.urlFinal, "%s", j.p.url);
    pthread_mutex_unlock(&r->m);
    memset(&st, 0, sizeof st);
    doc = coletar(&j, &st);
    pthread_mutex_lock(&r->m);
    if (r->st.pedido == j.id && r->ped.id == j.id && !r->sair) {
      r->st.bytes = j.bytes; r->st.pedidos = j.pedidos;
      if (doc) { r->st.fase = LEGREF_PRONTO; r->st.motivo = LEGREF_OK; r->doc = doc; doc = NULL; }
      else {
        r->st.fase = j.erro == LEGREF_PARADO ? LEGREF_CANCELADO : LEGREF_INDISPONIVEL;
        r->st.motivo = j.erro;
      }
      fprintf(stderr, "[legref] reference %s reason=%s track=%d events=%d/%d requests=%d bytes=%lld\n",
              r->st.fase == LEGREF_PRONTO ? "complete" : "unavailable", legref_motivo(r->st.motivo),
              r->st.faixa, r->st.feitos, r->st.total, j.pedidos, j.bytes);
    }
    pthread_mutex_unlock(&r->m);
    legenda_documento_liberar(doc);   // pedido velho: descarta fora do lock
  }
}

// --- HTTP -------------------------------------------------------------------------
typedef struct { int (*parar)(void *); void *u; } PararHttp;
static unsigned char *lerHttp(void *u, const char *url, long long ini, long n, long *tam,
                              int *status, int (*parar)(void *), void *pu) {
  char range[80]; const char *cab[2]; RedePedido p; RedeResposta r; unsigned char *b = NULL;
  (void)u;
  snprintf(range, sizeof range, "Range: bytes=%lld-%lld", ini, ini + n - 1);
  cab[0] = range; cab[1] = NULL;
  memset(&p, 0, sizeof p);
  p.url = url; p.cabecalhos = cab; p.seguir = 1;
  p.prazo_ms = 20000u + (unsigned)(n / 64);
  p.max_bytes = (size_t)n;
  p.parar = parar; p.parar_usuario = pu;
  *status = 0; *tam = 0;
  rede_pedir(&p, &r);
  *status = r.status;
  if (r.erro == REDE_OK && r.status == 206 && r.corpo) {
    b = (unsigned char *)r.corpo; r.corpo = NULL; *tam = (long)r.n_corpo;
    // Endereco final para os proximos Ranges (o Job guarda; nunca vai a log).
    if (r.final[0] && pu) { Job *j = pu; snprintf(j->urlFinal, sizeof j->urlFinal, "%s", r.final); }
  } else if (r.status == 200 && r.final[0] && pu) {
    Job *j = pu; snprintf(j->urlFinal, sizeof j->urlFinal, "%s", r.final);
  }
  rede_resposta_limpar(&r);
  return b;
}

int legref_disponivel(void) {
#if defined(__EMSCRIPTEN__) || defined(NV_TPK) || defined(NV_TPK40)
  return 0;
#else
  return (rede_pedido_capacidades() & REDE_CAP_JOB) != 0;
#endif
}

LegRef *legref_criar(LegRefLer ler, void *u) {
  LegRef *r = calloc(1, sizeof *r);
  if (!r) return NULL;
  r->ler = ler ? ler : lerHttp; r->lerU = u;
  if (pthread_mutex_init(&r->m, NULL)) { free(r); return NULL; }
  if (pthread_cond_init(&r->c, NULL)) { pthread_mutex_destroy(&r->m); free(r); return NULL; }
  if (pthread_create(&r->fio, NULL, trabalhar, r)) {
    pthread_cond_destroy(&r->c); pthread_mutex_destroy(&r->m); free(r); return NULL;
  }
  r->fioVivo = 1;
  return r;
}

void legref_destruir(LegRef *r) {
  if (!r) return;
  pthread_mutex_lock(&r->m); r->sair = 1; pthread_cond_broadcast(&r->c); pthread_mutex_unlock(&r->m);
  if (r->fioVivo) pthread_join(r->fio, NULL);
  legenda_documento_liberar(r->doc);
  pthread_cond_destroy(&r->c); pthread_mutex_destroy(&r->m); free(r);
}

uint64_t legref_pedir(LegRef *r, const char *url, uint64_t sessao, const char *idioma,
                      const int *excluidas, int nExcluidas, const LegRefOrcamento *orc) {
  LegendaDocumento *velho; uint64_t id; int i;
  if (!r || !url || !*url || strlen(url) >= sizeof r->ped.url || !orc ||
      orc->maxBytes <= 0 || orc->maxPedidos <= 0) return 0;
  pthread_mutex_lock(&r->m);
  velho = r->doc; r->doc = NULL;
  id = ++r->gerador;
  memset(&r->ped, 0, sizeof r->ped);
  r->ped.id = id; r->ped.sessao = sessao; r->ped.orc = *orc;
  snprintf(r->ped.url, sizeof r->ped.url, "%s", url);
  snprintf(r->ped.idioma, sizeof r->ped.idioma, "%s", idioma ? idioma : "");
  for (i = 0; i < nExcluidas && i < 16; i++) r->ped.excl[i] = excluidas[i];
  r->ped.nExcl = i;
  r->temPedido = 1;
  memset(&r->st, 0, sizeof r->st);
  r->st.fase = LEGREF_LENDO; r->st.pedido = id;
  pthread_cond_broadcast(&r->c);
  pthread_mutex_unlock(&r->m);
  legenda_documento_liberar(velho);
  return id;
}

void legref_cancelar(LegRef *r) {
  LegendaDocumento *velho;
  if (!r) return;
  pthread_mutex_lock(&r->m);
  velho = r->doc; r->doc = NULL;
  r->ped.id = 0; r->temPedido = 0;
  if (r->st.fase == LEGREF_LENDO) { r->st.fase = LEGREF_CANCELADO; r->st.motivo = LEGREF_PARADO; }
  pthread_cond_broadcast(&r->c);
  pthread_mutex_unlock(&r->m);
  legenda_documento_liberar(velho);
}

void legref_pausar(LegRef *r, int pausar) {
  if (!r) return;
  pthread_mutex_lock(&r->m); r->pausado = pausar != 0; pthread_cond_broadcast(&r->c); pthread_mutex_unlock(&r->m);
}

LegRefStatus legref_status(LegRef *r) {
  LegRefStatus s; memset(&s, 0, sizeof s);
  if (!r) { s.fase = LEGREF_INDISPONIVEL; s.motivo = LEGREF_PLATAFORMA; return s; }
  pthread_mutex_lock(&r->m); s = r->st; pthread_mutex_unlock(&r->m);
  return s;
}

LegendaDocumento *legref_tomar(LegRef *r, uint64_t pedido) {
  LegendaDocumento *d = NULL;
  if (!r || !pedido) return NULL;
  pthread_mutex_lock(&r->m);
  if (r->st.pedido == pedido && r->doc) { d = r->doc; r->doc = NULL; }
  pthread_mutex_unlock(&r->m);
  return d;
}

const char *legref_motivo(LegRefMotivo m) {
  static const char *const n[] = { "ok", "platform", "no_range", "not_matroska", "no_text_track",
    "no_index", "no_block_duration", "network", "budget", "incomplete", "out_of_memory", "stopped" };
  return m >= 0 && m < (int)(sizeof n / sizeof *n) ? n[m] : "invalid";
}
