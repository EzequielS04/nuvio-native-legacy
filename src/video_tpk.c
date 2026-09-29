// Player do .tpk da Samsung. Quem toca e o host .NET (Tizen.Multimedia.Player,
// tizen-tpk/Program.cs), no plano de video da TV, por baixo do GLWindow; aqui
// fica so o estado que o resto do app le (video.h) e as chamadas ao host.
//
// O host registra as funcoes dele uma vez (nv_tpk_video_registrar) e avisa o
// que acontece pelo nv_tpk_video_evento, de qualquer fio. O app le o estado no
// fio dele; por isso os campos sao volatile e nada aqui bloqueia.
//
// Faixas: o host manda a lista depois do prepare (nv_tpk_video_faixa) e o app
// escolhe por hEscolher. Legenda EMBUTIDA: o player entrega o texto por evento
// (SubtitleUpdated, com duracao) e o app desenha, igual ao Tizen web. Legenda
// EXTERNA (OpenSubtitles/addon) nem passa por aqui: legenda.c baixa e desenha.
#ifdef NV_TPK
#include "video.h"
#include "idioma.h"
#include "linguas.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

typedef void (*FnAbrir)(const char *url, const char *cabecalhos);
typedef void (*FnSemArg)(void);
typedef void (*FnInt)(int);
typedef void (*FnRet)(int x, int y, int w, int h);
typedef int  (*FnPos)(void);

static FnAbrir  hAbrir;
static FnSemArg hParar;
static FnInt    hPausar, hBuscar, hVolume;
static FnRet    hJanela;
static FnPos    hPos;
typedef void (*FnEscolher)(int tipo, int idx);
static FnEscolher hEscolher;

#define MAX_FAIXAS 32
static VideoFaixa faixaAudio[MAX_FAIXAS], faixaLeg[MAX_FAIXAS];
static volatile int nAudio, nLeg, audioAtual, legAtual = -1;
static SDL_mutex *travaLeg;
static char legTexto[1024];
static Uint32 legAte;

static char urlAtual[4096];
static char cabecalhos[2048];
static volatile int ativo, pronto, falhou, terminou, tocando, largura, altura;
static volatile int durMs, bufferando;
static volatile Uint32 bufferDesde;
static unsigned sessao;

__attribute__((visibility("default")))
void nv_tpk_video_registrar(FnAbrir abrir, FnSemArg parar, FnInt pausar, FnInt buscar,
                            FnInt volume, FnRet janela, FnPos pos) {
  hAbrir = abrir; hParar = parar; hPausar = pausar; hBuscar = buscar;
  hVolume = volume; hJanela = janela; hPos = pos;
}

__attribute__((visibility("default")))
void nv_tpk_video_registrar_faixas(FnEscolher escolher) { hEscolher = escolher; }

void video_escolher_audio(int i);

// Mesma regra do video.c / video_tizen.c: sem preferencia ou sem faixa que
// case, fica a do arquivo.
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

// Host, fio principal, depois do prepare: uma chamada por faixa (tipo 0 =
// audio, 1 = legenda) e no fim nv_tpk_video_faixas_fim. O contador so sobe no
// fim, com a lista inteira escrita: o app nunca le faixa pela metade.
static VideoFaixa novasA[MAX_FAIXAS], novasL[MAX_FAIXAS];
static int nNovasA, nNovasL;
__attribute__((visibility("default")))
void nv_tpk_video_faixa(int tipo, int idx, const char *lingua) {
  VideoFaixa *f;
  const char *l = lingua ? lingua : "";
  if (tipo == 0) { if (nNovasA >= MAX_FAIXAS) return; f = &novasA[nNovasA++]; }
  else           { if (nNovasL >= MAX_FAIXAS) return; f = &novasL[nNovasL++]; }
  memset(f, 0, sizeof *f);
  f->numero = idx;
  f->ordinalMkv = tipo ? idx : -1;
  if (strcmp(l, "und") && strcmp(l, "unknown")) snprintf(f->idioma, sizeof f->idioma, "%s", l);
  if (f->idioma[0]) snprintf(f->rotulo, sizeof f->rotulo, "%s", i18n(ling_nome(f->idioma)));
  else snprintf(f->rotulo, sizeof f->rotulo, "%s %d", i18n(tipo ? "Legenda" : "Áudio"),
                tipo ? nNovasL : nNovasA);
}
__attribute__((visibility("default")))
void nv_tpk_video_faixas_fim(int selAudio, int selLeg) {
  // Segunda leitura (o host rele o audio com o video ja tocando, #165): so o
  // audio muda; a legenda que o app ja escolheu fica.
  int releitura = (nAudio || nLeg) && !nNovasL && nLeg;
  memcpy(faixaAudio, novasA, sizeof novasA);
  audioAtual = selAudio >= 0 ? selAudio : 0;
  if (!releitura) {
    memcpy(faixaLeg, novasL, sizeof novasL);
    legAtual = -1;   // a TV ate pode ter uma escolhida; quem liga e o app (faixas.c)
    nLeg = nNovasL;
  }
  (void)selLeg;
  nAudio = nNovasA;
  nNovasA = nNovasL = 0;
  printf("[video] faixas: %d audio, %d legenda\n", nAudio, nLeg);
  fflush(stdout);
  escolherAudioPreferido();
}

// Host: texto da legenda embutida escolhida, valido por `durMs`.
__attribute__((visibility("default")))
void nv_tpk_video_legenda(const char *texto, int durMs) {
  if (!travaLeg) return;
  SDL_LockMutex(travaLeg);
  snprintf(legTexto, sizeof legTexto, "%s", texto ? texto : "");
  legAte = SDL_GetTicks() + (Uint32)(durMs > 0 ? durMs : 3000);
  SDL_UnlockMutex(travaLeg);
}

enum { EV_PRONTO = 1, EV_TOCANDO = 2, EV_PAUSADO = 3, EV_FIM = 4, EV_ERRO = 5,
       EV_TAMANHO = 6, EV_BUFFER = 7 };

__attribute__((visibility("default")))
void nv_tpk_video_evento(int tipo, int a, int b) {
  switch (tipo) {
    case EV_PRONTO:  durMs = a; pronto = 1; break;
    case EV_TOCANDO: tocando = 1; bufferando = 0; break;
    case EV_PAUSADO: tocando = 0; break;
    case EV_FIM:     terminou = 1; tocando = 0; break;
    case EV_ERRO:    falhou = 1; tocando = 0;
                     printf("[video] tpk: erro do player 0x%x (%d)\n", a, b); break;
    case EV_TAMANHO: largura = a; altura = b; break;
    case EV_BUFFER:
      if (a < 100 && !bufferando) { bufferando = 1; bufferDesde = SDL_GetTicks(); }
      else if (a >= 100) bufferando = 0;
      break;
    default: break;
  }
  if (tipo != EV_BUFFER) { printf("[video] tpk evento %d (%d, %d)\n", tipo, a, b); fflush(stdout); }
}

int  video_iniciar(void) { if (!travaLeg) travaLeg = SDL_CreateMutex(); return hAbrir != NULL; }
int  video_iniciar_auto(void) { return hAbrir != NULL; }
int  video_registro_negado(void) { return 0; }

int video_tocar(const char *u) {
  snprintf(urlAtual, sizeof urlAtual, "%s", u ? u : "");
  ativo = 1; pronto = falhou = terminou = tocando = 0;
  largura = altura = durMs = 0; bufferando = 1; bufferDesde = SDL_GetTicks();
  nAudio = nLeg = 0; audioAtual = 0; legAtual = -1; legAte = 0;
  if (!travaLeg) travaLeg = SDL_CreateMutex();
  sessao++;
  if (!hAbrir) { falhou = 1; printf("[video] tpk: host sem player\n"); return 0; }
  hAbrir(urlAtual, cabecalhos);
  return 1;
}

void video_bombear(void) {}
void video_parar(void) {
  if (ativo && hParar) hParar();
  ativo = pronto = tocando = 0;
}
void video_pausar(int p) { if (hPausar) hPausar(p); }
void video_volume(int pct) { if (hVolume) hVolume(pct); }
void video_buscar(double s) {
  if (hBuscar) hBuscar((int)(s * 1000.0));
  terminou = 0;
}
void video_janela(int x, int y, int w, int h) { if (hJanela) hJanela(x, y, w, h); }

// RECORTE DE FONTE EMULADO PELO RETANGULO DE DESTINO (ROI).
//
// O webOS recorta pela FONTE: o ACB aceita (sx,sy,sw,sh) do quadro decodificado
// mais um destino, e os modos de aspecto do player saem disso. O
// Tizen.Multimedia.Player (tizen-tpk/Video.cs) NAO tem retangulo de fonte — so
// DisplaySettings.SetRoi, que e o DESTINO na tela. A primeira versao disto
// descartava a fonte e aplicava so o destino, e o resultado era que TODO modo
// de aspecto desenhava o mesmo retangulo: na TV o botao de recorte/zoom nao
// mudava nada, em nenhum modo (#178).
//
// A conta que substitui: desenhar o recorte (sx,sy,sw,sh) dentro de
// (dx,dy,dw,dh) e o MESMO que desenhar o quadro INTEIRO num retangulo maior,
// deslocado para que o pedaco desejado caia sobre o destino.
//
//   escalaX = dw/sw            (quanto a fonte e ampliada na horizontal)
//   escalaY = dh/sh            (idem vertical)
//   W = qw * escalaX           (o quadro inteiro nessa escala)
//   H = qh * escalaY
//   X = dx - sx * escalaX      (recua a origem para o recorte cair em dx)
//   Y = dy - sy * escalaY
//
// O que sobra para fora da tela e o que o recorte descartaria. O ROI resultante
// pode ser MAIOR que a tela e ter origem NEGATIVA — e Video.cs.Janela deixa
// esse retangulo passar cru ao SetRoi (so cai em LetterBox no quadro cheio sem
// zoom). NAO VERIFICADO numa TV Samsung se o firmware honra ROI fora da tela;
// por isso esta build sai como canario. Se o firmware grampear, o zoom nao
// acontece, mas a imagem continua na tela — a causa fica do lado do firmware e
// o log abaixo mostra o retangulo pedido.
static int ultRoiX, ultRoiY, ultRoiW, ultRoiH, temRoi;
void video_janela_fonte(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {
  double qw = largura, qh = altura, ex, ey;
  int X, Y, W, H;

  // Sem as dimensoes do quadro, ou sem recorte de verdade, o destino cru serve.
  if (qw < 2.0 || qh < 2.0 || sw <= 0 || sh <= 0) { temRoi = 0; video_janela(dx, dy, dw, dh); return; }
  // Recorte que cobre o quadro inteiro E o caso sem zoom: mesma coisa.
  if (sx <= 0 && sy <= 0 && sw >= (int)qw && sh >= (int)qh) { temRoi = 0; video_janela(dx, dy, dw, dh); return; }

  ex = (double)dw / (double)sw;
  ey = (double)dh / (double)sh;
  W  = (int)(qw * ex + 0.5);
  H  = (int)(qh * ey + 0.5);
  X  = (int)(dx - sx * ex + 0.5);
  Y  = (int)(dy - sy * ey + 0.5);

  printf("[video] tpk recorte %d,%d %dx%d de %.0fx%.0f -> roi %d,%d %dx%d\n",
         sx, sy, sw, sh, qw, qh, X, Y, W, H);
  fflush(stdout);

  ultRoiX = X; ultRoiY = Y; ultRoiW = W; ultRoiH = H; temRoi = 1;
  video_janela(X, Y, W, H);
}
int  video_recorte_fonte(void) { return 1; }
// O host prende o plano em mais de um ponto depois do prepare; um ROI pedido
// cedo pode ser engolido. trailer.c/player.c repetem o pedido nos primeiros
// segundos por aqui — reenvia o ultimo ROI calculado, sem recalcular.
void video_recorte_reaplicar(void) { if (temRoi) video_janela(ultRoiX, ultRoiY, ultRoiW, ultRoiH); }
const char *video_url_atual(void) { return urlAtual; }
double video_pos(void) { return (hPos && pronto) ? hPos() / 1000.0 : 0; }
double video_duracao(void) { return durMs / 1000.0; }
double video_creditos(void) { return 0.0; }
double video_buffer_fim(void) { return 0; }
unsigned video_bufferando_ms(void) { return bufferando ? SDL_GetTicks() - bufferDesde : 0; }
void video_definir_dv(int dv) { (void)dv; }
void video_definir_cabecalhos(const char *c) { snprintf(cabecalhos, sizeof cabecalhos, "%s", c ? c : ""); }
void video_definir_mp4(int m) { (void)m; }
int  video_tocando(void) { return tocando; }
int  video_pronto(void) { return pronto; }
int  video_ativo(void) { return ativo; }
int  video_falhou(void) { return falhou; }
int  video_audio_nao_suportado(void) { return 0; }
int  video_terminou(void) { return terminou; }
int  video_n_audio(void) { return nAudio; }
int  video_n_legenda(void) { return nLeg; }
const VideoFaixa *video_audio(int i) { return (i >= 0 && i < nAudio) ? &faixaAudio[i] : 0; }
const VideoFaixa *video_legenda(int i) { return (i >= 0 && i < nLeg) ? &faixaLeg[i] : 0; }
int  video_legenda_ordinal_mkv(int i) { return (i >= 0 && i < nLeg) ? faixaLeg[i].ordinalMkv : -1; }
int  video_mkv_sondado(void) { return 2; }
void video_sondar_mkv_agora(void) {}
int  video_audio_atual(void) { return audioAtual; }
int  video_legenda_atual(void) { return legAtual; }
void video_escolher_audio(int i) {
  if (i < 0 || i >= nAudio) return;
  audioAtual = i;
  if (hEscolher) hEscolher(0, faixaAudio[i].numero);
}
void video_escolher_legenda(int i) {
  if (i >= nLeg) return;
  legAtual = i;
  if (travaLeg) { SDL_LockMutex(travaLeg); legTexto[0] = 0; legAte = 0; SDL_UnlockMutex(travaLeg); }
  if (i >= 0 && hEscolher) hEscolher(1, faixaLeg[i].numero);
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
// O atraso do estilo vale para a embutida (o player desloca o evento).
void video_legenda_estilo(const VideoLegendaEstilo *e) { if (e && hEscolher) hEscolher(2, e->atrasoMs); }
int  video_tem_atmos(void) { return 0; }
int  video_tem_dolby_vision(void) { return 0; }
const char *video_hdr(void) { return "none"; }
int  video_largura(void) { return largura; }
int  video_altura(void) { return altura; }
int  video_pode_forcar_sdr(void) { return 0; }
void video_forcar_sdr(void) {}
void video_encerrar(void) { video_parar(); }
#endif
