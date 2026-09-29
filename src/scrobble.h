// SCROBBLE DO TRAKT: "esta assistindo agora", progresso e historico.
//
// Issue #179 (LG OLED B5): "nao aparece em Now Watching, o progresso nao
// atualiza e, ao terminar, o episodio nao entra no historico; o app oficial
// faz tudo isso". O que existia era UMA chamada, ao SAIR do player
// (trakt_marcar -> /scrobble/pause ou /stop). Nunca havia /scrobble/start, e o
// Trakt so mostra "Now Watching" entre um start e o pause/stop (ou ate a
// duracao do titulo acabar). Alem disso:
//   - o log dizia "ok" para qualquer resposta que trouxesse corpo, inclusive
//     um 404/422 (id que o Trakt nao conhece): a falha ficava invisivel;
//   - um segundo pedido com o primeiro ainda no ar era DESCARTADO em silencio
//     (fioMarcaVivo), e o descartado podia ser justamente o /stop;
//   - ids que nao sao IMDb ("tmdb:m123", "kitsu:...") iam no campo imdb.
//
// Ciclo (o mesmo do app oficial):
//   passa a tocar            -> POST /scrobble/start
//   pausa / avanca (seek)    -> POST /scrobble/pause
//   volta a tocar            -> POST /scrobble/start
//   sai do player            -> /scrobble/stop se progress >= 80 (o Trakt marca
//                               como assistido a partir de 80), senao pause
//
// Sem ajuste proprio: se o Trakt esta vinculado, escrobla. Nao ha chave para
// isso no app oficial e o dono ja escolheu vincular.
//
// NUNCA bloqueia o player: as chamadas vao para uma fila e um fio unico as
// envia NA ORDEM (start antes de stop), com o token lido a cada chamada.
#ifndef NUVIO_SCROBBLE_H
#define NUVIO_SCROBBLE_H
#include <stddef.h>

// Chamada uma vez por quadro enquanto ha video. `id` e o mesmo do trakt_marcar
// ("tt123" ou "tt123:temporada:episodio"). `ms` e um relogio monotonico. So
// emite quando o estado (tocando/pausado) fica estavel por ~1,2 s, para que
// uma rajada de avancos nao vire uma rajada de pedidos.
void scrobble_passo(const char *id, double posSeg, double durSeg,
                    int tocando, int pronto, unsigned ms);

// Saida do player (ou fim do episodio): stop se >= 80%, senao pause.
void scrobble_sair(const char *id, double posSeg, double durSeg);

// Esquece o estado do titulo (novo titulo abrindo).
void scrobble_zerar(void);

// Corpo JSON da chamada, ou 0 quando o id nao serve ao Trakt.
int scrobble_corpo(const char *id, double pct, char *dst, size_t n);

// Espera a fila esvaziar. So os testes usam.
void scrobble_drenar(void);

#endif
