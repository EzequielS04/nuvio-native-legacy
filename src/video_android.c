// Player do Android TV. Quem toca e o Media3 ExoPlayer (NvPlayer.kt), numa
// SurfaceView ATRAS da SDLSurface do app; aqui fica so o estado que o resto do
// app le (video.h) e as chamadas ao Kotlin por JNI. Mesmo desenho do
// video_tpk.c (host .NET): o Kotlin avisa o que acontece pelos natives
// Java_space_nuvio_nativelegacy_NvPlayer_* de qualquer fio, o app le o estado
// no fio dele; por isso os campos sao volatile e nada aqui bloqueia.
//
// DIFERENCAS EM RELACAO AO .tpk
//  * Recorte de fonte de verdade (video_recorte_fonte = 1): o Kotlin posiciona
//    a SurfaceView por layout, entao um retangulo MAIOR que a tela e com origem
//    NEGATIVA e valido e o pai recorta o excedente. No Tizen 9 o ROI fora da
//    painel apagava o plano (#188/#195); aqui nao existe esse limite.
//  * O furo so abre com imagem: video_pronto() so fica 1 depois do evento 8
//    (onRenderedFirstFrame). player.c ja abre o furo quando player_com_video()
//    (comVideo && video_pronto()) e nao precisa mudar. Sem quadro nenhum
//    (so audio) o furo abre 3 s depois de tocar, como o .tpk faz.
//  * HDR/Dolby Vision/Atmos vem do Format das faixas (nativeHdr).
//  * Audio sem decoder: evento 9 (extensao do contrato), nao erro.
//
// Faixas: o Kotlin manda a lista depois que o player as conhece (nativeFaixa e
// nativeFaixasFim) e o app escolhe por escolher(tipo, idx). Legenda EMBUTIDA de
// texto: o Kotlin entrega os cues (nativeLegenda) e o app desenha. Legenda
// EXTERNA (OpenSubtitles/addon) nem passa por aqui: legenda.c baixa e desenha.
//
// TODO: ASS com libass (NV_ASS_LIBASS nao esta definido nesta fase, entao a
// sonda MKV segue "nao ha sonda", como no .tpk); capitulos do MKV para
// video_creditos; passthrough fino de AC3/EAC3 por AudioCapabilities.
#ifdef NV_ANDROID
#include "video.h"
#include "video_reconexao.h"
#include "idioma.h"
#include "linguas.h"
#include <SDL2/SDL.h>
#include <jni.h>
#include <stdio.h>
#include <string.h>

// --- ponte JNI ---------------------------------------------------------------
#define NV_CLASSE "space/nuvio/nativelegacy/NvPlayer"

static jclass    gCls;      // GlobalRef: FindClass de fio do SDL nao acha classe do app
static jmethodID mAbrir, mParar, mPausar, mBuscar, mVolume, mJanela, mEscolher;

static int resolverMetodos(JNIEnv *env) {
  mAbrir    = (*env)->GetStaticMethodID(env, gCls, "abrir", "(Ljava/lang/String;Ljava/lang/String;)V");
  mParar    = (*env)->GetStaticMethodID(env, gCls, "parar", "()V");
  mPausar   = (*env)->GetStaticMethodID(env, gCls, "pausar", "(I)V");
  mBuscar   = (*env)->GetStaticMethodID(env, gCls, "buscar", "(I)V");
  mVolume   = (*env)->GetStaticMethodID(env, gCls, "volume", "(I)V");
  mJanela   = (*env)->GetStaticMethodID(env, gCls, "janela", "(IIIII)V");
  mEscolher = (*env)->GetStaticMethodID(env, gCls, "escolher", "(II)V");
  if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); return 0; }
  return mAbrir && mParar && mPausar && mBuscar && mVolume && mJanela && mEscolher;
}

// Chamado pelo Kotlin (NvPlayer.iniciar), no fio principal, que ve a classe do
// app. Guarda a classe e os jmethodID para os fios do SDL usarem depois.
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeIniciar(JNIEnv *env, jclass cls) {
  if (!gCls) gCls = (*env)->NewGlobalRef(env, cls);
  if (!resolverMetodos(env)) { gCls = NULL; printf("[video] android: metodos do NvPlayer nao achados\n"); }
  else printf("[video] android: ponte do NvPlayer pronta\n");
  fflush(stdout);
}

// Plano B se o Kotlin nao chamou nativeIniciar antes do primeiro uso (lib
// carregada depois do iniciar): o ClassLoader da Activity enxerga a classe.
static int garantirPonte(JNIEnv *env) {
  jobject act;
  jclass cAct, cLoader, achada;
  jmethodID mGet, mLoad;
  jobject loader;
  jstring nome;
  if (gCls) return 1;
  act = (jobject)SDL_AndroidGetActivity();
  if (!act) return 0;
  cAct = (*env)->GetObjectClass(env, act);
  mGet = (*env)->GetMethodID(env, cAct, "getClassLoader", "()Ljava/lang/ClassLoader;");
  loader = mGet ? (*env)->CallObjectMethod(env, act, mGet) : NULL;
  cLoader = loader ? (*env)->GetObjectClass(env, loader) : NULL;
  mLoad = cLoader ? (*env)->GetMethodID(env, cLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;") : NULL;
  nome = (*env)->NewStringUTF(env, "space.nuvio.nativelegacy.NvPlayer");
  achada = (mLoad && nome) ? (jclass)(*env)->CallObjectMethod(env, loader, mLoad, nome) : NULL;
  if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); achada = NULL; }
  if (achada) { gCls = (*env)->NewGlobalRef(env, achada); if (!resolverMetodos(env)) gCls = NULL; }
  if (nome) (*env)->DeleteLocalRef(env, nome);
  if (achada) (*env)->DeleteLocalRef(env, achada);
  if (cLoader) (*env)->DeleteLocalRef(env, cLoader);
  if (loader) (*env)->DeleteLocalRef(env, loader);
  (*env)->DeleteLocalRef(env, cAct);
  (*env)->DeleteLocalRef(env, act);
  return gCls != NULL;
}

// UTF-8 -> jstring (UTF-16). O NewStringUTF do JNI e UTF-8 MODIFICADO e erra
// em tudo fora do BMP; cabecalhos e URLs raramente passam disso, mas o custo e
// pequeno.
static jstring paraJString(JNIEnv *env, const char *s) {
  jchar buf[4096];
  int n = 0;
  const unsigned char *p = (const unsigned char *)(s ? s : "");
  while (*p && n < 4094) {
    unsigned c = *p++, extra = 0;
    if (c >= 0xF0) { c &= 0x07; extra = 3; }
    else if (c >= 0xE0) { c &= 0x0F; extra = 2; }
    else if (c >= 0xC0) { c &= 0x1F; extra = 1; }
    else if (c >= 0x80) c = 0xFFFD;
    while (extra-- && (*p & 0xC0) == 0x80) c = (c << 6) | (*p++ & 0x3F);
    if (c >= 0x10000) { c -= 0x10000; buf[n++] = (jchar)(0xD800 + (c >> 10)); buf[n++] = (jchar)(0xDC00 + (c & 0x3FF)); }
    else buf[n++] = (jchar)c;
  }
  return (*env)->NewString(env, buf, n);
}

// jstring (UTF-16) -> UTF-8 de verdade em dst (GetStringUTFChars daria CESU-8
// para emoji, que o SDL_ttf nao le).
static void deJString(JNIEnv *env, jstring js, char *dst, size_t tam) {
  const jchar *u;
  jsize n, i;
  size_t o = 0;
  if (tam == 0) return;
  dst[0] = 0;
  if (!js) return;
  u = (*env)->GetStringChars(env, js, NULL);
  if (!u) return;
  n = (*env)->GetStringLength(env, js);
  for (i = 0; i < n && o + 5 < tam; i++) {
    unsigned c = u[i];
    if (c >= 0xD800 && c < 0xDC00 && i + 1 < n && u[i + 1] >= 0xDC00 && u[i + 1] < 0xE000) {
      c = 0x10000 + ((c - 0xD800) << 10) + (u[i + 1] - 0xDC00); i++;
    }
    if (c < 0x80) dst[o++] = (char)c;
    else if (c < 0x800) { dst[o++] = (char)(0xC0 | (c >> 6)); dst[o++] = (char)(0x80 | (c & 0x3F)); }
    else if (c < 0x10000) { dst[o++] = (char)(0xE0 | (c >> 12)); dst[o++] = (char)(0x80 | ((c >> 6) & 0x3F)); dst[o++] = (char)(0x80 | (c & 0x3F)); }
    else { dst[o++] = (char)(0xF0 | (c >> 18)); dst[o++] = (char)(0x80 | ((c >> 12) & 0x3F));
           dst[o++] = (char)(0x80 | ((c >> 6) & 0x3F)); dst[o++] = (char)(0x80 | (c & 0x3F)); }
  }
  dst[o] = 0;
  (*env)->ReleaseStringChars(env, js, u);
}

// Env do fio que chama (o SDL ja anexa o fio do app) ou NULL sem ponte.
static JNIEnv *ambiente(void) {
  JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
  if (!env || !garantirPonte(env)) return NULL;
  return env;
}
static void fimChamada(JNIEnv *env) {
  if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); printf("[video] android: excecao no NvPlayer\n"); fflush(stdout); }
}
static void kSemArg(jmethodID m) {
  JNIEnv *env = ambiente();
  if (!env) return;
  (*env)->CallStaticVoidMethod(env, gCls, m);
  fimChamada(env);
}
static void kInt(jmethodID m, int v) {
  JNIEnv *env = ambiente();
  if (!env) return;
  (*env)->CallStaticVoidMethod(env, gCls, m, (jint)v);
  fimChamada(env);
}

// --- estado ------------------------------------------------------------------
#define MAX_FAIXAS NV_FAIXA_MAX
static VideoFaixa faixaAudio[MAX_FAIXAS], faixaLeg[MAX_FAIXAS];
static volatile int nAudio, nLeg, audioAtual, legAtual = -1;
static SDL_mutex *travaLeg;
static char legTexto[1024];
static Uint32 legAte;

static char urlAtual[4096];
static char cabecalhos[2048];
// prontoLoad = o player preparou (evento 1); primeiroQuadro = evento 8.
// video_pronto() so e 1 com imagem (ver o cabecalho).
static volatile int ativo, prontoLoad, primeiroQuadro, falhou, terminou, tocando, largura, altura;
static volatile int conflito, semDecoderAudio;
static volatile int durMs, bufferando, posMs;
static volatile Uint32 bufferDesde, tocandoDesde;
static volatile const char *hdrAtual = "none";
static volatile int dvAtual, atmosAtual;
static unsigned sessao;
#define ANDROID_FURO_PRAZO_MS 3000u   // sem quadro (so audio): abre 3 s depois de tocar

// RECONEXAO (video_reconexao.h): igual ao .tpk. O evento 5 so ANOTA; a
// decisao e o recarregar sao do video_bombear.
static NvReconexao recon;
static int reconProxima, reconPermitida, reconIniciou;
static volatile int reconErroPend, reconErroCod;
static int reconAudio = -1, reconLeg = -1;
static volatile int reconFaixasPend;
static int reconBuscarMs = -1;
static char erroTxt[64];

// Codigos do PlaybackException (Media3) que sao de REDE: IO_UNSPECIFIED 2000,
// IO_NETWORK_CONNECTION_FAILED 2001, IO_NETWORK_CONNECTION_TIMEOUT 2002. NAO
// MEDIDO em aparelho: status HTTP (2004), 403/404 e formato quebrado ficam de
// fora de proposito, recarregar a mesma URL so repete a recusa.
static int erroDeRede(int cod) { return cod >= 2000 && cod <= 2002; }

void video_escolher_audio(int i);

// Mesma regra do video_tpk.c: sem preferencia ou sem faixa que case, fica a do
// arquivo.
static void escolherAudioPreferido(void) {
  const char *pref = ling_audio();
  int i;
  if (!pref[0] || nAudio < 2) return;
  if (audioAtual >= 0 && audioAtual < nAudio && faixaAudio[audioAtual].idioma[0] &&
      ling_casa(faixaAudio[audioAtual].idioma, pref)) return;
  for (i = 0; i < nAudio; i++) {
    if (!faixaAudio[i].idioma[0] || !ling_casa(faixaAudio[i].idioma, pref)) continue;
    printf("[video] audio preferido: %s (faixa %d de %d)\n", ling_nome(faixaAudio[i].idioma), i + 1, nAudio);
    video_escolher_audio(i);
    return;
  }
}

// --- natives (Kotlin -> C) ----------------------------------------------------
// Fio principal do Kotlin, depois que o player conhece as faixas: uma chamada
// por faixa (tipo 0 = audio, 1 = legenda) e no fim nativeFaixasFim. O contador
// so sobe no fim, com a lista inteira escrita.
static VideoFaixa novasA[MAX_FAIXAS], novasL[MAX_FAIXAS];
static int nNovasA, nNovasL;

JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixa(JNIEnv *env, jclass cls, jint tipo, jint idx, jstring lingua) {
  VideoFaixa *f;
  char l[16];
  (void)cls;
  deJString(env, lingua, l, sizeof l);
  if (tipo == 0) { if (nNovasA >= MAX_FAIXAS) return; f = &novasA[nNovasA++]; }
  else           { if (nNovasL >= MAX_FAIXAS) return; f = &novasL[nNovasL++]; }
  memset(f, 0, sizeof *f);
  f->numero = idx;
  f->ordinalMkv = tipo ? idx : -1;
  if (strcmp(l, "und") && strcmp(l, "unknown") && strcmp(l, "zxx")) snprintf(f->idioma, sizeof f->idioma, "%s", l);
  if (f->idioma[0]) snprintf(f->rotulo, sizeof f->rotulo, "%s", i18n(ling_nome(f->idioma)));
  else snprintf(f->rotulo, sizeof f->rotulo, "%s %d", i18n(tipo ? "Legenda" : "Áudio"),
                tipo ? nNovasL : nNovasA);
}

JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixasFim(JNIEnv *env, jclass cls, jint selAudio, jint selLeg) {
  // Lista de legendas VAZIA mandada com a de audio ja existente = releitura so
  // do audio (HLS que publica as faixas depois de tocar): a legenda que o app
  // ja escolheu fica.
  int releitura = (nAudio || nLeg) && !nNovasL && nLeg;
  (void)env; (void)cls; (void)selLeg;
  memcpy(faixaAudio, novasA, sizeof novasA);
  audioAtual = selAudio >= 0 ? selAudio : 0;
  if (!releitura) {
    memcpy(faixaLeg, novasL, sizeof novasL);
    legAtual = -1;   // o Kotlin sobe com legenda desligada; quem liga e o app (faixas.c)
    nLeg = nNovasL;
  }
  nAudio = nNovasA;
  nNovasA = nNovasL = 0;
  printf("[video] faixas: %d audio, %d legenda\n", nAudio, nLeg);
  fflush(stdout);
  // Recarregar de reconexao: devolve as faixas que a pessoa tinha.
  if (reconFaixasPend) {
    if (!releitura && reconLeg >= 0 && reconLeg < nLeg) video_escolher_legenda(reconLeg);
    if (nAudio > 0) {
      reconFaixasPend = 0;
      if (reconAudio > 0 && reconAudio < nAudio) video_escolher_audio(reconAudio);
    }
    printf("[video] reconexao: faixas devolvidas (audio %d, legenda %d)\n", reconAudio, reconLeg);
    fflush(stdout);
    return;
  }
  escolherAudioPreferido();
}

// Texto da legenda embutida em vigor, valido por `durMs` (estimada no Kotlin:
// o Media3 nao diz quando o cue acaba, so quando o proximo grupo chega).
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeLegenda(JNIEnv *env, jclass cls, jstring texto, jint dur) {
  char buf[sizeof legTexto];
  (void)cls;
  if (!travaLeg) return;
  deJString(env, texto, buf, sizeof buf);
  SDL_LockMutex(travaLeg);
  memcpy(legTexto, buf, sizeof legTexto);
  legAte = SDL_GetTicks() + (Uint32)(dur > 0 ? dur : 3000);
  SDL_UnlockMutex(travaLeg);
}

JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativePos(JNIEnv *env, jclass cls, jint ms) {
  (void)env; (void)cls;
  posMs = ms;
}

JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeHdr(JNIEnv *env, jclass cls, jstring hdr, jint dv, jint atmos) {
  char h[24];
  (void)cls;
  deJString(env, hdr, h, sizeof h);
  // So literais: video_hdr() devolve o ponteiro cru ao fio do app.
  hdrAtual = !strcmp(h, "HDR10") ? "HDR10" : !strcmp(h, "HLG") ? "HLG" :
             !strcmp(h, "DolbyVision") ? "DolbyVision" : "none";
  dvAtual = dv != 0;
  atmosAtual = atmos != 0;
  printf("[video] android hdr=%s dv=%d atmos=%d\n", (const char *)hdrAtual, dvAtual, atmosAtual);
  fflush(stdout);
}

enum { EV_PRONTO = 1, EV_TOCANDO = 2, EV_PAUSADO = 3, EV_FIM = 4, EV_ERRO = 5,
       EV_TAMANHO = 6, EV_BUFFER = 7, EV_PRIMEIRO_QUADRO = 8, EV_AUDIO_SEM_DECODER = 9 };

JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeEvento(JNIEnv *env, jclass cls, jint tipo, jint a, jint b) {
  (void)env; (void)cls;
  switch (tipo) {
    case EV_PRONTO:  durMs = a; prontoLoad = 1; break;
    case EV_TOCANDO: tocando = 1; bufferando = 0; if (!tocandoDesde) tocandoDesde = SDL_GetTicks() | 1; break;
    case EV_PAUSADO: tocando = 0; break;
    case EV_FIM:     terminou = 1; tocando = 0; break;
    // Sem `falhou` aqui: o video_bombear decide entre reconectar e desistir.
    case EV_ERRO:    reconErroCod = a; reconErroPend = 1; tocando = 0;
                     snprintf(erroTxt, sizeof erroTxt, "%d player error:%d", a, b);
                     printf("[video] android: erro do player %d (%d)\n", a, b); break;
    case EV_TAMANHO: largura = a; altura = b; break;
    case EV_BUFFER:
      if (a < 100 && !bufferando) { bufferando = 1; bufferDesde = SDL_GetTicks(); }
      else if (a >= 100) bufferando = 0;
      break;
    case EV_PRIMEIRO_QUADRO: primeiroQuadro = 1; break;
    case EV_AUDIO_SEM_DECODER: semDecoderAudio = 1; break;
    default: break;
  }
  if (tipo != EV_BUFFER) { printf("[video] android evento %d (%d, %d)\n", tipo, a, b); fflush(stdout); }
}

// --- video.h (C -> Kotlin) -----------------------------------------------------
int  video_iniciar(void) {
  JNIEnv *env;
  if (!travaLeg) travaLeg = SDL_CreateMutex();
  env = (JNIEnv *)SDL_AndroidGetJNIEnv();
  return env && garantirPonte(env);
}
int  video_iniciar_auto(void) { return video_iniciar(); }
int  video_registro_negado(void) { return 0; }

// Abre urlAtual no Kotlin. Serve a fonte nova e ao recarregar da reconexao.
static int abrirSessao(void) {
  JNIEnv *env;
  ativo = 1; prontoLoad = primeiroQuadro = falhou = terminou = tocando = 0;
  largura = altura = durMs = posMs = 0; bufferando = 1; bufferDesde = SDL_GetTicks();
  tocandoDesde = 0; semDecoderAudio = 0; erroTxt[0] = 0;
  hdrAtual = "none"; dvAtual = atmosAtual = 0;
  nAudio = nLeg = 0; audioAtual = 0; legAtual = -1; legAte = 0;
  nNovasA = nNovasL = 0;
  if (!travaLeg) travaLeg = SDL_CreateMutex();
  sessao++;
  env = ambiente();
  if (!env) { falhou = 1; printf("[video] android: NvPlayer indisponivel\n"); fflush(stdout); return 0; }
  {
    jstring u = paraJString(env, urlAtual), c = paraJString(env, cabecalhos);
    (*env)->CallStaticVoidMethod(env, gCls, mAbrir, u, c);
    (*env)->DeleteLocalRef(env, u);
    (*env)->DeleteLocalRef(env, c);
  }
  fimChamada(env);
  return 1;
}

int video_tocar(const char *u) {
  snprintf(urlAtual, sizeof urlAtual, "%s", u ? u : "");
  nv_recon_zerar(&recon);
  reconPermitida = reconProxima; reconProxima = 0;
  reconIniciou = 0; reconErroPend = 0;
  reconAudio = reconLeg = -1; reconFaixasPend = 0; reconBuscarMs = -1;
  return abrirSessao();
}

void video_definir_reconexao(int sim) { reconProxima = sim ? 1 : 0; }
int  video_reconectando(void) {
  return nv_recon_ativa(&recon) && (recon.pendente || !prontoLoad) ? recon.tentativa : 0;
}

void video_bombear(void) {
  double pos = video_pos();
  if (prontoLoad && pos > 0.5) reconIniciou = 1;
  if (prontoLoad && reconBuscarMs < 0) nv_recon_progresso(&recon, pos);
  // O seek do recarregar sai com o player ja tocando.
  if (reconBuscarMs >= 0 && prontoLoad && tocando) {
    kInt(mBuscar, reconBuscarMs);
    printf("[video] reconexao: retomado em %ds\n", reconBuscarMs / 1000);
    fflush(stdout);
    reconBuscarMs = -1;
  }
  if (reconErroPend) {
    int antes = recon.tentativa;
    int rede = erroDeRede(reconErroCod);
    reconErroPend = 0;
    if (reconPermitida && urlAtual[0] && (reconIniciou || recon.tentativa) &&
        nv_recon_erro(&recon, rede, SDL_GetTicks(), pos)) {
      if (!antes) { reconAudio = audioAtual; reconLeg = legAtual; }
      if (recon.tentativa != antes) {
        printf("[video] conexao caiu (%d): tentativa %d/%d, espera %us\n",
               reconErroCod, recon.tentativa, NV_RECON_MAX,
               nv_recon_espera_ms(recon.tentativa) / 1000u);
        fflush(stdout);
      }
      bufferando = 0;
    } else {
      if (recon.esgotou) { printf("[video] reconexao: desistiu depois de %d tentativas\n", NV_RECON_MAX); fflush(stdout); }
      falhou = 1;
    }
  }
  if (nv_recon_vencida(&recon, SDL_GetTicks())) {
    printf("[video] reconectando: alvo %.0fs, tentativa %d\n", recon.alvo, recon.tentativa);
    fflush(stdout);
    reconFaixasPend = 1;
    reconBuscarMs = recon.alvo > 1.0 ? (int)(recon.alvo * 1000.0) : -1;
    if (!abrirSessao()) { reconErroCod = -1; reconErroPend = 1; }
  }
}
void video_parar(void) {
  nv_recon_zerar(&recon);
  reconErroPend = 0; reconFaixasPend = 0; reconBuscarMs = -1;
  if (ativo) kSemArg(mParar);
  ativo = prontoLoad = primeiroQuadro = tocando = 0;
}
void video_pausar(int p) { kInt(mPausar, p ? 1 : 0); }
void video_volume(int pct) { kInt(mVolume, pct); }
void video_buscar(double s) {
  posMs = (int)(s * 1000.0);   // a barra nao pode voltar enquanto o seek corre
  kInt(mBuscar, posMs);
  terminou = 0;
}
// `encaixa` = 1: o quadro ENCAIXA no retangulo mantendo a proporcao (tarja),
// que e o que o plano de video da LG e o LetterBox da Samsung fazem com uma
// janela lisa — o nucleo conta com isso (trailer "Original", janela antes do
// videoInfo). 0: o retangulo e exato, ja calculado com a proporcao (recorte).
static void janelaKt(int x, int y, int w, int h, int encaixa) {
  JNIEnv *env = ambiente();
  if (!env) return;
  (*env)->CallStaticVoidMethod(env, gCls, mJanela, (jint)x, (jint)y, (jint)w, (jint)h, (jint)encaixa);
  fimChamada(env);
}
void video_janela(int x, int y, int w, int h) { janelaKt(x, y, w, h, 1); }

// RECORTE DE FONTE PELO RETANGULO DE DESTINO AMPLIADO. Mesma conta do ROI do
// .tpk (video_tpk.c, #178): desenhar o recorte (sx,sy,sw,sh) dentro de
// (dx,dy,dw,dh) e o MESMO que desenhar o quadro INTEIRO num retangulo maior,
// deslocado para o pedaco desejado cair sobre o destino:
//   escala = dw/sw, dh/sh;  W,H = quadro * escala;  X,Y = d - s * escala.
// Aqui o retangulo pode sair da tela e ter origem negativa: o Kotlin aplica por
// layout na SurfaceView e o pai recorta o excedente, sem a restricao do Tizen.
static int ultX, ultY, ultW, ultH, temUlt;
void video_janela_fonte(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {
  double qw = largura, qh = altura, ex, ey;
  int X, Y, W, H;

  // Sem as dimensoes do quadro, ou sem recorte de verdade, o destino cru serve.
  if (qw < 2.0 || qh < 2.0 || sw <= 0 || sh <= 0) { temUlt = 0; video_janela(dx, dy, dw, dh); return; }
  // Recorte que cobre o quadro inteiro E o caso sem zoom: mesma coisa.
  if (sx <= 0 && sy <= 0 && sw >= (int)qw && sh >= (int)qh) { temUlt = 0; video_janela(dx, dy, dw, dh); return; }

  ex = (double)dw / (double)sw;
  ey = (double)dh / (double)sh;
  W  = (int)(qw * ex + 0.5);
  H  = (int)(qh * ey + 0.5);
  X  = (int)(dx - sx * ex + (dx - sx * ex < 0 ? -0.5 : 0.5));
  Y  = (int)(dy - sy * ey + (dy - sy * ey < 0 ? -0.5 : 0.5));

  printf("[video] android recorte %d,%d %dx%d de %.0fx%.0f -> janela %d,%d %dx%d\n",
         sx, sy, sw, sh, qw, qh, X, Y, W, H);
  fflush(stdout);
  ultX = X; ultY = Y; ultW = W; ultH = H; temUlt = 1;
  janelaKt(X, Y, W, H, 0);
}
int  video_recorte_fonte(void) { return 1; }
void video_recorte_reaplicar(void) { if (temUlt) janelaKt(ultX, ultY, ultW, ultH, 0); }
// O Kotlin escala 1920x1080 de layout para a camada real; nada a fazer aqui.
void video_escala_definir(int sw, int sh) { (void)sw; (void)sh; }

const char *video_url_atual(void) { return urlAtual; }
double video_pos(void) { return prontoLoad ? posMs / 1000.0 : 0; }
double video_duracao(void) { return durMs / 1000.0; }
double video_creditos(void) { return 0.0; }   // TODO: capitulos do MKV
double video_buffer_fim(void) { return 0; }
unsigned video_bufferando_ms(void) {
  // Esperando para reconectar: o watchdog (app.c) nao troca de fonte.
  if (nv_recon_ativa(&recon)) return 0;
  return bufferando ? SDL_GetTicks() - bufferDesde : 0;
}
void video_definir_dv(int dv) { (void)dv; }
void video_definir_cabecalhos(const char *c) { snprintf(cabecalhos, sizeof cabecalhos, "%s", c ? c : ""); }
void video_definir_mp4(int m) { (void)m; }
int  video_tocando(void) { return tocando; }
// 1 SO COM IMAGEM: e o que abre o furo (player_com_video). Fonte sem quadro
// (so audio) cai no prazo de ANDROID_FURO_PRAZO_MS depois de tocar.
int  video_pronto(void) {
  Uint32 desde;
  if (!prontoLoad) return 0;
  if (primeiroQuadro) return 1;
  desde = tocandoDesde;
  return desde && tocando && SDL_GetTicks() - desde >= ANDROID_FURO_PRAZO_MS;
}
int  video_ativo(void) { return ativo; }
int  video_falhou(void) { return falhou; }
int  video_audio_nao_suportado(void) { return semDecoderAudio; }
int  video_terminou(void) { return terminou; }
// Foco de audio perdido = pausa (NvPlayer); nao ha "outro app com o video".
int  video_conflito_recurso(void) { return conflito; }
const char *video_erro_texto(void) { return falhou ? erroTxt : ""; }
// O Media3 nao da o sinal de decoder anunciado: valem os neutros do .tpk.
int  video_decoder_anunciou(void) { return 1; }

int  video_n_audio(void) { return nAudio; }
int  video_n_legenda(void) { return nLeg; }
const VideoFaixa *video_audio(int i) { return (i >= 0 && i < nAudio) ? &faixaAudio[i] : 0; }
const VideoFaixa *video_legenda(int i) { return (i >= 0 && i < nLeg) ? &faixaLeg[i] : 0; }
int  video_legenda_ordinal_mkv(int i) { return (i >= 0 && i < nLeg) ? faixaLeg[i].ordinalMkv : -1; }
// "Nao ha sonda": sem libass (NV_ASS_LIBASS) a sonda de MKV nao serve a nada.
// TODO quando o libass entrar: sonda real, como video.c.
int  video_mkv_sondado(void) { return 2; }
void video_sondar_mkv_agora(void) {}
int  video_audio_atual(void) { return audioAtual; }
int  video_legenda_atual(void) { return legAtual; }
void video_escolher_audio(int i) {
  if (i < 0 || i >= nAudio) return;
  audioAtual = i;
  {
    JNIEnv *env = ambiente();
    if (!env) return;
    (*env)->CallStaticVoidMethod(env, gCls, mEscolher, (jint)0, (jint)faixaAudio[i].numero);
    fimChamada(env);
  }
}
void video_escolher_legenda(int i) {
  if (i >= nLeg) return;
  legAtual = i;
  if (travaLeg) { SDL_LockMutex(travaLeg); legTexto[0] = 0; legAte = 0; SDL_UnlockMutex(travaLeg); }
  {
    JNIEnv *env = ambiente();
    if (!env) return;
    // -1 desliga a legenda no Kotlin.
    (*env)->CallStaticVoidMethod(env, gCls, mEscolher, (jint)1, (jint)(i >= 0 ? faixaLeg[i].numero : -1));
    fimChamada(env);
  }
}
int  video_legenda_nativa(char *d, int t) {
  if (!d || t < 2) return 0;
  d[0] = 0;
  if (!ativo || legAtual < 0 || !travaLeg) return 0;
  SDL_LockMutex(travaLeg);
  if (legTexto[0] && (Sint32)(legAte - SDL_GetTicks()) > 0) snprintf(d, (size_t)t, "%s", legTexto);
  SDL_UnlockMutex(travaLeg);
  return d[0] != 0;
}
void video_legenda_externa(const char *u) { (void)u; }
// O atraso do estilo vale para a embutida: o Kotlin guarda e adia os cues
// (so atraso positivo; adiantar um cue que ainda nao chegou nao da).
void video_legenda_estilo(const VideoLegendaEstilo *e) {
  static int ultAtraso = 0x7fffffff;
  if (!e || e->atrasoMs == ultAtraso) return;
  ultAtraso = e->atrasoMs;
  {
    JNIEnv *env = ambiente();
    if (!env) return;
    (*env)->CallStaticVoidMethod(env, gCls, mEscolher, (jint)2, (jint)e->atrasoMs);
    fimChamada(env);
  }
}
int  video_tem_atmos(void) { return atmosAtual; }
int  video_tem_dolby_vision(void) { return dvAtual; }
const char *video_hdr(void) { return (const char *)hdrAtual; }
int  video_largura(void) { return largura; }
int  video_altura(void) { return altura; }
int  video_pode_forcar_sdr(void) { return 0; }
void video_forcar_sdr(void) {}
void video_encerrar(void) { video_parar(); }
#endif
