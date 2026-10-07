#ifndef NV_QUEDA_H
#define NV_QUEDA_H
#include <stdio.h>
// ONDE O APP MORREU. "A sessao anterior nao se despediu" (avisos.c) diz que
// houve queda, nao onde: na 2.0.0 duas TVs webOS 5+ somaram 12 quedas e o log
// nao tinha uma linha sobre o motivo. Aqui o sinal fatal grava, num arquivo
// proprio, o sinal, o endereco da falha, pc/lr, um trecho da pilha e o mapa de
// modulos; a abertura seguinte traduz tudo para "modulo+deslocamento" e poe no
// log, que e o que o envio automatico leva.
//
// So liga onde o processo e nosso (webOS). No Android e no .tpk o processo e
// da ART / do .NET, que usam SIGSEGV por conta propria.

// Instala os tratadores. `arquivo` e o caminho completo do relato; a string e
// copiada. Chamar uma vez, cedo, no fio principal.
void queda_armar(const char *arquivo);
// Le o relato da sessao anterior, imprime as linhas "[queda] ..." e apaga o
// arquivo. Devolve 1 se havia relato.
int  queda_relatar(const char *arquivo);
// Segundo destino do MESMO relato: o tratador grava nos dois (so open/write).
// O rastro do arranque (arranque.c) rearma o tratador depois, para outro
// arquivo; a queda no meio do app_iniciar tem de continuar achavel no de /tmp.
void queda_espelho(const char *arquivo);
// O mesmo que queda_relatar, escrevendo em `saida` (ex.: open_memstream).
int  queda_relatar_em(const char *arquivo, FILE *saida);
#endif
