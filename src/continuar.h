#ifndef NV_CONTINUAR_H
#define NV_CONTINUAR_H
#include "catalogo.h"
#include "gfx.h"

// Conteudo sobre a capa; a home continua responsavel por imagem e foco.
// `raio`: o MESMO raio da arte do cartao (fracao da altura, raioDe em home.c).
void continuar_desenhar(const CatItem *item, GfxRect card, float raio);
#endif
