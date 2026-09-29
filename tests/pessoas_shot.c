// CAPTURA DA MODAL "ENCONTRAR PESSOAS", sem rede. recomenda.c e pessoas.c sao
// INCLUIDOS: os achados, o cartao e os pedidos moram em estaticos do modulo e
// semea-los por dentro e o unico jeito de ter as telas cheias sem servidor.
#define NV_REC_URL "http://127.0.0.1:1"
#include "../src/recomenda.c"
#include "../src/pessoas.c"
#include "dados.h"
#include <SDL2/SDL_image.h>
#include <assert.h>

static GLuint fbo, fboTex;

static void captura(const char *nome, SDL_Window *win) {
  int i;
  for (i = 0; i < 40; i++) {
    SDL_PumpEvents();
    txt_novo_quadro();
    tex_novo_quadro();
    tex_bombear(6);
    gfx_novo_quadro();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1920, 1080);
    glClearColor(0.025f, 0.025f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    pessoas_atualizar(1.0f / 30.0f, SDL_GetTicks());
    pessoas_desenhar(SDL_GetTicks());
    if (i == 39) {
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
    SDL_GL_SwapWindow(win);
  }
  printf("captura: %s\n", nome);
}

static void pessoa(RecPessoa *p, const char *pub, const char *ap, const char *bio,
                   unsigned g, const char *rel, int comum) {
  memset(p, 0, sizeof *p);
  snprintf(p->pub, sizeof p->pub, "%s", pub);
  snprintf(p->apelido, sizeof p->apelido, "%s", ap);
  snprintf(p->bio, sizeof p->bio, "%s", bio);
  snprintf(p->relacao, sizeof p->relacao, "%s", rel);
  p->generos = g; p->emComum = comum;
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nv-amigos-shots";
  const char *dir = getenv("NUVIO_DADOS");
  char nome[600];
  SDL_Window *w;
  SDL_GLContext gl;
  assert(dir && *dir);
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  dados_iniciar(dir);
  assert(!strcmp(dados_dir(), dir));
  ajustes_iniciar();
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  w = SDL_CreateWindow("Nuvio: captura pessoas", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(w);
  gl = SDL_GL_CreateContext(w);
  assert(gl);
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
  tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  if (!mtx) mtx = SDL_CreateMutex();

  /* SEMENTES: 4 achados (uma relacao de cada tipo), o cartao, 2 pedidos. */
  pessoa(&achados[0], "k9ptiwtmrb", "fabi cine", "fa de terror e ficcao", (1u << 13) | (1u << 9), "", 0);
  pessoa(&achados[1], "m3n4p5q6r7", "fabio filmes", "", 0, "enviado", 0);
  pessoa(&achados[2], "a2b3c4d5e6", "fabricio", "so drama coreano", 1u << 6, "amigo", 0);
  pessoa(&achados[3], "f7g8h9i2j3", "fabiana lima", "maratonista de series", 0, "recebido", 0);
  nAchados = 4; achadosOrigem = 1;
  pessoa(&cartaoP, "k9ptiwtmrb", "fabi cine", "fa de terror e ficcao cientifica. maratonista de fim de semana",
         (1u << 13) | (1u << 9) | (1u << 6), "", 0);
  temCartao = 1;
  snprintf(cartaoRec[0], sizeof cartaoRec[0], "Um Sonho de Liberdade");
  snprintf(cartaoRec[1], sizeof cartaoRec[1], "Origem");
  snprintf(cartaoRec[2], sizeof cartaoRec[2], "Duna: Parte Dois");
  nCartaoRec = 3;
  pessoa(&pedidosRec[0], "aaaaaaaaaa", "gui nerd", "", 0, "recebido", 0);
  pessoa(&pedidosRec[1], "bbbbbbbbbb", "helena tv", "", 0, "recebido", 0);
  nPedidos = 2;
  memset(&perfil, 0, sizeof perfil);
  snprintf(perfil.apelido, sizeof perfil.apelido, "henrique tv");
  snprintf(perfil.bio, sizeof perfil.bio, "cinema, series e muita tela grande");
  perfil.publicado = 1; perfil.foto = 0; perfil.recentes = 1; perfil.ativ = 1;
  perfil.generos = (1u << 10) | (1u << 6);

  pessoas_abrir();
  snprintf(nome, sizeof nome, "%s/pessoas-menu.bmp", saida); captura(nome, w);

  irPara(PG_LISTA);
  snprintf(nome, sizeof nome, "%s/pessoas-resultados.bmp", saida); captura(nome, w);
  foco = 1;
  snprintf(nome, sizeof nome, "%s/pessoas-resultados-foco.bmp", saida); captura(nome, w);

  irPara(PG_CARTAO);
  snprintf(nome, sizeof nome, "%s/pessoas-cartao.bmp", saida); captura(nome, w);

  irPara(PG_PEDIDOS);
  snprintf(nome, sizeof nome, "%s/pessoas-pedidos.bmp", saida); captura(nome, w);

  rasc = perfil; rascVivo = 1;
  irPara(PG_PERFIL);
  snprintf(nome, sizeof nome, "%s/pessoas-meu-perfil.bmp", saida); captura(nome, w);
  foco = 6;
  snprintf(nome, sizeof nome, "%s/pessoas-meu-perfil-ativ.bmp", saida); captura(nome, w);
  printf("pronto\n");
  return 0;
}
