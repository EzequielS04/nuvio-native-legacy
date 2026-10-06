// CAPTURAS DA CENTRAL DE CONTROLE (central.h), sem rede e sem TV: os atalhos de
// fabrica com foco num interruptor, o vidro com foco em "Editar" e a lista de
// edicao. NUVIO_DADOS e temporario (ver o .sh). Saida: <prefixo>-N-*.bmp.
#include "central.h"
#include "ajustes.h"
#include "ajustes_ux.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "anim.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint fbo, fboTex;

static void captura(const char *nome) {
  int i;
  for (i = 0; i < 30; i++) {
    SDL_PumpEvents();
    txt_novo_quadro();
    tex_novo_quadro();
    tex_bombear(6);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1920, 1080);
    glClearColor(0.025f, 0.025f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ajustes_ui_fundo();
    central_atualizar(1.0f / 60.0f, SDL_GetTicks());
    central_desenhar(SDL_GetTicks());
    SDL_Delay(8);
  }
  {
    unsigned char *pix = malloc(1920 * 1080 * 4);
    SDL_Surface *s;
    int y;
    assert(pix);
    glFinish();
    glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
    s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
    assert(s);
    for (y = 0; y < 1080; y++)
      memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
    assert(SDL_SaveBMP(s, nome) == 0);
    SDL_FreeSurface(s);
    free(pix);
  }
  printf("captura: %s\n", nome);
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nuvio-central";
  const char *dir = getenv("NUVIO_DADOS");
  char nome[600], arq[700];
  SDL_Window *w;
  SDL_GLContext gl;
  FILE *f;
  assert(dir && *dir);
  snprintf(arq, sizeof arq, "%s/ajustes.txt", dir);
  f = fopen(arq, "w"); assert(f);
  fprintf(f, "idioma 0\nselected_theme 2\ndolbyVision 0\n");
  fclose(f);
  ajustes_dir(dir);
  anim_politica_reduzida = 1;

  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  w = SDL_CreateWindow("Nuvio: central", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(w);
  gl = SDL_GL_CreateContext(w);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glGenTextures(1, &fboTex);
  glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  ajustes_recursos("deploy/app/art");
  gfx_icones_dir("deploy/app/art");
  ajustes_ui_arte(21);
  tex_iniciar(64);

  central_teste_foco(1, 0);
  snprintf(nome, sizeof nome, "%s-1-atalhos.bmp", saida);
  captura(nome);

  ajustes_definir_vidro(1);
  central_teste_foco(-1, 0);
  snprintf(nome, sizeof nome, "%s-2-vidro-editar.bmp", saida);
  captura(nome);

  central_teste_foco(-1, 1);
  snprintf(nome, sizeof nome, "%s-3-edicao.bmp", saida);
  captura(nome);

  tex_encerrar(); txt_encerrar(); gfx_encerrar();
  SDL_GL_DeleteContext(gl); SDL_DestroyWindow(w); SDL_Quit();
  puts("PASS: capturas da central");
  return 0;
}
