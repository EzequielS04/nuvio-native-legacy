// "Ja assisti" numa recomendacao recebida (recresp.c), a marca automatica no
// fim do player (atividade.c) e a resposta pelo cartao dos creditos (reacao.c).
//
//   bash tests/recresp.sh
//
// Sem janela e sem rede: com NV_REC_URL de mentira o fio de recomenda.c nunca
// e ligado aqui (ninguem chama recomenda_verificar), entao nada sai.
#include "recresp.h"
#include "reacao.h"
#include "atividade.h"
#include "recomenda.h"
#include "catalogo.h"
#include "dados.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int falhas;
#define CHECA(c, msg) do { if (!(c)) { printf("FALHOU: %s (linha %d)\n", msg, __LINE__); falhas++; } } while (0)

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  reacao_evento(&e, 0);
}

static void transicoes(void) {
  RecResp r;
  char j[256];
  memset(&r, 0, sizeof r);
  r.rec = 5;
  CHECA(recresp_aplicar(&r, RECRESP_EV_ASSISTIU, 0, NULL, 100), "assistiu muda");
  CHECA(r.assistida && !r.respondida && r.reacao == RECRESP_SEM_REACAO, "assistida sem resposta");
  CHECA(!recresp_aplicar(&r, RECRESP_EV_ASSISTIU, 0, NULL, 101), "assistiu de novo nao muda nada");
  CHECA(r.versao == 1, "versao so sobe com mudanca");
  recresp_json(&r, j, sizeof j);
  CHECA(!strcmp(j, "{\"id\":5,\"reacao\":null,\"texto\":\"\"}"), "json sem reacao");
  CHECA(recresp_aplicar(&r, RECRESP_EV_RESPONDEU, 1, "Valeu, AMEI!!", 102), "respondeu");
  CHECA(r.respondida && r.reacao == 1 && !strcmp(r.texto, "valeu amei"), "reacao e texto limpo");
  recresp_aplicar(&r, RECRESP_EV_RESPONDEU, RECRESP_SEM_REACAO, "", 103);
  CHECA(r.reacao == 1 && !strcmp(r.texto, "valeu amei"), "resposta vazia nao apaga");
  recresp_json(&r, j, sizeof j);
  CHECA(!strcmp(j, "{\"id\":5,\"reacao\":1,\"texto\":\"valeu amei\"}"), "json completo");
  memset(&r, 0, sizeof r);
  r.rec = 6;
  recresp_aplicar(&r, RECRESP_EV_PULOU, 0, NULL, 100);
  CHECA(r.assistida && r.respondida && r.reacao == RECRESP_SEM_REACAO, "pular = respondida sem reacao");
}

static void fila(void) {
  char corpo[256];
  long long rec = 0;
  unsigned v = 0;
  recresp_esquecer();
  CHECA(!recresp_pendente(corpo, sizeof corpo, &rec, &v), "vazio nao tem pendente");
  recresp_marcar_assistida(11);
  CHECA(recresp_assistida(11) && !recresp_respondida(11), "marcada");
  CHECA(recresp_pendente(corpo, sizeof corpo, &rec, &v) && rec == 11, "vai ao servidor");
  recresp_responder(11, -1, "nao curti");   // mudou DURANTE o envio
  recresp_confirmar(11, v);
  CHECA(recresp_pendente(corpo, sizeof corpo, &rec, &v) && strstr(corpo, "\"reacao\":-1"),
        "mudanca durante o envio continua pendente");
  recresp_confirmar(11, v);
  CHECA(!recresp_pendente(corpo, sizeof corpo, &rec, &v), "confirmada sai da fila");
  { char *b = dados_ler("recomendacoes-respostas.txt");
    CHECA(b && strstr(b, "11\t1\t1\t-1\t") && strstr(b, "nao curti"), "gravado no disco");
    free(b); }
}

static void fimNoPlayer(void) {
  CatItem ci;
  int i;
  recresp_esquecer();
  memset(&ci, 0, sizeof ci);
  snprintf(ci.imdb, sizeof ci.imdb, "tt0100");
  snprintf(ci.tipo, sizeof ci.tipo, "series");
  snprintf(ci.titulo, sizeof ci.titulo, "Serie");
  atividade_marcar_origem(42, "tt0100", "Ana");
  atividade_player_passo(&ci, 1, 3, 30.0, 3000.0, 1, 0, 0.1f);
  for (i = 0; i < 10; i++) atividade_player_passo(&ci, 1, 3, 1000.0 + i, 3000.0, 1, 0, 0.1f);
  CHECA(!recresp_assistida(42), "no meio do episodio ainda nao");
  atividade_player_passo(&ci, 1, 3, 2950.0, 3000.0, 1, 1, 0.1f);   // creditos
  CHECA(recresp_assistida(42), "fim de um EPISODIO da serie recomendada marca a rec");
  CHECA(!recresp_respondida(42), "marcar nao responde por ninguem");
  atividade_player_saiu(2950.0, 3000.0, 1);
  // Sem origem: nada.
  snprintf(ci.imdb, sizeof ci.imdb, "tt0200");
  snprintf(ci.tipo, sizeof ci.tipo, "movie");
  atividade_player_passo(&ci, 0, 0, 30.0, 6000.0, 1, 0, 0.1f);
  atividade_player_passo(&ci, 0, 0, 5900.0, 6000.0, 1, 0, 0.1f);   // 98 %
  atividade_player_saiu(5900.0, 6000.0, 0);
  CHECA(!recresp_assistida(43) && !recresp_assistida(0), "titulo sem origem nao marca nada");
}

static void cartao(void) {
  RecResp r;
  recresp_esquecer();
  // "Ja assisti" na aba: a aba marca, o cartao pergunta.
  recresp_marcar_assistida(7);
  CHECA(reacao_rec_abrir(7, "tt0300", "Filme", "movie", "", "Ana", 1500.0f), "abre");
  CHECA(reacao_painel_aberta() && reacao_passo() == 0, "passo da reacao");
  tecla(SDLK_RETURN);   // Gostei
  CHECA(recresp_ler(7, &r) && r.respondida && r.reacao == 1, "gostei vai a quem mandou");
  CHECA(reacao_passo() == 1, "com servico: oferece a mensagem no MESMO cartao");
  tecla(SDLK_RETURN);   // "Valeu pela dica!"
  CHECA(recresp_ler(7, &r) && !strcmp(r.texto, "valeu pela dica") && r.reacao == 1,
        "mensagem rapida sem perder a reacao");
  CHECA(!reacao_painel_aberta(), "fecha depois da mensagem");
  // Voltar no primeiro passo: assistida, sem resposta.
  recresp_marcar_assistida(8);
  reacao_rec_abrir(8, "tt0400", "Outro", "movie", "", "Bia", -1.0f);
  tecla(SDLK_ESCAPE);
  CHECA(recresp_assistida(8) && !recresp_respondida(8), "voltar nao responde");
  // "Agora nao".
  reacao_rec_abrir(8, "tt0400", "Outro", "movie", "", "Bia", -1.0f);
  tecla(SDLK_RIGHT); tecla(SDLK_RIGHT); tecla(SDLK_RETURN);   // Nao gostei
  tecla(SDLK_RIGHT); tecla(SDLK_RIGHT); tecla(SDLK_RETURN);   // Agora nao
  CHECA(recresp_ler(8, &r) && r.respondida && r.reacao == -1 && !r.texto[0], "agora nao");
}

int main(void) {
  const char *dir = getenv("NUVIO_DADOS");
  if (!dir || !*dir) { puts("recresp: NUVIO_DADOS ausente; recusando rodar"); return 1; }
  dados_iniciar(dir);
  if (strcmp(dados_dir(), dir)) { puts("recresp: dados_dir() nao e NUVIO_DADOS; recusando rodar"); return 1; }
  transicoes();
  fila();
  fimNoPlayer();
  cartao();
  if (falhas) { printf("recresp: %d falha(s)\n", falhas); return 1; }
  puts("recresp: ok");
  return 0;
}
