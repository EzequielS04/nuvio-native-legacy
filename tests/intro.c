// LEITURA DOS MARCADORES DO TheIntroDB.
//
// As cargas abaixo sao RESPOSTAS REAIS da API, capturadas com curl antes de o
// leitor existir — nao invencao minha. Sao elas que fixam as tres coisas que
// mudaram junto com a fonte e que dariam erro silencioso se trocadas:
//
//   1. as chaves valem ARRAY e nao objeto (um tipo pode ter varios trechos);
//   2. os tempos vem em MILISSEGUNDOS (segundos daria um numero mil vezes
//      errado sem parecer errado);
//   3. `null` tem significado: start = 0, end = "ate o fim da midia".
//
//   bash tests/intro.sh
#include "../src/intro.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// DUBLE DE REDE. intro.c chama rede_baixar no fio de download; este teste so
// exercita o LEITOR, entao o duble existe para linkar e nada mais. Devolver
// NULL e o comportamento certo caso alguem chame intro_pedir aqui por engano:
// zero marcadores, sem rede.
char *rede_baixar(const char *url, int segundos) {
  (void)url; (void)segundos; return NULL;
}


// Le uma resposta gravada de tests/fixtures/theintrodb (chamadas reais a
// api.theintrodb.org/v3/media em 07/10/2026, sem chave). Estatico: cabe.
static const char *fixture(const char *nome) {
  static char buf[4][2048]; static int k;
  char caminho[256]; FILE *f; size_t n;
  char *b = buf[k++ & 3];
  snprintf(caminho, sizeof caminho, "tests/fixtures/theintrodb/%s", nome);
  f = fopen(caminho, "rb");
  if (!f) { fprintf(stderr, "fixture ausente: %s\n", caminho); exit(1); }
  n = fread(b, 1, sizeof buf[0] - 1, f); fclose(f); b[n] = 0;
  return b;
}
static double credFinal(const IntroTrecho *v, int n) {
  double s = 0.0; int i;
  for (i = 0; i < n; i++) if (v[i].tipo == INTRO_CREDITOS && v[i].inicio > s) s = v[i].inicio;
  return s;
}

static const IntroTrecho *achar(const IntroTrecho *v, int n, int tipo) {
  int i;
  for (i = 0; i < n; i++) if (v[i].tipo == tipo) return &v[i];
  return NULL;
}

// Compara em milissegundos para a assercao nao depender de ponto flutuante.
static int mesmoSeg(double a, double esperado) {
  double d = a - esperado;
  return d < 0.001 && d > -0.001;
}

int main(void) {
  IntroTrecho v[8];
  int n;

  // FILME: /v3/media?imdb_id=tt0111161 (Shawshank). So creditos, sem fim.
  n = intro_extrair("{\"tmdb_id\":278,\"type\":\"movie\","
                    "\"credits\":[{\"start_ms\":8300000,\"end_ms\":null}]}", v, 8);
  assert(n == 1);
  { const IntroTrecho *c = achar(v, n, INTRO_CREDITOS);
    assert(c);
    assert(mesmoSeg(c->inicio, 8300.0));   // 8.300.000 ms, e nao 8.300.000 s
    // fim ZERO e o contrato de "ate o fim da midia" (end_ms nulo). Sem isto,
    // um fim 0 seria lido como trecho vazio e o marcador sumiria.
    assert(c->fim == 0.0); }
  puts("ok  filme: creditos em milissegundos, fim nulo vira 'ate o fim'");

  // FILME com abertura e SEM creditos: /v3/media?imdb_id=tt0816692
  // (Interstellar). start_ms nulo = comeca no zero.
  n = intro_extrair("{\"tmdb_id\":157336,\"type\":\"movie\","
                    "\"intro\":[{\"start_ms\":null,\"end_ms\":53000}]}", v, 8);
  assert(n == 1);
  { const IntroTrecho *a = achar(v, n, INTRO_ABERTURA);
    assert(a && mesmoSeg(a->inicio, 0.0) && mesmoSeg(a->fim, 53.0));
    assert(!achar(v, n, INTRO_CREDITOS)); }
  puts("ok  filme: start nulo vira zero, e ausencia de creditos nao inventa um");

  // SERIE: /v3/media?imdb_id=tt14688458&season=1&episode=1 (Silo T1E1).
  n = intro_extrair("{\"tmdb_id\":125988,\"type\":\"tv\",\"season\":1,\"episode\":1,"
                    "\"intro\":[{\"start_ms\":272500,\"end_ms\":366000}],"
                    "\"credits\":[{\"start_ms\":3503000,\"end_ms\":null}]}", v, 8);
  assert(n == 2);
  { const IntroTrecho *a = achar(v, n, INTRO_ABERTURA);
    const IntroTrecho *c = achar(v, n, INTRO_CREDITOS);
    assert(a && mesmoSeg(a->inicio, 272.5) && mesmoSeg(a->fim, 366.0));
    assert(c && mesmoSeg(c->inicio, 3503.0)); }
  puts("ok  serie: abertura e creditos na mesma resposta");

  // MAIS DE UM TRECHO DO MESMO TIPO — e a razao de a chave ser array. Um leitor
  // que so pegasse o primeiro elemento passaria nos casos acima e falharia aqui.
  n = intro_extrair("{\"type\":\"tv\",\"intro\":["
                    "{\"start_ms\":1000,\"end_ms\":2000},"
                    "{\"start_ms\":5000,\"end_ms\":6000}]}", v, 8);
  assert(n == 2);
  assert(mesmoSeg(v[0].inicio, 1.0) && mesmoSeg(v[1].inicio, 5.0));
  puts("ok  duas aberturas no mesmo episodio entram as duas");

  // RESUMO tem tipo proprio: o player pula abertura e resumo, mas o posplay so
  // se importa com creditos. Trocar os tipos faria o painel subir na abertura.
  n = intro_extrair("{\"type\":\"tv\",\"recap\":[{\"start_ms\":0,\"end_ms\":30000}]}", v, 8);
  assert(n == 1 && v[0].tipo == INTRO_RESUMO);
  puts("ok  recap vira INTRO_RESUMO e nao abertura");

  // TRECHO INVERTIDO OU VAZIO NAO ENTRA. Vem de dado errado na base, e um
  // trecho com fim <= inicio deixaria intro_ativo verdadeiro para sempre.
  n = intro_extrair("{\"type\":\"tv\",\"intro\":["
                    "{\"start_ms\":9000,\"end_ms\":9000},"
                    "{\"start_ms\":9000,\"end_ms\":1000}]}", v, 8);
  assert(n == 0);
  puts("ok  trecho vazio ou invertido e descartado");

  // O QUE A API RESPONDE QUANDO NAO CONHECE O TITULO, e o resto do lixo.
  assert(intro_extrair("{\"error\":\"media not found\"}", v, 8) == 0);
  assert(intro_extrair("{}", v, 8) == 0);
  assert(intro_extrair("nao e json", v, 8) == 0);
  assert(intro_extrair("", v, 8) == 0);
  assert(intro_extrair(NULL, v, 8) == 0);
  assert(intro_extrair("{\"credits\":[{\"start_ms\":1000,\"end_ms\":2000}]}", v, 0) == 0);
  puts("ok  404, vazio, lixo e max=0 devolvem zero sem estourar");

  // JANELA RECUSADA / ACEITA (#202): creditos "ate o fim" com inicio cedo
  // demais deixavam o botao 25-30 min na tela.
  {
    const char *m;
    assert(intro_janela_ok(INTRO_CREDITOS, 8300, 0, 8520, 1, &m) == 1);       // Shawshank
    assert(intro_janela_ok(INTRO_CREDITOS, 5400, 0, 8400, 1, &m) == 0);       // 50 min de janela
    assert(intro_janela_ok(INTRO_CREDITOS, 3000, 0, 8400, 1, &m) == 0);       // antes de 50%
    assert(intro_janela_ok(INTRO_CREDITOS, 7300, 7900, 8400, 1, &m) == 1);    // 10 min, depois de 50%
    assert(intro_janela_ok(INTRO_CREDITOS, 3503, 0, 3600, 0, &m) == 1);       // serie
    assert(intro_janela_ok(INTRO_ABERTURA, 60, 400, 3600, 0, &m) == 0);       // > 3 min
    assert(intro_janela_ok(INTRO_ABERTURA, 272, 366, 3600, 0, &m) == 1);
    assert(intro_janela_ok(INTRO_CREDITOS, 100, 0, 0, 1, &m) == 1);           // sem duracao: nao chuta
    puts("ok  guarda de janela");
  }
#ifdef NV_SHOT_HOOKS
  {
    // AUTO-HIDE: aparece sozinho, some em 10 s, volta so com os controles, e
    // nao reaparece sozinho. Creditos de filme: 7700 s ate o fim (dur 8400).
    IntroTrecho t[1] = {{7700, 0, INTRO_CREDITOS}};
    double fim; int tipo;
    intro_shot_definir(t, 1);
    intro_definir_duracao(8400, 1);
    assert(!intro_botao(7000, 100.0, 0, 0, &fim, &tipo));      // fora da janela
    assert(intro_botao(7701, 101.0, 0, 0, &fim, &tipo) && tipo == INTRO_CREDITOS);
    assert(intro_botao(7705, 105.0, 0, 0, &fim, &tipo));       // ainda 4 s
    assert(!intro_botao(7712, 112.0, 0, 0, &fim, &tipo));      // 11 s: sumiu
    assert(!intro_botao_visivel(NULL, NULL));                  // tecla nao pula as cegas
    assert(!intro_botao(8000, 700.0, 0, 0, &fim, &tipo));      // 25 min depois: nada
    assert(intro_botao(8000, 701.0, 1, 0, &fim, &tipo));       // controles: volta
    assert(intro_botao_visivel(&fim, &tipo));
    assert(!intro_botao(8001, 702.0, 0, 0, &fim, &tipo));      // controles somem: some
    // focado nao expira
    assert(intro_botao(8002, 900.0, 0, 1, &fim, &tipo));
    assert(intro_botao(8003, 905.0, 0, 1, &fim, &tipo));
    // creditos recusados (inicio cedo): botao nunca aparece, nem com controles
    {
      IntroTrecho r[1] = {{3000, 0, INTRO_CREDITOS}};
      intro_shot_definir(r, 1);
      assert(!intro_botao(3500, 1000.0, 1, 0, &fim, &tipo));
      assert(!intro_ativo(3500, &fim, &tipo));
    }
    puts("ok  botao some em 10 s, volta com controles, janela recusada nao aparece");
  }
#endif


  // --- 2.0.3: RESPOSTAS REAIS GRAVADAS + SANIDADE CONTRA A DURACAO ------------
  //
  // Duracoes: arquivo tipico de cada episodio (Breaking Bad T1E1 58:01, T1E2
  // 48:06; Silo T1E1 59 min pelo TVmaze; AoT 24:00; One Piece T1E1 25:00, que e
  // o end_ms da previa). O que se prova: unidade (ms -> s), o marcador cai nos
  // creditos DESTE episodio, e o marcador de OUTRO episodio/corte e recusado.
  { struct { const char *arq; int t, e; double dur, credEsperado; } fx[] = {
      { "bb_s01e01.json",       1, 1, 3481.0, 3431.0 },
      { "bb_s01e02.json",       1, 2, 2886.0, 2839.0 },
      { "silo_s01e01.json",     1, 1, 3540.0, 3503.0 },
      { "aot_s02e01.json",      2, 1, 1440.0, 1330.079 },
      { "onepiece_s01e01.json", 1, 1, 1500.0, 1389.0 },
    };
    const char *m;
    for (size_t k = 0; k < sizeof fx / sizeof *fx; k++) {
      const char *j = fixture(fx[k].arq);
      double c, resta;
      int i;
      assert(intro_resposta_confere(j, fx[k].t, fx[k].e));
      n = intro_extrair(j, v, 8);
      assert(n >= 2);
      c = credFinal(v, n);
      assert(mesmoSeg(c, fx[k].credEsperado));
      resta = fx[k].dur - c;
      // creditos nos ultimos 40-120 s: segundos, nao ms nem minutos
      assert(resta > 30.0 && resta < 130.0);
      for (i = 0; i < n; i++)
        if (!intro_janela_ok(v[i].tipo, v[i].inicio, v[i].fim, fx[k].dur, 0, &m)) {
          fprintf(stderr, "FALHOU %s trecho %d tipo %d: %s\n", fx[k].arq, i, v[i].tipo, m);
          assert(0);
        }
      printf("ok  %-22s creditos %.0fs de %.0fs (sobram %.0fs, janela %.0fs)\n",
             fx[k].arq, c, fx[k].dur, resta, intro_creditos_janela(fx[k].dur));
    }
    // MARCADOR DE OUTRO EPISODIO: o de T1E2 (2839 s) tocando T1E1 (3481 s)
    // sobra 642 s — meio do terceiro ato, recusado; o de T1E1 (3431 s) num
    // arquivo de T1E2 (2886 s) passa do fim.
    assert(!intro_janela_ok(INTRO_CREDITOS, 2839.0, 0, 3481.0, 0, &m));
    assert(!intro_janela_ok(INTRO_CREDITOS, 3431.0, 0, 2886.0, 0, &m));
    // OUTRO CORTE: One Piece T1E1 com fim explicito em 1459 s num arquivo de
    // 1420 s (sem a previa) — o marcador e de um arquivo mais longo.
    assert(!intro_janela_ok(INTRO_CREDITOS, 1389.0, 1459.0, 1420.0, 0, &m));
    // ABERTURA no meio do episodio nao e abertura.
    assert(!intro_janela_ok(INTRO_ABERTURA, 1800.0, 1890.0, 3481.0, 0, &m));
    assert( intro_janela_ok(INTRO_ABERTURA, 228.664, 246.143, 3481.0, 0, &m));
    puts("ok  marcador de outro episodio, de outro corte e abertura no meio sao recusados");
  }
  // O TheIntroDB REMAPEIA a numeracao: One Piece imdb T4E1 e T4E2 voltam os
  // dois como T1E48. A resposta que nao ecoa o par pedido e descartada.
  assert(!intro_resposta_confere(fixture("onepiece_imdb_s04e01_remap.json"), 4, 1));
  assert(!intro_resposta_confere(fixture("onepiece_imdb_s04e02_remap.json"), 4, 2));
  assert( intro_resposta_confere(fixture("shawshank.json"), 0, 0));          // filme
  assert( intro_resposta_confere("{\"intro\":[]}", 3, 4));                  // sem eco
  assert(intro_extrair(fixture("onepiece_cinemeta_s04e92_404.json"), v, 8) == 0);
  puts("ok  resposta de outro episodio (remapeada) nao vale");
  // A JANELA DOS CREDITOS DE SERIE e a ESTIMATIVA FIXA (sem porcentagem).
  assert(mesmoSeg(intro_creditos_janela(1320.0), 198.0));   // 22 min
  assert(mesmoSeg(intro_creditos_janela(1500.0), 225.0));   // 25 min (R8: 154 s cabe)
  assert(mesmoSeg(intro_creditos_janela(2700.0), 300.0));   // 45 min
  assert(mesmoSeg(intro_creditos_janela(10800.0), 300.0));  // 3 h: teto
  assert(mesmoSeg(intro_creditos_janela(480.0), 120.0));    // 8 min: piso
  assert(mesmoSeg(intro_creditos_janela(200.0), 100.0));    // nunca mais que metade
  assert(mesmoSeg(intro_fim_estimado(3600.0), INTRO_FIM_SERIE_S) && INTRO_FIM_SERIE_S == 40.0);
  assert(mesmoSeg(intro_fim_estimado(1320.0), 40.0));
  assert(mesmoSeg(intro_fim_estimado(480.0), INTRO_FIM_CURTO_S) && INTRO_FIM_CURTO_S == 15.0);
  assert(intro_fim_estimado(100.0) == 0.0);                 // clipe: nada
  puts("ok  janela dos creditos (15%, 2-5 min) e estimativa fixa (40 s / 15 s)");
#ifdef NV_SHOT_HOOKS
  {
    // intro_creditos_seg so devolve marcador ACEITO: o de "creditos" a 10 min
    // do fim de um episodio de 58 min nao chega ao cartao.
    IntroTrecho t[2] = {{2881.0, 0, INTRO_CREDITOS}, {228.0, 246.0, INTRO_ABERTURA}};
    intro_shot_definir(t, 2);
    intro_definir_duracao(3481.0, 0);
    assert(intro_creditos_seg() == 0.0);
    t[0].inicio = 3431.0;
    intro_shot_definir(t, 2);
    assert(mesmoSeg(intro_creditos_seg(), 3431.0));
    puts("ok  creditos recusados nao chegam ao cartao");
  }
#endif
  puts("intro: tudo ok");
  return 0;
}
