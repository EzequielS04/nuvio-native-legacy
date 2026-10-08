#include "arranque.h"
#ifdef NV_WEBOS
#include "queda.h"
#include "avisos.h"
#include "ajustes.h"
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#define ARQ_ETAPA   "/tmp/nuvio-arranque.txt"
#define ARQ_ANTES   "/tmp/nuvio-arranque-anterior.txt"
#define ARQ_QUEDA   "/tmp/nuvio-queda-arranque.txt"
#define ARQ_PEND    "/tmp/nuvio-arranque-pendente.txt"  // relato ainda nao enviado
#define ARQ_FIM     "quadro-1"   // passou daqui = nao e falha de arranque

void arranque_etapa(const char *nome) {
  char b[96];
  int n = snprintf(b, sizeof b, "%s t=%ld\n", nome, (long)time(NULL));
  int fd = open(ARQ_ETAPA, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0 || n <= 0) { if (fd >= 0) close(fd); return; }
  if (write(fd, b, (size_t)n) < 0) { /* sem rastro e so isso */ }
  close(fd);
}

// Antes de qualquer outro construtor (o padrao e 65535; libtorrent e FFmpeg
// estao no binario, e o C++ estatico deles roda nessa faixa).
__attribute__((constructor(101)))
static void arranqueConstrutor(void) {
  rename(ARQ_ETAPA, ARQ_ANTES);
  arranque_etapa("construtores");
  queda_armar(ARQ_QUEDA);
}

// Chamado por avisos_iniciar depois de rearmar o tratador para <dados>/queda.txt:
// o relato continua indo tambem para o arquivo fixo de /tmp.
void arranque_espelhar_queda(void) { queda_espelho(ARQ_QUEDA); }

static char *relatoPendente;
static char relatoTv[64];

void arranque_relatar(void) {
  char b[160] = "", linhaTv[256] = "";
  struct utsname u;
  char mem[64] = "", kern[160] = "";
  char *rq = NULL; size_t nrq = 0;
  FILE *f = fopen(ARQ_ANTES, "r"), *m;
  int morreuNoArranque, temQueda;
  if (f) { if (!fgets(b, sizeof b, f)) b[0] = 0; fclose(f); unlink(ARQ_ANTES); }
  f = fopen("/proc/meminfo", "r");
  if (f) { char l[96]; unsigned long kb;
    while (fgets(l, sizeof l, f))
      if (sscanf(l, "MemTotal: %lu", &kb) == 1) { snprintf(mem, sizeof mem, "MemTotal=%luMB", kb / 1024); break; }
    fclose(f); }
  if (uname(&u) == 0) snprintf(kern, sizeof kern, "[arranque] kernel=%s %s %s\n", u.release, u.machine, mem);
  fputs(kern, stdout);
  b[strcspn(b, "\n")] = 0;
  morreuNoArranque = b[0] && strncmp(b, ARQ_FIM, sizeof ARQ_FIM - 1) != 0;
  if (morreuNoArranque) printf("[arranque] sessao anterior parou na etapa: %s\n", b);
  // O relato de queda vai ao log E a um texto proprio (open_memstream), que e o
  // que o envio de arranque leva.
  m = open_memstream(&rq, &nrq);
  temQueda = queda_relatar_em(ARQ_QUEDA, m ? m : stdout);
  if (m) { fclose(m); if (rq) fputs(rq, stdout); }
  fflush(stdout);
  if (morreuNoArranque || temQueda) {
    FILE *p;
    char *novo = malloc(rq ? nrq + 2048 : 2048), antigo[8192] = "";
    // Um relato que nao saiu (sem rede) fica em ARQ_PEND e vai junto na proxima.
    p = fopen(ARQ_PEND, "r");
    if (p) { size_t k = fread(antigo, 1, sizeof antigo - 1, p); antigo[k] = 0; fclose(p); }
    avisos_tv_linha(linhaTv, sizeof linhaTv, relatoTv, sizeof relatoTv);
    if (novo) {
      snprintf(novo, (rq ? nrq : 0) + 2048, "%s%s\n%s%s%s%s%s",
               antigo[0] ? "--- relato anterior sem envio ---\n" : "", antigo, linhaTv[0] ? linhaTv : "[tv] ?",
               kern[0] ? "\n" : "", kern,
               b[0] ? "[arranque] ultima etapa: " : "[arranque] sem rastro de etapa", b);
      { size_t l = strlen(novo);
        snprintf(novo + l, (rq ? nrq : 0) + 2048 - l, "\n%s", rq ? rq : ""); }
      relatoPendente = novo;
      p = fopen(ARQ_PEND, "w");
      if (p) { fputs(novo, p); fclose(p); }
    }
  }
  free(rq);
}

// DEPOIS de rede_preparar, ANTES de app_iniciar (main.c). So manda quando a
// sessao anterior morreu no arranque (relato acima) e SO com "Enviar registros
// sozinho" ligado: e o consentimento do aparelho e o relato pode ter nome do
// modelo. Uma vez por relato: apaga ARQ_PEND ao confirmar; se falhar, o texto
// fica para a proxima abertura (sem laco, sem retentativa agora).
void arranque_enviar(void) {
  char *r = relatoPendente;
  relatoPendente = NULL;
  if (!r) return;
  if (!ajustes_envio_auto()) {
    printf("[arranque] relato de falha guardado, envio automatico desligado\n"); fflush(stdout);
    free(r); return;
  }
  arranque_etapa("enviar-relato");
  if (avisos_enviar_arranque(r, relatoTv[0] ? relatoTv : "webos", 4)) unlink(ARQ_PEND);
  free(r);
}
#else
void arranque_etapa(const char *nome) { (void)nome; }
void arranque_relatar(void) {}
void arranque_enviar(void) {}
void arranque_espelhar_queda(void) {}
#endif
