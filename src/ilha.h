// ILHA — o relogio no topo da tela, numa pilula, que se ABRE para os avisos.
//
// Pedido do dono (01/10/2026): "ilha dinamica estilo iOS". Em repouso a
// pilula mostra a hora; quando o app tem algo a dizer (aviso novo na central,
// erro, lembrete, versao nova, atualizacao baixando) a MESMA pilula cresce com
// mola, troca o conteudo por icone + frase curta e, vencido o prazo, encolhe
// de volta para o relogio. Um lugar so para o app falar, em vez de cada coisa
// abrir o proprio cartao num canto diferente.
//
// O QUE ELA NAO FAZ: nao aparece por cima do player (quem decide e app.c, que
// so a desenha com o player fechado), nao pega teclado fora do modal (o
// AZUL/CH+ da central continua em avisos_evento; o do modal, ver abaixo) e
// nao e camada de tela cheia — e uma sombra pequena, uma pilula e o texto,
// mesmo durante a mola, e o modal e a mesma pilula crescida.
//
// QUATRO CONTEUDOS, por prioridade: aviso (fila curta, um de cada vez) >
// atividade (algo em andamento, renovado a cada quadro por quem a tem) >
// cartao (atividade ao vivo / estreia, ao lado do relogio, so com ele) >
// relogio (so onde app.c disse que cabe: ilha_relogio_visivel).
//
// FIO PRINCIPAL em tudo.
#ifndef NV_ILHA_MODULO_H
#define NV_ILHA_MODULO_H
#include <SDL2/SDL.h>

enum { ILHA_INFO = 0, ILHA_OK, ILHA_ERRO, ILHA_ACENTO };

// Aviso curto. `chave` identifica o aviso: repetir a chave do que esta na
// tela troca o texto NO LUGAR (a pilula remorfa) e renova o prazo, em vez de
// entrar na fila de novo; NULL/"" = um aviso avulso. `icone` e um nome de
// art/icones (NULL = o do tipo). `ms` conta a partir do primeiro quadro em que
// ele aparece. `tecla` = 1 desenha a tecla da central (disco AZUL / CH+) com
// "abre" na ponta direita.
void ilha_avisar(const char *chave, int tipo, const char *icone,
                 const char *texto, unsigned ms, int tecla);
// Tira o aviso desta chave da tela e da fila (ex.: a central foi aberta).
void ilha_retirar(const char *chave);
// 1 enquanto o aviso desta chave esta na tela ou na fila.
int  ilha_tem(const char *chave);

// ATIVIDADE em andamento: chame A CADA QUADRO enquanto durar; sem renovacao
// por ~0,4 s ela sai sozinha. `progresso` de 0 a 1, ou < 0 quando nao ha numero.
void ilha_atividade(const char *texto, float progresso);

// Onde o relogio pode ficar, decidido por quadro por app.c (a home tem o topo
// esquerdo livre; Ajustes e Explorar tem titulo ali).
void ilha_relogio_visivel(int visivel);

// PONTO UNICO DA POSICAO. Sem chamada, a ilha fica no topo esquerdo, na coluna
// do conteudo — ou no topo direito no layout Dinamica, onde a pilula fechada
// da barra lateral ocupa o canto esquerdo. Quem expuser uma area (ex. a pilula
// do menu) chama isto a cada quadro antes de ilha_desenhar: (x, y) e o canto
// de cima da ilha; `daDireita` = 1 faz x ser a borda DIREITA e a ilha crescer
// para a esquerda. Vale para um quadro.
void ilha_ancorar(float x, float y, int daDireita);

// Aplica a escolha de Ajustes (Posicao do relogio) ao quadro: chamar antes de
// ilha_desenhar. `guia` = 1 na tela do Guia (titulo a esquerda: vai a direita).
void ilha_posicionar(int guia);

void ilha_desenhar(Uint32 agora);
// 1 quando ha aviso ou atividade aberta (o relogio sozinho nao conta).
int  ilha_ocupada(void);

// --- CARTOES PERSISTENTES: atividade ao vivo e estreia (pedido do dono, 01/10) ---
//
// SO COM O RELOGIO NA TELA (Ajustes > Relogio na tela e onde app.c diz que ele
// cabe). Um cartao fica AO LADO do relogio, na mesma pilula, ate quem o poe
// tira-lo (ilhacart.c): a sessao interrompida do player (ILHA_VIVO) e o
// episodio novo de uma serie com lembrete ainda nao lido (ILHA_ESTREIA). Os
// avisos e a atividade continuam passando na frente; dois cartoes alternam a
// cada ILHA_ALTERNA_MS. AZUL/CH+ com um cartao na pilula abre o MODAL — a
// pilula cresce ate um cartao maior com a arte e os botoes (ilha_evento).
enum { ILHA_VIVO = 0, ILHA_ESTREIA, ILHA_N_CARTOES };
#define ILHA_ALTERNA_MS 6000u
typedef struct {
  char chave[80];        // muda = outro conteudo (a pilula remorfa)
  char imdb[64];
  int  serie, t, e;      // t/e = 0 em filme
  char titulo[160], epNome[120], sinopse[420];
  char poster[1024], logo[512], arte[512];   // arte: still do episodio ou fundo
  float progresso;       // 0..1 na ILHA_VIVO; < 0 sem barra
  int  restanteMin;      // ILHA_VIVO
  char quando[24];       // ILHA_ESTREIA: "hoje" (ja traduzido) ou ""
  char avisoId[72];      // ILHA_ESTREIA: id do aviso em avisos.c
} IlhaCartao;
// NULL tira o cartao. A copia e da ilha; quem chama pode descartar o seu.
void ilha_cartao(int qual, const IlhaCartao *c);
// 1 quando ha cartao e o relogio esta na tela: e quando AZUL/CH+ abre o modal.
int  ilha_cartao_na_tela(void);

// O MODAL. Abrir pega o cartao que a pilula mostra; Voltar recolhe para ela.
int  ilha_modal_abrir(void);
void ilha_modal_fechar(int seco);   // seco = 1: some sem recolher (virou o painel)
int  ilha_modal_aberto(void);
int  ilha_modal_visivel(void);
// Teclado do modal (come tudo enquanto aberto). Fora dele devolve 0.
int  ilha_evento(const SDL_Event *e);
// Acao pedida no modal, entregue uma vez (o contrato de avisos_pediu). `c`
// recebe o cartao em que ela foi pedida.
enum { ILHA_PEDIU_NADA = 0, ILHA_PEDIU_TOCAR, ILHA_PEDIU_DETALHES,
       ILHA_PEDIU_DISPENSAR, ILHA_PEDIU_SALVOS };
int  ilha_pediu(IlhaCartao *c, int *qual);

// Retangulo da pilula (ou do modal, enquanto ele esta na tela) no ultimo
// quadro: e de onde o painel de Salvos nasce e para onde ele recolhe. 0 quando
// a ilha nao esta na tela.
int  ilha_rect(float *x, float *y, float *w, float *h);
// 1 = outra superficie nasceu da pilula e esta no lugar dela (o painel de
// Salvos): a ilha continua medindo, mas nao se desenha. Vale um quadro.
void ilha_coberta(int coberta);

#endif
