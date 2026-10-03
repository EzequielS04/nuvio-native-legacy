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

// Diagramas sem fotos, na mesma tinta e superficie das ilhas: miniaturas da
// tela que a categoria controla, sem contorno, em tres degraus de superficie.
// O acento so marca estado (ligado, selecionado, progresso, "agora"). A
// categoria define a cena; a opcao acrescenta seu proprio glifo e valor.
static float ajVisualCena(int sec, int op, float x, float y, float w) {
  float h = w * 0.5625f, s = w / 600.0f, ox = op >= 0 ? 0.0f : 80.0f, ar, ag, ab;
  int i, j;
  const char *ic = op >= 0 ? ajVisualIcone(op) : TELA[secIni[sec]].icone;
#define PX(a) (x + ((a) + ox) * s)
#define PY(b) (y + ((b) + 14) * s)
#define RR(a,b,c,d) (GfxRect){PX(a),PY(b),(c)*s,(d)*s}
  // degraus: 1 trilho, 2 cartao, 3 cartao claro; r em fracao da altura
#define SUP(n,a,b,c,d,r) ajNeutro(RR(a,b,c,d),(r)*(d)*s,(n)==1?0.055f:(n)==2?0.095f:0.15f,\
    (n)==1?0.115f:(n)==2?0.15f:0.20f,(n)==1?0.12f:(n)==2?0.155f:0.21f,(n)==1?0.14f:(n)==2?0.18f:0.24f,1)
#define PILL(n,a,b,c,d) SUP(n,a,b,c,d,0.5f)
#define BAR(n,a,b,c) SUP(n,a,b,c,8,0.5f)
#define ACC(a,b,c,d,al) gfx_cor(RR(a,b,c,d),0.5f,ar,ag,ab,al)
#define BRI(a,b,c,d,al) gfx_cor(RR(a,b,c,d),0.5f,0.953f,0.949f,0.937f,al)
#define LUZ(a,b,c,d,r,al) gfx_luz_canto(RR(a,b,c,d),(r),(c)*s*0.3f,0,(d)*s*1.1f,1,1,1,al)
#define ICON(n,a,b,d,al) ajIcone(n,PX(a),PY(b),(d)*s,al)
  ajustes_acento(&ar, &ag, &ab);
  ajNeutro((GfxRect){x,y,w,h},22,0.04f,0.106f,0.110f,0.129f,1);
  switch (sec) {
    case 0: {   // Tela inicial: barra lateral, destaque e fileiras de cartazes
      SUP(1,28,34,46,236,0.2f);
      for(i=0;i<5;i++) { SUP(i==0?3:2,39,50+i*42,24,24,0.3f); }
      ACC(31,56,4,12,1);
      SUP(2,92,34,340,110,0.09f); LUZ(92,34,340,110,0.09f,0.10f);
      BRI(110,86,150,10,0.75f); BAR(3,110,104,110); BAR(2,110,118,80);
      PILL(3,110,50,56,20);
      for(i=0;i<5;i++) { float bx=92+i*70; SUP(i==1?3:2,bx,160,62,88,0.09f); LUZ(bx,160,62,88,0.09f,0.07f); }
      break; }
    case 1: {   // Cartazes: tres cartazes (o do meio em foco) e as formas
      for(i=0;i<3;i++) { float f=i==1, bw=f?112:96, bh=f?172:150, bx=70+i*132-(f?8:0), by=f?34:52;
        SUP(f?3:2,bx,by,bw,bh,0.07f); LUZ(bx,by,bw,bh,0.07f,f?0.13f:0.08f);
        BRI(bx+14,by+bh-34,bw*0.6f,8,f?0.7f:0.35f); BAR(3,bx+14,by+bh-18,bw*0.4f); }
      for(i=0;i<3;i++) { float rr=i==0?0.08f:i==1?0.3f:0.5f; SUP(i==1?3:2,140+i*72,236,52,52,rr); }
      ACC(228,296,20,4,1); break; }
    case 2: {   // Trailers: a janela de video com o trailer tocando
      SUP(2,42,40,350,196,0.06f); LUZ(42,40,350,196,0.06f,0.14f);
      SUP(3,60,56,62,18,0.5f); BRI(70,63,42,4,0.6f);
      gfx_cor(RR(194,102,62,62),0.5f,0.953f,0.949f,0.937f,0.92f);
      gfx_icone((GfxRect){PX(209),PY(117),34*s,34*s},"aj_play",0.08f,0.08f,0.09f,0.9f);
      BAR(3,60,208,314); ACC(60,208,118,8,1);
      ICON("aj_audio-lines",338,162,26,0.6f);
      break; }
    case 3: {   // Pagina do titulo: capa, titulo, selos e notas
      SUP(2,40,36,360,86,0.1f); LUZ(40,36,360,86,0.1f,0.12f);
      SUP(3,58,64,70,106,0.08f); LUZ(58,64,70,106,0.08f,0.10f);
      BRI(144,128,170,14,0.8f); BAR(3,144,152,120);
      for(i=0;i<3;i++) PILL(2,144+i*58,170,50,16);
      for(i=0;i<3;i++) { SUP(2,58+i*128,196,114,36,0.5f); SUP(3,68+i*128,206,16,16,0.5f); BRI(94+i*128,209,i==0?44:34,10,0.7f); }
      BAR(2,58,248,320); BAR(2,58,266,260); break; }
    case 4: {   // Reproducao: o player, legenda, e a barra de tempo
      SUP(2,38,36,386,222,0.05f); LUZ(38,36,386,222,0.05f,0.12f);
      SUP(3,140,176,182,34,0.35f); BRI(156,185,150,6,0.7f); BRI(172,198,118,5,0.5f);
      PILL(3,62,226,300,8); ACC(62,226,128,8,1); gfx_cor(RR(184,222,16,16),0.5f,1,1,1,0.95f);
      gfx_cor(RR(204,90,54,54),0.5f,0.953f,0.949f,0.937f,0.92f); gfx_icone((GfxRect){PX(217),PY(103),28*s,28*s},"aj_play",0.08f,0.08f,0.09f,0.9f);
      break; }
    case 5: {   // Idiomas e legendas: tres linhas, cada uma com o codigo escolhido
      static const char *const cod[3] = { "PT", "EN", "ES" };
      static const char *const ico[3] = { "aj_languages", "aj_audio-lines", "aj_captions" };
      for(i=0;i<3;i++) { float by=40+i*76;
        SUP(2,40,by,370,62,0.3f); SUP(3,54,by+11,40,40,0.5f); ICON(ico[i],61,by+18,26,0.85f);
        BAR(3,112,by+20,i==0?120:96); BAR(3,112,by+36,70);
        for(j=0;j<3;j++) { float cx=282+j*40; TxtLinha tl=ajTxt(TXT_MINI,cod[(i+j)%3]);
          SUP(j==0?3:1,cx,by+14,34,34,0.45f);
          if(j==0) ACC(cx,by+14,34,34,0.22f);
          txt_desenhar_alpha(tl,PX(cx+17)-tl.w*0.5f,PY(by+31)-tl.h*0.5f,j==0?1.0f:0.55f); } }
      break; }
    case 6: {   // Aparencia: paleta de cores, vidro, tipo e relogio
      for(i=0;i<6;i++) { unsigned c=TEMA_ACENTO[1+i].fill;
        gfx_cor(RR(44+i*40,44,32,32),0.5f,((c>>16)&255)/255.0f,((c>>8)&255)/255.0f,(c&255)/255.0f,0.95f); }
      SUP(2,44,100,140,120,0.1f); LUZ(44,100,140,120,0.1f,0.14f);
      SUP(3,100,140,140,96,0.1f); BRI(116,160,70,8,0.7f); BAR(2,116,178,98); BAR(2,116,194,70);
      SUP(2,262,100,150,88,0.1f); 
      ICON("aj_type",284,114,36,0.9f); BAR(3,330,128,64); BAR(3,330,144,44);
      PILL(2,262,204,92,36); ICON("aj_clock",274,211,22,0.8f); BRI(302,218,36,8,0.7f);
      ACC(44,82,32,4,1);
      PILL(2,364,204,48,36); ACC(374,215,28,14,1);
      break; }
    case 7: {   // TV ao vivo: o guia de programacao
      for(i=0;i<5;i++) { float by=44+i*46;
        SUP(2,34,by,44,38,0.2f); BAR(3,44,by+15,24);
        for(j=0;j<2;j++) { float bx=j==0?88:88+(i%2?112:84)+8, bw=j==0?(i%2?104:76):(i%3?130:98);
          SUP(i==2&&j==0?3:2,bx,by,bw,38,0.2f); BAR(i==2&&j==0?2:3,bx+12,by+15,bw*0.55f); } }
      gfx_cor(RR(171,38,3,232),0.05f,ar,ag,ab,0.9f); ACC(166,28,13,13,1);
      break; }
    case 8: {   // Contas e servicos: perfis e o que esta conectado
      for(i=0;i<3;i++) { float f=i==0, d=f?66:52, bx=60+i*92+(f?0:7), by=f?40:48;
        gfx_cor(RR(bx,by,d,d),0.5f,1,1,1,f?0.22f:0.11f); ICON("aj_user-round",bx+d*0.22f,by+d*0.22f,d*0.56f,f?0.9f:0.5f); }
      for(i=0;i<3;i++) { static const char *const sv[3]={"aj_cloud","aj_plug","aj_key-round"};
        SUP(2,40,140+i*40,370,34,0.5f);
        ICON(sv[i],54,146+i*40,18,0.8f); BAR(3,86,153+i*40,i==0?110:80);
        if(i<2) ACC(380,152+i*40,14,14,1); else gfx_cor(RR(380,152+i*40,14,14),0.5f,1,1,1,0.18f); }
      break; }
    case 9: {   // Desempenho: o medidor e as barras de uso
      static const char *const ic9[4]={"aj_memory-stick","aj_zap","aj_images","aj_database"};
      static const float pc[4]={0.46f,0.72f,0.34f,0.58f};
      SUP(2,40,50,140,140,0.5f);
      ICON("aj_gauge",62,72,96,0.85f);
      SUP(1,80,206,60,20,0.5f); BRI(94,213,32,6,0.6f);
      for(i=0;i<4;i++) { float by=50+i*52; ICON(ic9[i],208,by+4,22,0.75f);
        PILL(1,244,by+6,170,12); ACC(244,by+6,170*pc[i],12,i==1?1.0f:0.55f); }
      break; }
    default: {  // Sobre e ajuda: o guia, a versao e a atualizacao
      SUP(2,40,40,180,220,0.06f); SUP(2,228,40,180,220,0.06f);
      for(i=0;i<5;i++) { BAR(3,60,70+i*28,i==4?70:140); BAR(3,248,70+i*28,i==2?80:140); }
      PILL(3,60,214,76,26); BRI(74,223,48,8,0.7f);
      SUP(3,248,206,140,40,0.5f); ICON("aj_download",262,215,22,0.85f); ACC(380,220,10,10,1); BAR(2,292,222,64);
      break; }
  }
  if (op >= 0) {   // a identidade da opcao (nunca de categoria): uma peca so
    char v[160];
    TxtLinha t;
    SUP(2,462,38,96,96,0.28f); ICON(ic,486,62,48,0.85f);
    // Valores de acao podem ser URL, usuario ou credencial: nunca na previa.
    if (OPCOES[op].tipo == OP_ESCOLHA || OPCOES[op].tipo == OP_NUMERO)
      uxValorTexto(op, valor[op], v, sizeof v);
    else snprintf(v,sizeof v,"%s",i18n(uxBloco(op)[0]?uxBloco(op):uxCaminho(op)));
    t = ajTxtC(TXT_V2_24,v,w-48*s);
    txt_desenhar_alpha(t,x+24*s,y+h-28*s-t.h,0.85f);
  }
#undef PX
#undef PY
#undef RR
#undef SUP
#undef PILL
#undef BAR
#undef ACC
#undef BRI
#undef LUZ
#undef ICON
  return h;
}
