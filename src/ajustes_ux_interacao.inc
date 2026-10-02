// Interação do seletor: só OK aplica; foco e rascunho nunca persistem.
static int uxVoltar(SDL_Keycode k) {
  return k == SDLK_ESCAPE || k == SDLK_AC_BACK || k == SDLK_BACKSPACE || k == SDLK_DELETE;
}
static int uxOk(SDL_Keycode k) { return k == SDLK_RETURN || k == SDLK_KP_ENTER; }
static void uxNotificar(const char *s) {
  snprintf(uxAviso, sizeof uxAviso, "%s", s);
  uxAvisoAte = SDL_GetTicks() + 4500;
}
static void uxCancelar(void) {
  uxEditor = uxRestaurar = uxConfirmar = uxRodape = uxAvisoRisco = 0;
  emEdicao = 0;
}
static void uxListarDiferencas(void) {
  int i;
  uxNDifs = 0;
  for (i = 0; i < AJ_N_TELA; i++)
    if (TELA[i].tipo == IT_OPC && uxDiferente(TELA[i].op)) uxDifs[uxNDifs++] = TELA[i].op;
  if (uxDifFoco >= uxNDifs) uxDifFoco = uxNDifs ? uxNDifs - 1 : 0;
}
static int uxRequisito(int op) {
  switch (op) {
    case AJ_VIDRO_CONTORNO: return AJ_VIDRO;
    case AJ_RAIL: case AJ_RAIL_BLUR: return AJ_RAIL_MODERNA;
    case AJ_HERO_CHEIO: return AJ_HOME_LAYOUT;
    case AJ_HERO_CATALOGOS: return AJ_HERO;
    case AJ_HERO_TRAILER_ESPERA: return AJ_HERO_TRAILER;
    case AJ_HERO_TRAILER_SOM:
      return !lig(AJ_HERO_TRAILER) ? AJ_HERO_TRAILER : -1;
    case AJ_DET_TRAILER_SOM:
      return !lig(AJ_DET_TRAILER_AUTO) ? AJ_DET_TRAILER_AUTO : -1;
    case AJ_EXPANDIR_ATRASO: case AJ_FOCO_TRAILER: return AJ_EXPANDIR;
    case AJ_CW_OK: case AJ_CW_FONTE: case AJ_CW_ESTILO: case AJ_CW_THUMB:
    case AJ_CW_FURTHEST: case AJ_CW_NAO_EXIBIDOS: case AJ_CW_ORDEM:
    case AJ_CW_CONCLUIDO: return AJ_CW_LIGADO;
    case AJ_CW_BLUR_PROX: return !lig(AJ_CW_LIGADO) ? AJ_CW_LIGADO : AJ_CW_THUMB;
    case AJ_PROF_BORDA: case AJ_PROF_BRILHO: case AJ_PROF_COBERTURA:
    case AJ_PROF_POSTERS: case AJ_PROF_CW: case AJ_PROF_EPS:
    case AJ_PROF_ELENCO: case AJ_PROF_TRAILERS: return AJ_PROF;
    case AJ_SEEKR_FITA: case AJ_SEEKR_AJUSTE: return AJ_SEEKR_LIGADO;
    case AJ_SAIDA_PLAYER: return AJ_RELOGIO;
    case AJ_TMDB_IDIOMA: case AJ_TMDB_ARTE: case AJ_TMDB_FICHA: case AJ_TMDB_DATAS:
    case AJ_TMDB_ELENCO: case AJ_TMDB_PROD: case AJ_TMDB_REDES: case AJ_TMDB_EPS:
    case AJ_TMDB_TRAILERS: case AJ_TMDB_MAIS: case AJ_TMDB_COL: case AJ_TMDB_CW:
      return AJ_TMDB_LIGADO;
    case AJ_MDB_CHAVE: case AJ_MDB_TRAKT: case AJ_MDB_IMDB: case AJ_MDB_TMDB:
    case AJ_MDB_LETTER: case AJ_MDB_TOMATES: case AJ_MDB_AUDIENCIA:
    case AJ_MDB_META: case AJ_MDB_MAL: return AJ_MDB_LIGADO;
    case AJ_NT_TOMATES: case AJ_NT_AUDIENCIA: case AJ_NT_META: case AJ_NT_METAUSER:
    case AJ_NT_TMDB: case AJ_NT_LETTER: case AJ_NT_MAL: case AJ_NT_EBERT: case AJ_NT_SCORE:
      return lig(AJ_MDB_LIGADO) ? AJ_MDB_CHAVE : AJ_MDB_LIGADO;
    case AJ_POSTER_INST: case AJ_POSTER_TOKEN: case AJ_POSTER_EXTRA:
    case AJ_POSTER_CHAVE: case AJ_POSTER_MODELO: case AJ_POSTER_TESTAR:
    case AJ_ADDON_POSTER: return AJ_POSTER_PROV;
    case AJ_ADDON_LOGO: return lig(AJ_TMDB_LIGADO) ? AJ_TMDB_ARTE : AJ_TMDB_LIGADO;
    default: return -1;
  }
}
static void uxAbrirEditor(int op) {
  if (op < 0 || op >= AJ_N || soLeitura(op)) return;
  uxCancelar();
  uxOp = op; uxOriginal = uxValorAtual(op); uxPendente = uxOriginal;
  uxEditor = inativa(op) ? 2 : 1;
}
static int uxRiscoFinal(int op, int novo) {
  if (op == AJ_FIL_LIMITE && novo > FIL_LIMITE_SEGURO && novo > uxValorAtual(op))
    return SEG_AVISO_FILEIRAS;
  if (op == AJ_ITENS_FILEIRA && novo > 0 && novo > uxValorAtual(op)) return SEG_AVISO_ITENS;
  return 0;
}
static void uxAplicar(void) {
  int antes = uxValorAtual(uxOp), bit;
  // Conta/sync pode ter mudado o valor enquanto o seletor estava aberto.
  if (antes != uxOriginal) {
    uxOriginal = uxPendente = antes;
    uxRestaurar = uxAvisoRisco = uxConfirmar = 0;
    uxNotificar("O valor mudou em outra tela. Confira antes de confirmar.");
    return;
  }
  if (!uxRestaurar && inativa(uxOp)) { uxEditor = 2; return; }
  bit = uxRiscoFinal(uxOp, uxPendente);
  if (bit && !seguro_aviso_visto(bit)) {
    uxAvisoRisco = bit; uxConfirmar = 0; return;
  }
  if (!definirValorDireto(uxOp, uxPendente)) {
    uxNotificar("Não foi possível salvar. O valor anterior foi mantido.");
    return;
  }
  riscoNotar(uxOp, antes);
  uxCancelar();
  uxNotificar(uxOp == AJ_PERFIL_PESQ ? "Pedido enviado ao perfil." : "Ajuste salvo nesta TV.");
  // A lista de diferenças permanece estável até sair dela, mesmo após restaurar.
}
static int uxEvento(SDL_Keycode k) {
  if (uxEditor) {
    if (uxVoltar(k)) {
      if (uxRestaurar || uxAvisoRisco) { uxRestaurar = uxAvisoRisco = uxConfirmar = 0; }
      else uxCancelar();
      return 1;
    }
    if (uxRestaurar || uxAvisoRisco) {
      if (k == SDLK_LEFT || k == SDLK_RIGHT) uxConfirmar = k == SDLK_LEFT;
      if (uxOk(k)) {
        if (!uxConfirmar) { uxRestaurar = uxAvisoRisco = 0; return 1; }
        if (uxAvisoRisco) { seguro_aviso_marcar(uxAvisoRisco); uxAvisoRisco = 0; }
        if (uxRestaurar) uxPendente = uxValorPadrao(uxOp);
        uxAplicar();
      }
      return 1;
    }
    if (k == SDLK_RIGHT && uxTemPadrao(uxOp)) { uxRodape = 1; return 1; }
    if (k == SDLK_LEFT) { uxRodape = 0; return 1; }
    if (uxOk(k)) {
      if (uxRodape) { uxRestaurar = 1; uxConfirmar = 0; return 1; }
      if (uxEditor == 2) {
        int req = uxRequisito(uxOp);
        if (req >= 0) { uxRetornarOp = uxOp; uxCancelar(); focarOpcao(req); }
        return 1;
      }
      uxAplicar(); return 1;
    }
    if (!uxRodape && uxEditor == 1 && (k == SDLK_UP || k == SDLK_DOWN)) {
      int d = k == SDLK_DOWN ? 1 : -1;
      if (OPCOES[uxOp].tipo == OP_NUMERO)
        uxPendente = limita(uxOp, uxPendente + d * OPCOES[uxOp].passo);
      else {
        uxPendente += d;
        if (uxPendente < 0) uxPendente = 0;
        if (uxPendente >= nValores(uxOp)) uxPendente = nValores(uxOp) - 1;
      }
    }
    return 1;
  }
  if (uxVoltar(k) && uxVeioBusca && uxRetornarOp < 0) {
    uxPediuBusca = 2; uxVeioBusca = 0; return 1;
  }
  if (focoIndice) {
    if (uxVoltar(k) || k == SDLK_LEFT) { sair = 1; return 1; }
    if (k == SDLK_UP || k == SDLK_DOWN) {
      int dest = uxIndice + (k == SDLK_DOWN ? 1 : -1);
      if (dest < 0 || dest >= nSecoes + 2) return 1;
      if (dest >= 2) focarSecao(dest - 2);
      uxIndice = dest; uxVeioBusca = 0;
      if (dest == 1) uxListarDiferencas();
      return 1;
    }
    if (uxOk(k) || k == SDLK_RIGHT) {
      if (uxIndice == 0) { uxVeioBusca = 0; uxPediuBusca = 1; return 1; }
      focoIndice = 0;
      if (uxIndice == 1) uxListarDiferencas();
      return 1;
    }
    return 1;
  }
  if (uxIndice == 1) {
    if (uxVoltar(k) || k == SDLK_LEFT) { focoIndice = 1; return 1; }
    if (k == SDLK_UP && uxDifFoco > 0) uxDifFoco--;
    if (k == SDLK_DOWN && uxDifFoco + 1 < uxNDifs) uxDifFoco++;
    if (uxOk(k) && uxNDifs) uxAbrirEditor(uxDifs[uxDifFoco]);
    return 1;
  }
  if (uxVoltar(k) && uxRetornarOp >= 0) {
    int op = uxRetornarOp; uxRetornarOp = -1; focarOpcao(op); return 1;
  }
  if (k == SDLK_UP || k == SDLK_DOWN || k == SDLK_LEFT ||
      k == SDLK_PAGEUP || k == SDLK_PAGEDOWN) uxRetornarOp = -1;
  if (uxVoltar(k) || k == SDLK_LEFT) {
    focoIndice = 1; uxCabecalho = 0; sairArmado = 0; return 1;
  }
  if (uxCabecalho) {
    if (uxOk(k)) {
      uxAvancados[secAtual] = !uxAvancados[secAtual];
      // Nunca deixar um foco escondido depois de recolher os avançados.
      if (!focavel(focoItem)) {
        focar(primeiroDaSecao(secAtual)); uxCabecalho = 1;
      }
    } else if (k == SDLK_DOWN) { focar(primeiroDaSecao(secAtual)); }
    return 1;
  }
  if (k == SDLK_UP || k == SDLK_DOWN) {
    int dir = k == SDLK_DOWN ? 1 : -1, i;
    for (i = focoItem + dir; i > secIni[secAtual] && i < secFim(secAtual); i += dir)
      if (focavel(i)) { focar(i); return 1; }
    if (dir < 0) uxCabecalho = 1;
    return 1;
  }
  if (k == SDLK_RIGHT) return 1;
  if (uxOk(k) && focoOp >= 0 && inativa(focoOp)) { uxAbrirEditor(focoOp); return 1; }
  return 0; // Ações existentes: conta, teclado, fileiras, diagnóstico.
}
