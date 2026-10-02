// O QUADRO DO VIDEO NA SAIDA PARA A ILHA (so Android).
//
// No Android o video e uma SurfaceView atras da superficie do app, e o
// PixelCopy le o ultimo quadro dela para um Bitmap. E ESSE quadro (o que a
// pessoa estava vendo, ja pausado) que encolhe ate a pilula, no lugar do still
// do episodio: a primeira imagem do voo e identica a ultima do player. Nas
// outras plataformas o plano de video e hardware e nao se le de volta (LG,
// Samsung): os stubs abaixo dizem "nao ha", e o voo usa o still como antes.
//
// Ciclo: video_quadro_pedir() junto da pausa; o Kotlin devolve os pixels por
// JNI (qualquer fio); o fio de desenho le com video_quadro_pixels() e solta com
// video_quadro_soltar(). Um pedido novo ou o soltar descartam o anterior.
#ifndef NV_VIDEO_QUADRO_H
#define NV_VIDEO_QUADRO_H

enum { VQ_NADA = -2, VQ_FALHOU = -1, VQ_ESPERANDO = 0, VQ_PRONTO = 1 };

#ifdef NV_ANDROID
void video_quadro_pedir(void);
int  video_quadro_estado(void);
// RGBA 8 bits; (x,y,w,h) = onde o quadro estava na tela, em 1920x1080.
const unsigned char *video_quadro_pixels(int *pw, int *ph, int *x, int *y, int *w, int *h);
void video_quadro_soltar(void);
#else
static inline void video_quadro_pedir(void) {}
static inline int  video_quadro_estado(void) { return VQ_NADA; }
static inline const unsigned char *video_quadro_pixels(int *pw, int *ph, int *x, int *y, int *w, int *h) {
  (void)pw; (void)ph; (void)x; (void)y; (void)w; (void)h; return 0;
}
static inline void video_quadro_soltar(void) {}
#endif
#endif
