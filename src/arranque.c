#include "arranque.h"
#ifdef NV_WEBOS
#include "queda.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#define ARQ_ETAPA   "/tmp/nuvio-arranque.txt"
#define ARQ_ANTES   "/tmp/nuvio-arranque-anterior.txt"
#define ARQ_QUEDA   "/tmp/nuvio-queda-arranque.txt"
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

void arranque_relatar(void) {
  char b[160] = "";
  struct utsname u;
  char mem[64] = "";
  FILE *f = fopen(ARQ_ANTES, "r");
  if (f) { if (!fgets(b, sizeof b, f)) b[0] = 0; fclose(f); unlink(ARQ_ANTES); }
  f = fopen("/proc/meminfo", "r");
  if (f) { char l[96]; unsigned long kb;
    while (fgets(l, sizeof l, f))
      if (sscanf(l, "MemTotal: %lu", &kb) == 1) { snprintf(mem, sizeof mem, "MemTotal=%luMB", kb / 1024); break; }
    fclose(f); }
  if (uname(&u) == 0) printf("[arranque] kernel=%s %s %s\n", u.release, u.machine, mem);
  b[strcspn(b, "\n")] = 0;
  if (b[0] && strncmp(b, ARQ_FIM, sizeof ARQ_FIM - 1) != 0)
    printf("[arranque] sessao anterior parou na etapa: %s\n", b);
  queda_relatar(ARQ_QUEDA);
  fflush(stdout);
}
#else
void arranque_etapa(const char *nome) { (void)nome; }
void arranque_relatar(void) {}
#endif
