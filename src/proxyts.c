// PROXY DE TS DA LIVE TV. Ver proxyts.h para o porque.
#include "proxyts.h"
#include <string.h>

#if defined(__EMSCRIPTEN__) || defined(NV_TPK) || defined(NV_ANDROID)
int  proxyts_disponivel(void) { return 0; }
int  proxyts_url(const char *f, char *s, size_t n) { (void)f; (void)s; (void)n; return 0; }
void proxyts_parar(void) {}
int  proxyts_e_url(const char *u) { (void)u; return 0; }
const char *proxyts_resolver(const char *url, char *buf, size_t n) {
  (void)buf; (void)n;
  return url && !strncmp(url, PROXYTS_PREFIXO, 8) ? url + 8 : url;
}
#else

#include "rede.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0   // Mac: SO_NOSIGPIPE no socket
#endif

#define PX_SEG_MAX      64        // segmentos lidos de uma playlist
#define PX_SEG_TETO     (48L << 20)  // um segmento nunca passa disto
#define PX_PONTA        3         // comeca 3 segmentos antes do fim (como os players)
#define PX_SEM_NOVO_S   40        // sem segmento novo por isto: a fonte morreu
#define PX_FALHAS_MAX   6         // pedidos de playlist seguidos que falharam

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static int sockOuvir = -1, porta;
static char fonteAtual[4096];
static atomic_uint sessaoAtual;
// O download do segmento em curso, para a troca de canal cortar na hora (a
// conta de 1 tela recusaria o canal novo com o velho ainda baixando).
static volatile int *cancelarEmCurso;
// A conexao que esta entregando HLS: uma nova da mesma sessao (o uMS reabre)
// encerra a anterior, para nunca haver dois downloads na conta de 1 tela.
static void *conexaoAtiva;

// --- playlist -----------------------------------------------------------------
typedef struct {
  int  alvo;            // EXT-X-TARGETDURATION (s)
  long seq0;            // EXT-X-MEDIA-SEQUENCE
  int  n;
  char uri[PX_SEG_MAX][1024];
  int  mestre;          // e master playlist: uri[0] e a variante escolhida
  int  fim;             // EXT-X-ENDLIST
} PxLista;

static void pxJuntar(const char *base, const char *rel, char *dst, size_t n) {
  size_t semQuery = strcspn(base, "?"), barra = 0, i;
  if (!strncmp(rel, "http://", 7) || !strncmp(rel, "https://", 8)) { snprintf(dst, n, "%s", rel); return; }
  if (rel[0] == '/') {
    const char *h = strstr(base, "://"), *b = h ? strchr(h + 3, '/') : NULL;
    snprintf(dst, n, "%.*s%s", b ? (int)(b - base) : (int)strlen(base), base, rel);
    return;
  }
  for (i = 0; i < semQuery; i++) if (base[i] == '/') barra = i;
  snprintf(dst, n, "%.*s/%s", (int)barra, base, rel);
}

// Le uma playlist (master ou de midia). Na master, fica a primeira variante
// (a ordem do provedor) em uri[0] com mestre=1.
static int pxLer(const char *texto, const char *base, PxLista *l) {
  const char *p = texto;
  int proximaEhVariante = 0;
  memset(l, 0, sizeof *l);
  l->alvo = 6;
  if (!texto || strncmp(texto, "#EXTM3U", 7)) return 0;
  while (p && *p) {
    size_t k = strcspn(p, "\r\n");
    char linha[1024];
    snprintf(linha, sizeof linha, "%.*s", (int)(k < sizeof linha ? k : sizeof linha - 1), p);
    if (!strncmp(linha, "#EXT-X-TARGETDURATION:", 22)) l->alvo = atoi(linha + 22);
    else if (!strncmp(linha, "#EXT-X-MEDIA-SEQUENCE:", 22)) l->seq0 = atol(linha + 22);
    else if (!strncmp(linha, "#EXT-X-ENDLIST", 14)) l->fim = 1;
    else if (!strncmp(linha, "#EXT-X-STREAM-INF", 17)) proximaEhVariante = 1;
    else if (linha[0] && linha[0] != '#') {
      if (proximaEhVariante) {
        if (!l->mestre) { l->mestre = 1; pxJuntar(base, linha, l->uri[0], sizeof l->uri[0]); l->n = 1; }
        proximaEhVariante = 0;
      } else if (!l->mestre && l->n < PX_SEG_MAX) {
        pxJuntar(base, linha, l->uri[l->n], sizeof l->uri[0]);
        l->n++;
      }
    }
    p += k;
    while (*p == '\r' || *p == '\n') p++;
  }
  if (l->alvo < 1) l->alvo = 1;
  if (l->alvo > 15) l->alvo = 15;
  return l->n > 0;
}

// --- MPEG-TS ------------------------------------------------------------------
// Guarda o PAT e o PMT do segmento (o ultimo visto) para repor no comeco de um
// segmento que nao comece por eles.
typedef struct { unsigned char pat[188], pmt[188]; int temPat, temPmt, pmtPid; } PxTabelas;

static int pxPid(const unsigned char *k) { return ((k[1] & 0x1F) << 8) | k[2]; }

static void pxColher(const unsigned char *b, long n, PxTabelas *t) {
  long i;
  for (i = 0; i + 188 <= n; i += 188) {
    const unsigned char *k = b + i;
    int pid;
    if (k[0] != 0x47) break;
    pid = pxPid(k);
    if (pid == 0 && (k[1] & 0x40)) {
      int af = (k[3] >> 4) & 3, off = 4 + (af == 3 ? 1 + k[4] : 0), p, sl, j;
      if (off >= 187) continue;
      p = off + 1 + k[off];
      if (p + 12 > 188 || k[p] != 0) continue;
      sl = ((k[p + 1] & 0x0F) << 8) | k[p + 2];
      for (j = p + 8; j + 4 <= p + 3 + sl - 4 && j + 4 <= 188; j += 4)
        if ((k[j] << 8 | k[j + 1]) != 0) { t->pmtPid = ((k[j + 2] & 0x1F) << 8) | k[j + 3]; break; }
      memcpy(t->pat, k, 188); t->temPat = 1;
    } else if (t->pmtPid && pid == t->pmtPid && (k[1] & 0x40)) {
      memcpy(t->pmt, k, 188); t->temPmt = 1;
    }
    if (t->temPat && t->temPmt && i > 188 * 64) break;
  }
}

// Onde comecam os pacotes alinhados (pode haver lixo antes); -1 = nao e TS.
static long pxInicio(const unsigned char *b, long n) {
  long i;
  for (i = 0; i < 188 && i + 376 < n; i++)
    if (b[i] == 0x47 && b[i + 188] == 0x47 && b[i + 376] == 0x47) return i;
  return -1;
}

// --- envio ----------------------------------------------------------------------
static int pxEnviar(int s, const void *b, size_t n) {
  const char *p = b;
  while (n) {
    ssize_t r = send(s, p, n, MSG_NOSIGNAL);
    if (r < 0 && errno == EINTR) continue;
    if (r <= 0) return 0;
    p += r; n -= (size_t)r;
  }
  return 1;
}

typedef struct { int s; unsigned sessao; char fonte[4096]; volatile int cancelar; } PxConexao;

static int pxViva(const PxConexao *c) {
  return !c->cancelar && atomic_load(&sessaoAtual) == c->sessao;
}

static void pxDormir(const PxConexao *c, int ms) {
  while (ms > 0 && pxViva(c)) { usleep(100 * 1000); ms -= 100; }
}

// A playlist (de midia) ao vivo, com a variante resolvida. `final` recebe o
// endereco depois dos redirecionamentos — e a base dos segmentos.
static int pxPlaylist(const char *url, PxLista *l, char *final, size_t nf) {
  char atual[4096];
  int volta;
  snprintf(atual, sizeof atual, "%s", url);
  for (volta = 0; volta < 3; volta++) {
    long n = 0;
    int st = 0, er = 0;
    char fin[4096];
    char *b;
    fin[0] = 0;
    b = rede_baixar_trecho_st(atual, 8, 0, 1048575, &n, &st, &er, fin, sizeof fin);
    if (!b || n <= 0) { free(b); return 0; }
    if (fin[0]) snprintf(atual, sizeof atual, "%s", fin);
    if (!pxLer(b, atual, l)) { free(b); return 0; }
    free(b);
    if (!l->mestre) { snprintf(final, nf, "%s", atual); return 1; }
    snprintf(atual, sizeof atual, "%s", l->uri[0]);
  }
  return 0;
}

static void pxHls(PxConexao *c, const char *urlLista) {
  PxLista *lp = malloc(sizeof *lp);
  PxTabelas tab;
  char base[4096];
  long ultimo = -1, segs = 0, bytes = 0;
  int falhas = 0;
  time_t desdeNovo = time(NULL), t0 = time(NULL);
  memset(&tab, 0, sizeof tab);
  if (!lp) return;
#define l (*lp)
  while (pxViva(c)) {
    int i, novos = 0;
    if (!pxPlaylist(urlLista, &l, base, sizeof base)) {
      if (++falhas >= PX_FALHAS_MAX) { printf("[proxy-ts] playlist falhou %d vezes: fim\n", falhas); break; }
      pxDormir(c, 1000);
      continue;
    }
    falhas = 0;
    // A PRIMEIRA LEITURA comeca perto da ponta; playlist que voltou atras
    // (painel reiniciou o canal) recomeca da ponta tambem.
    if (ultimo < 0 || l.seq0 + l.n - 1 < ultimo) {
      long ini = l.seq0 + (l.n > PX_PONTA ? l.n - PX_PONTA : 0);
      if (ultimo >= 0) printf("[proxy-ts] a playlist voltou (seq %ld < %ld): recomecando da ponta\n",
                              l.seq0 + l.n - 1, ultimo);
      else printf("[proxy-ts] HLS: %d segmento(s), alvo %d s, comecando no %ld\n", l.n, l.alvo,
                  ini - l.seq0);
      fflush(stdout);
      ultimo = ini - 1;
    }
    for (i = 0; i < l.n && pxViva(c); i++) {
      long seq = l.seq0 + i, n = 0, ini;
      RedeMedida md;
      RedeControle ctl;
      unsigned char *b;
      if (seq <= ultimo) continue;
      memset(&md, 0, sizeof md);
      ctl.max_bytes = PX_SEG_TETO;
      ctl.cancelado = &c->cancelar;
      pthread_mutex_lock(&trava); cancelarEmCurso = &c->cancelar; pthread_mutex_unlock(&trava);
      b = (unsigned char *)rede_baixar_bin_medido_controle(l.uri[i], 15, NULL, &ctl, &n, &md);
      pthread_mutex_lock(&trava); if (cancelarEmCurso == &c->cancelar) cancelarEmCurso = NULL; pthread_mutex_unlock(&trava);
      if (!pxViva(c)) { free(b); break; }
      ultimo = seq;
      if (!b || n < 188 || (ini = pxInicio(b, n)) < 0) {
        printf("[proxy-ts] segmento %ld sem TS (HTTP %d, %ld B): pulado\n", seq, md.status, n);
        fflush(stdout);
        free(b);
        continue;
      }
      // PAT/PMT ANTES DO PRIMEIRO PACOTE do segmento (o "203 AV Type Not
      // Founded" do segmento unico na C9): se o segmento nao abre com o PAT,
      // repoe o ultimo PAT/PMT visto antes dele.
      pxColher(b + ini, n - ini, &tab);
      if (pxPid(b + ini) != 0 && tab.temPat && tab.temPmt) {
        if (!pxEnviar(c->s, tab.pat, 188) || !pxEnviar(c->s, tab.pmt, 188)) { free(b); goto fim; }
      }
      if (!pxEnviar(c->s, b + ini, (size_t)((n - ini) / 188 * 188))) { free(b); goto fim; }
      segs++; bytes += n - ini; novos++;
      if (segs <= 3 || segs % 30 == 0) {
        printf("[proxy-ts] segmento %ld: %ld KB em %lu ms (%s)\n", segs, (n - ini) / 1024, md.ms,
               pxPid(b + ini) == 0 ? "abre com PAT" : tab.temPat ? "PAT/PMT reposto" : "sem PAT");
        fflush(stdout);
      }
      free(b);
      desdeNovo = time(NULL);
    }
    if (l.fim) { printf("[proxy-ts] playlist com ENDLIST: fim\n"); break; }
    if (!novos && time(NULL) - desdeNovo > PX_SEM_NOVO_S) {
      printf("[proxy-ts] %d s sem segmento novo: fim\n", PX_SEM_NOVO_S);
      break;
    }
    // REFRESH: metade do alvo quando nada veio (o segmento seguinte esta para
    // sair), o alvo inteiro depois de entregar.
    pxDormir(c, (novos ? l.alvo * 1000 : l.alvo * 500));
  }
fim:
#undef l
  free(lp);
  printf("[proxy-ts] sessao %u encerrada: %ld segmento(s), %ld KB em %ld s\n", c->sessao, segs,
         bytes / 1024, (long)(time(NULL) - t0));
  fflush(stdout);
}

static void *pxAtender(void *u) {
  PxConexao *c = u;
  char req[4096], cab[512];
  ssize_t r, tot = 0;
  unsigned s = 0;
  const char *q;
  // O PEDIDO: so a primeira linha interessa (GET /live.ts?s=N).
  while (tot < (ssize_t)sizeof req - 1) {
    r = recv(c->s, req + tot, sizeof req - 1 - (size_t)tot, 0);
    if (r <= 0) break;
    tot += r; req[tot] = 0;
    if (strstr(req, "\r\n\r\n") || strstr(req, "\n\n")) break;
  }
  req[tot > 0 ? tot : 0] = 0;
  q = strstr(req, "?s=");
  if (q) s = (unsigned)strtoul(q + 3, NULL, 10);
  pthread_mutex_lock(&trava);
  if (s && s == atomic_load(&sessaoAtual)) snprintf(c->fonte, sizeof c->fonte, "%s", fonteAtual);
  else c->fonte[0] = 0;
  pthread_mutex_unlock(&trava);
  c->sessao = s;
  if (!c->fonte[0] || strncmp(req, "GET ", 4)) {
    static const char nao[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    pxEnviar(c->s, nao, sizeof nao - 1);
    goto sair;
  }
  // O QUE A FONTE E: um trecho do comeco decide entre TS continuo e playlist.
  { long n = 0;
    int st = 0, er = 0;
    char fin[4096];
    char *b;
    fin[0] = 0;
    b = rede_baixar_trecho_st(c->fonte, 8, 0, 65535, &n, &st, &er, fin, sizeof fin);
    if (b && n >= 7 && !strncmp(b, "#EXTM3U", 7)) {
      free(b);
      snprintf(cab, sizeof cab, "HTTP/1.1 200 OK\r\nContent-Type: video/mp2t\r\n"
               "Cache-Control: no-cache\r\nConnection: close\r\n\r\n");
      pthread_mutex_lock(&trava);
      if (conexaoAtiva) ((PxConexao *)conexaoAtiva)->cancelar = 1;
      conexaoAtiva = c;
      pthread_mutex_unlock(&trava);
      if (pxEnviar(c->s, cab, strlen(cab))) {
        printf("[proxy-ts] sessao %u: a fonte e playlist HLS; entregando TS continuo\n", c->sessao);
        fflush(stdout);
        pxHls(c, fin[0] ? fin : c->fonte);
      }
      pthread_mutex_lock(&trava);
      if (conexaoAtiva == c) conexaoAtiva = NULL;
      pthread_mutex_unlock(&trava);
    } else {
      // TS CONTINUO (ou qualquer outra coisa): o player vai direto a fonte,
      // como antes do proxy. O 302 nao custa nada ao caminho que ja tocava.
      int ts = b && n > 376 && pxInicio((unsigned char *)b, n) >= 0;
      free(b);
      printf("[proxy-ts] sessao %u: a fonte %s (HTTP %d, %ld B): redirecionando o player\n",
             c->sessao, ts ? "ja e TS continuo" : "nao e playlist", st, n);
      fflush(stdout);
      snprintf(cab, sizeof cab, "HTTP/1.1 302 Found\r\nLocation: %s\r\nContent-Length: 0\r\n"
               "Connection: close\r\n\r\n", c->fonte);
      pxEnviar(c->s, cab, strlen(cab));
    }
  }
sair:
  close(c->s);
  free(c);
  return NULL;
}

static void *pxOuvir(void *u) {
  (void)u;
  for (;;) {
    int s = accept(sockOuvir, NULL, NULL);
    PxConexao *c;
    pthread_t t;
    if (s < 0) { if (errno == EINTR) continue; usleep(200 * 1000); continue; }
#ifdef SO_NOSIGPIPE
    { int um = 1; setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &um, sizeof um); }
#endif
    { struct timeval tv = { 10, 0 }; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv); }
    c = calloc(1, sizeof *c);
    if (!c) { close(s); continue; }
    c->s = s;
    if (pthread_create(&t, NULL, pxAtender, c) == 0) pthread_detach(t);
    else { close(s); free(c); }
  }
  return NULL;
}

static int pxSubir(void) {
  struct sockaddr_in a;
  socklen_t al = sizeof a;
  pthread_t t;
  int um = 1;
  if (sockOuvir >= 0) return 1;
  signal(SIGPIPE, SIG_IGN);
  sockOuvir = socket(AF_INET, SOCK_STREAM, 0);
  if (sockOuvir < 0) return 0;
  setsockopt(sockOuvir, SOL_SOCKET, SO_REUSEADDR, &um, sizeof um);
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  a.sin_port = 0;   // porta livre
  if (bind(sockOuvir, (struct sockaddr *)&a, sizeof a) < 0 || listen(sockOuvir, 8) < 0 ||
      getsockname(sockOuvir, (struct sockaddr *)&a, &al) < 0) {
    close(sockOuvir); sockOuvir = -1; return 0;
  }
  porta = ntohs(a.sin_port);
  if (pthread_create(&t, NULL, pxOuvir, NULL) != 0) { close(sockOuvir); sockOuvir = -1; return 0; }
  pthread_detach(t);
  printf("[proxy-ts] ouvindo em 127.0.0.1:%d\n", porta);
  fflush(stdout);
  return 1;
}

int proxyts_disponivel(void) { return 1; }

int proxyts_url(const char *fonte, char *saida, size_t n) {
  unsigned s;
  if (!fonte || !fonte[0] || !saida || n < 40) return 0;
  pthread_mutex_lock(&trava);
  if (!pxSubir()) { pthread_mutex_unlock(&trava); return 0; }
  if (cancelarEmCurso) *cancelarEmCurso = 1;
  snprintf(fonteAtual, sizeof fonteAtual, "%s", fonte);
  s = atomic_fetch_add(&sessaoAtual, 1) + 1;
  pthread_mutex_unlock(&trava);
  snprintf(saida, n, "http://127.0.0.1:%d/live.ts?s=%u", porta, s);
  return 1;
}

void proxyts_parar(void) {
  pthread_mutex_lock(&trava);
  if (cancelarEmCurso) *cancelarEmCurso = 1;
  atomic_fetch_add(&sessaoAtual, 1);
  fonteAtual[0] = 0;
  pthread_mutex_unlock(&trava);
}

int proxyts_e_url(const char *url) {
  return url && !strncmp(url, "http://127.0.0.1:", 17) && strstr(url, "/live.ts?s=");
}

const char *proxyts_resolver(const char *url, char *buf, size_t n) {
  if (url && !strncmp(url, PROXYTS_PREFIXO, 8)) {
    if (proxyts_url(url + 8, buf, n)) return buf;
    return url + 8;
  }
  if (atomic_load(&sessaoAtual) && fonteAtual[0]) proxyts_parar();
  return url;
}

#endif
