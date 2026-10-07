// CONFERIR VARIAS FONTES AO MESMO TEMPO (2.0.2, desligado de fabrica).
//
// A verificacao do automatico e em SERIE de proposito (issue #130, fonteauto.h):
// conferir uma fonte de debrid faz o servico adicionar o arquivo ao painel, e
// uma por vez e o que garante que so a fonte que vai tocar vira arquivo. Quem
// liga "Conferir varias fontes ao mesmo tempo" troca isso por velocidade: as
// primeiras `k` da fila sao conferidas JUNTAS, e a escolhida e a primeira que
// serve NA ORDEM DA FILA — a resposta mais rapida de uma fonte pior nunca passa
// na frente de uma melhor que ainda esta conferindo. Isto so muda quanto se
// espera, e nunca a qualidade escolhida.
//
// Sem rede e sem a lista de streams: quem confere e o callback (o mesmo
// FonteVerificar de fonteauto.h, que aqui roda em varios fios e tem de ser seguro
// para isso).
#ifndef NV_FONTEPARALELA_H
#define NV_FONTEPARALELA_H
#include "fonteauto.h"

#define FONTEPARALELA_MAX 4

// Confere fila[0..k-1] em paralelo (k limitado a n e a FONTEPARALELA_MAX) e
// devolve o primeiro, na ordem da fila, que serve — assim que ele e conhecido,
// sem esperar as que vem depois dele —, ou -1 se nenhuma das k serve.
// falhou(), se nao NULL, e chamada (num fio da conferencia) para cada uma que
// nao serviu. Fios que ainda conferem quando a funcao volta terminam sozinhos.
// *tocadas (se nao NULL) recebe quantas conferencias comecaram.
// `prazoMs` > 0 trata o que nao respondeu ate la como "nao serviu".
int fonteparalela(const int *fila, int n, int k, FonteVerificar verificar,
                  FonteFalhou falhou, void *u, int *tocadas, unsigned prazoMs);

#endif
