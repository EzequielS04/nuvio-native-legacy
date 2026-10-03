// MEDIDOR DE DESEMPENHO NA TELA (mockup do registro, quadros 15 e 16, aprovado
// em 03/10). Os numeros sao os da linha FPS= que main.c ja escreve a cada 3 s:
// quadros por segundo, o pior quadro da janela, janks, texturas na tela, a
// fila e os despejos de textura, a memoria do app e o cache de imagens. Liga
// em Ajustes > Desempenho desta TV > "Medidor de desempenho" (desligado de
// fabrica). Custa uma amostra a cada 3 s, nao por quadro.
//
// Duas formas, no canto oposto ao relogio: a ILHA aberta (numero grande,
// grafico do pior quadro, memoria) nas telas do app, e a PILULA fechada do
// tamanho da ilha do relogio durante o video, para nao cobrir o filme. Abaixo
// de 45 fps (o mesmo corte de [gpu-modos] lento) o ponto e o numero ficam
// ambar — e a unica cor.
#ifndef NV_DESEMPENHO_H
#define NV_DESEMPENHO_H
#include <SDL2/SDL.h>

void desempenho_amostra(float fps, float piorMs, int janks, int texTela, float texTelaMb,
                        int filaTex, int despejos, int despejosTela, float rssMb);
// `pilula` = 1 a forma fechada (app.c passa 1 com o player aberto).
void desempenho_desenhar(Uint32 agora, int pilula);

#ifdef DESEMPENHO_TESTE
void desempenho_teste_serie(const float *pior, int n);
#endif
#endif
