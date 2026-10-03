// CAPTURAS DO PLAYER NO GLASS UI, quadro a quadro, para comparar lado a lado
// com o mockup aprovado em 03/10 (player-mockup.html, um PNG por quadro e
// material). Fora da suite: janela GL e olho humano.
//
//   bash tests/player_glass_shot.sh <dir> [quadro...]
//
// Cada quadro sai em <dir>/<id>-vidro.bmp ou <id>-solido.bmp, conforme
// NUVIO_SHOT_VIDRO (1 = Glass UI ligado). NUVIO_SHOT_MOCK aponta para a pasta
// do mockup (as artes img/bd, img/lg, img/ep dele entram como o "video"); sem
// ela, as artes do pacote. A hora fica em 20:19, como no mockup.
//
// O que e de rede (Seekr, intro, guia parental) entra por gancho de captura
// (NV_SHOT_HOOKS): nada aqui consulta servico nem gasta cota.
#include "catalogo.h"
#include "player.h"
#include "pausao.h"
#include "ajustes.h"
#include "faixas.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "episodios.h"
#include "streams.h"
#include "plrilha.h"
#include "intro.h"
#include "video.h"
#include "extras.h"
#include "badges.h"
#include "seekr.h"
#include "parental.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static GLuint fbo, fboTex;
static int LW = 1920, LH = 1080;
static const char *saida, *material;
static char mock[600];

static void salvar(const char *id) {
  char nome[800];
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
  snprintf(nome, sizeof nome, "%s/%s-%s.bmp", saida, id, material);
  assert(SDL_SaveBMP(s, nome) == 0);
  SDL_FreeSurface(s);
  fprintf(stderr, "captura: %s\n", nome);
}

static Uint32 relogio = 100000;
static void quadros(int n) {
  int i;
  for (i = 0; i < n; i++) {
    relogio += 16;
    SDL_PumpEvents(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(8);
    player_atualizar(1.f / 60, relogio);
    episodios_atualizar(1.f / 60);
    stream_folha_atualizar(1.f / 60, relogio); faixas_atualizar(1.f / 60, relogio);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
    glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    player_desenhar(relogio);
    episodios_desenhar();
    stream_folha_desenhar(relogio);
    faixas_desenhar(relogio);
    if (stream_folha_anim() > 0.02f) plrilha_esconder();
    plrilha_desenhar(relogio);
    SDL_Delay(1);
  }
}

static const char *img(const char *rel) {
  static char b[8][700];
  static int k;
  k = (k + 1) % 8;
  if (mock[0]) snprintf(b[k], sizeof b[k], "%s/%s", mock, rel);
  else snprintf(b[k], sizeof b[k], "deploy/app/art/19.jpg");
  return b[k];
}

static CatItem filme, serie;
static void titulos(void) {
  memset(&filme, 0, sizeof filme);
  snprintf(filme.tipo, sizeof filme.tipo, "movie");
  snprintf(filme.titulo, sizeof filme.titulo, "Project Hail Mary");
  snprintf(filme.backdrop, sizeof filme.backdrop, "%s", img("img/bd/13.jpg"));
  snprintf(filme.logo, sizeof filme.logo, "%s", img("img/lg/13.png"));
  snprintf(filme.meta, sizeof filme.meta, "2026 \xc2\xb7 2h 37min \xc2\xb7 Aventura \xc2\xb7 Com\xc3\xa9" "dia \xc2\xb7 12");
  memset(&serie, 0, sizeof serie);
  snprintf(serie.tipo, sizeof serie.tipo, "series");
  snprintf(serie.titulo, sizeof serie.titulo, "Fallout");
  snprintf(serie.backdrop, sizeof serie.backdrop, "%s", img("img/bd/00.jpg"));
  snprintf(serie.logo, sizeof serie.logo, "%s", img("img/lg/00.png"));
  snprintf(serie.meta, sizeof serie.meta, "2024 \xc2\xb7 56 min \xc2\xb7 A\xc3\xa7\xc3\xa3o \xc2\xb7 Aventura \xc2\xb7 14");
  snprintf(serie.nomeEpisodio, sizeof serie.nomeEpisodio, "The Head");
  serie.temporada = 1; serie.episodio = 3;
}

static void abrir(const CatItem *c) {
  CatItem lista[1];
  lista[0] = *c;
  cat_definir(lista, 1);
  player_abrir(0, NULL);
  if (!strcmp(c->tipo, "series")) player_definir_episodio(c->temporada, c->episodio);
  player_erro_fonte(); player_limpar_erro_fonte();
}

static void simular(int largura, int altura, const char *hdr, int dv, int atmos) {
  VideoSimulacao v;
  memset(&v, 0, sizeof v);
  v.largura = largura; v.altura = altura; v.dv = dv; v.atmos = atmos;
  snprintf(v.hdr, sizeof v.hdr, "%s", hdr ? hdr : "");
  video_simular(&v);
}


static void faixa(VideoFaixa *f, const char *rot, const char *idioma, const char *codec) {
  memset(f, 0, sizeof *f);
  snprintf(f->rotulo, sizeof f->rotulo, "%s", rot);
  snprintf(f->idioma, sizeof f->idioma, "%s", idioma);
  snprintf(f->codec, sizeof f->codec, "%s", codec);
  f->ordinalMkv = -1;
}
// O filme com as faixas do mockup (4 de audio, 6 de legenda).
static void simularFaixas(void) {
  VideoSimulacao v;
  memset(&v, 0, sizeof v);
  v.largura = 3840; v.altura = 1606; v.dv = 1; v.atmos = 1;
  v.nAudio = 4; v.audioAtual = 0;
  faixa(&v.audio[0], "Ingl\xc3\xaas  \xc2\xb7  Dolby Atmos \xc2\xb7 TrueHD \xc2\xb7 7.1", "en", "");
  faixa(&v.audio[1], "Portugu\xc3\xaas (Brasil)  \xc2\xb7  E-AC3 \xc2\xb7 5.1", "pt", "");
  faixa(&v.audio[2], "Espanhol (Am\xc3\xa9rica Latina)  \xc2\xb7  E-AC3 \xc2\xb7 5.1", "es", "");
  faixa(&v.audio[3], "Ingl\xc3\xaas \xe2\x80\x94 Coment\xc3\xa1rio do diretor  \xc2\xb7  AAC \xc2\xb7 2.0", "en", "");
  v.nLeg = 6; v.legAtual = 0;
  faixa(&v.leg[0], "Portugu\xc3\xaas (Brasil)", "pt", "S_TEXT/UTF8");
  faixa(&v.leg[1], "Portugu\xc3\xaas (Brasil) \xe2\x80\x94 Letreiros", "pt", "S_TEXT/UTF8");
  faixa(&v.leg[2], "Ingl\xc3\xaas", "en", "S_TEXT/UTF8");
  faixa(&v.leg[3], "Ingl\xc3\xaas \xe2\x80\x94 SDH", "en", "S_TEXT/UTF8");
  faixa(&v.leg[4], "Espanhol", "es", "S_TEXT/UTF8");
  faixa(&v.leg[5], "Franc\xc3\xaas", "fr", "S_TEXT/UTF8");
  video_simular(&v);
}
static void teclaFaixas(SDL_Keycode k) {
  SDL_Event ev; memset(&ev, 0, sizeof ev);
  ev.type = SDL_KEYDOWN; ev.key.keysym.sym = k; faixas_evento(&ev);
  quadros(2);
}


// Uma arte do mockup como textura GL (as miniaturas do Seekr da captura).
static GLuint texDe(const char *rel) {
  SDL_Surface *sf = IMG_Load(img(rel)), *t;
  GLuint tex = 0;
  if (!sf) return 0;
  t = SDL_ConvertSurfaceFormat(sf, SDL_PIXELFORMAT_ABGR8888, 0);
  SDL_FreeSurface(sf);
  if (!t) return 0;
  glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, t->pitch / 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  SDL_FreeSurface(t);
  return tex;
}

static int quer(int argc, char **argv, const char *id) {
  int i;
  if (argc < 3) return 1;
  for (i = 2; i < argc; i++) if (!strcmp(argv[i], id)) return 1;
  return 0;
}

int main(int argc, char **argv) {
  saida = argc > 1 ? argv[1] : "/tmp/nv-player-glass";
  { const char *v = getenv("NUVIO_SHOT_VIDRO"); material = v && *v == '1' ? "vidro" : "solido"; }
  { const char *m = getenv("NUVIO_SHOT_MOCK"); snprintf(mock, sizeof mock, "%s", m ? m : ""); }
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *w = SDL_CreateWindow("Nuvio: player glass", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  SDL_GLContext gl = SDL_GL_CreateContext(w); assert(gl); SDL_GL_SetSwapInterval(0);
  glGenTextures(1, &fboTex); glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  badges_carregar("deploy/app/art");
  { char caminho[700]; FILE *f;
    snprintf(caminho, sizeof caminho, "%s/ajustes.txt", getenv("NUVIO_DADOS"));
    f = fopen(caminho, "w"); assert(f);
    fprintf(f, "idioma 0\nselected_theme 2\n");
    fclose(f);
    ajustes_dir(getenv("NUVIO_DADOS")); }
  ajustes_iniciar();
  if (!strcmp(material, "vidro")) ajustes_definir_vidro(1);
  { struct tm lt; time_t t = time(NULL);
    localtime_r(&t, &lt); lt.tm_hour = 20; lt.tm_min = 19; lt.tm_sec = 0;
    plrilha_shot_hora(mktime(&lt)); }
  titulos();
  { static const char *t[8] = { "The Martian", "Interstellar", "Arrival", "Gravity",
                                "Ad Astra", "Moon", "Sunshine", "Contact" };
    static const char *an[8] = { "2015", "2014", "2016", "2013", "2019", "2009", "2007", "1997" };
    static const char *po[8] = { "img/po/12.jpg", "img/po/10.jpg", "img/po/16.jpg", "img/po/14.jpg",
                                 "img/po/32.jpg", "img/po/03.jpg", "img/po/36.jpg", "img/po/02.jpg" };
    const char *pc[8];
    int i;
    for (i = 0; i < 8; i++) pc[i] = img(po[i]);
    extras_shot_relacionados(t, an, pc, 8); }

  if (quer(argc, argv, "osd")) {
    abrir(&filme); simular(3840, 1606, "", 1, 1);
    quadros(10);
    player_shot_estado(relogio, 4360.0f, 9420.0f, 1, 0, 0, 0);
    quadros(60);
    salvar("osd");
  }
  if (quer(argc, argv, "osd-serie")) {
    IntroTrecho tr[2] = { { 67.2, 235.2, INTRO_ABERTURA }, { 3124.8, 0.0, INTRO_CREDITOS } };
    abrir(&serie); simular(3840, 2160, "HDR10", 0, 1);
    intro_shot_definir(tr, 2);
    quadros(10);
    player_shot_estado(relogio, 1948.0f, 3360.0f, 1, 5, 0, 0);
    quadros(60);
    salvar("osd-serie");
  }
  if (quer(argc, argv, "osd-barra")) {
    abrir(&filme); simular(3840, 1606, "", 1, 1);
    quadros(10);
    player_shot_estado(relogio, 4820.0f, 9420.0f, 1, 0, 1, 1);
    quadros(60);
    salvar("osd-barra");
  }
  if (quer(argc, argv, "toast-proporcao")) {
    abrir(&filme); simular(3840, 1606, "", 1, 1);
    quadros(10);
    player_shot_toast(relogio, NULL, NULL, 0, 4);
    quadros(90);
    salvar("toast-proporcao");
  }
  if (quer(argc, argv, "reconectando")) {
    abrir(&filme); simular(3840, 1606, "", 1, 1);
    quadros(10);
    { VideoSimulacao v; memset(&v, 0, sizeof v);
      v.largura = 3840; v.altura = 1606; v.pronto = 1; v.bufferandoMs = 900; v.duracao = 9420; v.pos = 4360;
      video_simular(&v); }
    player_shot_video(1);
    player_shot_toast(relogio, "Conex\xc3\xa3o caiu, reconectando\xe2\x80\xa6", "aj_wifi-off", 1, 0);
    quadros(90);
    salvar("reconectando");
    player_shot_video(0); simular(0, 0, "", 0, 0);
  }
  if (quer(argc, argv, "audio") || quer(argc, argv, "audio-direita")) {
    int dir;
    for (dir = 0; dir < 2; dir++) {
      if (!quer(argc, argv, dir ? "audio-direita" : "audio")) continue;
      ajustes_shot_valor("relogioPosLocal", dir ? 2 : 1);
      abrir(&filme); simularFaixas();
      quadros(10);
      player_shot_estado(relogio, 4360.0f, 9420.0f, 1, 0, 0, 0);
      player_shot_esconder();
      faixas_abrir_em(0);
      teclaFaixas(SDLK_DOWN);
      quadros(90);
      salvar(dir ? "audio-direita" : "audio");
      faixas_evento(&(SDL_Event){ .key = { .type = SDL_KEYDOWN, .keysym = { .sym = SDLK_ESCAPE } } });
      quadros(60);
    }
    ajustes_shot_valor("relogioPosLocal", 1);
  }
  if (quer(argc, argv, "legendas") || quer(argc, argv, "legenda-estilo")) {
    abrir(&filme); simularFaixas();
    quadros(10);
    player_shot_estado(relogio, 4360.0f, 9420.0f, 1, 0, 0, 0);
    player_shot_esconder();
    faixas_abrir_em(1);
    teclaFaixas(SDLK_DOWN);
    quadros(90);
    if (quer(argc, argv, "legendas")) salvar("legendas");
    { VideoLegendaEstilo *e = player_leg_estilo();
      e->fundo = 2; e->borda = 2; e->atrasoMs = 250; player_leg_estilo_mudou(); }
    teclaFaixas(SDLK_RIGHT);
    teclaFaixas(SDLK_DOWN); teclaFaixas(SDLK_RIGHT); teclaFaixas(SDLK_RIGHT);
    quadros(90);
    if (quer(argc, argv, "legenda-estilo")) salvar("legenda-estilo");
    faixas_evento(&(SDL_Event){ .key = { .type = SDL_KEYDOWN, .keysym = { .sym = SDLK_ESCAPE } } });
    quadros(60);
  }
  if (quer(argc, argv, "carregando")) {
    Stream st;
    memset(&st, 0, sizeof st);
    snprintf(st.rotulo, sizeof st.rotulo, "Fallout S01E03 2160p");
    snprintf(st.provedor, sizeof st.provedor, "AIOStreams");
    snprintf(st.url, sizeof st.url, "http://exemplo/fallout.mkv");
    st.tamanhoMB = (long)(18.2 * 1024);
    st.badges = badges_bit("r-4k") | badges_bit("v-hdr10") | badges_bit("a-atmos");
    stream_definir_lista(&st, 1);
    abrir(&serie); simular(0, 0, "", 0, 0);
    stream_definir_atual(0);
    player_definir_tentativa(2, 3);
    player_shot_carregando(1);
    quadros(120);
    salvar("carregando");
    player_definir_tentativa(0, 0);
  }
  if (quer(argc, argv, "erro")) {
    abrir(&filme); simular(3840, 1606, "", 1, 1);
    quadros(10);
    player_erro_fonte_motivo("As fontes torrent desta lista n\xc3\xa3o est\xc3\xa3o no cache do debrid",
                             "Abra Fontes e escolha uma: o servi\xc3\xa7o come\xc3\xa7" "a a baixar.");
    quadros(120);
    salvar("erro");
    player_limpar_erro_fonte();
  }
  if (quer(argc, argv, "seekr") || quer(argc, argv, "seekr-fita") || quer(argc, argv, "seekr-carregando")) {
    static const char *ids[3] = { "seekr", "seekr-fita", "seekr-carregando" };
    int q;
    ajustes_shot_valor("seekrChave", 1);
    ajustes_shot_valor("seekrLocal", 0);
    for (q = 0; q < 3; q++) {
      GLuint t[3] = { 0, 0, 0 };
      double cue[3] = { -1, -1, -1 };
      float pos;
      if (!quer(argc, argv, ids[q])) continue;
      ajustes_shot_valor("seekrFitaLocal", q == 1 ? 0 : 1);
      abrir(&serie); simular(3840, 2160, "HDR10", 0, 1);
      quadros(10);
      if (q == 0) { pos = 2083.0f; t[1] = texDe("img/ep/00_1_04.jpg"); cue[1] = 2083.0; }
      else if (q == 1) { pos = 1040.0f; t[0] = texDe("img/ep/00_1_02.jpg"); t[1] = texDe("img/ep/00_1_03.jpg");
                         t[2] = texDe("img/ep/00_1_08.jpg"); cue[0] = 1030.0; cue[1] = 1040.0; cue[2] = 1050.0; }
      else { pos = 2620.0f; cue[1] = 2620.0; t[1] = texDe("img/ep/00_1_06.jpg"); }
      seekr_shot(SEEKR_PRONTO, t, cue, q == 2);
      player_shot_estado(relogio, pos, 3360.0f, 1, 0, 1, 1);
      player_shot_buscando(1);
      quadros(60);
      salvar(ids[q]);
      player_shot_buscando(0);
      seekr_shot(0, NULL, NULL, 0);
      quadros(30);
    }
    ajustes_shot_valor("seekrChave", 0);
  }
  if (quer(argc, argv, "guia-parental")) {
    static const char *rot[4] = { "Viol\xc3\xaancia", "Linguagem Impr\xc3\xb3pria", "Conte\xc3\xba" "do Assustador", "Drogas/\xc3\x81lcool" };
    static const char *gr[4] = { "Moderado", "Leve", "Severo", "Leve" };
    CatItem f = filme;
    snprintf(f.classificacao, sizeof f.classificacao, "12");
    abrir(&f); simular(3840, 1606, "", 1, 1);
    { VideoSimulacao v; memset(&v, 0, sizeof v);
      v.largura = 3840; v.altura = 1606; v.pronto = 1; v.duracao = 9420; v.pos = 30;
      video_simular(&v); }
    player_shot_video(1);
    parental_shot(rot, gr, 4);
    quadros(20);
    player_shot_esconder();
    quadros(80);
    salvar("guia-parental");
    player_shot_video(0); parental_shot(NULL, NULL, 0);
  }
  if (quer(argc, argv, "pular-abertura")) {
    IntroTrecho tr[1] = { { 60.0, 140.0, INTRO_ABERTURA } };
    abrir(&serie); simular(3840, 2160, "HDR10", 0, 1);
    intro_shot_definir(tr, 1);
    quadros(10);
    player_shot_estado(relogio, 90.0f, 3360.0f, 1, 0, 0, 0);
    player_shot_esconder();
    quadros(60);
    salvar("pular-abertura");
    intro_shot_definir(NULL, 0);
  }
  puts("player_glass_shot: ok");
  return 0;
}
