// Cola de sessao do AutoSync (src/legsync.c): geracao, download da principal,
// referencia embutida real (MKV de ffmpeg), offset aplicado UMA vez, desfazer,
// outra referencia, seek, troca de fonte, troca de dono e teardown.
//   bash tests/legsync.sh     SANITIZE=1 / SANITIZE=thread
#include "legsync.h"
#include "autosync.h"
#include "rede.h"
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *DIR;
static int capacidades = REDE_CAP_JOB;
unsigned rede_pedido_capacidades(void) { return (unsigned)capacidades; }
int rede_pedir(const RedePedido *p, RedeResposta *r) { (void)p; memset(r, 0, sizeof *r); r->erro = REDE_INDISPONIVEL; return 0; }
void rede_resposta_limpar(RedeResposta *r) { free(r->corpo); free(r->cabecalhos); r->corpo = r->cabecalhos = NULL; }

static char *arquivo(const char *nome, long *n) {
  char c[700]; FILE *f; char *b; long t;
  snprintf(c, sizeof c, "%s/%s", DIR, nome); f = fopen(c, "rb"); if (!f) return NULL;
  fseek(f, 0, SEEK_END); t = ftell(f); fseek(f, 0, SEEK_SET);
  b = malloc((size_t)t + 1); if (fread(b, 1, (size_t)t, f) != (size_t)t) { fclose(f); free(b); return NULL; }
  fclose(f); b[t] = 0; if (n) *n = t; return b;
}
// Download da legenda externa (legenda.c): "ext://atraso_ms/arquivo".
char *rede_baixar_bin(const char *url, int segundos, long *n) {
  int ms = 0; char nome[200]; (void)segundos;
  if (sscanf(url, "ext://%d/%199s", &ms, nome) != 2) return NULL;
  if (ms) usleep((useconds_t)ms * 1000);
  return arquivo(nome, n);
}

static _Atomic int travar, atrasoLeitorUs;   // escritos pelo teste, lidos pelo fio do legref
static int pedidosLeitor, paradas;   // paradas: leituras soltas pelo `parar` (cancelamento)
static pthread_mutex_t LM = PTHREAD_MUTEX_INITIALIZER;
static unsigned char *lerMkv(void *u, const char *url, long long ini, long n, long *tam, int *status,
                             int (*parar)(void *), void *pu) {
  const char *nome = strrchr(url, '/'); char c[700]; FILE *f; unsigned char *b; long long total;
  (void)u;
  pthread_mutex_lock(&LM); pedidosLeitor++; pthread_mutex_unlock(&LM);
  while (travar && !parar(pu)) usleep(1000);
  if (atrasoLeitorUs) usleep((useconds_t)atrasoLeitorUs);
  if (parar(pu)) { pthread_mutex_lock(&LM); paradas++; pthread_mutex_unlock(&LM); return NULL; }
  snprintf(c, sizeof c, "%s/%s", DIR, nome ? nome + 1 : url);
  f = fopen(c, "rb"); if (!f) { *status = 404; return NULL; }
  fseek(f, 0, SEEK_END); total = ftell(f);
  if (ini + n > total) n = (long)(total - ini);
  b = malloc((size_t)n + 1); fseek(f, (long)ini, SEEK_SET); *tam = (long)fread(b, 1, (size_t)n, f); fclose(f);
  *status = 206; return b;
}

static unsigned agora = 1000;
static void passo(const char *url, int sensivel) { agora += 50; legsync_passo(url, 30.0, 60.0, sensivel, agora); }
static LegSyncVisao esperarFase(const char *url, LegSyncFase f) {
  LegSyncVisao v;
  for (int i = 0; i < 4000; i++) { passo(url, 0); v = legsync_visao(0); if (v.fase == f) return v; usleep(1000); }
  fprintf(stderr, "esperava fase %d, ficou %d motivo %d\n", f, v.fase, v.motivo);
  assert(!"timeout");
  return v;
}

static const char *MKV = "https://cdn.example/a/ff.mkv";
static const char *MKV2 = "https://cdn.example/b/mm.mkv";

// O documento de referencia em `t` e o que a pessoa ve com o offset total.
static void conferirNaTela(int total) {
  long n; char *b = arquivo("emb.srt", &n); LegendaDocumentoInfo i = { .flags = LEGENDA_DOC_COMPLETO };
  LegendaDocumento *emb = legenda_documento_bytes(b, n, &i); int casou = 0;
  free(b);
  for (double t = 30; t < 590; t += 0.37) {
    LegendaCue a, e;
    int na = legenda_cues(t, total, &a, 1), ne = legenda_documento_cues(emb, t, 0, &e, 1);
    if (na && ne) {
      // A externa e a traducao "Fala traduzida numero N"; o embutido "Line number N".
      int x = -1, y = -2; sscanf(a.texto, "Fala traduzida numero %d", &x); sscanf(e.texto, "Line number %d", &y);
      if (x != y) fprintf(stderr, "t=%.2f '%s' x '%s'\n", t, a.texto, e.texto);
      assert(x == y); casou++;
    }
  }
  assert(casou > 50);
  legenda_documento_liberar(emb);
}

int main(int argc, char **argv) {
  LegSyncVisao v; int casos = 0;
  DIR = argc > 1 ? argv[1] : "/tmp/nv-legref-fx";
  legsync_teste_leitor(lerMkv, NULL);

  // Antes de criar e no slot 1: honesto.
  assert(legsync_visao(0).fase == LEGSYNC_INDISPONIVEL);
  assert(legsync_offset_ms(250) == 250);
  legsync_iniciar(MKV);
  assert(legsync_visao(1).fase == LEGSYNC_DEPOIS);
  assert(legsync_visao(0).motivo == LEGSYNC_M_SEM_EXTERNA);
  legsync_primaria_outra(1); assert(legsync_visao(0).motivo == LEGSYNC_M_EMBUTIDA);
  assert(!legsync_acao(LEGSYNC_ACAO_RAPIDA)); casos++;

  // 1. Externa +2,5 s; Rapida le a faixa embutida e aceita +2500.
  legsync_primaria_externa("ext://0/ext_mais2500.srt", "pt", "Provedor");
  v = esperarFase(MKV, LEGSYNC_PRONTA);
  assert(legsync_offset_ms(250) == 250);          // nada aceito ainda: so o manual
  assert(legsync_acao(LEGSYNC_ACAO_RAPIDA));
  v = esperarFase(MKV, LEGSYNC_ACEITA);
  assert(abs(v.offsetAutoMs - 2500) <= 25);
  assert(!strcmp(v.idiomaRef, "eng"));
  // Offset total = manual + automatico, UMA vez; o manual muda, o automatico fica.
  assert(legsync_offset_ms(250) == 250 + v.offsetAutoMs);
  assert(legsync_offset_ms(-500) == -500 + v.offsetAutoMs);
  assert(legsync_offset_ms(0) == v.offsetAutoMs);
  conferirNaTela(legsync_offset_ms(0));            // a fala certa no instante certo
  casos++;

  // 2. Desfazer: so o manual volta a valer.
  assert(legsync_acao(LEGSYNC_ACAO_DESFAZER));
  v = legsync_visao(0); assert(v.fase == LEGSYNC_DESFEITA && v.offsetAutoMs == 0);
  assert(legsync_offset_ms(250) == 250); casos++;

  // 3. Completa com a mesma referencia (sem reler o arquivo): aceita de novo.
  { int p0 = pedidosLeitor; assert(legsync_acao(LEGSYNC_ACAO_COMPLETA));
    v = esperarFase(MKV, LEGSYNC_ACEITA); assert(abs(v.offsetAutoMs - 2500) <= 25 && pedidosLeitor == p0); casos++; }

  // 4. Seek durante a analise: cancela, mostra pausada e retoma sozinha.
  { int pegou = 0;
    for (int k = 0; k < 200 && !pegou; k++) {
      legsync_acao(LEGSYNC_ACAO_DESFAZER);
      assert(legsync_acao(LEGSYNC_ACAO_COMPLETA));
      passo(MKV, 1);
      v = legsync_visao(0);
      if (v.fase == LEGSYNC_PAUSADA) pegou = 1;
      else esperarFase(MKV, LEGSYNC_ACEITA);
    }
    assert(pegou);
    assert(legsync_offset_ms(0) == 0);           // nada aplicado enquanto pausada
    for (int k = 0; k < 10; k++) { passo(MKV, 1); assert(legsync_visao(0).fase == LEGSYNC_PAUSADA); }
    v = esperarFase(MKV, LEGSYNC_ACEITA); assert(abs(v.offsetAutoMs - 2500) <= 25); casos++; }

  // 5. Outra referencia: ff.mkv so tem uma faixa de texto -> sem outra, offset zerado.
  assert(legsync_acao(LEGSYNC_ACAO_OUTRA));
  v = esperarFase(MKV, LEGSYNC_INDISPONIVEL);
  assert(v.motivo == LEGSYNC_M_SEM_OUTRA && v.acoes == 0 && legsync_offset_ms(100) == 100); casos++;

  // 6. Troca de fonte no meio da sessao (mm.mkv): geracao nova, a escolha fica,
  //    o automatico zera; a outra faixa (ASS) serve de referencia.
  passo(MKV2, 0);
  v = legsync_visao(0); assert(v.fase == LEGSYNC_PRONTA && legsync_offset_ms(0) == 0);
  // A principal e "pt": a faixa 4 (pt) vem primeiro. Ela e a propria externa
  // remuxada (+2,5 s), entao contra ela o par ja esta junto: aceito ~0.
  assert(legsync_acao(LEGSYNC_ACAO_RAPIDA));
  v = esperarFase(MKV2, LEGSYNC_ACEITA); assert(abs(v.offsetAutoMs) <= 25);
  // Outra referencia: a faixa 3 (ASS ingles, a do video) -> +2500; o letreiro nunca.
  assert(legsync_acao(LEGSYNC_ACAO_OUTRA));
  v = esperarFase(MKV2, LEGSYNC_ACEITA); assert(abs(v.offsetAutoMs - 2500) <= 25); casos++;

  // 7. Dono do overlay mudou por fora (desligar): o automatico nao vale mais.
  legsync_acao(LEGSYNC_ACAO_OUTRA);   // faixa 4 excluida -> volta a nada
  legsync_primaria_externa("ext://0/ext_menos1200.srt", "en", "Outro");
  esperarFase(MKV2, LEGSYNC_PRONTA);
  assert(legsync_acao(LEGSYNC_ACAO_RAPIDA));
  v = esperarFase(MKV2, LEGSYNC_ACEITA); assert(abs(v.offsetAutoMs + 1200) <= 25);
  legenda_desligar();
  assert(legsync_offset_ms(300) == 300 && legsync_visao(0).motivo == LEGSYNC_M_SEM_EXTERNA); casos++;

  // 8. Download atrasado de uma escolha antiga nao vira a principal.
  legsync_primaria_externa("ext://300/ext_mais2500.srt", "pt", "Lenta");
  legsync_primaria_externa("ext://0/ext_menos1200.srt", "en", "Rapida");
  esperarFase(MKV2, LEGSYNC_PRONTA);
  usleep(400000);   // a lenta chega agora e e descartada
  assert(legsync_acao(LEGSYNC_ACAO_RAPIDA));
  v = esperarFase(MKV2, LEGSYNC_ACEITA); assert(abs(v.offsetAutoMs + 1200) <= 25); casos++;

  // 9. Leitura da referencia pausa no seek e com buffer curto.
  legsync_iniciar(MKV);
  legsync_primaria_externa("ext://0/ext_mais2500.srt", "pt", "P");
  esperarFase(MKV, LEGSYNC_PRONTA);
  atrasoLeitorUs = 3000;
  assert(legsync_acao(LEGSYNC_ACAO_RAPIDA));
  usleep(30000); passo(MKV, 1); usleep(20000);
  { int p0, p1; pthread_mutex_lock(&LM); p0 = pedidosLeitor; pthread_mutex_unlock(&LM);
    for (int k = 0; k < 6; k++) { passo(MKV, 1); usleep(20000); }
    pthread_mutex_lock(&LM); p1 = pedidosLeitor; pthread_mutex_unlock(&LM);
    assert(p1 <= p0 + 1);
    agora += 50; legsync_passo(MKV, 30, 5.0, 0, agora);    // buffer de 5 s: ainda pausa
    usleep(60000);
    pthread_mutex_lock(&LM); assert(pedidosLeitor <= p1 + 1); pthread_mutex_unlock(&LM); }
  atrasoLeitorUs = 0;
  v = esperarFase(MKV, LEGSYNC_ACEITA); casos++;

  // 9b. CANCELAMENTO com a leitura da referencia PRESA no meio: fechar o
  //     player, trocar a fonte, trocar a faixa (embutida, outra externa,
  //     desligar). Cada um precisa soltar o Range em curso pelo `parar`, nao
  //     aplicar nada depois, e o automatico nunca vazar para a escolha nova.
  for (int caso = 0; caso < 5; caso++) {
    int p0, pa0, ok = 0;
    legsync_iniciar(MKV);
    legsync_primaria_externa("ext://0/ext_mais2500.srt", "pt", "P");
    esperarFase(MKV, LEGSYNC_PRONTA);
    travar = 1;
    pthread_mutex_lock(&LM); p0 = pedidosLeitor; pa0 = paradas; pthread_mutex_unlock(&LM);
    assert(legsync_acao(LEGSYNC_ACAO_RAPIDA));
    for (int k = 0; k < 2000; k++) {   // o leitor entrou e esta preso
      pthread_mutex_lock(&LM); ok = pedidosLeitor > p0; pthread_mutex_unlock(&LM);
      if (ok) break; usleep(1000);
    }
    assert(ok); passo(MKV, 0); assert(legsync_visao(0).fase == LEGSYNC_LENDO);
    switch (caso) {
      case 0: legsync_encerrar(); break;                         // player fechou
      case 1: passo(MKV2, 0); break;                             // fonte trocou
      case 2: legsync_primaria_outra(1); break;                  // foi para a embutida
      case 3: legsync_primaria_externa("ext://0/ext_menos1200.srt", "en", "Q"); break;  // outra externa
      case 4: legsync_primaria_outra(0); break;                  // desligou
    }
    ok = 0;
    for (int k = 0; k < 2000; k++) {
      pthread_mutex_lock(&LM); ok = paradas > pa0; pthread_mutex_unlock(&LM);
      if (ok) break; usleep(1000);
    }
    assert(ok);                                                  // Range solto pelo cancelamento
    travar = 0;
    for (int k = 0; k < 300; k++) {                              // nada atrasado chega a ser aplicado
      passo(caso == 1 ? MKV2 : MKV, 0);
      v = legsync_visao(0);
      assert(v.fase != LEGSYNC_ACEITA && v.fase != LEGSYNC_LENDO && v.fase != LEGSYNC_ANALISANDO);
      assert(legsync_offset_ms(40) == 40);
      usleep(1000);
    }
    if (caso == 0) assert(v.fase == LEGSYNC_INDISPONIVEL && v.motivo == LEGSYNC_M_SEM_EXTERNA);
    if (caso == 1) assert(v.fase == LEGSYNC_PRONTA);             // a escolha da pessoa fica
    if (caso == 2) assert(v.motivo == LEGSYNC_M_EMBUTIDA);
    if (caso == 3) assert(v.fase == LEGSYNC_PRONTA);             // a nova escolha, sem automatico herdado
    if (caso == 4) assert(v.motivo == LEGSYNC_M_SEM_EXTERNA);
  }
  // Fechar o player LOGO depois de pedir a analise (referencia ja lida): o
  // resultado da sessao velha nunca aparece na sessao nova do mesmo arquivo.
  for (int k = 0; k < 20; k++) {
    legsync_iniciar(MKV);
    legsync_primaria_externa("ext://0/ext_mais2500.srt", "pt", "P");
    esperarFase(MKV, LEGSYNC_PRONTA);
    assert(legsync_acao(LEGSYNC_ACAO_RAPIDA));
    if (k & 1) esperarFase(MKV, LEGSYNC_ACEITA);
    legsync_encerrar();
    legsync_iniciar(MKV);
    legsync_primaria_externa("ext://0/ext_mais2500.srt", "pt", "P");
    for (int j = 0; j < 100; j++) {
      passo(MKV, 0); v = legsync_visao(0);
      assert(v.fase != LEGSYNC_ACEITA && legsync_offset_ms(0) == 0);
      usleep(500);
    }
  }
  casos++;

  // 10. Fim de sessao e corridas de teardown: leitura presa, download pendente,
  //     analise em curso, 40 sessoes seguidas; depois destruir com tudo no ar.
  for (int k = 0; k < 40; k++) {
    legsync_iniciar(k & 1 ? MKV : MKV2);
    legsync_primaria_externa(k % 3 ? "ext://0/ext_mais2500.srt" : "ext://20/ext_menos1200.srt", "pt", "P");
    for (int j = 0; j < 30 && legsync_visao(0).fase != LEGSYNC_PRONTA; j++) { passo(k & 1 ? MKV : MKV2, 0); usleep(1000); }
    legsync_acao(k & 2 ? LEGSYNC_ACAO_COMPLETA : LEGSYNC_ACAO_RAPIDA);
    passo(k & 1 ? MKV : MKV2, k % 5 == 0);
    if (k % 4 == 0) legsync_encerrar();
  }
  travar = 1;
  legsync_iniciar(MKV);
  legsync_primaria_externa("ext://0/ext_mais2500.srt", "pt", "P");
  esperarFase(MKV, LEGSYNC_PRONTA);
  legsync_acao(LEGSYNC_ACAO_RAPIDA); usleep(20000);
  legsync_primaria_externa("ext://200/ext_menos1200.srt", "en", "P");   // download no ar
  legsync_destruir();
  travar = 0;
  assert(legsync_offset_ms(77) == 77 && legsync_visao(0).fase == LEGSYNC_INDISPONIVEL);
  usleep(400000);    // o download pendente termina depois do destruir: sem efeito
  casos++;

  // 11. Plataforma sem pedido cancelavel: indisponivel, sem fingir.
  legsync_teste_leitor(NULL, NULL); capacidades = 0;
  legsync_iniciar(MKV);
  legsync_primaria_externa("ext://0/ext_mais2500.srt", "pt", "P");
  for (int j = 0; j < 200 && legsync_visao(0).motivo != LEGSYNC_M_PLATAFORMA; j++) usleep(1000);
  v = legsync_visao(0);
  assert(v.fase == LEGSYNC_INDISPONIVEL && v.motivo == LEGSYNC_M_PLATAFORMA && !legsync_acao(LEGSYNC_ACAO_RAPIDA));
  legsync_destruir(); casos++;
  printf("legsync: %d casos ok\n", casos);
  return 0;
}
