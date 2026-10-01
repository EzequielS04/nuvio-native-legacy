// Ponte do nucleo com o Android. Ver android.h.
#ifdef NV_ANDROID
#include "android.h"
#include <SDL2/SDL.h>
#include <android/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define AND_TAG "nuvio"

// A linha de modelo que o host .tpk escreve (tizen-tpk/Program.cs, LogaTv):
//   [tv] modelo=<modelo> host=<host> dotnet=<...> tela=<WxH>
// Aqui o "host" e o Android e nao ha .NET. NUVIO_TV_INFO vem do NuvioActivity:
//   "<MANUFACTURER> <MODEL>|<SDK_INT>|<RELEASE>|<versionName>"
// A tela ainda nao existe neste ponto (o SDL nem iniciou); a linha `janela=` do
// main.c ja diz o drawable.
static void logaTv(void) {
  char info[256], *campo[4] = { "", "", "", "" }, *p = info;
  const char *e = getenv("NUVIO_TV_INFO");
  int n = 0;
  snprintf(info, sizeof info, "%s", e ? e : "");
  campo[n++] = p;
  while (*p && n < 4) { if (*p == '|') { *p = 0; campo[n++] = p + 1; } p++; }
  printf("[tv] modelo=%s host=android-%s dotnet=- tela=- sdk=%s app=%s\n",
         campo[0][0] ? campo[0] : "?", campo[2][0] ? campo[2] : "?",
         campo[1][0] ? campo[1] : "?", campo[3][0] ? campo[3] : "?");
  fflush(stdout);
}

// ESPELHO NO LOGCAT. stdout e stderr (o mesmo descritor do arquivo de log, ver
// main.c) passam a entrar num pipe; um fio le o pipe e grava os mesmos bytes no
// arquivo (descritor guardado antes) e, linha a linha, no logcat. O arquivo
// continua inteiro: e ele que o painel de log e o "Enviar registro" leem.
static int fdArquivo = -1, fdLeitura = -1;

static void *espelho(void *arg) {
  char buf[2048], linha[1100];
  size_t nl = 0;
  ssize_t n;
  (void)arg;
  while ((n = read(fdLeitura, buf, sizeof buf)) > 0) {
    ssize_t i;
    if (fdArquivo >= 0) { ssize_t off = 0; while (off < n) { ssize_t w = write(fdArquivo, buf + off, (size_t)(n - off)); if (w <= 0) break; off += w; } }
    for (i = 0; i < n; i++) {
      if (buf[i] == '\n' || nl + 1 >= sizeof linha) {
        linha[nl] = 0;
        if (nl) __android_log_write(ANDROID_LOG_INFO, AND_TAG, linha);
        nl = 0;
        if (buf[i] != '\n') linha[nl++] = buf[i];
      } else linha[nl++] = buf[i];
    }
  }
  return NULL;
}

static void espelharNoLogcat(void) {
  int p[2];
  pthread_t t;
  fflush(stdout); fflush(stderr);
  if (pipe(p) != 0) return;
  fdArquivo = dup(STDOUT_FILENO);   // o arquivo de log (ou /dev/null)
  fdLeitura = p[0];
  if (pthread_create(&t, NULL, espelho, NULL) != 0) {
    close(p[0]); close(p[1]); if (fdArquivo >= 0) close(fdArquivo); fdArquivo = -1;
    return;
  }
  pthread_detach(t);
  dup2(p[1], STDOUT_FILENO);
  dup2(p[1], STDERR_FILENO);
  close(p[1]);
  setvbuf(stdout, NULL, _IOLBF, 0);
}

void android_iniciar(void) {
  __android_log_write(ANDROID_LOG_INFO, AND_TAG, "nucleo C iniciando");
  // O Voltar chega como SDLK_AC_BACK ao app; sem isto o SDL fecha a Activity.
  SDL_SetHint("SDL_ANDROID_TRAP_BACK_BUTTON", "1");
  espelharNoLogcat();
  logaTv();
}
#endif
