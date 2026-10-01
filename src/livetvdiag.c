// DIAGNOSTICO DA LIVE TV (livetvdiag.h).
//
// POR QUE EXISTE (#158). Na LG C4 do pasha o .ts do Xtream chegava (36 s de
// buffer), o uMS reservava o decodificador e nunca mostrava imagem, enquanto
// outro app na mesma TV tocava o canal. O registro so contava isso para quem
// soubesse ler; esta tela mede na TV da pessoa o que separa um canal que toca
// de um que nao toca, e transforma em ajuste:
//
//   1. a CONTA Xtream (formatos declarados, telas, validade);
//   2. para cada canal da fileira em foco no guia, a REDE: o .ts e a .m3u8
//      pedidos direto ao provedor (os dois, mesmo com a conta declarando um so
//      — e a hipotese "o provedor serve HLS assim mesmo"), o que chega (e TS?
//      que codecs a PMT declara? 10 bits?) e a que velocidade;
//   3. o PLAYER: o formato que respondeu vai ao pipeline de verdade, num quadro
//      da tela, ate o primeiro quadro (tempo) ou ate desistir (por que);
//   4. a RECOMENDACAO (livetv_regras.h), com "Aplicar".
//
// UMA COISA DE CADA VEZ: a conta do pasha tem 1 tela (telas=0/1), e pedido em
// paralelo ao mesmo provedor e recusado (os 40404 em sequencia do registro
// 14115). O fio de rede e o pipeline nunca se cruzam, e entre um e outro ha
// uma pausa para o provedor soltar a conexao.
//
// Nada aqui grava URL, usuario ou senha no registro: so o nome do canal.
#include "livetvdiag.h"
#include "livetv_regras.h"
#include "ts_sonda.h"
#include "ajustes.h"
#include "addons.h"
#include "avisos.h"
#include "fontecache.h"  /* addons_consultar */
#include "gfx.h"
#include "guia.h"
#include "idioma.h"
#include "layout.h"
#include "player.h"
#include "rede.h"
#include "streams.h"
#include "text.h"
#include "vazao.h"
#include "video.h"
#include "xtream.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__)
#define LTD_TEM_PLAYER 0   // o Mac nao tem pipeline (video.c: stubs)
#else
#define LTD_TEM_PLAYER 1
#endif

#define LTD_MAX          6
#define LTD_PLAYER_MS    18000u  // acima dos 15 s do Xtream: ve quem abre aos 14
#define LTD_MOSTRA_MS    1500u   // o quadro fica na tela um instante depois de abrir
#define LTD_PAUSA_MS     1000u   // o provedor solta a conexao entre um pedido e outro
#define LTD_TRECHO_B     786431L
#define LTD_LINHA        60.0f   // altura de cada canal na lista

enum { F_HLS = 0, F_TS = 1 };
enum { E_PARADO, E_REDE, E_PLAYER, E_PRONTO };

typedef struct {
  int  tentado, servido;  // pedido feito; respondeu 2xx com video
  int  http, curl;
  int  ms;                // pedido -> corpo
  long bytes;
  int  kbps;
  int  playlist;          // HLS: 1 = #EXTM3U com segmento
  int  ehTs;
  char codec[96];
  int  dezBits, embaralhado;
  char url[4096];         // o que vai ao player (nunca ao registro)
  // player
  int  testou, tocou, decoder, quadroMs, largura, altura, falha;
  char erro[64];
  // MODOS DO LOAD (video_definir_modo_live): cada formato passa pelos modos
  // A/B/C ate um tocar; o resultado de cada um fica para o registro e a
  // recomendacao. modoOk = o que tocou, -1 nenhum.
  int  modoFeito, modoOk;
  int  modoFalha[3], modoQuadroMs[3];
} LtdFormato;

typedef struct {
  char nome[140], id[80], base[600], cabs[512];
  int  xt, alturaNome;
  int  semFonte;          // addon: nenhuma fonte
  LtdFormato f[2];
  int  pronto;
} LtdItem;

static struct {
  int estado, sair, n, atual, botao;
  LtdItem it[LTD_MAX];
  char grupo[64];
  int xtConfig, contaLida;
  XtreamConta conta;
  int kbps, kbpsPior, latenciaMs, redeMedida;
  // player em curso
  int pfFormato, pfModo, pfVivo;
  Uint32 pfDesde, pfTocouEm, pausaAte;
  LtdRecomendacao rec;
  int recModo, tocouModo[3];   // modo do load que mais tocou; -1 = nenhum
  int aplicado, enviou;
  _Atomic int fioOcupado;
  volatile int cancelado;
  pthread_t fio;
  int fioCriado;
} L;

// --- rede -------------------------------------------------------------------
static const char *const *vetorCabs(const char *cab, char *copia, size_t n, const char *v[8]) {
  int k = 0;
  char *l, *ctx = NULL;
  if (!cab || !cab[0]) return NULL;
  snprintf(copia, n, "%s", cab);
  for (l = strtok_r(copia, "\n", &ctx); l && k < 7; l = strtok_r(NULL, "\n", &ctx)) v[k++] = l;
  v[k] = NULL;
  return k ? v : NULL;
}

static void juntarUrl(const char *base, const char *rel, char *dst, size_t n) {
  const char *q;
  if (!strncmp(rel, "http://", 7) || !strncmp(rel, "https://", 8)) { snprintf(dst, n, "%s", rel); return; }
  if (rel[0] == '/') {
    const char *h = strstr(base, "://");
    const char *barra = h ? strchr(h + 3, '/') : NULL;
    int len = barra ? (int)(barra - base) : (int)strlen(base);
    snprintf(dst, n, "%.*s%s", len, base, rel);
    return;
  }
  q = strrchr(base, '/');
  { const char *interroga = strchr(base, '?');
    if (interroga && q && q > interroga) q = NULL; }
  snprintf(dst, n, "%.*s/%s", q ? (int)(q - base) : (int)strlen(base), base, rel);
}

static void codecDe(const TsSonda *t, LtdFormato *f) {
  int i, u = 0;
  f->codec[0] = 0;
  f->ehTs = t->deslocamento >= 0;
  f->embaralhado = t->embaralhados > 0;
  for (i = 0; i < t->nEs && u < (int)sizeof f->codec - 24; i++) {
    const char *nome = ts_tipo_nome(t->es[i].tipo, t->es[i].desc);
    if (!strcmp(nome, "?") || !strcmp(nome, "privado") || !strcmp(nome, "legenda DVB") ||
        !strcmp(nome, "teletexto")) continue;
    if (strstr(f->codec, nome)) continue;
    u += snprintf(f->codec + u, sizeof f->codec - (size_t)u, "%s%s", u ? " · " : "", nome);
  }
  f->dezBits = t->videoPid >= 0 && ((!t->hevc && (t->perfil == 110 || t->perfil == 122)) ||
                                    (t->hevc && t->perfil == 2));
}

// Le o comeco de `url` e preenche servido/http/kbps/codec. Devolve 1 com corpo.
static int sondarTrecho(LtdFormato *f, const char *url, const char *const *cabs) {
  long n = 0;
  int st = 0, er = 0;
  Uint32 t0 = SDL_GetTicks();
  char *b;
  (void)cabs;  // rede_baixar_trecho_st nao leva cabecalho; os canais com cabecalho sao HLS de addon
  b = rede_baixar_trecho_st(url, 8, 0, LTD_TRECHO_B, &n, &st, &er, NULL, 0);
  f->ms = (int)(SDL_GetTicks() - t0);
  if (!f->http) f->http = st;
  f->curl = er;
  f->bytes = n;
  if (b && n > 0 && n < 4096) {
    // CORPO PEQUENO DEMAIS PARA SER VIDEO (#158, registro 14520: o .ts direto
    // devolveu 663 B ao curl enquanto o uMS bufferizava). Diz o que e, sem
    // imprimir o corpo (pode trazer URL com usuario e senha).
    const char *tipo = strstr(b, "#EXTM3U") ? "playlist m3u" : (strstr(b, "<html") || strstr(b, "<HTML") ||
                       strstr(b, "<!DOCTYPE")) ? "html" : (b[0] == '{' || b[0] == '[') ? "json" : "outro";
    long imprim = 0, k;
    for (k = 0; k < n; k++) if ((b[k] >= 32 && b[k] < 127) || b[k] == '\n' || b[k] == '\r') imprim++;
    printf("[livetv-diag] corpo curto: %ld B, %s, %ld%% texto, 0x47 no inicio=%d, tem http=%d\n", n, tipo,
           imprim * 100 / n, (unsigned char)b[0] == 0x47, strstr(b, "http") != NULL);
  }
  if (b && n > 0) {
    TsSonda t;
    ts_sondar((const unsigned char *)b, n, &t);
    codecDe(&t, f);
    if (f->ms > 0) f->kbps = (int)((double)n * 8.0 / (double)f->ms);
    f->servido = f->ehTs;
  }
  free(b);
  return n > 0;
}

// HLS: a playlist, a variante (pela resolucao principal), e o primeiro
// segmento, que diz codec e velocidade como o .ts direto.
static void sondarHls(LtdFormato *f, const char *url, const char *const *cabs) {
  char atual[4096], prox[4096], fin[4096];
  int volta;
  snprintf(atual, sizeof atual, "%s", url);
  for (volta = 0; volta < 2; volta++) {
    long n = 0;
    int st = 0, er = 0;
    char *b;
    const char *l, *escolhida = NULL;
    int alvo = nv_res_opcao_altura(ajustes_livetv_resolucao()), melhorDif = 1 << 30;
    fin[0] = 0;
    b = rede_baixar_trecho_st(atual, 6, 0, 262143, &n, &st, &er, fin, sizeof fin);
    if (!volta) { f->http = st; f->curl = er; }
    if (!b || n < 7 || strncmp(b, "#EXTM3U", 7)) { free(b); return; }
    if (fin[0]) snprintf(atual, sizeof atual, "%s", fin);
    if (strstr(b, "#EXT-X-STREAM-INF")) {
      // MASTER: a variante mais perto da resolucao principal (sem ela, a
      // primeira, que e a ordem do provedor).
      for (l = b; l && *l; l = strchr(l, '\n') ? strchr(l, '\n') + 1 : NULL) {
        if (strncmp(l, "#EXT-X-STREAM-INF", 17)) continue;
        { const char *r = strstr(l, "RESOLUTION="), *fimLinha = strchr(l, '\n');
          int h = 0, dif;
          if (r && (!fimLinha || r < fimLinha)) { const char *x = strchr(r, 'x'); if (x) h = atoi(x + 1); }
          dif = alvo ? abs(h - alvo) : (escolhida ? 1 << 30 : 0);
          if (!escolhida || dif < melhorDif) { melhorDif = dif; escolhida = fimLinha ? fimLinha + 1 : NULL; } }
      }
    } else {
      for (l = b; l && *l; l = strchr(l, '\n') ? strchr(l, '\n') + 1 : NULL)
        if (*l != '#' && *l != '\n' && *l != '\r') { escolhida = l; break; }
      f->playlist = escolhida != NULL;
    }
    if (!escolhida) { free(b); return; }
    { size_t k = strcspn(escolhida, "\r\n");
      char rel[2048];
      snprintf(rel, sizeof rel, "%.*s", (int)(k < sizeof rel ? k : sizeof rel - 1), escolhida);
      juntarUrl(atual, rel, prox, sizeof prox); }
    { int mestre = strstr(b, "#EXT-X-STREAM-INF") != NULL;
      free(b);
      if (mestre) { snprintf(atual, sizeof atual, "%s", prox); continue; } }
    { int http = f->http;
      sondarTrecho(f, prox, cabs);
      f->http = http; }
    return;
  }
}

static void medirVelocidade(const char *url, const char *const *cabs) {
  int kps[8], i, n = 0, pior = 0;
  RedeVazao r;
  memset(&r, 0, sizeof r);
  n = rede_medir_vazao(url, cabs, 6, 0, 0, &L.cancelado, kps, 8, &r, NULL, 0);
  if (n <= 0 && r.bytes <= 0) return;
  L.latenciaMs = (int)r.esperaMs;
  if (r.ms > 0) L.kbps = (int)((double)r.bytes * 8.0 / (double)r.ms);
  for (i = 0; i < n; i++) if (!pior || (kps[i] > 0 && kps[i] < pior)) pior = kps[i];
  L.kbpsPior = pior;
  L.redeMedida = 1;
  printf("[livetv-diag] rede ate o provedor: %d kbps (pior segundo %d), primeiro byte em %d ms\n",
         L.kbps, L.kbpsPior, L.latenciaMs);
}

// A melhor fonte de um canal de addon: a da resolucao principal, senao a
// primeira (a ordem do addon e o ranking dele).
static void fonteDoAddon(LtdItem *it) {
  Stream *lista = NULL;
  int n = addons_consultar(it->id, "tv", it->base, 2, NULL, NULL, &lista), i, k = 0;
  int alvo = nv_res_opcao_altura(ajustes_livetv_resolucao());
  if (n <= 0 || !lista) { it->semFonte = 1; free(lista); return; }
  for (i = 0; i < n; i++) {
    int a = lista[i].altura ? lista[i].altura : nv_res_do_texto(lista[i].rotulo);
    if (lista[i].url[0] && nv_res_preferida(a, alvo)) { k = i; break; }
  }
  while (k < n && !lista[k].url[0]) k++;
  if (k >= n) { it->semFonte = 1; free(lista); return; }
  { int hls = strstr(lista[k].url, ".m3u8") != NULL;
    LtdFormato *f = &it->f[hls ? F_HLS : F_TS];
    f->tentado = 1;
    snprintf(f->url, sizeof f->url, "%s", lista[k].url);
    snprintf(it->cabs, sizeof it->cabs, "%s", lista[k].cabecalhos);
    it->alturaNome = lista[k].altura ? lista[k].altura : nv_res_do_texto(lista[k].rotulo); }
  free(lista);
}

static void *fioRede(void *u) {
  LtdItem *it = u;
  char copia[512];
  const char *v[8];
  const char *const *cabs;
  int f;
  if (it == &L.it[0] && L.xtConfig) {
    L.contaLida = xtream_conta_ler(&L.conta);
    if (L.conta.valido)
      printf("[livetv-diag] conta: status=%s formatos=%s%s telas=%d/%d vence=%lld\n", L.conta.status,
             L.conta.formatosDeclarados ? (L.conta.temM3u8 ? "m3u8 " : "") : "nao declarados",
             L.conta.formatosDeclarados ? (L.conta.temTs ? "ts" : "") : "",
             L.conta.conexoes, L.conta.maxConexoes, L.conta.expira);
    else printf("[livetv-diag] conta: sem user_info (HTTP %d)\n", L.conta.http);
  }
  if (it->xt) {
    // OS DOIS FORMATOS, sempre: e assim que se sabe se o provedor serve .m3u8
    // com a conta declarando so .ts.
    if (xtream_url_formato(it->id, "m3u8", it->f[F_HLS].url, sizeof it->f[F_HLS].url)) it->f[F_HLS].tentado = 1;
    if (xtream_url_formato(it->id, "ts", it->f[F_TS].url, sizeof it->f[F_TS].url)) it->f[F_TS].tentado = 1;
  } else fonteDoAddon(it);
  cabs = vetorCabs(it->cabs, copia, sizeof copia, v);
  for (f = 0; f < 2 && !L.cancelado; f++) {
    LtdFormato *x = &it->f[f];
    if (!x->tentado) continue;
    if (f == F_HLS) sondarHls(x, x->url, cabs);
    else sondarTrecho(x, x->url, cabs);
    // O MESMO .ts COM USER-AGENT DE PLAYER: o provedor pode entregar outra
    // coisa ao "Nuvio/1.0" do curl e ao player (hipotese 3 do #158).
    if (f == F_TS && x->tentado && !x->servido && !L.cancelado) {
      static const char *const ua[] = { "User-Agent: VLC/3.0.20 LibVLC/3.0.20", NULL };
      RedeControle ctl = { LTD_TRECHO_B + 1, &L.cancelado };
      RedeMedida md;
      long n = 0;
      char *b2;
      memset(&md, 0, sizeof md);
      b2 = rede_baixar_bin_medido_controle(x->url, 8, ua, &ctl, &n, &md);
      { TsSonda t; char r[300] = "-";
        if (b2 && n > 0) { ts_sondar((const unsigned char *)b2, n, &t); ts_resumo(&t, r, sizeof r); }
        printf("[livetv-diag] rede %s TS com UA de player: HTTP %d %ld B em %lu ms | %s\n", it->nome,
               md.status, n, md.ms, r);
        if (b2 && n > 0 && t.deslocamento >= 0) {
          codecDe(&t, x); x->servido = 1; x->bytes = n; x->ms = (int)md.ms;
          if (md.ms) x->kbps = (int)((double)n * 8.0 / (double)md.ms);
        } }
      free(b2);
    }
    printf("[livetv-diag] rede %s %s: HTTP %d curl=%d %ld B em %d ms (%d kbps) %s%s%s%s\n",
           it->nome, f == F_HLS ? "HLS" : "TS", x->http, x->curl, x->bytes, x->ms, x->kbps,
           x->servido ? "video TS" : (f == F_HLS && x->http >= 200 && x->http < 300) ? "playlist sem video" : "sem video",
           x->codec[0] ? " | " : "", x->codec, x->dezBits ? " | 10 bits" : "");
    fflush(stdout);
  }
  // VELOCIDADE: 6 s de um canal que respondeu, uma vez (o primeiro).
  if (!L.redeMedida && !L.cancelado) {
    int g = it->f[F_TS].servido ? F_TS : it->f[F_HLS].servido ? F_HLS : -1;
    if (g == F_TS) medirVelocidade(it->f[F_TS].url, cabs);
    // HLS: a playlist nao serve para medir; o segmento ja deu a vazao dele.
    else if (g == F_HLS && it->f[F_HLS].kbps > 0) {
      L.kbps = it->f[F_HLS].kbps; L.latenciaMs = -1; L.redeMedida = 1;
    }
  }
  fflush(stdout);
  atomic_store(&L.fioOcupado, 0);
  return NULL;
}

static void juntarFio(void) {
  if (L.fioCriado) { pthread_join(L.fio, NULL); L.fioCriado = 0; }
}

static void iniciarRede(int i) {
  juntarFio();
  atomic_store(&L.fioOcupado, 1);
  L.estado = E_REDE;
  L.atual = i;
  if (pthread_create(&L.fio, NULL, fioRede, &L.it[i]) == 0) L.fioCriado = 1;
  else atomic_store(&L.fioOcupado, 0);
}

// --- player -------------------------------------------------------------------
// Qual formato vai ao pipeline primeiro: o que a REDE mostrou servindo video,
// na ordem do Xtream (xtream_formatos: o que a conta declara e o que ja tocou).
static int proximoFormato(const LtdItem *it) {
  const char *ext[2];
  int k = it->xt ? xtream_formatos(ext) : 0, i;
  for (i = 0; i < k; i++) {
    int f = !strcmp(ext[i], "ts") ? F_TS : F_HLS;
    if (it->f[f].servido && !it->f[f].testou) return f;
  }
  for (i = 0; i < 2; i++) if (it->f[i].servido && !it->f[i].testou) return i;
  // Nada pareceu video pela rede: tenta mesmo assim o primeiro pedido que
  // respondeu (um addon pode exigir cabecalho que a sonda nao manda).
  for (i = 0; i < 2; i++) if (it->f[i].tentado && it->f[i].http >= 200 && it->f[i].http < 300 && !it->f[i].testou) return i;
  return -1;
}

static int algumTocou(const LtdItem *it) { return it->f[0].tocou || it->f[1].tocou; }

static GfxRect quadroPlayer;   // onde o video aparece; o desenho atualiza
// Quantos modos cada formato tenta: o HLS (que a rede provou ser video) os
// tres; o TS os tres nos dois primeiros canais e so o padrao nos outros —
// para o teste caber em poucos minutos.
static int nModos(int f) {
  if (!LTD_TEM_PLAYER) return 1;
  return f == F_HLS || L.atual < 2 ? 3 : 1;
}
static const char *letraModo(int m) { return m == 1 ? "B" : m == 2 ? "C" : "A"; }

static void iniciarPlayer(int f) {
  LtdItem *it = &L.it[L.atual];
  LtdFormato *x = &it->f[f];
  if (!x->modoFeito) x->modoOk = -1;
  L.pfFormato = f;
  L.pfModo = x->modoFeito;
  video_definir_modo_live(L.pfModo);
  L.pfDesde = SDL_GetTicks();
  L.pfTocouEm = 0;
  L.estado = E_PLAYER;
  video_definir_reconexao(0);
  video_definir_cabecalhos(it->cabs);
  L.pfVivo = video_tocar(x->url);
  // O QUADRO do painel "No player agora" desde o load: sem isto o primeiro
  // quadro sairia em tela cheia por cima da tela ate o desenho seguinte.
  if (L.pfVivo && quadroPlayer.w > 1.0f)
    video_janela((int)quadroPlayer.x, (int)quadroPlayer.y, (int)quadroPlayer.w, (int)quadroPlayer.h);
  if (!L.pfVivo) {
    x->falha = LTD_TEM_PLAYER ? LTD_ERRO_PLAYER : LTD_SEM_TESTE;
    snprintf(x->erro, sizeof x->erro, "%s", LTD_TEM_PLAYER ? "load recusado" : "");
  }
}

static void fecharPlayer(void) {
  if (L.pfVivo) video_parar();
  L.pfVivo = 0;
  L.pausaAte = SDL_GetTicks() + LTD_PAUSA_MS;
}

static void logPlayer(const LtdItem *it, const LtdFormato *x, int f) {
  if (x->tocou && x->modoOk == L.pfModo)
    printf("[livetv-diag] player %s %s modo %s: tocou em %d ms, %dx%d\n", it->nome, f == F_HLS ? "HLS" : "TS",
           letraModo(L.pfModo), x->quadroMs, x->largura, x->altura);
  else
    printf("[livetv-diag] player %s %s modo %s: nao tocou (%s%s%s) decoder=%d buffer=%.1fs\n", it->nome,
           f == F_HLS ? "HLS" : "TS", letraModo(L.pfModo),
           x->falha == LTD_SEM_DECODER ? "dado chegou, decoder mudo" :
           x->falha == LTD_SEM_RESPOSTA ? "nada chegou" :
           x->falha == LTD_SEM_TESTE ? "sem pipeline nesta plataforma" : "erro",
           x->erro[0] ? ": " : "", x->erro, x->decoder, video_buffer_fim());
  fflush(stdout);
}

static void recomendar(void);

static void avancar(void) {
  L.it[L.atual].pronto = 1;
  if (L.atual + 1 < L.n && !L.cancelado) { iniciarRede(L.atual + 1); return; }
  L.estado = E_PRONTO;
  recomendar();
}

static void passoPlayer(Uint32 agora) {
  LtdItem *it = &L.it[L.atual];
  LtdFormato *x = &it->f[L.pfFormato];
  Uint32 d = agora - L.pfDesde;
  int acabou = 0;
  if (!L.pfVivo) acabou = 1;
  else if (L.pfTocouEm) {
    if (agora - L.pfTocouEm >= LTD_MOSTRA_MS) acabou = 1;
  } else if (video_falhou()) {
    x->falha = LTD_ERRO_PLAYER;
    snprintf(x->erro, sizeof x->erro, "%s", video_erro_texto());
    acabou = 1;
  } else if (video_pos() > 0.05 || video_tocando()) {
    x->tocou = 1;
    x->modoOk = L.pfModo;
    x->falha = LTD_OK;
    x->quadroMs = (int)d;
    x->decoder = 1;
    x->largura = video_largura();
    x->altura = video_altura();
    L.pfTocouEm = agora;
  } else if (d >= LTD_PLAYER_MS) {
    x->decoder = video_decoder_anunciou();
    x->falha = video_buffer_fim() > 0.5 ? LTD_SEM_DECODER : LTD_SEM_RESPOSTA;
    acabou = 1;
  }
  if (!acabou) return;
  if (L.pfVivo || x->falha != LTD_SEM_TESTE) logPlayer(it, x, L.pfFormato);
  if (L.pfModo < 3) {
    x->modoFalha[L.pfModo] = x->modoOk == L.pfModo ? LTD_OK : x->falha;
    x->modoQuadroMs[L.pfModo] = x->modoOk == L.pfModo ? x->quadroMs : 0;
  }
  x->modoFeito++;
  // O formato acaba quando tocou ou quando os modos dele acabaram; no Mac
  // (sem pipeline) ele nem conta como testado.
  if (LTD_TEM_PLAYER && (x->tocou || x->modoFeito >= nModos(L.pfFormato))) x->testou = 1;
  if (!LTD_TEM_PLAYER) x->modoFeito = 99;
  fecharPlayer();
  // O OUTRO FORMATO so se este nao tocou: ve se o HLS abre onde o TS nao abre.
  if (!x->tocou && LTD_TEM_PLAYER) {
    int g = proximoFormato(it);
    if (g >= 0) { L.estado = E_PLAYER; L.pfFormato = g; L.pfVivo = 0; L.pfDesde = 0; return; }
  }
  avancar();
}

// --- recomendacao ----------------------------------------------------------------
static void recomendar(void) {
  LtdCanal c[LTD_MAX * 2];
  int n = 0, i, f;
  for (i = 0; i < L.n; i++)
    for (f = 0; f < 2; f++) {
      const LtdFormato *x = &L.it[i].f[f];
      if (!x->tentado || (!x->servido && !x->testou)) continue;
      c[n].formato = f;
      c[n].tocou = x->tocou;
      c[n].testouPlayer = x->testou && x->falha != LTD_SEM_TESTE;
      c[n].falha = x->testou ? x->falha : x->servido ? LTD_SEM_TESTE : LTD_NAO_E_VIDEO;
      c[n].quadroMs = x->quadroMs;
      c[n].esperaMs = -1;
      c[n].kbpsFluxo = x->kbps;
      c[n].alturaVista = x->altura;
      c[n].dezBits = x->dezBits;
      n++;
    }
  ltd_recomendar(c, n, &L.rec);
  // MODO DO LOAD: o que mais tocou; empate fica com o menor (A, o de sempre).
  memset(L.tocouModo, 0, sizeof L.tocouModo);
  for (i = 0; i < L.n; i++)
    for (f = 0; f < 2; f++)
      if (L.it[i].f[f].tocou && L.it[i].f[f].modoOk >= 0 && L.it[i].f[f].modoOk < 3)
        L.tocouModo[L.it[i].f[f].modoOk]++;
  L.recModo = -1;
  for (f = 0; f < 3; f++) if (L.tocouModo[f] && (L.recModo < 0 || L.tocouModo[f] > L.tocouModo[L.recModo])) L.recModo = f;
  if (L.redeMedida && L.kbps > 0) {
    L.rec.kbpsMediana = L.kbps;
    L.rec.resolucao = ltd_resolucao_pela_vazao(L.kbpsPior > 0 && L.kbpsPior < L.kbps ? (L.kbps + L.kbpsPior) / 2 : L.kbps);
  }
  L.rec.latenciaMs = L.latenciaMs;
  printf("[livetv-diag] recomenda: formato=%d resolucao=%d espera=%d | tocaram HLS %d/%d TS %d/%d | "
         "sem decoder %d | 10 bits %d | %d kbps | tocaram por modo A %d B %d C %d -> modo %d\n",
         L.rec.formato, L.rec.resolucao, L.rec.espera,
         L.rec.tocaram[0], L.rec.tentados[0], L.rec.tocaram[1], L.rec.tentados[1], L.rec.semDecoder,
         L.rec.dezBits, L.rec.kbpsMediana, L.tocouModo[0], L.tocouModo[1], L.tocouModo[2], L.recModo);
  fflush(stdout);
}

// --- ciclo -----------------------------------------------------------------------
static void comecar(void) {
  GuiaVariante v[LTD_MAX];
  static char bases[LTD_MAX][600];
  int i;
  L.cancelado = 0;
  juntarFio();
  memset(L.it, 0, sizeof L.it);
  L.kbps = L.kbpsPior = 0; L.latenciaMs = -1; L.redeMedida = 0;
  L.aplicado = 0; L.enviou = 0; L.contaLida = 0;
  memset(&L.conta, 0, sizeof L.conta);
  memset(&L.rec, 0, sizeof L.rec);
  L.xtConfig = xtream_configurado();
  L.n = guia_canais_para_teste(v, bases, LTD_MAX, L.grupo, sizeof L.grupo);
  for (i = 0; i < L.n; i++) {
    snprintf(L.it[i].nome, sizeof L.it[i].nome, "%s", v[i].nome);
    snprintf(L.it[i].id, sizeof L.it[i].id, "%s", v[i].id);
    snprintf(L.it[i].base, sizeof L.it[i].base, "%s", bases[i]);
    L.it[i].xt = xtream_e_id(v[i].id);
    L.it[i].alturaNome = v[i].altura;
  }
  printf("[livetv-diag] inicio: %d canal(is) de \"%s\", xtream=%d, resolucao principal=%d, formato=%d, espera=%u ms\n",
         L.n, L.grupo, L.xtConfig, ajustes_livetv_resolucao(), ajustes_livetv_formato(),
         ajustes_livetv_espera_ms());
  fflush(stdout);
  L.botao = 0;
  if (L.n > 0) iniciarRede(0);
  else L.estado = E_PRONTO;
}

void livetvdiag_iniciar(void) {
  L.sair = 0;
  // O PREVIEW DO GUIA solta o pipeline: o teste precisa dele, e a conta de
  // 1 tela nao aguenta o preview e o teste juntos.
  if (player_mini_ativo()) player_fechar_mini();
  comecar();
}

int livetvdiag_quer_sair(void) { return L.sair; }

void livetvdiag_encerrar(void) {
  L.cancelado = 1;
  if (L.estado == E_PLAYER) fecharPlayer();
  juntarFio();
  if (L.estado != E_PRONTO) L.estado = E_PARADO;
}

void livetvdiag_atualizar(float dt, Uint32 agora) {
  (void)dt;
  if (L.sair) return;
  if (L.estado == E_REDE && !atomic_load(&L.fioOcupado)) {
    LtdItem *it = &L.it[L.atual];
    int f;
    juntarFio();
    if (L.cancelado) { L.estado = E_PRONTO; return; }
    f = proximoFormato(it);
    if (f < 0) {
      printf("[livetv-diag] player %s: nada para tocar (%s)\n", it->nome,
             it->semFonte ? "addon sem fonte" : "a rede nao trouxe video em nenhum formato");
      L.pausaAte = agora + LTD_PAUSA_MS;
      avancar();
      return;
    }
    L.estado = E_PLAYER; L.pfFormato = f; L.pfVivo = 0; L.pfDesde = 0;
    L.pausaAte = agora + LTD_PAUSA_MS;
    return;
  }
  if (L.estado == E_PLAYER) {
    if (!L.pfDesde) {
      if ((int)(agora - L.pausaAte) < 0) return;
      iniciarPlayer(L.pfFormato);
      return;
    }
    passoPlayer(agora);
  }
}

// --- teclas --------------------------------------------------------------------
enum { B_APLICAR, B_ENVIAR, B_DENOVO, B_N };
static int botoes(int *lista) {
  int n = 0;
  if (L.estado != E_PRONTO) return 0;
  if (L.rec.confianca > 0 || L.redeMedida || L.recModo >= 0) lista[n++] = B_APLICAR;
  lista[n++] = B_ENVIAR;
  lista[n++] = B_DENOVO;
  return n;
}

void livetvdiag_evento(const SDL_Event *e) {
  SDL_Keycode k;
  int lista[B_N], n;
  if (e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      e->key.keysym.scancode == NV_SCANCODE_BACK) {
    livetvdiag_encerrar();
    L.sair = 1;
    return;
  }
  n = botoes(lista);
  if (!n) return;
  if (L.botao >= n) L.botao = n - 1;
  if (k == SDLK_LEFT && L.botao > 0) L.botao--;
  else if (k == SDLK_RIGHT && L.botao + 1 < n) L.botao++;
  else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
    int b = lista[L.botao];
    if (b == B_APLICAR) {
      ajustes_livetv_aplicar(L.rec.resolucao, L.rec.formato, L.rec.espera);
      if (L.recModo >= 0) ajustes_livetv_aplicar_modo(L.recModo);
      L.aplicado = 1;
      printf("[livetv-diag] aplicado: resolucao=%d formato=%d espera=%d\n", L.rec.resolucao,
             L.rec.formato, L.rec.espera);
      fflush(stdout);
    } else if (b == B_ENVIAR) {
      avisos_enviar_registro_atual();
      L.enviou = 1;
    } else comecar();
  }
}

// --- desenho -------------------------------------------------------------------
static float areaX(void) { float x; ajustes_area_conteudo(NV_MARGEM_X, NV_MARGEM_X, &x, NULL); return x; }
static float areaW(void) { float w; ajustes_area_conteudo(NV_MARGEM_X, NV_MARGEM_X, NULL, &w); return w; }
static char sepDec(void) { return idioma_ponto_decimal(ajustes_idioma()) ? '.' : ','; }

static void painel(GfxRect r, float ar, float ag, float ab) {
  gfx_cor(r, 22.0f / r.h, 0.055f, 0.065f, 0.085f, 0.97f);
  gfx_luz_canto(r, 26.0f / r.h, 150.0f, -70.0f, 500.0f, ar, ag, ab, 0.11f);
  gfx_cor((GfxRect){ r.x, r.y, r.w, 2.0f }, 1.0f, ar, ag, ab, 0.48f);
}
static void titulo(GfxRect r, const char *t, const char *sub) {
  txt_desenhar(txt_linha_corta(TXT_BODY, i18n(t), 238, 242, 248, 255, r.w - 56.0f), r.x + 28.0f, r.y + 22.0f);
  if (sub) txt_desenhar(txt_linha_corta(TXT_CAPTION, sub, 154, 164, 178, 255, r.w - 56.0f),
                        r.x + 28.0f, r.y + 58.0f);
}
static void metrica(GfxRect r, float y, const char *rot, const char *val, int cr, int cg, int cb) {
  TxtLinha v = txt_linha_corta(TXT_BODY, val, cr, cg, cb, 255, r.w * 0.55f);
  txt_desenhar(txt_linha_corta(TXT_CAPTION, i18n(rot), 154, 164, 178, 255, r.w - 80.0f - v.w),
               r.x + 28.0f, y + 3.0f);
  txt_desenhar(v, r.x + r.w - 28.0f - v.w, y);
}
static void segundos(char *d, size_t n, int ms) {
  snprintf(d, n, "%d%c%d s", ms / 1000, sepDec(), (ms % 1000) / 100);
}
static void mbps(char *d, size_t n, int kbps) {
  char b[24];
  vazao_fmt_mbps(b, sizeof b, kbps, sepDec());
  snprintf(d, n, "%s Mbps", b);
}

static const char *nomeFormatoOpcao(int f) {
  return f == 1 ? "HLS (.m3u8)" : f == 2 ? "TS (.ts)" : "Automático";
}
static const char *nomeResOpcao(int r) {
  static const char *const N[] = { "Automática", "4K", "1080p", "720p", "SD" };
  return r >= 0 && r < 5 ? N[r] : "Automática";
}
static const char *nomeEsperaOpcao(int e) {
  return e == 1 ? "25 s" : e == 2 ? "45 s" : "Automática";
}

static void desenharRede(GfxRect r, float ar, float ag, float ab) {
  char a[96], b[64];
  painel(r, ar, ag, ab);
  titulo(r, "Rede até o provedor", NULL);
  if (!L.redeMedida) {
    txt_desenhar(txt_linha_corta(TXT_CAPTION, i18n(L.estado == E_PRONTO ? "Nenhum canal respondeu para medir."
                                                                         : "Medindo…"),
                                 170, 178, 190, 255, r.w - 56.0f), r.x + 28.0f, r.y + 76.0f);
    return;
  }
  mbps(a, sizeof a, L.kbps);
  metrica(r, r.y + 72.0f, "Velocidade", a, 238, 242, 248);
  if (L.kbpsPior > 0) { mbps(a, sizeof a, L.kbpsPior); metrica(r, r.y + 112.0f, "Pior segundo", a, 214, 220, 230); }
  if (L.latenciaMs >= 0) {
    snprintf(b, sizeof b, i18n("%d ms"), L.latenciaMs);
    metrica(r, r.y + 152.0f, "Latência (primeiro byte)", b, L.latenciaMs > 800 ? 244 : 214,
            L.latenciaMs > 800 ? 196 : 220, L.latenciaMs > 800 ? 150 : 230);
  }
}

static void desenharConta(GfxRect r, float ar, float ag, float ab) {
  char a[128];
  painel(r, ar, ag, ab);
  titulo(r, "Conta Xtream", NULL);
  if (!L.xtConfig) {
    txt_bloco(TXT_CAPTION, i18n("Sem conta Xtream neste perfil: os canais testados são dos addons."),
              170, 178, 190, r.x + 28.0f, r.y + 76.0f, r.w - 56.0f, 30.0f, 1, 3);
    return;
  }
  if (!L.contaLida && L.estado != E_PRONTO && L.atual == 0) {
    txt_desenhar(txt_linha_corta(TXT_CAPTION, i18n("Lendo a conta…"), 170, 178, 190, 255, r.w - 56.0f),
                 r.x + 28.0f, r.y + 76.0f);
    return;
  }
  if (!L.conta.valido) {
    snprintf(a, sizeof a, i18n("O servidor não respondeu a conta (HTTP %d)."), L.conta.http);
    txt_bloco(TXT_CAPTION, a, 244, 196, 150, r.x + 28.0f, r.y + 76.0f, r.w - 56.0f, 30.0f, 1, 3);
    return;
  }
  metrica(r, r.y + 72.0f, "Situação", L.conta.status, 238, 242, 248);
  if (L.conta.formatosDeclarados)
    snprintf(a, sizeof a, "%s%s%s", L.conta.temM3u8 ? "HLS" : "", L.conta.temM3u8 && L.conta.temTs ? " · " : "",
             L.conta.temTs ? "TS" : "");
  else snprintf(a, sizeof a, "%s", i18n("não declarados"));
  metrica(r, r.y + 112.0f, "Formatos permitidos", a, 214, 220, 230);
  snprintf(a, sizeof a, i18n("%d de %d"), L.conta.conexoes, L.conta.maxConexoes);
  metrica(r, r.y + 152.0f, "Telas em uso", a, L.conta.maxConexoes > 0 && L.conta.conexoes >= L.conta.maxConexoes ? 244 : 214,
          220, 230);
  if (L.conta.expira > 0) {
    time_t t = (time_t)L.conta.expira;
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(a, sizeof a, "%d/%m/%Y", &tm);
  } else snprintf(a, sizeof a, "%s", i18n("sem vencimento"));
  metrica(r, r.y + 192.0f, "Validade", a, 214, 220, 230);
}

static void textoResultado(const LtdItem *it, char *res, size_t nr, char *det, size_t nd, int *cor) {
  const LtdFormato *ok = it->f[F_HLS].tocou ? &it->f[F_HLS] : it->f[F_TS].tocou ? &it->f[F_TS] : NULL;
  const LtdFormato *ref = ok;
  char t[32], k[32];
  int f;
  *cor = 0;
  det[0] = 0;
  if (!it->pronto && &L.it[L.atual] == it && L.estado != E_PRONTO) {
    snprintf(res, nr, "%s", i18n(L.estado == E_REDE ? "Pedindo ao provedor…" : "Abrindo no player…"));
    *cor = 3;
    return;
  }
  if (!it->pronto) { snprintf(res, nr, "%s", i18n("Na fila")); *cor = 3; return; }
  if (ok) {
    segundos(t, sizeof t, ok->quadroMs);
    { char fm[48];
      if (ok->modoOk > 0) snprintf(fm, sizeof fm, i18n("%s · modo %s"), ok == &it->f[F_HLS] ? "HLS" : "TS",
                                   letraModo(ok->modoOk));
      else snprintf(fm, sizeof fm, "%s", ok == &it->f[F_HLS] ? "HLS" : "TS");
      snprintf(res, nr, i18n("Tocou em %s (%s)"), t, fm); }
    *cor = 1;
  } else {
    // A falha mais informativa: a do player, senao a da rede.
    const LtdFormato *p = it->f[F_TS].testou ? &it->f[F_TS] : it->f[F_HLS].testou ? &it->f[F_HLS] : NULL;
    *cor = 2;
    if (!p && ((it->f[F_TS].servido && it->f[F_TS].falha == LTD_SEM_TESTE) ||
               (it->f[F_HLS].servido && it->f[F_HLS].falha == LTD_SEM_TESTE)))
      p = it->f[F_TS].servido ? &it->f[F_TS] : &it->f[F_HLS];
    if (it->semFonte) snprintf(res, nr, "%s", i18n("O addon não mandou fonte"));
    else if (p && p->falha == LTD_SEM_TESTE) { snprintf(res, nr, "%s", i18n("Responde; o player só é testado na TV")); *cor = 3; }
    else if (p && p->falha == LTD_SEM_DECODER) snprintf(res, nr, "%s", i18n("Os dados chegam, mas a TV não decodifica"));
    else if (p && p->falha == LTD_SEM_RESPOSTA) snprintf(res, nr, "%s", i18n("O player não recebeu nada"));
    else if (p && p->falha == LTD_ERRO_PLAYER) snprintf(res, nr, i18n("Erro do player: %s"), p->erro);
    else {
      int h = it->f[F_TS].http ? it->f[F_TS].http : it->f[F_HLS].http;
      if (h >= 400) snprintf(res, nr, i18n("O provedor recusou (HTTP %d)"), h);
      else if (!h) snprintf(res, nr, "%s", i18n("O provedor não respondeu"));
      else snprintf(res, nr, "%s", i18n("A resposta não é vídeo"));
    }
    ref = p ? p : it->f[F_TS].servido ? &it->f[F_TS] : &it->f[F_HLS];
  }
  // DETALHE: resolucao (do player, senao do nome), codec e vazao do trecho.
  { int u = 0, alt = ref && ref->altura ? ref->altura : it->alturaNome;
    if (alt) u += snprintf(det + u, nd - (size_t)u, "%dp", alt);
    for (f = 0; f < 2 && !(ref && ref->codec[0]); f++) if (it->f[f].codec[0]) ref = &it->f[f];
    if (ref && ref->codec[0]) u += snprintf(det + u, nd - (size_t)u, "%s%s", u ? " · " : "", ref->codec);
    if (ref && ref->dezBits) u += snprintf(det + u, nd - (size_t)u, " · %s", i18n("10 bits"));
    if (ref && ref->kbps > 0) { mbps(k, sizeof k, ref->kbps); snprintf(det + u, nd - (size_t)u, "%s%s", u ? " · " : "", k); } }
}

static void desenharCanais(GfxRect r, float ar, float ag, float ab) {
  char sub[160], res[160], det[160];
  int i;
  painel(r, ar, ag, ab);
  if (L.n) snprintf(sub, sizeof sub, i18n("%d canais de “%s”, um de cada vez"), L.n, L.grupo);
  else snprintf(sub, sizeof sub, "%s", i18n("Abra o guia numa fileira com canais e volte aqui."));
  titulo(r, "Canais testados", sub);
  for (i = 0; i < L.n; i++) {
    const LtdItem *it = &L.it[i];
    float y = r.y + 100.0f + (float)i * LTD_LINHA;
    int cor;
    TxtLinha tr;
    textoResultado(it, res, sizeof res, det, sizeof det, &cor);
    if (i == L.atual && L.estado != E_PRONTO)
      gfx_cor((GfxRect){ r.x + 16.0f, y - 6.0f, r.w - 32.0f, LTD_LINHA - 4.0f }, 12.0f / (LTD_LINHA - 4.0f), ar, ag, ab, 0.10f);
    txt_desenhar(txt_linha_corta(TXT_BODY, it->nome, 232, 236, 244, 255, r.w * 0.38f), r.x + 28.0f, y);
    tr = txt_linha_corta(TXT_BODY, res, cor == 1 ? 150 : cor == 2 ? 244 : 190,
                         cor == 1 ? 222 : cor == 2 ? 170 : 196, cor == 1 ? 170 : cor == 2 ? 140 : 206,
                         255, r.w * 0.56f - 28.0f);
    txt_desenhar(tr, r.x + r.w - 28.0f - tr.w, y);
    if (det[0])
      txt_desenhar(txt_linha_corta(TXT_CAPTION, det, 150, 160, 174, 255, r.w - 56.0f),
                   r.x + 28.0f, y + 30.0f);
  }
}

static void desenharTeste(GfxRect r, float ar, float ag, float ab, Uint32 agora) {
  GfxRect v = { r.x + 28.0f, r.y + 76.0f, r.w - 56.0f, (r.w - 56.0f) * 9.0f / 16.0f };
  if (v.y + v.h > r.y + r.h - 20.0f) { v.h = r.y + r.h - 20.0f - v.y; v.w = v.h * 16.0f / 9.0f; }
  quadroPlayer = v;
  painel(r, ar, ag, ab);
  if (L.estado == E_PLAYER && L.pfVivo) {
    char a[96], t[32];
    segundos(t, sizeof t, (int)(agora - L.pfDesde));
    snprintf(a, sizeof a, i18n("%s em %s · %s"), L.it[L.atual].nome, L.pfFormato == F_HLS ? "HLS" : "TS", t);
    titulo(r, "No player agora", a);
    video_janela((int)v.x, (int)v.y, (int)v.w, (int)v.h);
    gfx_furo_raio(v, 10.0f / v.h);
    return;
  }
  if (L.estado == E_PRONTO && (L.rec.confianca || L.redeMedida)) {
    // AJUSTES SUGERIDOS: o de agora -> o recomendado, os tres que "Aplicar" muda.
    char a[96];
    int ac = ajustes_livetv_resolucao(), fc = ajustes_livetv_formato(), ec = (int)ajustes_livetv_espera_ms();
    ec = ec == 25000 ? 1 : ec == 45000 ? 2 : 0;
    titulo(r, "Ajustes sugeridos", NULL);
    snprintf(a, sizeof a, "%s  \xe2\x86\x92  %s", i18n(nomeResOpcao(ac)), i18n(nomeResOpcao(L.rec.resolucao)));
    metrica(r, r.y + 72.0f, "Resolução principal", a, ac == L.rec.resolucao ? 214 : (int)(ar * 255),
            ac == L.rec.resolucao ? 220 : (int)(ag * 255), ac == L.rec.resolucao ? 230 : (int)(ab * 255));
    snprintf(a, sizeof a, "%s  \xe2\x86\x92  %s", i18n(nomeFormatoOpcao(fc)), i18n(nomeFormatoOpcao(L.rec.formato)));
    metrica(r, r.y + 112.0f, "Formato do Xtream", a, fc == L.rec.formato ? 214 : (int)(ar * 255),
            fc == L.rec.formato ? 220 : (int)(ag * 255), fc == L.rec.formato ? 230 : (int)(ab * 255));
    snprintf(a, sizeof a, "%s  \xe2\x86\x92  %s", i18n(nomeEsperaOpcao(ec)), i18n(nomeEsperaOpcao(L.rec.espera)));
    metrica(r, r.y + 152.0f, "Espera para abrir o canal", a, ec == L.rec.espera ? 214 : (int)(ar * 255),
            ec == L.rec.espera ? 220 : (int)(ag * 255), ec == L.rec.espera ? 230 : (int)(ab * 255));
    if (L.recModo >= 0) {
      int mc = ajustes_livetv_modo();
      snprintf(a, sizeof a, "%s  \xe2\x86\x92  %s", letraModo(mc), letraModo(L.recModo));
      metrica(r, r.y + 192.0f, "Modo do player da Live TV", a, mc == L.recModo ? 214 : (int)(ar * 255),
              mc == L.recModo ? 220 : (int)(ag * 255), mc == L.recModo ? 230 : (int)(ab * 255));
    }
    txt_bloco(TXT_CAPTION, i18n("Ficam em Ajustes › Conteúdo › Live TV, e dá para mudar à mão depois."),
              150, 160, 174, r.x + 28.0f, r.y + 246.0f, r.w - 56.0f, 30.0f, 1, 3);
    return;
  }
  titulo(r, "No player agora", NULL);
  txt_bloco(TXT_CAPTION, i18n(L.estado == E_PRONTO ? "Teste concluído. O vídeo de cada canal aparece aqui enquanto ele é testado."
                                                   : "Cada canal abre aqui por até 18 s em cada modo do player, depois de a rede responder."),
            170, 178, 190, r.x + 28.0f, r.y + 76.0f, r.w - 56.0f, 30.0f, 1, 4);
}

// Uma linha (ou duas) de recomendacao; a que nao cabe acima dos botoes fica de
// fora — por isso a ordem de chamada e a ordem de importancia.
static int linhaRec(GfxRect r, float *y, const char *s, int cr, int cg, int cb) {
  float lim = r.y + r.h - 96.0f, w = r.w - 56.0f;
  int linhas = (float)txt_linha(TXT_CAPTION, s, cr, cg, cb, 255).w > w ? 2 : 1;
  if (*y + 30.0f * (float)linhas > lim) return 0;
  txt_bloco(TXT_CAPTION, s, cr, cg, cb, r.x + 28.0f, *y, w, 30.0f, 1, 2);
  *y += 30.0f * (float)linhas + 8.0f;
  return 1;
}

static void desenharRecomendacoes(GfxRect r, float ar, float ag, float ab) {
  char a[240];
  float y = r.y + 72.0f;
  int lista[B_N], n, i, semVideo = 0, hlsServido = 0;
  painel(r, ar, ag, ab);
  titulo(r, "Recomendações", NULL);
  if (L.estado != E_PRONTO) {
    snprintf(a, sizeof a, i18n("Testando %d de %d…"), L.atual + 1, L.n);
    txt_desenhar(txt_linha_corta(TXT_BODY, a, 214, 220, 230, 255, r.w - 56.0f), r.x + 28.0f, y);
    return;
  }
  for (i = 0; i < L.n; i++) {
    if (!algumTocou(&L.it[i])) semVideo++;
    if (L.it[i].xt && L.it[i].f[F_HLS].servido) hlsServido++;
  }
  if (L.aplicado) linhaRec(r, &y, i18n("Aplicado. Os próximos canais já usam estes ajustes."), 150, 222, 170);
  if (L.enviou) {
    int st = avisos_envio_estado();
    linhaRec(r, &y, i18n(st == 2 ? "Resultado enviado no registro." : st == 3 ? "O envio do registro falhou."
                                                                             : "Enviando o registro…"),
             st == 3 ? 244 : 150, st == 3 ? 170 : 222, st == 3 ? 140 : 170);
  }
  if (L.rec.semDecoder) {
    snprintf(a, sizeof a, i18n("%d canal(is): o vídeo chega, mas a TV não começa a decodificar. Esperar mais não resolve; tente o outro formato ou outra resolução do canal."), L.rec.semDecoder);
    linhaRec(r, &y, a, 244, 196, 150);
  }
  if (L.rec.dezBits)
    linhaRec(r, &y, i18n("Há canal em 10 bits: muitas TVs não decodificam esse vídeo. Prefira a versão HD ou SD dele."), 244, 196, 150);
  if (L.xtConfig && L.conta.valido && L.conta.maxConexoes > 0 && L.conta.conexoes >= L.conta.maxConexoes)
    linhaRec(r, &y, i18n("Todas as telas da conta estão em uso: feche o Xtream em outro aparelho."), 244, 196, 150);
  if (L.recModo > 0) {
    snprintf(a, sizeof a, i18n("Os canais abriram no modo %s do player e não no padrão: Aplicar passa a usá-lo nos canais."),
             letraModo(L.recModo));
    linhaRec(r, &y, a, (int)(ar * 255), (int)(ag * 255), (int)(ab * 255));
  }
  if (L.rec.formato == 1)
    linhaRec(r, &y, i18n("O HLS abriu mais canais que o TS nesta TV: o formato do Xtream passa a pedir HLS primeiro."), 214, 220, 230);
  else if (L.rec.formato == 2)
    linhaRec(r, &y, i18n("O TS abriu mais canais que o HLS nesta TV: o formato do Xtream passa a pedir TS primeiro."), 214, 220, 230);
  if (L.xtConfig && L.conta.valido && L.conta.formatosDeclarados && !L.conta.temM3u8 && hlsServido)
    linhaRec(r, &y, i18n("O provedor entrega HLS mesmo com a conta declarando só TS."), 214, 220, 230);
  if (L.redeMedida && L.rec.resolucao) {
    char m[32];
    mbps(m, sizeof m, L.rec.kbpsMediana);
    snprintf(a, sizeof a, i18n("Com %s até o provedor, %s é a resolução que toca sem travar."), m,
             i18n(nomeResOpcao(L.rec.resolucao)));
    linhaRec(r, &y, a, 214, 220, 230);
  }
  if (L.rec.espera)
    linhaRec(r, &y, i18n("Um canal levou mais de 11 s para abrir: a espera sobe para não cortar quem ia abrir."), 214, 220, 230);
  if (L.latenciaMs > 800)
    linhaRec(r, &y, i18n("O provedor demora para responder: a troca de canal vai ser lenta mesmo com rede boa."), 244, 196, 150);
  if (semVideo && semVideo < L.n) {
    snprintf(a, sizeof a, i18n("%d de %d canais não abriram: são esses canais, não a TV. Use a versão em outra resolução deles."), semVideo, L.n);
    linhaRec(r, &y, a, 214, 220, 230);
  } else if (L.n && semVideo == L.n)
    linhaRec(r, &y, i18n("Nenhum canal abriu: confira a conta, a rede e envie o resultado no registro."), 244, 196, 150);
  // BOTOES no rodape do painel.
  n = botoes(lista);
  { static const char *const ROT[B_N] = { "Aplicar recomendadas", "Enviar no registro", "Testar de novo" };
    float x = r.x + 28.0f, by = r.y + r.h - 88.0f;
    if (L.botao >= n) L.botao = n ? n - 1 : 0;
    for (i = 0; i < n; i++) {
      int foco = i == L.botao;
      TxtLinha t = txt_linha(TXT_BODY, i18n(ROT[lista[i]]), foco ? 16 : 232, foco ? 18 : 236, foco ? 22 : 244, 255);
      float w = (float)t.w + 56.0f;
      gfx_cor((GfxRect){ x, by, w, 60.0f }, 0.5f, foco ? ar : 0.13f, foco ? ag : 0.15f, foco ? ab : 0.19f, 1.0f);
      txt_desenhar(t, x + 28.0f, by + (60.0f - (float)t.h) * 0.5f);
      x += w + 20.0f;
    } }
}

void livetvdiag_desenhar(Uint32 agora) {
  float ar, ag, ab, x0 = areaX(), W = areaW(), y0, colE = 560.0f, vao = 40.0f;
  ajustes_acento(&ar, &ag, &ab);
  { TxtLinha tit = txt_linha(TXT_TITULO1, i18n("Diagnóstico da Live TV"), 255, 255, 255, 255);
    TxtLinha sub = txt_linha_corta(TXT_BODY, i18n("Mede a rede até o provedor e testa canais de verdade nesta TV."),
                                   178, 182, 190, 255, W);
    txt_desenhar(tit, x0, NV_MARGEM_Y);
    txt_desenhar(sub, x0, NV_MARGEM_Y + (float)tit.h + 4.0f);
    y0 = NV_MARGEM_Y + (float)tit.h + 4.0f + (float)sub.h + 28.0f;
    if (y0 < 196.0f) y0 = 196.0f; }
  { float yb = NV_TELA_H - NV_MARGEM_Y, xd = x0 + colE + vao, wd = W - colE - vao;
    float hRede = 200.0f, hConta = 250.0f, hCanais = 100.0f + LTD_MAX * LTD_LINHA + 4.0f;
    desenharRede((GfxRect){ x0, y0, colE, hRede }, ar, ag, ab);
    desenharConta((GfxRect){ x0, y0 + hRede + 20.0f, colE, hConta }, ar, ag, ab);
    desenharTeste((GfxRect){ x0, y0 + hRede + hConta + 40.0f, colE, yb - (y0 + hRede + hConta + 40.0f) },
                  ar, ag, ab, agora);
    desenharCanais((GfxRect){ xd, y0, wd, hCanais }, ar, ag, ab);
    desenharRecomendacoes((GfxRect){ xd, y0 + hCanais + 20.0f, wd, yb - (y0 + hCanais + 20.0f) }, ar, ag, ab); }
}
