// O CARTAO DE NOVIDADES DA VERSAO ATUAL (novidades_cartao.h), sem janela nem
// rede: quando abre, o que grava, a corrente com os cartoes antigos (2.0.1 e
// 2.0.2 nunca abrem depois dele), a porta da Home (nunca por cima do player
// nem da pagina do titulo), as paginas e os botoes, e o que cada plataforma
// ve. Cada cenario limpa as marcas e esquece as decisoes, como uma abertura
// nova do app.
//   bash tests/novidades_cartao.sh
#include "novidades_cartao.h"
#include "novidades20.h"
#include "novidades201.h"
#include "novidades202.h"
#include "dados.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int existe(const char *a) { char *s = dados_ler(a); int ok = s != NULL; free(s); return ok; }

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  novcartao_evento(&e);
}

// Uma abertura do app: as marcas do disco ficam, as decisoes da sessao nao.
static void novaSessao(void) {
  novcartao_teste_esquecer();
  novidades202_teste_esquecer();
  novidades201_teste_esquecer();
}

static void limpar(void) {
  dados_apagar(novcartao_arquivo());
  dados_apagar(N202_ARQ);
  dados_apagar(N201_ARQ);
  dados_apagar("novidades-20-guia.txt");
  novaSessao();
}

// O QUADRO DA HOME COMO O app.c O RODA (atualizar, "primeira vez"): o cartao
// atual decide primeiro, com a porta dele; os antigos so com a mesma porta.
static void quadroHome(int homePronta, int player, int detalhe) {
  novcartao_decidir(homePronta, player, detalhe);
  if (homePronta && !player && !detalhe) {
    novidades202_primeira_vez();
    novidades201_primeira_vez();
  }
}

static void instalacaoNova(void) {
  limpar();
  quadroHome(1, 0, 0);
  assert(!novcartao_aberto());
  assert(existe(novcartao_arquivo()));
  assert(!novidades202_aberto() && !novidades201_aberto());
  assert(existe(N202_ARQ) && existe(N201_ARQ));
  puts("PASS: instalacao nova: o guia da 2.0 abre, este cartao fica visto (e os antigos)");
}

static void vindoDa202(void) {
  limpar();
  dados_gravar("novidades-20-guia.txt", "1\n");
  dados_gravar(N202_ARQ, "1\n");
  dados_gravar(N201_ARQ, "1\n");
  quadroHome(1, 0, 0);
  assert(novcartao_aberto());
  assert(!existe(novcartao_arquivo()));   // so ao fechar
  tecla(SDLK_ESCAPE);                      // Voltar na primeira pagina = Agora nao
  assert(!novcartao_aberto() && existe(novcartao_arquivo()));
  // Outra sessao: nao abre de novo, e os antigos tambem nao.
  novaSessao();
  quadroHome(1, 0, 0);
  assert(!novcartao_aberto() && !novidades202_aberto() && !novidades201_aberto());
  puts("PASS: vindo da 2.0.2: abre uma vez, grava a marca, nao volta");
}

static void correnteAntigos(void) {
  // Vindo da 2.0.0 ou 2.0.1: o guia ja foi visto e nenhum cartao da 2.0.x.
  // Abre o da versao atual; os da 2.0.2 e 2.0.1 NAO abrem, nem no mesmo
  // quadro, nem depois de ele fechar, nem em outra sessao.
  limpar();
  dados_gravar("novidades-20-guia.txt", "1\n");
  quadroHome(1, 0, 0);
  assert(novcartao_aberto());
  assert(!novidades202_aberto() && !novidades201_aberto());
  assert(existe(N202_ARQ) && existe(N201_ARQ));
  tecla(SDLK_LEFT);
  tecla(SDLK_RETURN);                      // Agora nao
  assert(!novcartao_aberto());
  quadroHome(1, 0, 0);
  assert(!novidades202_aberto() && !novidades201_aberto());
  novaSessao();
  quadroHome(1, 0, 0);
  assert(!novcartao_aberto() && !novidades202_aberto() && !novidades201_aberto());
  // Mesmo com a marca do atual e SEM as antigas (disco de uma versao de
  // teste, arquivo apagado a mao): o antigo nao abre depois do atual.
  dados_apagar(N202_ARQ);
  dados_apagar(N201_ARQ);
  novaSessao();
  quadroHome(1, 0, 0);
  assert(!novcartao_aberto() && !novidades202_aberto() && !novidades201_aberto());
  puts("PASS: o cartao da 2.0.2 (e o da 2.0.1) nunca abre depois do da versao atual");
}

static void portaDaHome(void) {
  // Com o player ou a pagina do titulo abertos, nao abre E nao gasta a
  // decisao: abre no primeiro quadro em que a Home esta livre.
  limpar();
  dados_gravar("novidades-20-guia.txt", "1\n");
  quadroHome(1, 1, 0);
  assert(!novcartao_aberto() && !existe(N202_ARQ));
  quadroHome(1, 0, 1);
  assert(!novcartao_aberto());
  quadroHome(1, 1, 1);
  assert(!novcartao_aberto());
  quadroHome(0, 0, 0);                     // outra tela, ou a Home ainda montando
  assert(!novcartao_aberto());
  quadroHome(1, 0, 0);
  assert(novcartao_aberto());
  tecla(SDLK_ESCAPE);
  assert(!novcartao_aberto());
  puts("PASS: nunca abre por cima do player nem da pagina do titulo; espera a Home");
}

static void paginas(void) {
  int n, i;
  limpar();
  novcartao_teste_plataforma(NOV_LG);
  novcartao_abrir();
  n = novcartao_paginas();
  assert(n >= 2);
  assert(novcartao_pagina() == 0 && novcartao_foco() == 1);
  // OK em Continuar anda ate "Apoie o projeto"; Concluir fecha e grava.
  for (i = 1; i < n; i++) { tecla(SDLK_RETURN); assert(novcartao_aberto() && novcartao_pagina() == i); }
  // Voltar (tecla) volta uma pagina; OK em Voltar tambem.
  tecla(SDLK_ESCAPE);
  assert(novcartao_aberto() && novcartao_pagina() == n - 2);
  tecla(SDLK_RETURN);
  assert(novcartao_pagina() == n - 1);
  tecla(SDLK_LEFT);
  tecla(SDLK_RETURN);
  assert(novcartao_aberto() && novcartao_pagina() == n - 2 && novcartao_foco() == 1);
  tecla(SDLK_RETURN);
  tecla(SDLK_RETURN);                      // Concluir
  assert(!novcartao_aberto() && existe(novcartao_arquivo()));
  // A esquerda/direita nao saem dos dois botoes.
  novcartao_abrir();
  tecla(SDLK_LEFT); tecla(SDLK_LEFT);
  assert(novcartao_foco() == 0);
  tecla(SDLK_RIGHT); tecla(SDLK_RIGHT);
  assert(novcartao_foco() == 1);
  tecla(SDLK_ESCAPE);
  assert(!novcartao_aberto());
  printf("PASS: %d paginas (a ultima e Apoie o projeto); Continuar, Voltar e Concluir\n", n);
}

static void plataformas(void) {
  int lg, tpk, wgt, and;
  novcartao_teste_plataforma(NOV_LG);
  lg = novcartao_itens_visiveis();
  assert(novcartao_cenas() == 2);          // a tela do Dolby Vision e os Ajustes
  novcartao_teste_plataforma(NOV_TPK);
  tpk = novcartao_itens_visiveis();
  assert(novcartao_cenas() == 1);          // sem a tela do Dolby Vision
  novcartao_teste_plataforma(NOV_WGT);
  wgt = novcartao_itens_visiveis();
  novcartao_teste_plataforma(NOV_ANDROID);
  and = novcartao_itens_visiveis();
  assert(novcartao_cenas() == 1);
  // LG: + Dolby Vision em MKV e o ponteiro; Android: + a abertura no ponto
  // salvo; Samsung (.tpk e .wgt) ve so o que e de todas.
  assert(lg == wgt + 2);
  assert(and == wgt + 1);
  assert(tpk == wgt);
  novcartao_teste_plataforma(0);
  printf("PASS: itens por plataforma: LG %d, .tpk %d, .wgt %d, Android %d\n", lg, tpk, wgt, and);
}

int main(void) {
  const char *dir = getenv("NUVIO_DADOS");
  if (!dir || !dir[0]) return 2;
  dados_iniciar(dir);
  if (strcmp(dados_dir(), dir)) return 2;
  assert(!strcmp(novcartao_versao(), "2.0.3"));
  instalacaoNova();
  vindoDa202();
  correnteAntigos();
  portaDaHome();
  paginas();
  plataformas();
  puts("novidades_cartao: ok");
  return 0;
}
