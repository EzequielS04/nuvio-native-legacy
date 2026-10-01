#include "trailer.h"
#include "layout.h"
#include "ajustes.h"
#include "trailerfonte.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#ifndef NV_REC_URL
#define NV_REC_URL ""
#endif

static int    aberto, cheia, comSom;
static int    falhouUltima;
static GfxRect rect;
static char   fonteAtual[1024];
static Uint32 abertoEm;   // SDL_GetTicks da abertura da fonte atual, para o log
static int    dono;       // TRAILER_DONO_* (trailer.h)
static char   donoImdb[32];

#ifdef __EMSCRIPTEN__
// O iframe fica ATRAS do canvas (z-index 0 contra 1 do canvas), no mesmo
// enquadramento 16:9 que a folha de estilo da ao canvas — por isso as
// medidas sao calculadas a partir do retangulo REAL do canvas na janela, e
// nao de 1920x1080 diretos. `origin` no embed e o que a IFrame API exige
// para aceitar postMessage; a origem de um wgt e "file://" ou "null", e ai
// vai sem.
EM_JS(int, trailer_js_abrir, (const char *fonte, float x, float y, float w, float h, int som, float zoom, const char *proxy), {
  var id = UTF8ToString(fonte);
  var base = proxy ? UTF8ToString(proxy) : '';
  // DOIS ELEMENTOS, um por fonte: id do YouTube -> <iframe> embed; URL http
  // (MP4 do IMDb, trailerimdb.h) -> <video> do proprio navegador, mudo pelo
  // atributo, sem AVPlay (o open do AVPlay custa ~1,8 s de fio principal
  // nesta TV, e o hero troca de titulo a cada seta).
  // Sem regex com "//" aqui: o pre-processador C le como comentario.
  var ehVideo = (id.indexOf('http://') === 0 || id.indexOf('https://') === 0);
  if (!ehVideo && !/^[A-Za-z0-9_-]{6,20}$/.test(id)) return 0;
  var T = Module.nvTrailer || (Module.nvTrailer = {
    f: null, id: '', estado: -1, som: 0, ehVideo: 0, geracao: 0
  });
  var cv = document.getElementById('canvas');
  if (!cv) return 0;
  var r = cv.getBoundingClientRect();
  var sx = r.width / 1920, sy = r.height / 1080;
  if (!T.f || T.id !== id) {
    var geracao = (T.geracao || 0) + 1;
    if (T.f && T.f.parentNode) { try { if (T.ehVideo) { T.f.pause(); T.f.removeAttribute('src'); T.f.load(); } } catch (e) {} T.f.parentNode.removeChild(T.f); }
    var f;
    if (ehVideo) {
      f = document.createElement('video');
      f.muted = !som; f.autoplay = true; f.playsInline = true; f.preload = 'auto';
      // O elemento anterior pode emitir `error`/`ended` depois de ser
      // removido. Publicar esse estado no objeto global fazia a sessao nova
      // fechar ou ficar em buffering assim que A -> B -> A acontecia.
      // DIAGNOSTICO (log 1646, Samsung 1.4.1: o C abria o HLS e depois nao
      // havia UMA linha sobre o video). Uma linha por transicao, pelo mesmo
      // canal do printf (window.__nvDiag -> painel, nv-log, "Enviar
      // registro"); waiting/stalled so a primeira de cada, para nao inundar.
      var t0 = Date.now(), vistos = {};
      var diz = function (ev, extra) {
        if (vistos[ev]) return; vistos[ev] = 1;
        var s = '[trailer-js] g' + geracao + ' ' + ev + ' +' + (Date.now() - t0) + 'ms' + (extra ? ' ' + extra : '');
        try { console.log(s); if (window.__nvDiag) window.__nvDiag(s); } catch (e) {}
      };
      f.addEventListener('loadedmetadata', function () { diz('loadedmetadata', f.videoWidth + 'x' + f.videoHeight + ' dur=' + (f.duration || 0).toFixed(1)); });
      f.addEventListener('canplay', function () { diz('canplay'); });
      f.addEventListener('stalled', function () { diz('stalled'); });
      f.addEventListener('playing', function () { diz('playing'); if (T.f === f && T.geracao === geracao) T.estado = 1; });
      f.addEventListener('waiting', function () { diz('waiting'); if (T.f === f && T.geracao === geracao && T.estado === 1) T.estado = 3; });
      f.addEventListener('ended', function () { diz('ended'); if (T.f === f && T.geracao === geracao) T.estado = 0; });
      f.addEventListener('error', function () {
        var me = f.error;
        diz('error', 'code=' + (me ? me.code : '?') + ' ' + (me && me.message ? me.message : ''));
        if (T.f === f && T.geracao === geracao) T.estado = -3;
      });
      setTimeout(function () { if (!vistos.playing && T.f === f) diz('sem playing em 8 s', 'readyState=' + f.readyState + ' networkState=' + f.networkState); }, 8000);
      T.f = f; T.id = id; T.estado = -1; T.som = som; T.ehVideo = 1; T.geracao = geracao;
      f.src = id;
      f.style.cssText = 'position:absolute;border:0;z-index:0;background:#000;pointer-events:none;object-fit:cover;';
      (document.body || document.documentElement).appendChild(f);
      var pr = f.play(); if (pr && pr.catch) pr.catch(function (e) { diz('play() recusado', e && e.name ? e.name : ''); if (T.f === f && T.geracao === geracao) T.estado = -3; });
    } else {
      f = document.createElement('iframe');
      // ERRO 153 (#136, AU7000): o embed direto de um wgt vai sem Referer e
      // sem origem, e o YouTube responde "Video player configuration error".
      // Com o servico de recomendacoes na build, o iframe abre a PAGINA dele
      // (/v1/trailer/yt), que embute o player com origem https valida e
      // repassa o postMessage. Sem o servico, o embed direto de antes.
      var viaProxy = base.indexOf('https://') === 0;
      var org = (location.origin && location.origin !== 'null' && location.origin.indexOf('http') === 0) ? '&origin=' + encodeURIComponent(location.origin) : '';
      f.src = viaProxy
        ? base + '/v1/trailer/yt?id=' + id + '&mute=' + (som ? 0 : 1)
        : 'https://www.youtube.com/embed/' + id + '?autoplay=1&mute=' + (som ? 0 : 1) +
          '&controls=0&enablejsapi=1&rel=0&modestbranding=1&playsinline=1&iv_load_policy=3&fs=0&disablekb=1' + org;
      f.nvOrigem = viaProxy ? base.replace(/^(https:[/][/][^/]+).*$/, '$1') : '';
      f.setAttribute('allow', 'autoplay; encrypted-media');
      f.setAttribute('frameborder', '0');
      f.tabIndex = -1;
      f.style.cssText = 'position:absolute;border:0;z-index:0;background:#000;pointer-events:none;';
      f.nvGeracao = geracao;
      T.f = f; T.id = id; T.estado = -1; T.som = som; T.ehVideo = 0; T.geracao = geracao;
      T.pronto = 0;
      (document.body || document.documentElement).appendChild(f);
      // SEM onReady EM 10 s = o player nao vai falar (rota fora do ar, erro
      // antes da API, rede): vira erro (-3) e quem chamou anda para a
      // proxima fonte ou fecha, em vez de deixar a tela de erro do YouTube.
      setTimeout(function () {
        if (T.f !== f || T.geracao !== geracao || T.pronto || T.estado === 1 || T.estado === 3) return;
        var s = '[trailer-js] g' + geracao + ' youtube sem onReady em 10 s';
        try { console.log(s); if (window.__nvDiag) window.__nvDiag(s); } catch (e) {}
        T.estado = -3;
      }, 10000);
      if (!T.ouvinte) {
        T.ouvinte = 1;
        window.addEventListener('message', function (ev) {
          // Direto: a mensagem vem do youtube.com. Pela pagina do servico:
          // vem da origem dele (f.nvOrigem), repassada do player la dentro.
          if (T.ehVideo || !T.f || !T.f.contentWindow || ev.source !== T.f.contentWindow ||
              T.f.nvGeracao !== T.geracao || typeof ev.data !== 'string' ||
              !(ev.origin.indexOf('youtube.com') >= 0 || (T.f.nvOrigem && ev.origin === T.f.nvOrigem))) return;
          var m; try { m = JSON.parse(ev.data); } catch (e) { return; }
          if (!m) return;
          if (m.event === 'onReady' && T.f) {
            T.pronto = 1;
            T.f.contentWindow.postMessage(JSON.stringify({ event: 'listening', id: 1 }), '*');
          }
          // 2/5/100/101/150/152/153: o video nao toca aqui. 153 e o da AU7000
          // (#136), sem Referer valido. -3 e o mesmo erro do <video>: o C
          // fecha e passa a vez, a tela de erro do YouTube nao fica.
          if (m.event === 'onError') {
            var s = '[trailer-js] g' + T.geracao + ' youtube onError ' + m.info;
            try { console.log(s); if (window.__nvDiag) window.__nvDiag(s); } catch (e) {}
            T.estado = -3;
            return;
          }
          if (T.estado === -3) return;   // erro e final para este elemento
          if (m.event === 'infoDelivery' && m.info && typeof m.info.playerState === 'number') T.estado = m.info.playerState;
          if (m.event === 'onStateChange' && typeof m.info === 'number') T.estado = m.info;
        });
      }
      // A IFrame API so fala depois de "listening"; mandamos ao carregar.
      f.addEventListener('load', function () {
        if (T.f !== f || T.geracao !== geracao) return;
        try { f.contentWindow.postMessage(JSON.stringify({ event: 'listening', id: 1 }), '*'); } catch (e) {}
      });
    }
  }
  // `zoom` > 1 amplia o elemento em volta do centro do retangulo: o furo do
  // canvas so deixa ver o retangulo, entao o que sobra e cortado — e o que
  // tira as tarjas pretas de um trailer 2.39:1 dentro de um quadro 16:9.
  T.f.style.left = (r.left + (x - w * (zoom - 1) / 2) * sx) + 'px';
  T.f.style.top = (r.top + (y - h * (zoom - 1) / 2) * sy) + 'px';
  T.f.style.width = (w * zoom * sx) + 'px';
  T.f.style.height = (h * zoom * sy) + 'px';
  if (T.som !== som) {
    T.som = som;
    if (T.ehVideo) T.f.muted = !som;
    else if (T.f.contentWindow) { try { T.f.contentWindow.postMessage(JSON.stringify({ event: 'command', func: som ? 'unMute' : 'mute', args: [] }), '*'); } catch (e) {} }
  }
  return 1;
});
EM_JS(void, trailer_js_cmd, (const char *cmd), {
  var T = Module.nvTrailer;
  if (!T || !T.f) return;
  var c = UTF8ToString(cmd);
  if (T.ehVideo) { try { if (c === 'pauseVideo') T.f.pause(); else if (c === 'playVideo') T.f.play(); } catch (e) {} return; }
  if (!T.f.contentWindow) return;
  try { T.f.contentWindow.postMessage(JSON.stringify({ event: 'command', func: c, args: [] }), '*'); } catch (e) {}
});
EM_JS(void, trailer_js_fechar, (), {
  var T = Module.nvTrailer;
  if (!T) return;
  if (T.f) { try { if (T.ehVideo) { T.f.pause(); T.f.removeAttribute('src'); T.f.load(); } } catch (e) {} if (T.f.parentNode) T.f.parentNode.removeChild(T.f); }
  T.f = null; T.id = ''; T.estado = -1; T.geracao = (T.geracao || 0) + 1;
});
EM_JS(int, trailer_js_estado, (), {
  var T = Module.nvTrailer;
  return (T && T.f) ? T.estado : -2;
});
EM_JS(int, trailer_js_pausado, (), {
  var T = Module.nvTrailer;
  if (!T || !T.f) return 0;
  return T.ehVideo ? (T.f.paused ? 1 : 0) : (T.estado === 2 ? 1 : 0);
});
int trailer_suportado(void) { return 1; }
#else
// LG (e Mac, onde video_iniciar devolve 0 e nada disto acontece): o trailer
// e um MP4 no plano de video da TV. O volume so pode ser mexido depois de o
// pipeline existir (mediaId), e o recorte que tira a tarja preta so depois de
// o quadro ter tamanho — os dois ficam pendentes e trailer_atualizar aplica.
#include "video.h"
static int volumePendente, recortePendente, pausado;
// O recorte e REPETIDO nos primeiros segundos (ver reaplicarAte): o pipeline
// desta TV prende o plano em mais de um ponto depois do load (bind do ACB,
// `playing`), e um recorte pedido cedo demais pode ser engolido por um deles.
static Uint32 reaplicarAte, reaplicarEm, tocandoDesde;
static int quadroInteiroEnviado;
#ifdef NV_TPK
// O PLANO ESCONDIDO ATE O RECORTE ASSENTAR (#178, trailer_mostra_video).
// `recorteEnviadoEm` e o instante do primeiro recorte desta fonte; o ROI vai
// ao host por fila (Video.cs, Principal), e o respiro cobre essa viagem e o
// SetRoi. Sem recorte em NV_TRAILER_TPK_ROI_PRAZO_MS de tocando (quadro sem
// tamanho, host que nao responde), mostra assim mesmo.
#define NV_TRAILER_TPK_ROI_ASSENTA_MS 300
#define NV_TRAILER_TPK_ROI_PRAZO_MS   2000
// Sem nem `tocando` (o host sempre manda logo depois do Start), a tela cheia
// abre o furo mesmo assim neste prazo: e o comportamento de antes.
#define NV_TRAILER_TPK_SEM_TOCAR_MS   6000
static Uint32 recorteEnviadoEm;
static int    mostraLogado;
// O FURO SO COM IMAGEM (#188, #195). O host manda `pronto` e `tocando` logo
// depois do prepare e do Start, antes do primeiro quadro; um furo aberto ai
// mostra o que esta atras do app (no Tizen 9, a tela inicial da Samsung) ate a
// imagem chegar. A posicao do player (Video.cs, Tique: so le com o estado
// Playing) andar e o sinal de que ha quadro no plano. Pegajoso por fonte: a
// pausa do trailer em tela cheia nao fecha o furo.
#define NV_TRAILER_TPK_POS_MIN 0.25
// A posicao nao andou (host que nao le a posicao): abre neste prazo de
// `tocando`, como antes.
#define NV_TRAILER_TPK_POS_PRAZO_MS 4000u
static int    imagemVista;
static Uint32 tocandoVisto;
#endif
int trailer_suportado(void) {
#if defined(__APPLE__)
  return 0;
#else
  // O FRACASSO NAO TRAVA, MAS TEM TETO. O deploy mata o processo e relanca
  // em seguida, e o hub LS2 pode demorar para soltar o nome anterior; so o
  // SUCESSO e definitivo. O recuo e o teto moram em video_iniciar_auto
  // (lsregistro.h): sem teto, os registros 1720-1774 chamaram LSRegister a cada
  // 15 s pela sessao inteira — 1327 recusas de PERMISSION, que nao muda
  // sozinha. O play continua tentando (video_iniciar, pedido da pessoa).
  static int sabe = 0;
#ifdef NV_TPK
  // Outro app ja tomou o video da TV nesta sessao (#178): abrir trailer de
  // novo so abriria o furo em cima do video DELE. Fica a arte.
  if (video_conflito_recurso()) return 0;
#endif
  if (sabe) return 1;
  if (video_iniciar_auto()) sabe = 1;
  return sabe;
#endif
}
static void nativoAplicar(void) {
  if (!aberto) return;
  // O uMS setVolume funciona nesta TV (provado ao contrario: sem ele o
  // trailer tocou com som).
  if (volumePendente && video_ativo()) { video_volume(comSom ? 100 : 0); volumePendente = 0; }
  // O RECORTE SO DEPOIS DE `playing` + um respiro. E a ordem do player, a
  // unica em que o recorte comprovadamente pega nesta TV: la o modo salvo vai
  // ao plano no videoInfo como quadro INTEIRO e o zoom de verdade so e pedido
  // pela pessoa com o filme ja tocando. Pedido antes de tocar, o recorte era
  // aceito (-> 1) e ignorado (20/09/2026, meia noite de tentativas).
  if (recortePendente && video_pronto() && video_largura() > 0 && video_altura() > 0 &&
      !video_tocando() && !quadroInteiroEnviado) {
    video_janela_fonte(0, 0, video_largura(), video_altura(),
                       (int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h);
    quadroInteiroEnviado = 1;
  }
  if (recortePendente && video_tocando() && !tocandoDesde) tocandoDesde = SDL_GetTicks();
  if (recortePendente && video_pronto() && video_largura() > 0 && video_altura() > 0 &&
      tocandoDesde && SDL_GetTicks() - tocandoDesde >= 800) {
    int vw = video_largura(), vh = video_altura();
    float z = ajustes_trailer_zoom();
    int sw, sh, sx, sy;
    // QUADRO MATTED (Apple: 1920x804, 3836x1606) nao tem tarja embutida — a
    // tarja e o proprio plano encaixando 2.39 em 16:9. Encher a tela e
    // recortar as LATERAIS ate 16:9, sem o zoom fixo. Quadro 16:9 (IMDb, com
    // a tarja dentro da imagem) leva o zoom do ajuste. "Original" (1.0) nao
    // recorta nada em nenhum dos dois.
    if (z <= 1.001f) {
      // ORIGINAL: o quadro inteiro ENCAIXADO no retangulo, com tarja. O
      // destino vai calculado aqui, e nao deixado para o plano de video: no
      // Android (video_android.c) o destino de video_janela_fonte e exato e o
      // quadro 2,4:1 da Apple esticava em 16:9.
      float esc = rect.w / (float)vw < rect.h / (float)vh ? rect.w / (float)vw : rect.h / (float)vh;
      int fw = (int)(vw * esc + 0.5f), fh = (int)(vh * esc + 0.5f);
      if (video_recorte_fonte())
        video_janela_fonte(0, 0, vw, vh, (int)(rect.x + (rect.w - fw) * 0.5f),
                           (int)(rect.y + (rect.h - fh) * 0.5f), fw, fh);
      recortePendente = 0;
      if (!reaplicarAte) { reaplicarAte = SDL_GetTicks() + 6000; reaplicarEm = SDL_GetTicks() + 1500; }
      return;
    }
    else if ((float)vw / (float)vh > 1.85f) { sh = vh; sw = (int)(vh * 16.0f / 9.0f); if (sw > vw) sw = vw; }
    else { sw = (int)(vw / z); sh = (int)(vh / z); }
    // PAR, como o player faz (player.c, aplicarAspecto): o escalonador
    // trabalha em 4:2:0 e origem ou tamanho impar da meio pixel de croma na
    // borda — e 803 de altura era o que saia daqui.
    sw &= ~1; sh &= ~1;
    sx = ((vw - sw) / 2) & ~1; sy = ((vh - sh) / 2) & ~1;
    if (video_recorte_fonte())
      video_janela_fonte(sx, sy, sw, sh,
                         (int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h);
    recortePendente = 0;
#ifdef NV_TPK
    if (!recorteEnviadoEm) recorteEnviadoEm = SDL_GetTicks() | 1;
#endif
    if (!reaplicarAte) { reaplicarAte = SDL_GetTicks() + 6000; reaplicarEm = SDL_GetTicks() + 1500; }
  }
  if (reaplicarAte && SDL_GetTicks() >= reaplicarEm) {
    if (SDL_GetTicks() >= reaplicarAte) reaplicarAte = 0;
    else { reaplicarEm = SDL_GetTicks() + 1500; video_recorte_reaplicar(); }
  }
}
#endif

void trailer_abrir(const char *fonte, GfxRect r, int som, int modoCheia) {
  int nova;
  if (!trailer_suportado() || !fonte || !fonte[0]) return;
  // SAMSUNG E SEMPRE MUDO, tambem em tela cheia (dono, 22/09/2026: "trailer
  // fica mudo"). A trava fica AQUI, no unico caminho ate o elemento, e nao so
  // em quem chama: um `som` 1 esquecido num chamador novo desmutaria o embed
  // do YouTube (mute=0) e voltaria a prometer som onde a Apple nao tem.
  if (!trailerfonte_com_som(trailerfonte_tizen())) som = 0;
  nova = strcmp(fonteAtual, fonte) != 0;
#ifdef __EMSCRIPTEN__
  // O ESTADO DE SESSAO a cada tentativa (dono: "tocou um trailer e depois
  // nenhum toca mais"). Se algo ficasse preso aqui — elemento velho ainda
  // aberto, falha antiga, estado JS que nao volta a -2 — esta linha mostra
  // no registro da proxima Samsung, sem precisar de TV na mesa.
  if (nova) {
    printf("[trailer] tentativa: aberto=%d cheia=%d falhou=%d estado=%d anterior=%.40s\n",
           aberto, cheia, falhouUltima, trailer_js_estado(), fonteAtual[0] ? fonteAtual : "-");
    fflush(stdout);
  }
#endif
  if (nova) falhouUltima = 0;
#ifdef __EMSCRIPTEN__
  if (!trailer_js_abrir(fonte, r.x, r.y, r.w, r.h, som, ajustes_trailer_zoom(), NV_REC_URL)) return;
#else
  if (nova) {
    if (!video_tocar(fonte)) return;
    volumePendente = 1; recortePendente = 1; pausado = 0; reaplicarAte = 0;
    tocandoDesde = 0; quadroInteiroEnviado = 0;
#ifdef NV_TPK
    recorteEnviadoEm = 0; mostraLogado = 0; imagemVista = 0; tocandoVisto = 0;
#endif
  } else if (comSom != som) volumePendente = 1;
  video_janela((int)r.x, (int)r.y, (int)r.w, (int)r.h);
  if (!nova) recortePendente = 1;
#endif
  if (nova) {
    snprintf(fonteAtual, sizeof fonteAtual, "%s", fonte);
    abertoEm = SDL_GetTicks();
    printf("[trailer] %.60s %s%s\n", fonte, modoCheia ? "tela cheia" : "no fundo", som ? " com som" : " mudo");
    fflush(stdout);
  }
  rect = r; aberto = 1; cheia = modoCheia; comSom = som;
#ifndef __EMSCRIPTEN__
  nativoAplicar();
#endif
}

void trailer_rect(GfxRect r) {
  if (!aberto) return;
  rect = r;
#ifdef __EMSCRIPTEN__
  trailer_js_abrir(fonteAtual, r.x, r.y, r.w, r.h, comSom, ajustes_trailer_zoom(), NV_REC_URL);
#else
  video_janela((int)r.x, (int)r.y, (int)r.w, (int)r.h);
  recortePendente = 1;
  nativoAplicar();
#endif
}

void trailer_fechar(void) {
  if (!aberto) return;
#ifdef __EMSCRIPTEN__
  trailer_js_fechar();
#else
  video_parar();
#endif
  aberto = 0; cheia = 0; fonteAtual[0] = 0;
  dono = TRAILER_DONO_NENHUM; donoImdb[0] = 0;
}

void trailer_marcar_dono(int d, const char *imdb) {
  if (!aberto) return;
  dono = d;
  snprintf(donoImdb, sizeof donoImdb, "%s", imdb ? imdb : "");
}
int trailer_dono(void) { return aberto ? dono : TRAILER_DONO_NENHUM; }
const char *trailer_dono_imdb(void) { return aberto ? donoImdb : ""; }

int trailer_continuar(GfxRect r, int som) {
  char f[sizeof fonteAtual];
  if (!aberto || cheia || !fonteAtual[0]) return 0;
  // Copia: trailer_abrir le `fonte` e grava fonteAtual. Mesma fonte = nao e
  // "nova": so o volume e o retangulo (com o recorte do zoom) mudam.
  snprintf(f, sizeof f, "%s", fonteAtual);
  trailer_abrir(f, r, som, 0);
  printf("[trailer] continua %.60s %s\n", f, comSom ? "com som" : "mudo");
  fflush(stdout);
  return 1;
}

int trailer_aberto(void)  { return aberto; }
int trailer_cheia(void)   { return aberto && cheia; }
int trailer_tocando(void) {
#ifdef __EMSCRIPTEN__
  // 1 = PLAYING, 3 = BUFFERING (ja ha imagem), na IFrame API.
  int e = aberto ? trailer_js_estado() : -2;
  return e == 1 || e == 3;
#else
  return aberto && video_pronto() && !video_falhou() && !video_terminou() &&
         !video_conflito_recurso();
#endif
}
int trailer_mostra_video(void) {
#if defined(NV_TPK) && !defined(__EMSCRIPTEN__)
  Uint32 t = SDL_GetTicks();
  const char *porque = NULL;
  if (!aberto) return 0;
  if (!imagemVista) {
    if (video_tocando() && !tocandoVisto) tocandoVisto = t | 1;
    if (video_pos() < NV_TRAILER_TPK_POS_MIN &&
        !(tocandoVisto && t - tocandoVisto >= NV_TRAILER_TPK_POS_PRAZO_MS)) return 0;
    imagemVista = 1;
  }
  // "Original", ou o alvo sem recorte (video_tpk.c, o padrao desde #188): nada
  // a esperar, o LetterBox de sempre ja e a imagem certa.
  if (ajustes_trailer_zoom() <= 1.001f || !video_recorte_fonte()) porque = "sem recorte";
  else if (recorteEnviadoEm && t - recorteEnviadoEm >= NV_TRAILER_TPK_ROI_ASSENTA_MS) porque = "recorte assentou";
  else if (tocandoDesde && t - tocandoDesde >= NV_TRAILER_TPK_ROI_PRAZO_MS) porque = "sem recorte em 2 s, mostra assim";
  else if (!tocandoDesde && abertoEm && t - abertoEm >= NV_TRAILER_TPK_SEM_TOCAR_MS) porque = "sem tocando, mostra assim";
  if (!porque) return 0;
  if (!mostraLogado) {
    mostraLogado = 1;
    printf("[trailer] tpk: plano visivel +%ums (%s, posicao %.2fs)\n", (unsigned)(t - abertoEm), porque, video_pos());
    fflush(stdout);
  }
  return 1;
#else
  return 1;
#endif
}
GfxRect trailer_retangulo(void) { return rect; }
int trailer_estado(void) {
#ifdef __EMSCRIPTEN__
  return aberto ? trailer_js_estado() : -2;
#else
  return -9;
#endif
}

int trailer_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto || !cheia || e->type != SDL_KEYDOWN) return 0;
  k = e->key.keysym.sym;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE || k == SDLK_DELETE ||
      e->key.keysym.scancode == NV_SCANCODE_BACK) { trailer_fechar(); return 1; }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) {
#ifdef __EMSCRIPTEN__
    trailer_js_cmd(trailer_js_pausado() ? "playVideo" : "pauseVideo");
#else
    pausado = !pausado;
    video_pausar(pausado);
#endif
    return 1;
  }
  return 1;   // em tela cheia o resto do teclado nao vai a pagina de tras
}

void trailer_atualizar(Uint32 agora) {
  (void)agora;
#ifdef __EMSCRIPTEN__
  // TRANSICAO DO ESTADO como o C a ve (-1 criado, 1 tocando, 3 buffering,
  // 0 fim, -3 erro, -2 sem elemento), com o tempo desde a abertura. Com o
  // `[trailer-js]` do elemento ao lado, o registro diz se o video falhou, se
  // ficou sem `playing` ou se quem fechou foi a tela. So na borda: 1 linha
  // por mudanca, nao por quadro.
  { static int visto = -2;
    int e = aberto ? trailer_js_estado() : -2;
    if (e != visto) {
      // `agora` vem do inicio do quadro e abertoEm de SDL_GetTicks no meio
      // dele: no quadro da abertura a diferenca e negativa, vira 0.
      Sint32 ms = abertoEm ? (Sint32)(agora - abertoEm) : 0;
      printf("[trailer] estado %d -> %d +%dms\n", visto, e, ms > 0 ? (int)ms : 0);
      fflush(stdout);
      visto = e;
    } }
  // Acabou (0 = ENDED) ou falhou (-3, so o <video>): fecha e a arte volta.
  if (aberto && trailer_js_estado() == -3) {
    falhouUltima = 1;
    trailer_fechar();
  } else if (aberto && trailer_js_estado() == 0) trailer_fechar();
#else
  if (!aberto) return;
  video_bombear();
  nativoAplicar();
  // Acabou ou a fonte falhou: fecha e a pagina volta a arte.
  // conflito: outro app (YouTube) tomou o plano de video — fecha, fica a arte.
  if (video_terminou() || video_falhou() || video_conflito_recurso()) {
    falhouUltima = video_falhou() || video_conflito_recurso() ? 1 : 0;
    trailer_fechar();
  }
#endif
}

int trailer_falhou(void) { return falhouUltima; }
