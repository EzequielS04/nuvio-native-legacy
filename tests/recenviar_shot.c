// Visual fixtures only. Renders the real modal without starting Social/network.
#include "../src/recenviar.c"
#include "vidro_fundo.h"
#include "shot_arte.h"
#include <SDL2/SDL_image.h>
#include <assert.h>

extern void ajustes_teste_vidro_env(void);
static void captura(const char *nome, SDL_Window *win) {
  int q, y;
  for (q = 0; q < 70; q++) {
    SDL_PumpEvents(); txt_novo_quadro(); tex_novo_quadro();
    tex_bombear(6); gfx_novo_quadro();
    glClearColor(.03f,.03f,.035f,1); glClear(GL_COLOR_BUFFER_BIT);
    if (vidroFundoAtivo()) vidroFundoDesenhar(); else shot_arte_desenhar(0);
    recenviar_desenhar(SDL_GetTicks());
    if (q == 69) {
      unsigned char *pix = malloc(1920*1080*4);
      SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0,1920,1080,32,SDL_PIXELFORMAT_RGBA32);
      assert(pix && s); glReadPixels(0,0,1920,1080,GL_RGBA,GL_UNSIGNED_BYTE,pix);
      for(y=0;y<1080;y++) memcpy((char*)s->pixels+y*s->pitch,pix+(1079-y)*1920*4,1920*4);
      assert(IMG_SavePNG(s,nome)==0); SDL_FreeSurface(s); free(pix);
    }
    SDL_GL_SwapWindow(win);
  }
  printf("capture: %s\n",nome);
}
int main(int argc,char **argv) {
  const char *dir=getenv("NUVIO_DADOS"), *out=argc>1?argv[1]:"/tmp/nuvio-send";
  char path[700]; FILE *f; SDL_Window *win; SDL_GLContext gl;
  assert(dir && *dir);
  snprintf(path,sizeof path,"%s/ajustes.txt",dir); f=fopen(path,"w"); assert(f);
  fprintf(f,"idioma 0\nselected_theme 2\nvidroLocal %d\n",getenv("NUVIO_SHOT_SOLIDO")?1:0); fclose(f);
  ajustes_dir(dir); ajustes_teste_vidro_env();
  assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0); IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
  win=SDL_CreateWindow("Nuvio: Send modal",0,0,1920,1080,SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);assert(win);
  gl=SDL_GL_CreateContext(win);assert(gl);SDL_GL_SetSwapInterval(0);
  glViewport(0,0,1920,1080);gfx_tamanho_alvo(1920,1080);
  assert(gfx_iniciar() && txt_iniciar("deploy/app",1));tex_iniciar(64);gfx_icones_dir("deploy/app/art");
  vidroFundoPreparar();
  aberto=1;anim=1;pagina=RE_PAG_CONTATOS;temItem=1;nCtts=3;foco=1;focoAnim[1]=1;
  snprintf(item.titulo,sizeof item.titulo,"Fallout");snprintf(item.meta,sizeof item.meta,"2024 · 56 min");
  snprintf(item.logo,sizeof item.logo,"deploy/app/art/logo/00.png");
  snprintf(ctts[0].nome,sizeof ctts[0].nome,"Ana");snprintf(ctts[0].id,sizeof ctts[0].id,"fixture-ana");
  snprintf(ctts[1].nome,sizeof ctts[1].nome,"Pedro");snprintf(ctts[1].id,sizeof ctts[1].id,"fixture-pedro");
  snprintf(ctts[2].nome,sizeof ctts[2].nome,"Kevin");snprintf(ctts[2].id,sizeof ctts[2].id,"fixture-kevin");
  snprintf(path,sizeof path,"%s-contacts.png",out);captura(path,win);
  pagina=RE_PAG_MODELOS;foco=0;memset(focoAnim,0,sizeof focoAnim);focoAnim[0]=1;
  snprintf(alvoNome,sizeof alvoNome,"Pedro");
  snprintf(path,sizeof path,"%s-message.png",out);captura(path,win);
  // Five visible friends + code page exceed the old hardcoded700px estimate.
  pagina=RE_PAG_AMIGOS;nCtts=5;foco=0;topo=0;temItem=0;
  assert(alturaCartao(NULL,NULL) * 1.3f > 1048.0f);
  gfx_escala_ui_definir(1.3f);
  snprintf(path,sizeof path,"%s-friends-zoom130.png",out);captura(path,win);
  assert(gfx_escala()==1.0f);
  tex_encerrar();txt_encerrar();gfx_encerrar();SDL_GL_DeleteContext(gl);SDL_DestroyWindow(win);SDL_Quit();
  return 0;
}
