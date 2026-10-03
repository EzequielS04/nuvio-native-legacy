// TAMANHO DA INTERFACE: A TELA VIRTUAL DAS CAMADAS AMPLIADAS (gfx.h).
//
// Uma camada ampliada (Ajustes, player, folhas, ilhas, menus, modais) faz o
// layout numa tela de NV_VTELA_W x NV_VTELA_H — 1920/s x 1080/s — e desenha
// entre ESCALA_INI e ESCALA_FIM: o gfx multiplica tudo por s e a tela virtual
// cobre a real de borda a borda. Quem ancora na borda direita ou de baixo usa a
// tela VIRTUAL, e o layout REFLUI (menos linhas, folha mais estreita) em vez de
// vazar. Com s = 1 os dois tamanhos sao os de sempre, bit a bit.
//
// REGRA: o modulo que mede pela tela virtual e o mesmo que liga a escala no
// proprio desenho publico. Assim o layout e o desenho nunca discordam, venha a
// chamada do app, de outra tela ou de um teste de captura.
//
// Um arquivo inteiro na tela virtual define NV_ESCALA_TELA antes do include:
// NV_TELA_W/H passam a ser os virtuais ali dentro.
#ifndef NV_ESCALA_H
#define NV_ESCALA_H
#include "layout.h"
#include "gfx.h"

#define NV_VTELA_W (1920.0f / gfx_escala_ui())
#define NV_VTELA_H (1080.0f / gfx_escala_ui())

// Liga a escala configurada ate o fim do bloco. Aninhar e seguro.
#define ESCALA_INI() float escalaAnt_ = gfx_escala_entrar()
#define ESCALA_FIM() gfx_escala_sair(escalaAnt_)
// O contrario: desenha em 1080p mesmo chamado de dentro de uma camada ampliada
// (o teclado, que ja ocupa a largura da tela em 100%).
#define ESCALA_REAL_INI() float escalaAnt_ = gfx_escala(); gfx_escala_sair(1.0f)
#define ESCALA_REAL_FIM() gfx_escala_sair(escalaAnt_)

#endif

#ifdef NV_ESCALA_TELA
#undef NV_TELA_W
#undef NV_TELA_H
#define NV_TELA_W NV_VTELA_W
#define NV_TELA_H NV_VTELA_H
#endif
