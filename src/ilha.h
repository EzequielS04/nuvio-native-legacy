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
// so a desenha com o player fechado), nao tem teclado (o AZUL/CH+ da central
// continua em avisos_evento) e nao e camada de tela cheia — e uma sombra
// pequena, uma pilula e o texto, mesmo durante a mola.
//
// TRES CONTEUDOS, por prioridade: aviso (fila curta, um de cada vez) >
// atividade (algo em andamento, renovado a cada quadro por quem a tem) >
// relogio (so onde app.c disse que cabe: ilha_relogio_visivel).
//
// FIO PRINCIPAL em tudo.
#ifndef NV_ILHA_H
#define NV_ILHA_H
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

#endif
