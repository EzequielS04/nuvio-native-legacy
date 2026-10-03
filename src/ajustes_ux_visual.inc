// Mapa sem fotos: SVG Lucide embarcado, rasterizado no pacote para OpenGL.
static const char *ajVisualIcone(int op) {
  switch (op) {
    case AJ_HOME_LAYOUT: return "aj_layout-dashboard";
    case AJ_LANDSCAPE: return "aj_layout-dashboard";
    case AJ_HERO: return "aj_layout-dashboard";
    case AJ_HERO_CHEIO: return "aj_layout-dashboard";
    case AJ_HERO_CATALOGOS: return "aj_layout-dashboard";
    case AJ_HERO_TRANSICAO: return "aj_layout-dashboard";
    case AJ_FIL_ORDEM: return "aj_rows-3";
    case AJ_FIL_LIMITE: return "aj_rows-3";
    case AJ_ITENS_FILEIRA: return "aj_rows-3";
    case AJ_OCULTAR_NLANC: return "aj_rows-3";
    case AJ_NOTAS_HOME: return "aj_rows-3";
    case AJ_ROTULOS: return "aj_rows-3";
    case AJ_NOME_ADDON: return "aj_rows-3";
    case AJ_SUFIXO_TIPO: return "aj_rows-3";
    case AJ_SELO_VISTO: return "aj_rows-3";
    case AJ_CW_LIGADO: return "aj_rotate-ccw-clock";
    case AJ_CW_OK: return "aj_rotate-ccw-clock";
    case AJ_CW_ESTILO: return "aj_rotate-ccw-clock";
    case AJ_CW_THUMB: return "aj_rotate-ccw-clock";
    case AJ_CW_BLUR_PROX: return "aj_rotate-ccw-clock";
    case AJ_CW_ORDEM: return "aj_rotate-ccw-clock";
    case AJ_CW_NAO_EXIBIDOS: return "aj_rotate-ccw-clock";
    case AJ_CW_FURTHEST: return "aj_rotate-ccw-clock";
    case AJ_RAIL_MODERNA: return "aj_panel-left";
    case AJ_RAIL: return "aj_panel-left";
    case AJ_MENU_EXPLORAR: return "aj_panel-left";
    case AJ_MENU_GUIA: return "aj_panel-left";
    case AJ_MENU_AGENDA: return "aj_panel-left";
    case AJ_MENU_PERFIL: return "aj_panel-left";
    case AJ_RAIL_BLUR: return "aj_panel-left";
    case AJ_DESCOBRIR: return "aj_panel-left";
    case AJ_PS_FUNDO: return "aj_users";
    case AJ_EXPANDIR: return "aj_maximize-2";
    case AJ_EXPANDIR_ATRASO: return "aj_clock";
    case AJ_BORDA_FOCO: return "aj_maximize-2";
    case AJ_GRAD_CLASSICO: return "aj_maximize-2";
    case AJ_LARGURA_DP: return "aj_images";
    case AJ_RAIO_DP: return "aj_images";
    case AJ_PROF: return "aj_images";
    case AJ_PROF_BORDA: return "aj_images";
    case AJ_PROF_BRILHO: return "aj_images";
    case AJ_PROF_COBERTURA: return "aj_images";
    case AJ_PROF_POSTERS: return "aj_images";
    case AJ_PROF_CW: return "aj_images";
    case AJ_PROF_EPS: return "aj_images";
    case AJ_PROF_ELENCO: return "aj_images";
    case AJ_PROF_TRAILERS: return "aj_images";
    case AJ_HERO_FUNDO: return "aj_image";
    case AJ_HERO_ARTE_DIF: return "aj_image";
    case AJ_POSTER_PROV: return "aj_image";
    case AJ_ADDON_POSTER: return "aj_image";
    case AJ_ADDON_FUNDO: return "aj_image";
    case AJ_ADDON_LOGO: return "aj_image";
    case AJ_COL_ARTE_CONTA: return "aj_image";
    case AJ_POSTER_INST: return "aj_image";
    case AJ_POSTER_TOKEN: return "aj_image";
    case AJ_POSTER_EXTRA: return "aj_image";
    case AJ_POSTER_CHAVE: return "aj_image";
    case AJ_POSTER_MODELO: return "aj_image";
    case AJ_POSTER_TESTAR: return "aj_image";
    case AJ_HERO_TRAILER: return "aj_clapperboard";
    case AJ_FOCO_TRAILER: return "aj_clapperboard";
    case AJ_DET_TRAILER_AUTO: return "aj_clapperboard";
    case AJ_DET_TRAILER: return "aj_clapperboard";
    case AJ_HERO_TRAILER_SOM: return "aj_audio-lines";
    case AJ_DET_TRAILER_SOM: return "aj_audio-lines";
    case AJ_TRAILER_FONTE: return "aj_server";
    case AJ_HERO_TRAILER_ESPERA: return "aj_clock";
    case AJ_TRAILER_QUAL: return "aj_monitor-play";
    case AJ_TRAILER_ASPECTO: return "aj_maximize-2";
    case AJ_DET_VEU: return "aj_file-text";
    case AJ_DET_DATA_CHEIA: return "aj_file-text";
    case AJ_DET_BLUR_NAO_VISTOS: return "aj_file-text";
    case AJ_NT_IMDB: return "aj_star";
    case AJ_NT_TOMATES: return "aj_star";
    case AJ_NT_AUDIENCIA: return "aj_star";
    case AJ_NT_META: return "aj_star";
    case AJ_NT_METAUSER: return "aj_star";
    case AJ_NT_TRAKT: return "aj_star";
    case AJ_NT_TMDB: return "aj_star";
    case AJ_NT_LETTER: return "aj_star";
    case AJ_NT_MAL: return "aj_star";
    case AJ_NT_EBERT: return "aj_star";
    case AJ_NT_SCORE: return "aj_star";
    case AJ_MDB_LIGADO: return "aj_star";
    case AJ_MDB_TRAKT: return "aj_star";
    case AJ_MDB_IMDB: return "aj_star";
    case AJ_MDB_TMDB: return "aj_star";
    case AJ_MDB_LETTER: return "aj_star";
    case AJ_MDB_TOMATES: return "aj_star";
    case AJ_MDB_AUDIENCIA: return "aj_star";
    case AJ_MDB_META: return "aj_star";
    case AJ_MDB_MAL: return "aj_star";
    case AJ_TMDB_LIGADO: return "aj_database";
    case AJ_TMDB_ARTE: return "aj_database";
    case AJ_TMDB_BASICO: return "aj_database";
    case AJ_TMDB_FICHA: return "aj_database";
    case AJ_TMDB_DATAS: return "aj_database";
    case AJ_TMDB_ELENCO: return "aj_database";
    case AJ_TMDB_PROD: return "aj_database";
    case AJ_TMDB_REDES: return "aj_database";
    case AJ_TMDB_EPS: return "aj_database";
    case AJ_TMDB_TRAILERS: return "aj_database";
    case AJ_TMDB_MAIS: return "aj_database";
    case AJ_TMDB_COL: return "aj_database";
    case AJ_TMDB_CW: return "aj_database";
    case AJ_DET_META_EXT: return "aj_database";
    case AJ_DET_SO_CINEMETA: return "aj_database";
    case AJ_FONTE_MANUAL: return "aj_list-video";
    case AJ_FONTE_AUTO: return "aj_list-video";
    case AJ_FONTE_REPOR: return "aj_list-video";
    case AJ_FONTE_PRAZO: return "aj_list-video";
    case AJ_FONTE_TEXTO: return "aj_list-video";
    case AJ_SELOS_CORES: return "aj_list-video";
    case AJ_SELOS_PACOTE: return "aj_list-video";
    case AJ_SELOS_PACOTE_ADD: return "aj_list-video";
    case AJ_SELOS_PACOTE_REM: return "aj_list-video";
    case AJ_QUALIDADE: return "aj_monitor-play";
    case AJ_DV: return "aj_monitor-play";
    case AJ_ATMOS: return "aj_monitor-play";
    case AJ_PAUSA_OVERLAY: return "aj_circle-play";
    case AJ_REACAO_CREDITOS: return "aj_circle-play";
    case AJ_CW_CONCLUIDO: return "aj_circle-play";
    case AJ_SAIDA_PLAYER: return "aj_circle-play";
    case AJ_SEEKR_LIGADO: return "aj_film";
    case AJ_SEEKR_CHAVE: return "aj_film";
    case AJ_SEEKR_TESTAR: return "aj_film";
    case AJ_SEEKR_FITA: return "aj_film";
    case AJ_SEEKR_AJUSTE: return "aj_film";
    case AJ_IDIOMA: return "aj_languages";
    case AJ_AUD_LINGUA: return "aj_audio-lines";
    case AJ_LEG_LINGUA: return "aj_captions";
    case AJ_TMDB_IDIOMA: return "aj_file-text";
    case AJ_TEMA: return "aj_palette";
    case AJ_FUNDO: return "aj_images";
    case AJ_COR_LOGO: return "aj_palette";
    case AJ_VIDRO: return "aj_layers";
    case AJ_FONTE_UI: return "aj_type";
    case AJ_TAMANHO_UI: return "aj_maximize-2";
    case AJ_ANIM: return "aj_activity";
    case AJ_RELOGIO: return "aj_clock";
    case AJ_RELOGIO_POS: return "aj_clock";
    case AJ_VIDRO_CONTORNO: return "aj_layers";
    case AJ_VIDRO_OPAC: return "aj_layers";
    case AJ_VIDRO_FOSCO: return "aj_layers";
    case AJ_XTREAM_SERVIDOR: return "aj_server";
    case AJ_XTREAM_USUARIO: return "aj_server";
    case AJ_XTREAM_SENHA: return "aj_server";
    case AJ_XTREAM_CONTA: return "aj_server";
    case AJ_XTREAM_LIMPAR: return "aj_server";
    case AJ_STALKER_PORTAL: return "aj_server";
    case AJ_STALKER_MAC: return "aj_server";
    case AJ_STALKER_LIMPAR: return "aj_server";
    case AJ_EPG_PAIS: return "aj_server";
    case AJ_LIVETV_DIAG: return "aj_stethoscope";
    case AJ_LIVETV_RES: return "aj_stethoscope";
    case AJ_LIVETV_ESPERA: return "aj_stethoscope";
    case AJ_LIVETV_FORMATO: return "aj_stethoscope";
    case AJ_LIVETV_MODO: return "aj_stethoscope";
    case AJ_LIVETV_PROXY: return "aj_stethoscope";
    case AJ_PERFIL_ATIVO: return "aj_user-round";
    case AJ_SYNC: return "aj_cloud";
    case AJ_ADDONS: return "aj_user-round";
    case AJ_ADDONS_PRINCIPAL: return "aj_user-round";
    case AJ_PERFIL_PESQ: return "aj_user-round";
    case AJ_PERFIL_EDITAR: return "aj_user-round";
    case AJ_SAIR: return "aj_log-out";
    case AJ_TRAKT: return "aj_plug";
    case AJ_SIMKL: return "aj_plug";
    case AJ_SALVOS_DEST: return "aj_plug";
    case AJ_CW_FONTE: return "aj_plug";
    case AJ_DEBRID_RD: return "aj_key-round";
    case AJ_DEBRID_TB: return "aj_key-round";
    case AJ_DEBRID_PM: return "aj_key-round";
    case AJ_DEBRID_AD: return "aj_key-round";
    case AJ_DEBRID_AD_TESTAR: return "aj_key-round";
    case AJ_MDB_CHAVE: return "aj_key-round";
    case AJ_FANART_CHAVE: return "aj_key-round";
    case AJ_P2P_LIGADO: return "aj_user-round";
    case AJ_P2P_URL: return "aj_user-round";
    case AJ_P2P_TESTAR: return "aj_user-round";
    case AJ_RESOLUCAO: return "aj_monitor-play";
    case AJ_QUALIDADE_IMG: return "aj_images";
    case AJ_NAV_RAPIDA: return "aj_zap";
    case AJ_MEDIDOR: return "aj_activity";
    case AJ_DIAGNOSTICO: return "aj_stethoscope";
    case AJ_VELOCIDADE: return "aj_gauge";
    case AJ_ESPACO: return "aj_database";
    case AJ_TEX_MB: return "aj_memory-stick";
    case AJ_GUIA: return "aj_book-open";
    case AJ_VERSAO_I: return "aj_info";
    case AJ_ATUALIZAR: return "aj_download";
    case AJ_VER_REGISTRO: return "aj_scroll-text";
    case AJ_ENVIAR_LOG: return "aj_send";
    case AJ_ENVIO_AUTO: return "aj_scroll-text";
#if defined(NV_TPK) || defined(NV_ANDROID)
    case AJ_GPU_EFEITOS: return "aj_zap";
#endif
    default: return "aj_settings-2";
  }
}

// Diagramas sem fotos, na mesma tinta e superficie das ilhas. A categoria
// define a cena; a opcao acrescenta seu proprio glifo e estado, sem segredos.
static float ajVisualCena(int sec, int op, float x, float y, float w) {
  float h = w * 0.5625f, s = w / 600.0f;
  int i, j;
  const char *ic = op >= 0 ? ajVisualIcone(op) : TELA[secIni[sec]].icone;
#define VR(a,b,c,d) (GfxRect){x+(a)*s,y+(b)*s,(c)*s,(d)*s}
#define BOX(a,b,c,d) gfx_cor(VR(a,b,c,d),0.08f,1,1,1,0.08f)
#define ICON(n,a,b,d) ajIcone(n,x+(a)*s,y+(b)*s,(d)*s,0.82f)
  ajNeutro((GfxRect){x,y,w,h},22,0.04f,0.106f,0.110f,0.129f,1);
  // Distinct silhouettes: dashboard, posters, clapper, dossier, player,
  // captions, palette, programme guide, profiles, diagnostics, manual.
  switch (sec) {
    case 0:
      BOX(35,35,55,225); ICON("aj_panel-left",46,51,32);
      BOX(104,35,315,100); ICON("aj_layout-dashboard",230,55,60);
      for(i=0;i<4;i++) { BOX(104+i*81,150,68,110); ICON("aj_image",119+i*81,183,38); }
      break;
    case 1:
      for(i=0;i<3;i++) { float cy=i==1?35:58, ch=i==1?225:180;
        BOX(44+i*124,cy,106,ch); ICON("aj_image",67+i*124,cy+45,60);
        BOX(60+i*124,cy+ch-43,74,8); }
      break;
    case 2:
      BOX(40,42,374,214); BOX(40,42,374,44);
      for(i=0;i<6;i++) ICON("aj_clapperboard",52+i*60,49,30);
      ICON("aj_play",170,107,84); ICON("aj_audio-lines",334,202,30);
      BOX(65,230,220,6); break;
    case 3:
      BOX(42,40,113,212); ICON("aj_file-text",65,100,66);
      for(i=0;i<4;i++) BOX(178,48+i*40,230-i*28,12);
      for(i=0;i<5;i++) ICON("aj_star",178+i*44,221,29);
      break;
    case 4:
      BOX(38,38,380,220); ICON("aj_circle-play",182,79,92);
      BOX(61,210,331,7); ICON("aj_audio-lines",64,231,22);
      ICON("aj_captions",367,231,22); break;
    case 5:
      BOX(40,40,240,106); ICON("aj_languages",65,60,66);
      BOX(167,157,248,100); ICON("aj_captions",191,178,56);
      BOX(267,186,120,10); BOX(267,213,90,10);
      ICON("aj_audio-lines",70,185,60); break;
    case 6:
      ICON("aj_palette",47,56,118);
      for(i=0;i<3;i++) { BOX(202+i*66,54,48,48); ICON(i==0?"aj_type":i==1?"aj_clock":"aj_layers",211+i*66,63,30); }
      BOX(197,140,218,111); ICON("aj_sliders-horizontal",264,158,74); break;
    case 7:
      ICON("aj_tv-minimal-play",46,87,112);
      for(i=0;i<3;i++) { BOX(181,43+i*73,48,55);
        for(j=0;j<2;j++) BOX(241+j*87,43+i*73,77,55); }
      break;
    case 8:
      ICON("aj_user-round",54,80,106);
      ICON("aj_cloud",221,48,74); ICON("aj_plug",332,158,60);
      BOX(182,143,95,4); BOX(274,145,4,48);
      ICON("aj_shield-check",211,202,52); break;
    case 9:
      ICON("aj_gauge",45,60,126);
      for(i=0;i<4;i++) { BOX(205,49+i*53,208,24); BOX(205,49+i*53,48+i*43,24); }
      ICON("aj_memory-stick",80,212,52); break;
    default:
      BOX(48,40,165,219); BOX(225,40,165,219);
      ICON("aj_book-open",155,100,123);
      ICON("aj_info",74,62,38); ICON("aj_download",326,199,38); break;
  }
  // One identity tile, never a category-dependent color or a focus ring.
  BOX(454,42,112,112); ICON(ic,478,66,64);
  if (op >= 0) {
    char v[160];
    TxtLinha t;
    // Action values may be URLs, usernames or credentials: never put them
    // in decorative previews. Their existing editor retains its safeguards.
    if (OPCOES[op].tipo == OP_ESCOLHA || OPCOES[op].tipo == OP_NUMERO)
      uxValorTexto(op, valor[op], v, sizeof v);
    else snprintf(v,sizeof v,"%s",i18n(uxBloco(op)[0]?uxBloco(op):uxCaminho(op)));
    t = ajTxtC(TXT_V2_24,v,w-48*s);
    txt_desenhar_alpha(t,x+24*s,y+h-28*s-t.h,0.85f);
  }
#undef VR
#undef BOX
#undef ICON
  return h;
}
