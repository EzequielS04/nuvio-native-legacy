// Ponte do nucleo com o Android. Ver android.h.
#ifdef NV_ANDROID
#include "android.h"
#include <SDL2/SDL.h>
#include <android/log.h>
#include <jni.h>
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
  // Mesma pilha para os fios criados pelo SDL (SDL_CreateThread).
  SDL_SetHint(SDL_HINT_THREAD_STACK_SIZE, "8388608");
  __android_log_write(ANDROID_LOG_INFO, AND_TAG, "nucleo C iniciando");
  // O Voltar chega como SDLK_AC_BACK ao app; sem isto o SDL fecha a Activity.
  SDL_SetHint("SDL_ANDROID_TRAP_BACK_BUTTON", "1");
  espelharNoLogcat();
  logaTv();
}

// SUPERFICIE 4K. A TCL Smart TV Pro (Android 14) tem painel 3840x2160 mas poe
// os apps numa tela LOGICA de 1920x1080 (`wm size` override): a interface de
// todo app e desenhada em 1080p e ampliada. O plano de video nao passa por isso
// (o 4K do filme sai nitido). Uma superficie com buffer fixo de 3840x2160 e
// composta pelo SurfaceFlinger no espaco FISICO; se o plano de graficos da TV
// aceitar, a UI sai em 4K de verdade. O que a TV concedeu aparece na linha
// `janela=... drawable=...` do main.c.
int android_pedir_superficie(int w, int h) {
  JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
  jobject act = (jobject)SDL_AndroidGetActivity();
  jclass cls;
  jmethodID m;
  int ok = 0;
  if (!env || !act) return 0;
  // Classe pela propria Activity: FindClass deste fio (o do SDL) usa o
  // carregador do sistema e nao acha classe do app.
  cls = (*env)->GetObjectClass(env, act);
  m = cls ? (*env)->GetMethodID(env, cls, "pedirSuperficie", "(II)Z") : NULL;
  if (m) ok = (*env)->CallBooleanMethod(env, act, m, (jint)w, (jint)h) ? 1 : 0;
  if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); ok = 0; }
  if (cls) (*env)->DeleteLocalRef(env, cls);
  (*env)->DeleteLocalRef(env, act);
  printf("[4k] android: superficie %dx%d %s\n", w, h, ok ? "concedida" : "NAO veio (segue o tamanho da tela)");
  fflush(stdout);
  return ok;
}

int android_instalar_apk(const char *caminho) {
  JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
  jobject act = (jobject)SDL_AndroidGetActivity();
  jclass cls;
  jmethodID m;
  jstring js;
  int r = 0;
  if (!env || !act || !caminho) return 0;
  cls = (*env)->GetObjectClass(env, act);
  m = cls ? (*env)->GetMethodID(env, cls, "instalarApk", "(Ljava/lang/String;)I") : NULL;
  js = (*env)->NewStringUTF(env, caminho);
  if (m && js) r = (int)(*env)->CallIntMethod(env, act, m, js);
  if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); r = 0; }
  if (js) (*env)->DeleteLocalRef(env, js);
  if (cls) (*env)->DeleteLocalRef(env, cls);
  (*env)->DeleteLocalRef(env, act);
  printf("[atualizacao] android: instalador %s\n", r == 1 ? "aberto" : r == 2 ? "pede permissao" : "falhou");
  fflush(stdout);
  return r;
}

// TEXTO DO SISTEMA (sistexto.h): teclado do sistema e voz. Tudo chamado do fio
// do SDL; o NuvioActivity faz o trabalho no fio da interface e devolve o que
// aconteceu numa fila de strings que android_st_evento drena.
static int chamaBool(const char *nome, const char *assin, const char *s, int i, int temInt) {
  JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
  jobject act = (jobject)SDL_AndroidGetActivity();
  jclass cls;
  jmethodID m;
  jstring js = NULL;
  int ok = 0;
  if (!env || !act) return 0;
  cls = (*env)->GetObjectClass(env, act);
  m = cls ? (*env)->GetMethodID(env, cls, nome, assin) : NULL;
  if (s) js = (*env)->NewStringUTF(env, s);
  if (m) {
    if (s && temInt) ok = (*env)->CallBooleanMethod(env, act, m, js, (jint)i) ? 1 : 0;
    else if (s) ok = (*env)->CallBooleanMethod(env, act, m, js) ? 1 : 0;
    else ok = (*env)->CallBooleanMethod(env, act, m) ? 1 : 0;
  }
  if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); ok = 0; }
  if (js) (*env)->DeleteLocalRef(env, js);
  if (cls) (*env)->DeleteLocalRef(env, cls);
  (*env)->DeleteLocalRef(env, act);
  return ok;
}

int android_st_teclado(const char *inicial, int max) {
  return chamaBool("abrirTeclado", "(Ljava/lang/String;I)Z", inicial ? inicial : "", max, 1);
}
int android_st_ditar(const char *idioma) {
  return chamaBool("ditar", "(Ljava/lang/String;)Z", idioma ? idioma : "", 0, 0);
}
void android_st_fechar(void) { chamaBool("fecharEntrada", "()Z", NULL, 0, 0); }

int android_st_evento(char *dst, size_t n) {
  JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
  jobject act = (jobject)SDL_AndroidGetActivity();
  jclass cls;
  jmethodID m;
  jstring js = NULL;
  int r = 0;
  if (n) dst[0] = 0;
  if (!env || !act) return 0;
  cls = (*env)->GetObjectClass(env, act);
  m = cls ? (*env)->GetMethodID(env, cls, "proximoEvento", "()Ljava/lang/String;") : NULL;
  if (m) js = (jstring)(*env)->CallObjectMethod(env, act, m);
  if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); js = NULL; }
  if (js) {
    const char *c = (*env)->GetStringUTFChars(env, js, NULL);
    if (c) { snprintf(dst, n, "%s", c); r = 1; (*env)->ReleaseStringUTFChars(env, js, c); }
    (*env)->DeleteLocalRef(env, js);
  }
  if (cls) (*env)->DeleteLocalRef(env, cls);
  (*env)->DeleteLocalRef(env, act);
  return r;
}

// PILHA DOS FIOS. O bionic da ~1 MB a um pthread criado sem atributo; o glibc
// da LG e do Tizen da 8 MB, e o nucleo foi escrito contando com isso (vetores
// de CatItem, DiagAddon, VazCand... na pilha dos fios de descoberta e
// diagnostico). O CMake liga com --wrap=pthread_create: so as chamadas desta
// biblioteca passam por aqui. Quem ja pede tamanho (descoberta.c, mapa.c) fica
// como esta.
#define AND_PILHA_FIO (8u << 20)
int __real_pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*f)(void *), void *arg);
int __wrap_pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*f)(void *), void *arg) {
  pthread_attr_t at;
  int r;
  if (a) return __real_pthread_create(t, a, f, arg);
  pthread_attr_init(&at);
  pthread_attr_setstacksize(&at, AND_PILHA_FIO);
  r = __real_pthread_create(t, &at, f, arg);
  pthread_attr_destroy(&at);
  return r;
}

#endif
