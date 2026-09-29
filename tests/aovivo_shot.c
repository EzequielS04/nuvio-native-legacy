// Capturas do modo AO VIVO: o OSD proprio do canal (logo, numero, categoria, AO
// VIVO, agora/a seguir com barra, botoes), o banner do zapping, o cartao de erro
// e o player de verdade com um canal marcado (fiacao das teclas).
//
//   bash tests/aovivo_shot.sh /tmp/nv-player-live-shots/aovivo
#include "catalogo.h"
#include "player.h"
#include "aovivo.h"
#include "ajustes.h"
#include "faixas.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "episodios.h"
#include "streams.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint fbo, fboTex;
static int LW = 1920, LH = 1080;
static const char *LOGO = "tests/fixtures/aovivo/logo.png";

static void salvar(const char *nome) {
  SDL_Surface *s = SDL_CreateRGBSurface(0, LW, LH, 24, 0xff, 0xff00, 0xff0000, 0);
  int y; unsigned char *p, *t;
  glFinish(); glBindFramebuffer(GL_FRAMEBUFFER, fbo); glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, LW, LH, GL_RGB, GL_UNSIGNED_BYTE, s->pixels);
  p = s->pixels; t = malloc((size_t)s->pitch);
  for (y = 0; y < LH / 2; y++) {
    memcpy(t, p + y * s->pitch, (size_t)s->pitch);
    memcpy(p + y * s->pitch, p + (LH - 1 - y) * s->pitch, (size_t)s->pitch);
    memcpy(p + (LH - 1 - y) * s->pitch, t, (size_t)s->pitch);
  }
  free(t);
  assert(SDL_SaveBMP(s, nome) == 0);
  SDL_FreeSurface(s);
  printf("captura: %s\n", nome);
}

typedef void (*Desenho)(void *);
// Fundo de "cena" (a arte do pacote) + o desenho, por N quadros para o raster
// de texto e de textura assentar.
static void foto(const char *nome, Desenho d, void *u) {
  int i;
  for (i = 0; i < 30; i++) {
    SDL_PumpEvents(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(6);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
    glClearColor(.16f, .19f, .24f, 1); glClear(GL_COLOR_BUFFER_BIT);
    { GLuint bg = tex_obter_larg_qualquer("deploy/app/art/19.jpg", 1920);
      if (bg) gfx_rect((GfxRect){0, 0, 1920, 1080}, bg, GFX_TEXTO, 0, 0, 0, 0, 1, 1, 1, 1); }
    d(u);
    SDL_Delay(2);
  }
  salvar(nome);
}

static AoVivoOsd base(void) {
  AoVivoOsd o; time_t t = time(NULL);
  memset(&o, 0, sizeof o);
  o.nome = "Sportv 2 HD"; o.categoria = "Esportes"; o.logo = LOGO;
  o.desc = "Transmissao ao vivo dos principais eventos esportivos do dia, com comentarios e reportagens.";
  o.numero = 12; o.total = 768;
  o.epg.temAgora = 1; snprintf(o.epg.agoraTit, sizeof o.epg.agoraTit, "Campeonato Brasileiro: Rodada 24");
  o.epg.agoraIni = t - 47 * 60; o.epg.agoraFim = t + 73 * 60; o.epg.progresso = 47.0f / 120.0f;
  o.epg.temProx = 1; snprintf(o.epg.proxTit, sizeof o.epg.proxTit, "Bate-bola: Os melhores momentos");
  o.epg.proxIni = o.epg.agoraFim;
  { int b[] = { AV_B_GUIA, AV_B_ANT, AV_B_PROX, AV_B_FAV, AV_B_AUDIO, AV_B_LEGENDA, AV_B_INFO, AV_B_RECARREGAR, AV_B_FONTE };
    o.nBotoes = 9; memcpy(o.botoes, b, sizeof b); }
  o.foco = 0; snprintf(o.res, sizeof o.res, "HD");
  o.fr = .24f; o.fg = .52f; o.fb = .95f;
  return o;
}
static void dOsd(void *u) { aovivo_osd_desenhar((AoVivoOsd *)u, 1.0f); }
static void dBanner(void *u) { aovivo_banner_desenhar((AoVivoBanner *)u, 1.0f); }
typedef struct { const char *n, *l, *t, *d; } Erro;
static void dErro(void *u) { Erro *e = u; aovivo_erro_desenhar(e->n, e->l, e->t, e->d, 1.0f); aovivo_osd_desenhar(&(AoVivoOsd){0}, 1.0f); }
static void dPlayer(void *u) { (void)u; player_atualizar(1.f / 60, SDL_GetTicks()); player_desenhar(SDL_GetTicks()); }

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nv-player-live-shots/aovivo";
  char nome[600];
  const char *e4 = getenv("NUVIO_SHOT_4K");
  if (e4 && *e4 == '1') { LW = 3840; LH = 2160; }
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *w = SDL_CreateWindow("Nuvio: ao vivo", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  SDL_GLContext gl = SDL_GL_CreateContext(w); assert(gl); SDL_GL_SetSwapInterval(0);
  glGenTextures(1, &fboTex); glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  ajustes_iniciar();
  if (getenv("NUVIO_SHOT_VIDRO")) ajustes_definir_vidro(1);

  { AoVivoOsd o = base();
    snprintf(nome, sizeof nome, "%s-osd.bmp", saida); foto(nome, dOsd, &o);
    o.foco = 4; o.favorito = 1;   // foco no Favorito, ja favoritado
    snprintf(nome, sizeof nome, "%s-osd-foco-favorito.bmp", saida); foto(nome, dOsd, &o);
    o.foco = 6; o.infoAberta = 1; o.nInfo = 5; o.bufferando = 1;
    snprintf(o.info[0], sizeof o.info[0], "Resolução: 1920x1080");
    snprintf(o.info[1], sizeof o.info[1], "Imagem: HDR10");
    snprintf(o.info[2], sizeof o.info[2], "Áudio: Dolby Atmos");
    snprintf(o.info[3], sizeof o.info[3], "Buffer: carregando há 4 s");
    snprintf(o.info[4], sizeof o.info[4], "Codec, quadros e taxa: a TV não informa");
    snprintf(nome, sizeof nome, "%s-osd-info-buffer.bmp", saida); foto(nome, dOsd, &o);
    // sem logo, sem grade e com o botao de pausa (fluxo com janela de tempo)
    { AoVivoOsd v = base();
      int b[] = { AV_B_PAUSA, AV_B_GUIA, AV_B_ANT, AV_B_PROX, AV_B_AUDIO, AV_B_LEGENDA, AV_B_INFO, AV_B_RECARREGAR, AV_B_FONTE };
      v.logo = ""; v.numero = 0; v.epg.temAgora = v.epg.temProx = 0; v.pausado = 1; v.res[0] = 0;
      v.nBotoes = 9; memcpy(v.botoes, b, sizeof b);
      snprintf(nome, sizeof nome, "%s-osd-sem-logo-sem-grade.bmp", saida); foto(nome, dOsd, &v); } }
  { AoVivoBanner b = { "Globo News", LOGO, "Em Foco com Andréia Sadi", 13, 1 };
    snprintf(nome, sizeof nome, "%s-zap-banner.bmp", saida); foto(nome, dBanner, &b);
    b.salto = 3; b.numero = 15; b.nome = "SporTV 3 com um nome muito longo para testar o corte"; b.logo = "";
    snprintf(nome, sizeof nome, "%s-zap-banner-salto.bmp", saida); foto(nome, dBanner, &b); }
  { Erro e = { "Sportv 2 HD", LOGO, "O provedor recusou o usuario e a senha",
               "Confira o cadastro do Xtream em Ajustes > Conta de TV ao vivo." };
    snprintf(nome, sizeof nome, "%s-erro.bmp", saida); foto(nome, dErro, &e); }

  // O PLAYER DE VERDADE com um canal marcado: prova a fiacao (OSD proprio no
  // lugar dos controles de filme; OK/direita percorrem os botoes).
  { CatItem c; memset(&c, 0, sizeof c);
    snprintf(c.tipo, sizeof c.tipo, "channel"); snprintf(c.imdb, sizeof c.imdb, "cs:channel:teste");
    snprintf(c.titulo, sizeof c.titulo, "Canal Teste HD");
    snprintf(c.poster, sizeof c.poster, "%s", LOGO);
    snprintf(c.genero, sizeof c.genero, "Canal · Filmes");
    snprintf(c.sinopse, sizeof c.sinopse, "Filmes o dia inteiro.");
    cat_definir(&c, 1);
    player_abrir(0, NULL); player_marcar_canal(&c);
    player_erro_fonte(); player_limpar_erro_fonte();
    { SDL_Event ev = {0}; ev.type = SDL_KEYDOWN;
      int i; for (i = 0; i < 40; i++) { player_atualizar(1.f / 60, SDL_GetTicks()); }
      ev.key.keysym.sym = SDLK_RIGHT; player_evento(&ev); player_evento(&ev); }
    snprintf(nome, sizeof nome, "%s-player.bmp", saida); foto(nome, dPlayer, NULL);
    // CH+ duas vezes: o banner sobe e o OSD sai (o alvo so existe com o guia
    // carregado; sem ele o banner nao tem para quem apontar).
    { SDL_Event ev = {0}; ev.type = SDL_KEYDOWN; ev.key.keysym.scancode = 480;
      player_evento(&ev); player_evento(&ev);
      assert(player_pediu_zap() == 0); }
    SDL_Delay(700);
    player_atualizar(1.f / 60, SDL_GetTicks());
    assert(player_pediu_zap() == 2);
    player_erro_fonte_motivo("Assinatura Xtream vencida", "Renove com o provedor ou troque a lista em Ajustes.");
    snprintf(nome, sizeof nome, "%s-player-erro.bmp", saida); foto(nome, dPlayer, NULL); }
  puts("aovivo_shot: ok");
  return 0;
}
