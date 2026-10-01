// Rotulos das faixas a partir do cabecalho do MKV, para o player que so sabe o
// idioma (#206, .tpk da Samsung).
//
// O Tizen.Multimedia.Player (tizen-tpk/Video.cs) entrega por faixa so o
// GetLanguageCode: nem o Name da TrackEntry, nem a FlagForced, nem os canais.
// O .wgt mostra "forced" porque video_tizen.c le o MKV e poe o Name no rotulo;
// o .tpk nao lia. Este modulo e a regra, sem rede e sem estado, para o teste do
// Mac poder exercitar o que a TV desenha.
//
// CASAMENTO POR ORDINAL DENTRO DO TIPO, com a mesma guarda do video_tizen.c:
// so aplica num tipo quando a QUANTIDADE bate entre o player e o arquivo.
// Contagem diferente quer dizer que um dos lados filtrou faixa e o ordinal ja
// nao alinha — e rotulo errado e pior que "Audio 1".
#ifndef NV_FAIXASMKV_H
#define NV_FAIXASMKV_H

#include "video.h"
#include "mkv.h"

// Reescreve idioma/rotulo/letreiro de `aud` e `leg` com as TrackEntry de `fx`.
// Nao mexe em `numero` nem em `ordinalMkv` (quem escolhe a faixa e o player).
// Devolve quantas faixas mudaram de rotulo.
int faixasmkv_aplicar(VideoFaixa *aud, int nAud, VideoFaixa *leg, int nLeg,
                      const MkvFaixa *fx, int n);

// "2.0", "5.1", "7.1" ou "" (mesma tabela do sourceInfo da LG, video.c).
const char *faixasmkv_canais(int canais);

#endif
