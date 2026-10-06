#include "queda.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(__EMSCRIPTEN__) || defined(_WIN32)
void queda_armar(const char *arquivo) { (void)arquivo; }
int  queda_relatar(const char *arquivo) { (void)arquivo; return 0; }
#else
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#if defined(__linux__)
#include <ucontext.h>
#endif

#define QD_PILHA_N   192          // palavras da pilha copiadas
#define QD_MAPAS_MAX 24000        // bytes de /proc/self/maps copiados
#define QD_ALT_TAM   (64 * 1024)  // pilha alternativa: estouro de pilha tambem e SIGSEGV

static char qdArq[512];
static unsigned char qdAlt[QD_ALT_TAM];

// TUDO NO TRATADOR E SO write(): nada de printf, malloc ou trava. O processo
// esta quebrado e o heap pode ser justamente o que quebrou.
static void qdTexto(int fd, const char *s) { size_t n = strlen(s); while (n) { ssize_t w = write(fd, s, n); if (w <= 0) return; s += w; n -= (size_t)w; } }
static void qdHex(int fd, uintptr_t v) {
  char b[2 + sizeof v * 2 + 1]; int i = (int)sizeof b - 1;
  b[i] = 0;
  do { b[--i] = "0123456789abcdef"[v & 15]; v >>= 4; } while (v && i > 2);
  b[--i] = 'x'; b[--i] = '0';
  qdTexto(fd, b + i);
}
static void qdCampo(int fd, const char *nome, uintptr_t v) { qdTexto(fd, nome); qdHex(fd, v); }

static void qdTratar(int sig, siginfo_t *si, void *ctx) {
  uintptr_t pc = 0, lr = 0, sp = 0;
  int fd = open(qdArq, O_WRONLY | O_CREAT | O_TRUNC, 0644);
#if defined(__linux__) && defined(__arm__)
  { ucontext_t *u = ctx;
    pc = u->uc_mcontext.arm_pc; lr = u->uc_mcontext.arm_lr; sp = u->uc_mcontext.arm_sp; }
#elif defined(__linux__) && defined(__aarch64__)
  { ucontext_t *u = ctx;
    pc = u->uc_mcontext.pc; lr = u->uc_mcontext.regs[30]; sp = u->uc_mcontext.sp; }
#else
  (void)ctx;
#endif
  if (fd >= 0) {
    qdCampo(fd, "sinal=", (uintptr_t)sig);
    qdCampo(fd, " codigo=", (uintptr_t)(si ? si->si_code : 0));
    qdCampo(fd, " addr=", (uintptr_t)(si ? si->si_addr : 0));
    qdCampo(fd, " pc=", pc); qdCampo(fd, " lr=", lr); qdCampo(fd, " sp=", sp);
    qdTexto(fd, "\npilha=");
    // A pilha crua: quem le separa depois o que cai dentro de codigo. Sem sp
    // (alvo sem registradores) nao ha o que copiar. Alinhado a palavra.
    if (sp && !(sp & (sizeof(uintptr_t) - 1)))
      for (int i = 0; i < QD_PILHA_N; i++) { qdHex(fd, ((uintptr_t *)sp)[i]); qdTexto(fd, " "); }
    qdTexto(fd, "\nmapas:\n");
    { int m = open("/proc/self/maps", O_RDONLY);
      if (m >= 0) {
        char b[2048]; long total = 0; ssize_t n;
        while (total < QD_MAPAS_MAX && (n = read(m, b, sizeof b)) > 0) {
          ssize_t o = 0;
          while (o < n) { ssize_t w = write(fd, b + o, (size_t)(n - o)); if (w <= 0) break; o += w; }
          total += n;
        }
        close(m);
      } }
    close(fd);
  }
  qdTexto(1, "[queda] sinal fatal: relato gravado\n");
  // SA_RESETHAND ja devolveu a acao padrao: voltar repete a falha e o sistema
  // mata o processo como mataria sem nos (e gera o crash report dele).
  if (sig == SIGABRT) { signal(SIGABRT, SIG_DFL); raise(SIGABRT); }
}

void queda_armar(const char *arquivo) {
  static const int sinais[] = { SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL };
  struct sigaction sa;
  stack_t alt;
  if (!arquivo || !arquivo[0] || strlen(arquivo) >= sizeof qdArq) return;
  snprintf(qdArq, sizeof qdArq, "%s", arquivo);
  alt.ss_sp = qdAlt; alt.ss_size = sizeof qdAlt; alt.ss_flags = 0;
  sigaltstack(&alt, NULL);
  memset(&sa, 0, sizeof sa);
  sa.sa_sigaction = qdTratar;
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
  sigemptyset(&sa.sa_mask);
  for (size_t i = 0; i < sizeof sinais / sizeof *sinais; i++) sigaction(sinais[i], &sa, NULL);
}

// --- leitura, na abertura seguinte --------------------------------------------
typedef struct { uintptr_t ini, fim, desl; int exec; char nome[96]; } QdMapa;

static const char *qdBase(const char *caminho) {
  const char *b = strrchr(caminho, '/');
  return b ? b + 1 : caminho;
}
// "modulo+0xdesl" para um endereco; 0 se nao cai em mapa nenhum.
static int qdOnde(const QdMapa *m, int n, uintptr_t e, int soExec, char *out, size_t cap) {
  for (int i = 0; i < n; i++) {
    if (e < m[i].ini || e >= m[i].fim || (soExec && !m[i].exec)) continue;
    snprintf(out, cap, "%s+0x%lx", m[i].nome[0] ? m[i].nome : "?", (unsigned long)(e - m[i].ini + m[i].desl));
    return 1;
  }
  return 0;
}

int queda_relatar(const char *arquivo) {
  FILE *f = arquivo ? fopen(arquivo, "rb") : NULL;
  char *txt, *pilha, *mapas, *p;
  QdMapa *m; int nm = 0, capm = 512;
  long tam; size_t lido;
  unsigned long sinal = 0, codigo = 0, addr = 0, pc = 0, lr = 0, sp = 0;
  char onde[160];
  if (!f) return 0;
  fseek(f, 0, SEEK_END); tam = ftell(f); fseek(f, 0, SEEK_SET);
  if (tam <= 0 || tam > 256 * 1024) { fclose(f); remove(arquivo); return 0; }
  txt = malloc((size_t)tam + 1);
  m = calloc((size_t)capm, sizeof *m);
  if (!txt || !m) { free(txt); free(m); fclose(f); return 0; }
  lido = fread(txt, 1, (size_t)tam, f); txt[lido] = 0;
  fclose(f);
  remove(arquivo);
  if (sscanf(txt, "sinal=%lx codigo=%lx addr=%lx pc=%lx lr=%lx sp=%lx", &sinal, &codigo, &addr, &pc, &lr, &sp) < 3) {
    printf("[queda] relato ilegivel (%ld bytes)\n", tam);
    free(txt); free(m); return 1;
  }
  pilha = strstr(txt, "\npilha=");
  mapas = strstr(txt, "\nmapas:\n");
  if (mapas) { *mapas = 0; mapas += 8; }
  for (p = mapas; p && *p && nm < capm; ) {
    char *fimL = strchr(p, '\n'), perm[8] = "", cam[256] = "";
    unsigned long a, b, d;
    if (fimL) *fimL = 0;
    if (sscanf(p, "%lx-%lx %7s %lx %*s %*s %255[^\n]", &a, &b, perm, &d, cam) >= 4) {
      m[nm].ini = a; m[nm].fim = b; m[nm].desl = d; m[nm].exec = perm[2] == 'x';
      snprintf(m[nm].nome, sizeof m[nm].nome, "%s", qdBase(cam));
      nm++;
    }
    if (!fimL) break;
    p = fimL + 1;
  }
  printf("[queda] a sessao anterior morreu com sinal %lu (codigo %lu), endereco 0x%lx\n", sinal, codigo, addr);
  if (qdOnde(m, nm, pc, 0, onde, sizeof onde)) printf("[queda] pc %s\n", onde);
  else printf("[queda] pc 0x%lx (fora de qualquer modulo)\n", pc);
  if (qdOnde(m, nm, lr, 0, onde, sizeof onde)) printf("[queda] lr %s\n", onde);
  // Da pilha so interessa o que aponta para CODIGO: sao os candidatos a
  // endereco de retorno. Nao e um desenrolar exato (ha lixo de quadros velhos),
  // mas com os simbolos do pacote mostra o caminho ate a falha.
  if (pilha) {
    int achados = 0;
    for (p = pilha + 7; *p && *p != '\n' && achados < 24; ) {
      char *fimN; unsigned long v = strtoul(p, &fimN, 16);
      if (fimN == p) break;
      if (qdOnde(m, nm, v, 1, onde, sizeof onde)) { printf("[queda] pilha %s\n", onde); achados++; }
      p = fimN; while (*p == ' ') p++;
    }
  }
  fflush(stdout);
  free(txt); free(m);
  return 1;
}
#endif
