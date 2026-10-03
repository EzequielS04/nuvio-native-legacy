// Abertura do app (#213). Ver abertura.h.
#include "abertura.h"
#include "gfx.h"
#include "anim.h"
#include "layout.h"
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// O fundo do splash.png (#0E0F12, lido do arquivo) e o lugar da marca nele:
// 780x247 a partir de (570,416), o centro em (960,540).
#define AB_FUNDO_R (14.0f / 255.0f)
#define AB_FUNDO_G (15.0f / 255.0f)
#define AB_FUNDO_B (18.0f / 255.0f)
#define AB_MARCA_W 780.0f
#define AB_CENTRO_Y 540.0f
// Parada minima com a marca inteira: le como "abriu", nao como piscada.
#define ABERTURA_MIN_MS 350u
// Nunca segura a home alem disto, com arte chegando ou nao.
#define ABERTURA_TETO_MS 1500u
// Duracao da saida.
#define ABERTURA_SAIDA_MS 560.0f

static GLuint marca;
static float marcaAsp;
// ARTE CHEIA (splash 1.7.2): a mesma imagem do splash.png da LG e do fundo da
// janela no Android, em 1280x720 (decodifica em ~1/2 do tempo da 1920 e some em
// menos de 1,5 s, entao a ampliacao nao chega a ser vista). Com ela, a marca e
// o fundo liso ficam de fora: a arte ja tem os dois.
static int arteCheia;
static int fundoFica;

void abertura_fundo_fica(int sim) { fundoFica = sim; }
static int estado;          // 0 nao iniciou, 1 parada, 2 saindo, 3 acabou
static Uint32 inicio, saidaEm;
static float escala = 1.0f, escalaVel;

void abertura_iniciar(const char *dirArte) {
  char cam[600];
  SDL_Surface *s, *c;
  estado = 0;
  arteCheia = 0;
  snprintf(cam, sizeof cam, "%s/marcas/abertura.jpg", dirArte ? dirArte : ".");
  s = IMG_Load(cam);
  if (s) arteCheia = 1;
  else {
    snprintf(cam, sizeof cam, "%s/marcas/nuvio_wordmark.png", dirArte ? dirArte : ".");
    s = IMG_Load(cam);
  }
  if (!s) { printf("[abertura] sem a marca (%s)\n", cam); return; }
  c = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ABGR8888, 0);   // RGBA em bytes
  SDL_FreeSurface(s);
  if (!c) return;
  glGenTextures(1, &marca);
  glBindTexture(GL_TEXTURE_2D, marca);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, c->w, c->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, c->pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gfx_tex_esquecer(0);   // o gfx guarda a ultima textura ligada
  marcaAsp = c->h > 0 ? (float)c->w / (float)c->h : 3.15f;
  SDL_FreeSurface(c);
}

void abertura_tecla(void) {
  if (estado == 1) { estado = 2; saidaEm = SDL_GetTicks(); }
}

int abertura_ativa(void) { return estado < 3; }

static void acabar(void) {
  estado = 3;
  if (marca) { gfx_tex_esquecer(marca); glDeleteTextures(1, &marca); marca = 0; }
  printf("[abertura] fim em %u ms\n", (unsigned)(SDL_GetTicks() - inicio));
  fflush(stdout);
}

int abertura_desenhar(Uint32 agora, float dt, int pendentes) {
  float veu = 1.0f, alfaMarca = 1.0f, alvo = 1.018f;
  if (estado == 3) return 0;
  if (estado == 0) { estado = 1; inicio = agora; escala = 1.0f; escalaVel = 0.0f; }
  if (estado == 1) {
    Uint32 passou = agora - inicio;
    // NUVIO_ABERTURA_MS segura a parada (captura de tela no Mac/TV).
    static long segura = -1;
    if (segura < 0) { const char *v = getenv("NUVIO_ABERTURA_MS"); segura = v ? atol(v) : 0; }
    if (segura > 0 && passou < (Uint32)segura) pendentes = 1, passou = 0;
    // Sai quando as artes do primeiro quadro chegaram (nada em voo) ou no teto.
    if ((passou >= ABERTURA_MIN_MS && pendentes <= 0) || passou >= ABERTURA_TETO_MS) {
      estado = 2; saidaEm = agora;
    }
  }
  if (estado == 2) {
    float p = (float)(agora - saidaEm) / ABERTURA_SAIDA_MS;
    float q;
    if (p >= 1.0f) { acabar(); return 0; }
    // O veu sai com cubica de saida; a marca, mais depressa, e cresce um pouco
    // por mola (a mesma de segunda ordem do resto do app) — abre para a home.
    q = 1.0f - p;
    veu = q * q * q;
    { float pm = p * 1.6f; if (pm > 1.0f) pm = 1.0f;
      alfaMarca = (1.0f - pm) * (1.0f - pm); }
    alvo = 1.09f;
  }
  // Reduzidas: so o esvanecimento, sem crescer.
  escala = anim_politica_reduzida ? 1.0f : anim_mola2(&escalaVel, escala, alvo, dt, 9.0f);
  if (marca && arteCheia) {
    // A arte inteira cresce a partir do centro e sai junto com o veu. Sobre o
    // login ela fica parada: o fundo de baixo continua as listras, e quem
    // some e o logo.
    float k = fundoFica ? 1.0f : escala;
    float w = NV_TELA_W * k, h = NV_TELA_H * k;
    gfx_tex_aspect_atual = 0.0f;
    gfx_rect((GfxRect){ (NV_TELA_W - w) * 0.5f, (NV_TELA_H - h) * 0.5f, w, h },
             marca, GFX_TEXTO, 0, 0, 0, 0.0f, 1, 1, 1, veu);
    return 1;
  }
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f,
          AB_FUNDO_R, AB_FUNDO_G, AB_FUNDO_B, veu);
  if (marca && alfaMarca > 0.004f) {
    float w = AB_MARCA_W * escala, h = w / marcaAsp;
    gfx_tex_aspect_atual = 0.0f;
    gfx_rect((GfxRect){ NV_TELA_W * 0.5f - w * 0.5f, AB_CENTRO_Y - h * 0.5f, w, h },
             marca, GFX_TEXTO, 0, 0, 0, 0.0f, 1, 1, 1, alfaMarca);
  }
  return 1;
}
