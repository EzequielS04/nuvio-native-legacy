// CENTRAL DE CONTROLE (dono, 06/10: "control centre tipo da Apple TV quando
// segura o CH+"). Uma ilha na borda direita, por cima de qualquer tela, com:
//  - o que o app ja sabe e e util ver sem procurar: hora, perfil, internet,
//    memoria de imagens, versao e plataforma, e o video que esta tocando
//    (resolucao, HDR, Atmos — so o que o pipeline disse, nunca palpite);
//  - botoes de atalho para opcoes de Ajustes: OK passa ao proximo valor ali
//    mesmo, gravado como na tela de Ajustes (ajustes_rapido_*);
//  - "Editar atalhos": a lista do catalogo (centrallista.h), OK poe ou tira.
//    Guardado por perfil nesta TV (central-p<N>.txt).
//
// A TECLA. Segurar CH+ abre; o toque curto continua com o dono de antes
// (Salvos, ilha, zap), so que entregue quando a tecla sobe (chsegura.h). Com
// canal na tela (zap, guia) o CH+ nao passa por aqui. Com a central aberta,
// Voltar ou CH+ fecham.
#ifndef NV_CENTRAL_H
#define NV_CENTRAL_H
#include <SDL2/SDL.h>

void central_abrir(void);
void central_fechar(void);
int  central_aberta(void);
// Aberta: come o teclado todo.
void central_evento(const SDL_Event *e);
void central_atualizar(float dt, Uint32 agora);
void central_desenhar(Uint32 agora);

// A TECLA (main.c). `ehCh` = este evento e o CH+ cru (antes do remapeamento
// para Salvos). Devolve 1 quando o evento foi engolido: o toque curto volta
// depois, por central_tecla_quadro, que chama `entregar` com o KEYDOWN e o
// KEYUP originais. `pode` = 0 quando a central nao pode abrir agora (login,
// zap): o CH+ segue direto, sem atraso.
int  central_tecla(const SDL_Event *e, int pode);
void central_tecla_quadro(Uint32 agora, void (*entregar)(SDL_Event *e));
// 1 enquanto um gesto de CH+ esta em curso (tecla embaixo ou esperando).
int  central_tecla_ocupada(void);

#ifdef CENTRAL_TESTE
// Capturas: foco num botao (-1 = "Editar atalhos") e modo edicao.
void central_teste_foco(int foco, int editar);
#endif

#endif
