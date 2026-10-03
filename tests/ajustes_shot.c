// CAPTURA DA TELA DE AJUSTES, sem interacao e sem rede.
//
// Existe pelo motivo que tests/player_regression.c ja registra: interface de TV
// julgada so por codigo sai ilegivel a 3 m. Aqui as duas telas que mudam neste
// trabalho — a lista de Ajustes e a folha "Ordenar e ativar fileiras" — sao
// desenhadas com dados de mentira e gravadas em BMP, para serem OLHADAS.
//
// NAO CHAMA dados_iniciar DE PROPOSITO. Sem ela `dados_dir()` e "", entao
// fileiras.c nao le nem escreve arquivo nenhum: a lista comeca vazia (o estado
// de quem nunca abriu o app) e a captura nao mexe no fileirasui.txt de quem
// roda o teste.
#include "ajustes.h"
#include "ajustes_ux.h"
#include "atualizacao.h"
#include "badges.h"
#include "rail_shot.h"
#include "spotlight.h"
#include "fileiras.h"
#include "gfx.h"
#include "ilha.h"
#include "text.h"
#include "tex_cache.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const AJ_IDS[] = {
  "AJ_QUALIDADE", "AJ_DV", "AJ_ATMOS", "AJ_LEG_LINGUA",
  "AJ_AUD_LINGUA", "AJ_PAUSA_OVERLAY", "AJ_FONTE_MANUAL", "AJ_FONTE_AUTO",
  "AJ_FONTE_REPOR", "AJ_LANDSCAPE", "AJ_HERO_CHEIO", "AJ_HERO_FUNDO",
  "AJ_HERO_ARTE_DIF", "AJ_HERO_TRAILER", "AJ_FIL_LIMITE", "AJ_FIL_ORDEM",
  "AJ_RAIL", "AJ_RAIL_MODERNA", "AJ_RAIL_BLUR", "AJ_HERO",
  "AJ_HERO_CATALOGOS", "AJ_PS_FUNDO", "AJ_DESCOBRIR", "AJ_ROTULOS",
  "AJ_NOME_ADDON", "AJ_SUFIXO_TIPO", "AJ_OCULTAR_NLANC", "AJ_NOTAS_HOME",
  "AJ_GRAD_CLASSICO", "AJ_CW_LIGADO", "AJ_CW_OK", "AJ_CW_FONTE",
  "AJ_CW_ESTILO", "AJ_CW_THUMB", "AJ_CW_BLUR_PROX", "AJ_CW_FURTHEST",
  "AJ_CW_NAO_EXIBIDOS", "AJ_CW_ORDEM", "AJ_DET_BLUR_NAO_VISTOS", "AJ_DET_TRAILER",
  "AJ_DET_META_EXT", "AJ_DET_DATA_CHEIA", "AJ_DET_VEU", "AJ_DET_TRAILER_AUTO",
  "AJ_TRAILER_QUAL", "AJ_TRAILER_ASPECTO", "AJ_TRAILER_FONTE", "AJ_EXPANDIR",
  "AJ_EXPANDIR_ATRASO", "AJ_NAV_RAPIDA", "AJ_BORDA_FOCO", "AJ_PROF",
  "AJ_PROF_BORDA", "AJ_PROF_BRILHO", "AJ_PROF_COBERTURA", "AJ_PROF_POSTERS",
  "AJ_PROF_CW", "AJ_PROF_EPS", "AJ_PROF_ELENCO", "AJ_PROF_TRAILERS",
  "AJ_LARGURA_DP", "AJ_RAIO_DP", "AJ_QUALIDADE_IMG", "AJ_IDIOMA",
  "AJ_ANIM", "AJ_RESOLUCAO", "AJ_TEMA", "AJ_COR_LOGO",
  "AJ_PERFIL_ATIVO", "AJ_SYNC", "AJ_ADDONS", "AJ_STALKER_PORTAL",
  "AJ_STALKER_MAC", "AJ_STALKER_LIMPAR", "AJ_XTREAM_SERVIDOR", "AJ_XTREAM_USUARIO",
  "AJ_XTREAM_SENHA", "AJ_XTREAM_LIMPAR", "AJ_SALVOS_DEST", "AJ_TRAKT",
  "AJ_SIMKL", "AJ_SAIR", "AJ_VERSAO_I", "AJ_ATUALIZAR",
  "AJ_ENVIAR_LOG", "AJ_ENVIO_AUTO", "AJ_ESPACO", "AJ_TEX_MB",
  "AJ_TMDB_LIGADO", "AJ_TMDB_IDIOMA", "AJ_TMDB_ARTE", "AJ_TMDB_BASICO",
  "AJ_TMDB_FICHA", "AJ_TMDB_DATAS", "AJ_TMDB_ELENCO", "AJ_TMDB_PROD",
  "AJ_TMDB_REDES", "AJ_TMDB_EPS", "AJ_TMDB_TRAILERS", "AJ_TMDB_MAIS",
  "AJ_TMDB_COL", "AJ_TMDB_CW", "AJ_MDB_LIGADO", "AJ_MDB_CHAVE",
  "AJ_MDB_TRAKT", "AJ_MDB_IMDB", "AJ_MDB_TMDB", "AJ_MDB_LETTER",
  "AJ_MDB_TOMATES", "AJ_MDB_AUDIENCIA", "AJ_MDB_META", "AJ_MDB_MAL",
  "AJ_FANART_CHAVE", "AJ_DIAGNOSTICO", "AJ_VELOCIDADE", "AJ_FONTE_UI",
};

extern int ajustes_teste_focar_opcao(int op);
extern void ajustes_teste_ux_captura(int cenario);
extern void atualizacao_teste_estado(int busca, const char *tag);
extern int ajustes_teste_op_atualizar(void);
extern int ajustes_teste_familia_previa(int op);
extern void ajustes_teste_fonte_interface(int familia);
extern void ajustes_teste_tema(int tema, int vidro);
extern int ajustes_teste_quadro(const char *id);
static int quadrosCaptura = 60;

static void tecla(SDL_Keycode k) {
  SDL_Event e = { 0 };
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  ajustes_evento(&e);
}

static void captura(const char *nome, SDL_Window *win) {
  int i;
  rail_shot_aplicar();
  for (i = 0; i < quadrosCaptura; i++) {
    SDL_PumpEvents();
    txt_novo_quadro();
    tex_novo_quadro();
    tex_bombear(6);
    ajustes_atualizar(1.0f / 60.0f, SDL_GetTicks());
    glClearColor(0.025f, 0.025f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ajustes_desenhar(SDL_GetTicks());
    rail_shot_desenhar(MENU_AJUSTES);
    if (spot_visivel()) {   // "Buscar no guia" (o Spotlight no modo guia)
      spot_atualizar(1.0f / 60.0f, SDL_GetTicks());
      spot_desenhar(SDL_GetTicks(), 0);   // o veu de 55% e do proprio Spotlight
    }
    if (getenv("NUVIO_AJ_QUADROS")) {   // a ilha do relogio, como o app poe
      ilha_relogio_visivel(ajustes_relogio_cabe());
      ilha_ancorar(ajustes_ilha_x(), 36, 0);
      ilha_desenhar(SDL_GetTicks());
    }
    if (i == quadrosCaptura - 1) {
      unsigned char *pix = malloc(1920 * 1080 * 4);
      SDL_Surface *s;
      int y;
      assert(pix);
      glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
      assert(s);
      for (y = 0; y < 1080; y++)
        memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
      assert(strstr(nome, ".png") ? IMG_SavePNG(s, nome) == 0 : SDL_SaveBMP(s, nome) == 0);
      SDL_FreeSurface(s);
      free(pix);
    }
    SDL_GL_SwapWindow(win);
    if (getenv("NUVIO_AJ_QUADROS")) SDL_Delay(16);   // a mola da ilha anda no relogio
  }
  printf("captura: %s\n", nome);
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nuvio-ajustes";
  char nome[600];
  SDL_Window *w;
  SDL_GLContext gl;
  int i;

  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  w = SDL_CreateWindow("Nuvio: revisao dos Ajustes", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1920, 1080,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(w);
  gl = SDL_GL_CreateContext(w);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  ajustes_recursos("deploy/app/art");
  badges_carregar("deploy/app/art");   // marcas de formato (Dolby Vision, 4K...); no app quem faz e home_iniciar

  // FILEIRAS DE MENTIRA cobrindo as origens que a folha sabe distinguir: o
  // catalogo de addon (com nome de addon e tipo), o grupo de colecoes, e as
  // fileiras que o proprio app monta.
  fil_registrar("continue_watching", "Continuar assistindo", "", "", 12);
  fil_registrar("social_activity", "Entre amigos", "", "", 6);
  fil_registrar("com.linvo.cinemeta_movie_top", "Popular", "Cinemeta", "movie", 40);
  fil_registrar("xperience_series_foryou", "For You", "Xperience", "series", 24);
  fil_registrar("collection_a24", "A24", "", "", 9);
  fil_registrar("tmdb.addon_movie_trending", "Em alta", "TMDB", "movie", 20);
  fil_registrar("aiostreams_series_novos", "Séries novas", "AIOStreams", "series", 18);
  fil_registrar("akashi_movie_anime", "Anime", "Akashi", "movie", 30);
  fil_registrar("mdblist_movie_oscar", "Vencedores do Oscar", "MDBList", "movie", 15);
  fil_registrar("sem.nome_movie_x", "", "", "", -1);
  // Mais catalogos do que o limite (7): os que passam ficam NA FILA. E dois
  // removidos, para a aba "Fora da Home" ter o que agrupar por addon.
  fil_registrar("xperience_movie_acao", "Ação", "Xperience", "movie", 12);
  fil_registrar("xperience_movie_terror", "Terror", "Xperience", "movie", 12);
  fil_registrar("xperience_series_animes", "Animes", "Xperience", "series", 12);
  fil_registrar("aiostreams_movie_top", "Top 100", "AIOStreams", "movie", 12);
  fil_registrar("akashi_series_dorama", "Doramas", "Akashi", "series", 12);
  fil_registrar("akashi_movie_bollywood", "Bollywood", "Akashi", "movie", 12);
  fil_remover(7);   // Anime
  fil_remover(9);   // sem nome
  fil_remover(12);  // Animes

  ajustes_iniciar();
  // NUVIO_TAMANHO_UI=1.2|1.3|1.5: o "Tamanho da interface" (escala.h).
  if (getenv("NUVIO_TAMANHO_UI")) gfx_escala_ui_definir((float)atof(getenv("NUVIO_TAMANHO_UI")));
  // NUVIO_SHOT_TEMA=<indice> (0 branco, 12 Dinamica) e NUVIO_SHOT_VIDRO=1: o
  // foco no acento claro com e sem vidro (#202).
  if (getenv("NUVIO_SHOT_TEMA") || getenv("NUVIO_SHOT_VIDRO"))
    ajustes_teste_tema(getenv("NUVIO_SHOT_TEMA") ? atoi(getenv("NUVIO_SHOT_TEMA")) : -1,
                       getenv("NUVIO_SHOT_VIDRO") && atoi(getenv("NUVIO_SHOT_VIDRO")));

  // OS QUADROS DO MOCKUP (ajustes-mockup.html): NUVIO_AJ_QUADROS="principal
  // cor ..." grava <saida>-<id>.png de cada um, com a ilha do relogio.
  if (getenv("NUVIO_AJ_QUADROS")) {
    char lista[1024], *id, *ctx = NULL;
    snprintf(lista, sizeof lista, "%s", getenv("NUVIO_AJ_QUADROS"));
    for (id = strtok_r(lista, " ,", &ctx); id; id = strtok_r(NULL, " ,", &ctx)) {
      if (!ajustes_teste_quadro(id)) { printf("quadro desconhecido: %s\n", id); continue; }
      if (!strcmp(id, "guia-busca")) {
        int k;
        SDL_Event e = { 0 };
        spot_abrir_guia();
        spot_texto_externo("legenda");
        e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_DOWN;
        for (k = 0; k < 4 && spot_linha_focada() < 0; k++) spot_evento(&e);
      }
      snprintf(nome, sizeof nome, "%s-%s.png", saida, id);
      captura(nome, w);
      if (spot_aberto()) spot_fechar();
    }
    goto fim_capturas;
  }

  // A LINHA "Procurar atualização" NOS ESTADOS (01/10/2026): sem versao nova
  // (antes de procurar, procurando, em dia, sem rede) e com versao nova.
  if (getenv("NUVIO_AJUSTES_ATUALIZAR")) {
    static const struct { int busca; const char *tag; const char *nome; } est[] = {
      { ATUALIZACAO_BUSCA_NADA, "", "nada" },
      { ATUALIZACAO_BUSCA_PROCURANDO, "", "procurando" },
      { ATUALIZACAO_BUSCA_EM_DIA, "", "em-dia" },
      { ATUALIZACAO_BUSCA_ERRO, "", "erro" },
      { ATUALIZACAO_BUSCA_NOVA, "9.9.9", "nova" },
    };
    int op = ajustes_teste_op_atualizar();
    for (i = 0; i < (int)(sizeof est / sizeof *est); i++) {
      atualizacao_teste_estado(est[i].busca, est[i].tag);
      assert(ajustes_teste_focar_opcao(op));
      snprintf(nome, sizeof nome, "%s-atualizar-%s.png", saida, est[i].nome);
      captura(nome, w);
    }
    goto fim_capturas;
  }

  if (getenv("NUVIO_AJUSTES_OPCOES")) {
    static const char *const FAMILIAS[] = {
      "reproducao", "home", "continuar", "detalhe", "foco", "profundidade",
      "cartaz", "interface", "conta", "rastreio", "sobre", "tmdb", "mdblist",
      "tv", "acao"
    };
    FILE *manifest;
    const char *dir = saida;
    quadrosCaptura = 60;
    snprintf(nome, sizeof nome, "%s/coverage.tsv", dir);
    manifest = fopen(nome, "w");
    assert(manifest);
    assert(sizeof AJ_IDS / sizeof *AJ_IDS == 116);
    for (i = 0; i < (int)(sizeof AJ_IDS / sizeof *AJ_IDS); i++) {
      int familia = ajustes_teste_familia_previa(i);
      assert(familia >= 0 && familia < (int)(sizeof FAMILIAS / sizeof *FAMILIAS));
      assert(ajustes_teste_focar_opcao(i));
      snprintf(nome, sizeof nome, "%s/%s.png", dir, AJ_IDS[i]);
      captura(nome, w);
      fprintf(manifest, "%s\t%s\n", AJ_IDS[i], FAMILIAS[familia]);
    }
    /* A amostra mostra também as alternativas mais pedidas, sem gravá-las. */
    { static const struct { int familia; const char *sufixo; } fontes[] = {
        { TXT_FAMILIA_INTER, "Inter" }, { TXT_FAMILIA_MONTSERRAT, "Montserrat" },
        { TXT_FAMILIA_ROBOTO, "Roboto" }, { TXT_FAMILIA_ATKINSON, "Atkinson" }
      };
      for (i = 0; i < (int)(sizeof fontes / sizeof *fontes); i++) {
        ajustes_teste_fonte_interface(fontes[i].familia);
        assert(ajustes_teste_focar_opcao((int)(sizeof AJ_IDS / sizeof *AJ_IDS) - 1));
        snprintf(nome, sizeof nome, "%s/AJ_FONTE_UI-%s.png", dir, fontes[i].sufixo);
        captura(nome, w);
      }
    }
    fclose(manifest);
    goto fim_capturas;
  }

  // Cenários do novo contrato: categoria, seleção, cancelamento,
  // dependência e restauração. Só fixtures em memória, sem dados pessoais.
  static const char *const cenarios[] = {
    "indice", "tela-inicial", "seletor", "dependencia", "diferencas", "restaurar", "avancados", "numero", "cabecalho", "english"
  };
  for (i = 0; i < (int)(sizeof cenarios / sizeof *cenarios); i++) {
    ajustes_teste_ux_captura(i);
    snprintf(nome, sizeof nome, "%s-%s.png", saida, cenarios[i]);
    captura(nome, w);
  }
  for (i = 21; i <= 22; i++) {
    ajustes_teste_ux_captura(i);
    snprintf(nome, sizeof nome, "%s-%s.png", saida, i == 21 ? "trailers" : "trailer-seletor");
    captura(nome, w);
  }
  if (!getenv("NUVIO_AJUSTES_UX")) {
    for (i = 0; i < 11; i++) {
      ajustes_teste_ux_captura(10 + i);
      snprintf(nome, sizeof nome, "%s-categoria-%02d.png", saida, i + 1);
      captura(nome, w);
    }
  }

  fim_capturas:
  tex_encerrar();
  txt_encerrar();
  gfx_encerrar();
  SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(w);
  SDL_Quit();
  puts("PASS: capturas da tela de Ajustes gravadas.");
  return 0;
}
