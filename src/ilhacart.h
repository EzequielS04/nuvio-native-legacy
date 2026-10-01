// QUEM POE E TIRA OS CARTOES DA ILHA (ilha.h): a atividade ao vivo e a estreia.
//
// ATIVIDADE AO VIVO. O player, ao fechar, entrega a sessao (ilhacart_player_saiu).
// Ela vira cartao pelo MESMO criterio da faixa "Retomar agora" (entre 1% e o
// Percentual assistido, home_retorno_vale) e fica enquanto a faixa ficaria: a
// proxima saida do player a troca ou a tira (retomou e terminou, ou largou
// outro titulo no meio). Alem disso ela SAI SOZINHA depois de 30 min sem
// nenhuma tecla (VIVO_OCIOSO_MS): a TV esquecida ligada na home nao fica a
// noite inteira oferecendo um episodio de horas atras, e quem volta ainda tem
// a faixa da home. "Fechar" no modal tira na hora. Nada e gravado: fechar o
// app encerra.
//
// ESTREIA. O episodio novo de serie com lembrete (AV_AGENDA em avisos.c) ainda
// nao lido: o mais recente fica na pilula ate ser lido — a aba Avisos aberta,
// "Marcar como visto" no modal, ou a pagina do titulo aberta (com o relogio
// ligado; desligado nada aqui marca coisa alguma).
//
// FIO PRINCIPAL.
#ifndef NV_ILHACART_H
#define NV_ILHACART_H
#include <SDL2/SDL.h>

void ilhacart_player_saiu(int indice, double posSeg, double durSeg, int t, int e);
// Toda tecla ou clique (app_evento): o relogio da ociosidade recomeca.
void ilhacart_tecla(Uint32 agora);
// Por quadro. `imdbAberto` = o titulo com a pagina aberta agora, ou NULL.
void ilhacart_atualizar(Uint32 agora, const char *imdbAberto);
// O modal pediu para tirar o cartao (Fechar / Marcar como visto).
void ilhacart_dispensar(int qual);

#endif
