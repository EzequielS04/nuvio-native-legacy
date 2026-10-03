// A TELA DE AJUSTES NO GLASS UI: inspetor, lista, diferencas e os modais
// (editor de escolha longa, editor numerico, confirmacao, dependencia).
// Incluido por ajustes_ux_desenho.inc, depois das pecas e das previas.

// --- INSPETOR -----------------------------------------------------------------------
static const char *ajTipoRotulo(int op) {
  if (inativa(op)) return "Indisponível agora";
  if (op == AJ_ENVIAR_LOG) return "Ação";
  switch (ajTipo(op)) {
    case AJL_TOG: return "Ligar ou desligar";
    case AJL_ACAO: return "Abre uma tela";
    case AJL_LER: return "Informação";
    default: return "Personalizar";
  }
}
static const char *ajIconeOpcao(int op) {
  switch (op) {
    case AJ_ESPACO: case AJ_TEX_MB: return "aj_memory-stick";
    case AJ_RELOGIO: case AJ_RELOGIO_POS: return "aj_clock";
    case AJ_TRAKT: case AJ_SIMKL: return "aj_plug";
    case AJ_ENVIAR_LOG: return "aj_send";
    case AJ_VER_REGISTRO: return "aj_scroll-text";
    case AJ_MEDIDOR: return "aj_activity";
    default: break;
  }
  return TELA[secIni[uxSecaoDe(op)]].icone;
}
// As teclas que valem AGORA, pelo tipo da linha em foco.
static int ajDicasDe(int op, AjDica *d) {
  if (focoIndice) {
    d[0] = (AjDica){ "↑ ↓", "Navegar" }; d[1] = (AjDica){ "OK", "Abrir" };
    d[2] = (AjDica){ "Voltar", "Sair" };
    return 3;
  }
  if (uxCabecalho) {
    d[0] = (AjDica){ "OK", "Mostrar ou ocultar" }; d[1] = (AjDica){ "↓", "Lista" };
    d[2] = (AjDica){ "←", "Categorias" };
    return 3;
  }
  if (op >= 0 && ajEditandoLinha(op)) {
    d[0] = (AjDica){ "← →", "Escolher" }; d[1] = (AjDica){ "OK", "Confirmar" };
    d[2] = (AjDica){ "Voltar", "Cancelar" };
    return 3;
  }
  if (op >= 0 && ajSegmentada(op) && !inativa(op)) {
    d[0] = (AjDica){ "OK", "Mudar" }; d[1] = (AjDica){ "← →", "Escolher" };
    d[2] = (AjDica){ "Voltar", "Categorias" };
    return 3;
  }
  { int n = 0;
    if (op >= 0 && inativa(op) && uxRequisito(op) >= 0) d[n++] = (AjDica){ "OK", "Mostrar" };
    else if (op == AJ_ENVIAR_LOG && !inativa(op)) d[n++] = (AjDica){ "OK", "Enviar" };
    else if (op >= 0 && !inativa(op)) switch (ajTipo(op)) {
      case AJL_TOG: d[n++] = (AjDica){ "OK", "Mudar" }; break;
      case AJL_ESC: d[n++] = (AjDica){ "OK", op == AJ_TEMA ? "Mudar" : "Escolher" }; break;
      case AJL_NUM: d[n++] = (AjDica){ "OK", "Ajustar" }; break;
      case AJL_ACAO: d[n++] = (AjDica){ "OK", "Abrir" }; break;
      default: break;
    }
    d[n++] = (AjDica){ "↑ ↓", "Navegar" };
    d[n++] = (AjDica){ "←", "Categorias" };
    return n; }
}
static void ajDesenharInspetor(void) {
  GfxRect ir = ajInspR();
  float x = ir.x + 32.0f, w = ir.w - 64.0f, y = ir.y + 32.0f;
  int categoria = focoIndice || uxCabecalho;
  int op = categoria ? -1 : focoOp;
  AjDica d[4];
  int nd;
  ajIlha(ir, 36.0f, 1, 0);
  if (op < 0 && !categoria) op = -1;
  { int opPv = op >= 0 ? op : -1;
    if (opPv < 0) {   // categoria: a previa da primeira opcao dela
      int it = primeiroDaSecao(secAtual);
      if (TELA[it].tipo == IT_OPC) opPv = TELA[it].op;
    }
    if (opPv >= 0) {
      // Editando na linha, a previa ja mostra o valor PENDENTE (so desenho).
      int salvo = valor[opPv], ed = ajEditandoLinha(opPv);
      float ph;
      if (ed) valor[opPv] = uxPendente;
      ph = ajPrevia(opPv, x, y, w);
      if (ed) valor[opPv] = salvo;
      if (ph > 0) y += ph;
    } }
  x += 6.0f; w -= 12.0f;
  y += 26.0f;
  { const char *tipo = categoria ? "Categoria" : ajTipoRotulo(op);
    const char *ic = categoria ? TELA[secIni[secAtual]].icone : ajIconeOpcao(op);
    ajIcone(ic, x, y - 1, 20, 0.45f);
    ajCaps(TXT_MINI, tipo, 2.1f, x + 30, y, 0.45f);
    y += 18.0f + 10.0f; }
  { const char *t = categoria ? TELA[secIni[secAtual]].titulo : rotuloOpcao(op);
    y += ajBloco(TXT_AJ_INSP, t, x, y, w, 38.4f, 1, 2); }
  y += 12.0f;
  y += ajBloco(TXT_AJ_SUB, categoria ? SECAO_AJUDA[secAtual] : uxResumo(op), x, y, w, 28.5f, 0.62f, 7);
  if (!categoria) {
    const char *ef = efeitoOpcao(op);
    if (ef && ef[0]) { y += 18.0f; y += ajEf("aj_info", i18n(ef), x, y, w, 1); }
    if (uxTemPadrao(op)) {
      char padrao[160];
      uxValorTexto(op, uxValorPadrao(op), padrao, sizeof padrao);
      y += 18.0f;
      y += ajMeta(i18n("Vale para"), i18n(uxEscopo(op)), x, y, w, 1);
      y += ajMeta(i18n("Padrão"), padrao, x, y, w, 1);
    }
    // ENVIAR REGISTRO: o ultimo envio, o ultimo codigo e o destino.
    if (op == AJ_ENVIAR_LOG) {
      AvisosEnvio env;
      char v[96], ha[48];
      time_t ult;
      int autom;
      avisos_envio_info(&env);
      autom = env.autoQuando > env.quando;
      ult = autom ? env.autoQuando : env.quando;
      y += 18.0f;
      if (ult) {
        long s = (long)(time(NULL) - ult);
        if (s < 60) snprintf(ha, sizeof ha, i18n("há %ld s"), s);
        else if (s < 3600) snprintf(ha, sizeof ha, i18n("há %ld min"), s / 60);
        else snprintf(ha, sizeof ha, i18n("há %ld h"), s / 3600);
        snprintf(v, sizeof v, "%s · %s", ha, autom ? i18n("automático") : i18n("manual"));
      } else snprintf(v, sizeof v, "%s", i18n("nenhum nesta sessão"));
      y += ajMeta(i18n("Último envio"), v, x, y, w, 1);
      if (env.ultimoCodigo[0]) {
        struct tm a, b;
        time_t agora = time(NULL);
        localtime_r(&env.ultimoCodigoQuando, &a);
        localtime_r(&agora, &b);
        if (a.tm_yday == b.tm_yday && a.tm_year == b.tm_year)
          snprintf(v, sizeof v, i18n("%s · hoje %02d:%02d"), env.ultimoCodigo, a.tm_hour, a.tm_min);
        else if (b.tm_yday - a.tm_yday == 1 && a.tm_year == b.tm_year)
          snprintf(v, sizeof v, i18n("%s · ontem %02d:%02d"), env.ultimoCodigo, a.tm_hour, a.tm_min);
        else snprintf(v, sizeof v, i18n("%s · %02d/%02d %02d:%02d"), env.ultimoCodigo, a.tm_mday, a.tm_mon + 1, a.tm_hour, a.tm_min);
        y += ajMeta(i18n("Último código"), v, x, y, w, 1);
      }
      y += ajMeta(i18n("Destino"), env.disponivel ? i18n("servidor de registros do app") : i18n("indisponível nesta versão"), x, y, w, 1);
    }
  }
  nd = ajDicasDe(op, d);
  ajDicas(d, nd, x - 6.0f, ir.y + ir.h - 32.0f, w + 12.0f, 1);
}

// --- ALTURAS DA LISTA ------------------------------------------------------------
// Tem de casar com o laco de desenho: e o que a rolagem (ajustes_atualizar) usa.
static int ajPrimeiroVisivel(int i) {
  int j;
  for (j = i - 1; j > secIni[secDoItem[i]]; j--) if (visivel(j)) return 0;
  return 1;
}
static float ajAlturaItem(int i) {
  if (!visivel(i)) return 0.0f;
  switch (TELA[i].tipo) {
    case IT_SEC: return 0.0f;
    case IT_ROT: return (ajPrimeiroVisivel(i) ? 14.0f : 16.0f) + 38.0f + 6.0f;
    case IT_GRP: return AJI_LN_H + AJI_LN_GAP;
    default: return ajLinhaH(i) + AJI_LN_GAP;
  }
}

// --- A FOLHA ---------------------------------------------------------------------
// Alfa da mascara da lista: some nos ultimos 80 px de baixo e, rolada, nos 60
// de cima (o mask-image do mockup), medido no centro da peca.
static float ajMascara(float yc, float topo, float base) {
  float a = 1.0f;
  if (yc > base - 80.0f) a = (base - yc) / 80.0f;
  if (scrollY > 1.0f && yc < topo + 60.0f) { float b = (yc - topo) / 60.0f; if (b < a) a = b; }
  return a < 0 ? 0 : a > 1 ? 1 : a;
}
static void ajDesenharFolha(void) {
  GfxRect fr = ajFolhaR();
  int sec = secAtual, i;
  float topo, base, y, aPag = anim_suave(paginaA);
  const Item *s = &TELA[secIni[sec]];
  float wChip = 0;
  ajIlha(fr, 36.0f, 1, 0);
  wChip = ajChipAvancados(fr.x + fr.w - 32.0f, fr.y + 34.0f, uxCabecalho && !focoIndice, aPag);
  topo = ajCabecalhoFolha(fr, "Ajustes", s->titulo, s->sub, wChip + 20.0f, aPag);
  topo = AJ_TOPO;
  base = AJ_BASE;
  gfx_recorte(fr.x, topo, fr.w, base - topo);
  y = topo - scrollY;
  for (i = secIni[sec] + 1; i < secFim(sec); i++) {
    float h = ajAlturaItem(i);
    if (h <= 0.0f) continue;
    if (y + h >= topo - 4 && y <= base + 4) {
      if (TELA[i].tipo == IT_ROT) {
        float yy = y + (ajPrimeiroVisivel(i) ? 14.0f : 16.0f);
        ajDesenharGrupo(i, yy, ajMascara(yy + 19, topo, base) * aPag);
      } else if (TELA[i].tipo == IT_OPC) {
        float lh = ajLinhaH(i);
        ajDesenharLinha(i, y, animItem[i], ajMascara(y + (lh > 150 ? 38 : lh * 0.5f), topo, base) * aPag);
      }
    }
    y += h;
  }
  gfx_sem_recorte();
}

// "Buscar ajuste" e "Diferentes do padrao" no lugar da lista.
static void ajDesenharEspecial(void) {
  GfxRect fr = ajFolhaR(), ir = ajInspR();
  float topo;
  ajIlha(fr, 36.0f, 1, 0);
  if (uxIndice == 0) {
    topo = ajCabecalhoFolha(fr, "Ajustes", "Buscar ajuste",
      "Encontre pelo nome ou pelo que quer mudar. Experimente “legenda”, “trailer” ou “TV lenta”.", 0, 1);
    { TxtLinha t = ajTxt(TXT_AJ_SEG, "Buscar nos ajustes desta TV");
      GfxRect b = { fr.x + 44, topo + 10, 22 + 22 + 10 + t.w + 22, 56 };
      if (focoIndice) ajNeutro(b, 28, 0.08f, AJ_CHIP_SOL, 1);
      else ajPilulaAcento(b, 28, 1);
      ajIcone("aj_search", b.x + 22, b.y + 17, 22, 0.85f);
      txt_desenhar_alpha(t, b.x + 54, b.y + (56 - t.h) * 0.5f, 0.85f); }
    ajIlha(ir, 36.0f, 1, 0);
    { AjDica d[3] = { { "OK", "Buscar" }, { "↑ ↓", "Navegar" }, { "Voltar", "Sair" } };
      ajDicas(d, 3, ir.x + 32, ir.y + ir.h - 32, ir.w - 64, 1); }
    return;
  }
  topo = ajCabecalhoFolha(fr, "Ajustes", "Diferentes do padrão",
    "Valores diferentes da configuração de fábrica. Podem ter vindo da conta ou de outra versão do app.", 0, 1);
  { float y = topo, base = AJ_BASE;
    int primeiro = uxDifFoco > 8 ? uxDifFoco - 8 : 0, i;
    float x = fr.x + 18, w = fr.w - 36;
    gfx_recorte(fr.x, topo, fr.w, base - topo);
    if (!uxNDifs) {
      TxtLinha t = ajTxt(TXT_AJ_ROTULO, "Tudo está no padrão");
      txt_desenhar(t, x + 22, y + 20);
      y += 76;
    }
    for (i = primeiro; i < uxNDifs; i++) {
      char atual[160], padrao[160], det[300];
      int op = uxDifs[i], foco = !focoIndice && i == uxDifFoco;
      GfxRect r = { x, y, w, 80 };
      float xd = x + w - 22, a = ajMascara(y + 40, topo, base);
      TxtLinha la, lp, lr, ls;
      if (y > base) break;
      if (foco) ajFocoLinha(r, 22, 1, a);
      uxValorTexto(op, uxValorAtual(op), atual, sizeof atual);
      uxValorTexto(op, uxValorPadrao(op), padrao, sizeof padrao);
      la = ajTxtC(TXT_AJ_VALOR18, atual, 200);
      lp = ajTxtC(TXT_AJ_VALOR18, padrao, 200);
      txt_desenhar_alpha(la, xd - la.w, y + 40 - la.h * 0.5f, (foco ? 1.0f : 0.8f) * a);
      xd -= la.w + 10;
      { float sr, sg, sb; int sw = op == AJ_TEMA;
        if (sw) {
          ajustes_acento(&sr, &sg, &sb);
          gfx_cor((GfxRect){ xd - 22, y + 29, 22, 22 }, 0.5f, sr, sg, sb, a);
          xd -= 32;
        } }
      ajIcone("aj_chevron-right", xd - 18, y + 31, 18, 0.35f * a);
      xd -= 18 + 12;
      txt_desenhar_alpha(lp, xd - lp.w, y + 40 - lp.h * 0.5f, 0.4f * a);
      gfx_cor((GfxRect){ xd - lp.w, y + 40, lp.w, 1.5f }, 0, 0.953f, 0.949f, 0.937f, 0.25f * a);
      xd -= lp.w;
      if (op == AJ_TEMA) {
        float pr = TEMA_ACENTO[uxValorPadrao(op) % AJ_N_TEMAS].r, pg = TEMA_ACENTO[uxValorPadrao(op) % AJ_N_TEMAS].g, pb = TEMA_ACENTO[uxValorPadrao(op) % AJ_N_TEMAS].b;
        gfx_cor((GfxRect){ xd - 32, y + 29, 22, 22 }, 0.5f, pr, pg, pb, 0.4f * a);
        xd -= 32;
      }
      lr = ajTxtC(TXT_AJ_ROTULO, rotuloOpcao(op), xd - x - 44);
      snprintf(det, sizeof det, "%s · %s", i18n(uxCaminho(op)), i18n(uxEscopo(op)));
      ls = ajTxtC(TXT_ILHA_GENERO, det, xd - x - 44);
      txt_desenhar_alpha(lr, x + 22, y + 40 - (lr.h + 4 + ls.h) * 0.5f, (foco ? 1.0f : 0.86f) * a);
      txt_desenhar_alpha(ls, x + 22, y + 40 - (lr.h + 4 + ls.h) * 0.5f + lr.h + 4, 0.42f * a);
      y += 84;
    }
    if (y < base - 40) {
      TxtLinha n = ajTxt(TXT_ILHA_GENERO, "Seus serviços e credenciais não entram nesta lista.");
      txt_desenhar_alpha(n, x + 22, y + 20, 0.42f);
    }
    gfx_sem_recorte(); }
  // Inspetor: a opcao em foco, padrao e atual lado a lado.
  ajIlha(ir, 36.0f, 1, 0);
  if (uxNDifs) {
    int op = uxDifs[uxDifFoco], salvo = valor[op], i;
    float x = ir.x + 32, w = ir.w - 64, y = ir.y + 32, cw = (w - 14) * 0.5f, ph = 0;
    char padrao[160], atual[160], t[220];
    uxValorTexto(op, uxValorPadrao(op), padrao, sizeof padrao);
    uxValorTexto(op, salvo, atual, sizeof atual);
    for (i = 0; i < 2; i++) {
      float h;
      valor[op] = i == 0 ? uxValorPadrao(op) : salvo;
      h = ajPreviaCompacta(op, x + i * (cw + 14), y, cw);
      if (h > ph) ph = h;
    }
    valor[op] = salvo;
    if (ph > 0) {
      TxtLinha c;
      snprintf(t, sizeof t, i18n("Padrão · %s"), padrao);
      c = ajTxtC(TXT_ILHA_APOIO, t, cw);
      txt_desenhar_alpha(c, x, y + ph + 10, 0.45f);
      snprintf(t, sizeof t, i18n("Atual · %s"), atual);
      c = ajTxtC(TXT_ILHA_APOIO, t, cw);
      txt_desenhar(c, x + cw + 14, y + ph + 10);
      y += ph + 10 + 20;
    }
    x += 6; w -= 12; y += 26;
    { char k[40];
      snprintf(k, sizeof k, i18n("%d de %d"), uxDifFoco + 1, uxNDifs);
      ajIcone("aj_rotate-ccw-clock", x, y - 1, 20, 0.45f);
      ajCaps(TXT_MINI, k, 2.1f, x + 30, y, 0.45f);
      y += 28; }
    y += ajBloco(TXT_AJ_INSP, rotuloOpcao(op), x, y, w, 38.4f, 1, 2) + 12;
    y += ajBloco(TXT_AJ_SUB, uxResumo(op), x, y, w, 28.5f, 0.62f, 6) + 18;
    y += ajMeta(i18n("Vale para"), i18n(uxEscopo(op)), x, y, w, 1);
    { const char *b = uxBloco(op);
      char sb[200];
      if (b[0]) snprintf(sb, sizeof sb, "%s › %s", i18n(uxCaminho(op)), i18n(b));
      else snprintf(sb, sizeof sb, "%s", i18n(uxCaminho(op)));
      ajMeta(i18n("Seção"), sb, x, y, w, 1); }
  }
  { AjDica d[3] = { { "OK", "Revisar" }, { "↑ ↓", "Navegar" }, { "←", "Categorias" } };
    ajDicas(d, 3, ir.x + 32, ir.y + ir.h - 32, ir.w - 64, 1); }
}

// --- MODAIS ----------------------------------------------------------------------
// Atras do modal a tela RECUA: as ilhas a 16% e um veu preto de 40% (42% no
// solido) por cima de tudo. O modal e uma ilha centrada com o miolo a 86%.
static void ajVeuModal(void) {
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0, 0, 0, 0, ajVid() ? 0.40f : 0.42f);
}
// Botao pilula 60 (21/600): o focado cheio no acento, o outro neutro 8%.
static float ajBotao(const char *rot, const char *icone, float x, float y, int foco, float a) {
  TxtLinha t = foco ? ajTxtT(TXT_ILHA_ITEM, rot) : ajTxt(TXT_ILHA_ITEM, rot);
  float w = 28 + (icone ? 22 + 10 : 0) + t.w + 28;
  if (foco) ajPilulaAcento((GfxRect){ x, y, w, 60 }, 30, a);
  else ajNeutro((GfxRect){ x, y, w, 60 }, 30, 0.08f, AJ_CHIP_SOL, a);
  if (icone) {
    if (foco) ajIconeT(icone, x + 28, y + 19, 22, a);
    else ajIcone(icone, x + 28, y + 19, 22, 0.88f * a);
  }
  txt_desenhar_alpha(t, x + 28 + (icone ? 32 : 0), y + (60 - t.h) * 0.5f, (foco ? 1.0f : 0.88f) * a);
  return w;
}
// Chip 56 (19/600) com icone: "Restaurar padrao" no rodape dos editores.
static float ajChip(const char *rot, const char *icone, float x, float y, int foco, float a) {
  TxtLinha t = foco ? ajTxtT(TXT_AJ_SEG, rot) : ajTxt(TXT_AJ_SEG, rot);
  float w = 22 + (icone ? 20 + 10 : 0) + t.w + 22;
  if (foco) ajPilulaAcento((GfxRect){ x, y, w, 56 }, 28, a);
  else ajNeutro((GfxRect){ x, y, w, 56 }, 28, 0.08f, AJ_CHIP_SOL, a);
  if (icone) {
    if (foco) ajIconeT(icone, x + 22, y + 18, 20, a);
    else ajIcone(icone, x + 22, y + 18, 20, 0.85f * a);
  }
  txt_desenhar_alpha(t, x + 22 + (icone ? 30 : 0), y + (56 - t.h) * 0.5f, (foco ? 1.0f : 0.85f) * a);
  return w;
}
// Rodape dos editores: fio, "Restaurar padrao" a esquerda e as dicas a direita.
static void ajRodapeEditor(GfxRect m, const AjDica *d, int n) {
  float y = m.y + m.h - 44 - 56;
  gfx_cor((GfxRect){ m.x + 44, y - 22, m.w - 88, 1 }, 0, 1, 1, 1, 0.08f);
  if (uxTemPadrao(uxOp)) ajChip("Restaurar padrão", "aj_rotate-ccw-clock", m.x + 44, y, uxRodape, 1);
  { float w = ajDicasLargura(d, n);
    ajDicasLinha(d, n, m.x + m.w - 44 - w, y + 13, 1); }
}
// Cabecalho dos editores: "Secao · Bloco", o nome da opcao e "Escopo · Padrao: x".
static float ajCabecalhoEditor(GfxRect m) {
  char k[200], linha[320], padrao[160];
  const char *b = uxBloco(uxOp);
  float x = m.x + 44, y = m.y + 44;
  if (b[0]) snprintf(k, sizeof k, "%s · %s", i18n(uxCaminho(uxOp)), i18n(b));
  else snprintf(k, sizeof k, "%s", i18n(uxCaminho(uxOp)));
  ajKicker(k, x, y, 1);
  y += 18 + 8;
  { TxtLinha t = ajTxtC(TXT_ILHA_TITULO, rotuloOpcao(uxOp), m.w - 88);
    txt_desenhar(t, x, y); y += 48; }
  if (ajPadraoVisual(uxOp) >= 0) uxValorTexto(uxOp, ajPadraoVisual(uxOp), padrao, sizeof padrao);
  if (ajPadraoVisual(uxOp) >= 0) snprintf(linha, sizeof linha, i18n("%s · Padrão: %s"), i18n(uxEscopoValor(uxOp, uxPendente)), padrao);
  else snprintf(linha, sizeof linha, "%s", i18n(uxEscopoValor(uxOp, uxPendente)));
  { TxtLinha t = ajTxtC(TXT_AJ_SUB, linha, m.w - 88);
    txt_desenhar_alpha(t, x, y + 8, 0.55f); }
  return y + 8 + 27;
}
// A previa da opcao com o valor PENDENTE (so o desenho; nada e gravado).
static float ajPreviaPendente(float x, float y, float w) {
  int salvo = valor[uxOp];
  float h;
  valor[uxOp] = uxPendente;
  h = ajPreviaEditor(uxOp, x, y, w);
  valor[uxOp] = salvo;
  return h;
}

// Editor de escolha com lista longa.
static void ajEditorEscolha(void) {
  GfxRect m = { 360, 170, 1200, 740 };
  const Opcao *o = &OPCOES[uxOp];
  int n = nValores(uxOp), i, primeiro, vis;
  float y, lx, lw;
  ajIlha(m, 36, 1, 1);
  y = ajCabecalhoEditor(m) + 30;
  lx = m.x + 44; lw = m.w - 88 - 440 - 44;
  vis = (int)((m.y + m.h - 44 - 56 - 22 - 20 - y) / 82.0f);
  if (vis < 3) vis = 3;
  primeiro = uxPendente - vis / 2;
  if (primeiro + vis > n) primeiro = n - vis;
  if (primeiro < 0) primeiro = 0;
  for (i = primeiro; i < n && i < primeiro + vis; i++) {
    char t[160];
    int foco = !uxRodape && i == uxPendente;
    GfxRect r = { lx, y, lw, 76 };
    int fv;
    uxValorTexto(uxOp, i, t, sizeof t);
    if (foco) ajFocoLinha(r, 22, 1, 1);
    fv = marcaDoValor(uxOp, o->valores ? o->valores[i] : NULL);
    if (fv >= 0) marca_formato((FormatoMarca)fv, r.x + 22, r.y + 18, 40, 1, 1, 1, foco ? 0.9f : 0.6f);
    else {
      TxtLinha l = ajTxtC(TXT_AJ_ROTULO, t, lw - 200);
      txt_desenhar_alpha(l, r.x + 22, r.y + (76 - l.h) * 0.5f, foco ? 1.0f : 0.86f);
    }
    if (i == uxOriginal) {
      TxtLinha s = ajTxt(TXT_AJ_18, "Salvo");
      float sx = r.x + r.w - 22 - s.w;
      txt_desenhar_alpha(s, sx, r.y + (76 - s.h) * 0.5f, 0.55f);
      ajIcone("aj_check", sx - 9 - 20, r.y + 28, 20, 0.55f);
    }
    y += 82;
  }
  { float px = m.x + m.w - 44 - 440, py = ajCabecalhoEditor(m) + 30;
    ajPreviaPendente(px, py, 440); }
  { AjDica d[3] = { { "↑ ↓", "Escolher" }, { "OK", "Confirmar" }, { "Voltar", "Cancelar" } };
    ajRodapeEditor(m, d, 3); }
}

// Editor numerico: numero grande, trilho com o padrao e a faixa vigiada.
static void ajEditorNumero(void) {
  GfxRect m = { 300, 0, 1320, 0 };
  const Opcao *o = &OPCOES[uxOp];
  float y, lx, lw, ar, ag, ab;
  char v[64], salvo[64], uni[64];
  int risco = uxOp == AJ_FIL_LIMITE ? FIL_LIMITE_SEGURO : -1;
  m.h = uxOp == AJ_FIL_LIMITE ? 830 : 760; m.y = (NV_TELA_H - m.h) * 0.5f;
  ajustes_acento(&ar, &ag, &ab);
  ajIlha(m, 36, 1, 1);
  y = ajCabecalhoEditor(m) + 26;
  lx = m.x + 44; lw = m.w - 88 - 440 - 50;
  snprintf(v, sizeof v, "%d", uxPendente);
  uxValorTexto(uxOp, uxOriginal, salvo, sizeof salvo);
  if (uxOp == AJ_FIL_LIMITE) snprintf(uni, sizeof uni, "%s", i18n("fileiras"));
  else snprintf(uni, sizeof uni, "%s", o->sufixo ? o->sufixo + (o->sufixo[0] == ' ') : "");
  { GfxRect r = { lx, y, lw, 240 };
    float fr = ajFracao(uxOp, uxPendente), fp = ajPadraoVisual(uxOp) >= 0 ? ajFracao(uxOp, ajPadraoVisual(uxOp)) : -1;
    float tx = r.x + 34, tw = r.w - 68, ty = r.y + 30 + 120 + 34;
    TxtLinha big = ajTxt(TXT_AJ_NUM110, v), u = ajTxt(TXT_PG_RELOGIO, uni);
    char sv[96];
    TxtLinha ls;
    if (uxOp == AJ_HERO_TRAILER_ESPERA) { snprintf(v, sizeof v, "%d,%d", uxPendente / 10, uxPendente % 10); big = ajTxt(TXT_AJ_NUM110, v); }
    ajFocoLinha(r, 22, uxRodape ? 0 : 1, 1);
    txt_desenhar(big, tx, r.y + 30 + (120 - big.h) * 0.5f);
    snprintf(sv, sizeof sv, i18n("salvo: %s"), salvo);
    ls = ajTxt(TXT_AJ_ESTADO, sv);
    txt_desenhar(u, tx + big.w + 26, r.y + 30 + 60 - (u.h + 4 + ls.h) * 0.5f);
    txt_desenhar_alpha(ls, tx + big.w + 26, r.y + 30 + 60 - (u.h + 4 + ls.h) * 0.5f + u.h + 4, 0.5f);
    ajIcone("aj_plus", r.x + r.w - 34 - 30, r.y + 30 + 60 - 30 - 3, 30, 0.7f);
    gfx_cor((GfxRect){ r.x + r.w - 34 - 30, r.y + 30 + 60, 30, 1 }, 0, 1, 1, 1, 0.15f);
    ajIcone("aj_minus", r.x + r.w - 34 - 30, r.y + 30 + 60 + 3, 30, 0.7f);
    gfx_cor((GfxRect){ tx, ty, tw, 8 }, 0.5f, 1, 1, 1, 0.12f);
    if (fr > 0) gfx_cor((GfxRect){ tx, ty, tw * (fr > 1 ? 1 : fr), 8 }, 0.5f, ar, ag, ab, 1);
    if (risco >= o->min && risco < o->max) {
      float fx = ajFracao(uxOp, risco), sx;
      // Faixa vigiada: listras ambar (o repeating-linear-gradient do mockup).
      gfx_recorte(tx + tw * fx, ty, tw * (1 - fx), 8);
      for (sx = tx + tw * fx - 8; sx < tx + tw; sx += 12.0f) {
        int q;
        for (q = 0; q < 4; q++) gfx_cor((GfxRect){ sx + q * 2.0f, ty + q * 2.0f, 6, 2 }, 0, 0.961f, 0.784f, 0.298f, 0.32f);
      }
      gfx_sem_recorte();
    }
    if (fp >= 0) gfx_cor((GfxRect){ tx + tw * fp - 1, ty - 9, 2, 26 }, 0.5f, 0.953f, 0.949f, 0.937f, 0.6f);
    { char t[64]; TxtLinha l;
      float ly = ty + 18;
      snprintf(t, sizeof t, "%d", o->min); l = ajTxt(TXT_ILHA_APOIO, t); txt_desenhar_alpha(l, tx, ly, 0.45f);
      snprintf(t, sizeof t, "%d", o->max); l = ajTxt(TXT_ILHA_APOIO, t); txt_desenhar_alpha(l, tx + tw - l.w, ly, 0.45f);
      if (fp >= 0) {
        char pv[64];
        uxValorTexto(uxOp, ajPadraoVisual(uxOp), pv, sizeof pv);
        snprintf(t, sizeof t, i18n("padrão %s"), pv); l = ajTxt(TXT_ILHA_APOIO, t);
        txt_desenhar_alpha(l, tx + tw * fp - l.w * 0.5f, ly, 0.45f);
      }
      if (risco >= o->min && risco < o->max) {
        snprintf(t, sizeof t, "%d", risco);
        l = txt_linha(TXT_ILHA_APOIO, t, 245, 200, 76, 255);
        txt_desenhar(l, tx + tw * ajFracao(uxOp, risco) - l.w * 0.5f, ly);
      } }
    y += r.h; }
  if (uxOp == AJ_FIL_LIMITE) {
    char t[400];
    snprintf(t, sizeof t, i18n("Mais fileiras usam mais memória e rede: em TV com 1 GB de memória a Home pode ficar lenta ou fechar. Acima de %d a TV pede confirmação, e se o app fechar ele volta sozinho ao valor anterior."), FIL_LIMITE_SEGURO);
    y += 20; y += ajEf("aj_triangle-alert", t, lx, y, lw, 1);
  }
  { const char *ef = efeitoOpcao(uxOp);
    if (ef && ef[0]) { y += 12; y += ajEf("aj_info", i18n(ef), lx, y, lw, 1); } }
  { float px = m.x + m.w - 44 - 440, py = ajCabecalhoEditor(m) + 26;
    ajPreviaPendente(px, py, 440); }
  { AjDica d[3] = { { "↑ ↓", "Ajustar" }, { "OK", "Confirmar" }, { "Voltar", "Cancelar" } };
    ajRodapeEditor(m, d, 3); }
}

// Confirmacao: kicker com o alerta ambar, pergunta 36/700, texto e "de › para".
static void ajConfirmacao(const char *kick, int alerta, const char *pergunta, const char *texto,
                          const char *de, const char *para, const char *sim, const char *nao, int focoSim) {
  GfxRect m = { 560, 300, 800, 0 };
  float x = m.x + 44, y, hTxt;
  hTxt = txt_bloco(TXT_AJ_TEXTO, texto, 0, 0, 0, 0, -4000, m.w - 88, 30, 0, 6);
  m.h = 44 + 22 + 8 + 44 + 14 + hTxt + (de ? 22 + 24 : 0) + 34 + 60 + 44;
  m.y = m.h < 480 ? 300 : (NV_TELA_H - m.h) * 0.5f;
  ajIlha(m, 36, 1, 1);
  y = m.y + 44;
  if (alerta) { gfx_icone((GfxRect){ x, y - 2, 22, 22 }, "aj_triangle-alert", 0.961f, 0.784f, 0.298f, 1); ajKicker(kick, x + 34, y, 1); }
  else ajKicker(kick, x, y, 1);
  y += 22 + 8;
  { TxtLinha t = ajTxtC(TXT_ILHA_PERGUNTA, pergunta, m.w - 88); txt_desenhar(t, x, y); y += 44; }
  y += 14;
  y += ajBloco(TXT_AJ_TEXTO, texto, x, y, m.w - 88, 30, 0.62f, 6);
  if (de) {
    TxtLinha a = ajTxt(TXT_AJ_SUB, de), b = ajTxt(TXT_AJ_SEG, para);
    y += 22;
    txt_desenhar_alpha(a, x, y, 0.55f);
    ajIcone("aj_chevron-right", x + a.w + 16, y + (a.h - 20) * 0.5f, 20, 1);
    txt_desenhar(b, x + a.w + 16 + 20 + 16, y);
    y += 24;
  }
  y += 34;
  { float bx = x;
    bx += ajBotao(sim, NULL, bx, y, focoSim, 1) + 12;
    ajBotao(nao, NULL, bx, y, !focoSim, 1); }
}

// Dependencia: a opcao esta apagada por causa de outra.
static void ajDependencia(void) {
  int req = uxRequisito(uxOp);
  GfxRect m = { 560, 0, 800, 0 };
  float x = m.x + 44, y, hTxt;
  char k[200];
  const char *b = uxBloco(uxOp);
  hTxt = txt_bloco(TXT_AJ_TEXTO, ajudaOpcao(uxOp), 0, 0, 0, 0, -4000, m.w - 88, 30, 0, 6);
  m.h = 44 + 30 + 44 + 14 + hTxt + 34 + 60 + 44;
  m.y = (NV_TELA_H - m.h) * 0.5f;
  ajIlha(m, 36, 1, 1);
  y = m.y + 44;
  if (b[0]) snprintf(k, sizeof k, "%s · %s", i18n(uxCaminho(uxOp)), i18n(b));
  else snprintf(k, sizeof k, "%s", i18n(uxCaminho(uxOp)));
  ajKicker(k, x, y, 1);
  y += 30;
  { TxtLinha t = ajTxtC(TXT_ILHA_PERGUNTA, rotuloOpcao(uxOp), m.w - 88); txt_desenhar(t, x, y); y += 44 + 14; }
  y += ajBloco(TXT_AJ_TEXTO, ajudaOpcao(uxOp), x, y, m.w - 88, 30, 0.62f, 6) + 34;
  { float bx = x;
    if (req >= 0) bx += ajBotao("Ver o ajuste necessário", NULL, bx, y, 1, 1) + 12;
    ajBotao("Fechar", NULL, bx, y, req < 0, 1); }
}

static void ajDesenharModais(void) {
  if (!uxEditor || ajEditandoLinha(uxOp)) return;
  ajVeuModal();
  if (uxRestaurar || uxAvisoRisco) {
    char padrao[160], de[96], para[96], texto[500];
    uxValorTexto(uxOp, uxValorPadrao(uxOp), padrao, sizeof padrao);
    if (uxAvisoRisco) {
      int fil = uxAvisoRisco == SEG_AVISO_FILEIRAS;
      char k[200];
      const char *b = uxBloco(uxOp);
      if (b[0]) snprintf(k, sizeof k, "%s · %s", i18n(uxCaminho(uxOp)), i18n(b));
      else snprintf(k, sizeof k, "%s", i18n(uxCaminho(uxOp)));
      uxValorTexto(uxOp, uxOriginal, de, sizeof de);
      uxValorTexto(uxOp, uxPendente, para, sizeof para);
      if (fil) { snprintf(de, sizeof de, i18n("%d fileiras"), uxOriginal); snprintf(para, sizeof para, i18n("%d fileiras"), uxPendente); }
      ajConfirmacao(k, 1, fil ? "Mais fileiras na Home" : "Mais itens por fileira",
                    fil ? i18n("Mais fileiras usam mais memória e rede; em TVs com 1 GB a Home pode ficar lenta ou fechar. Se o app fechar, ele volta sozinho ao valor anterior.")
                        : i18n("Mais itens por fileira usam mais memória; em TVs com 1 GB a Home pode ficar lenta ou fechar. Se o app fechar, ele volta sozinho ao valor anterior."),
                    de, para, "Continuar", "Cancelar", uxConfirmar);
    } else {
      snprintf(texto, sizeof texto, i18n("O valor voltará para “%s”. %s"), padrao,
        !strcmp(uxEscopoValor(uxOp, uxValorPadrao(uxOp)), "Conta/perfil") ? i18n("Esta preferência participa dos ajustes do perfil. Nenhum serviço será desconectado.") : i18n("A mudança vale só nesta TV. Nenhum serviço será desconectado."));
      uxValorTexto(uxOp, uxValorAtual(uxOp), de, sizeof de);
      ajConfirmacao(rotuloOpcao(uxOp), 0, "Restaurar este ajuste?", texto, de, padrao, "Restaurar", "Cancelar", uxConfirmar);
    }
  } else if (uxEditor == 2) ajDependencia();
  else if (OPCOES[uxOp].tipo == OP_NUMERO) ajEditorNumero();
  else ajEditorEscolha();
  if (uxAviso[0]) {
    TxtLinha t = ajTxtC(TXT_AJ_SUB, uxAviso, 1100);
    txt_desenhar_alpha(t, (NV_TELA_W - t.w) * 0.5f, NV_TELA_H - 70, 0.8f);
  }
}

// A folha do aviso de memoria das setas (riscoFolha), no mesmo modal.
static void desenhaRiscoFolha(void) {
  char de[64], para[64];
  int fil = riscoFolha == SEG_AVISO_FILEIRAS;
  int v = riscoAtual(riscoFolhaOp), nv = v + riscoFolhaDir * (OPCOES[riscoFolhaOp].tipo == OP_NUMERO ? OPCOES[riscoFolhaOp].passo : 1);
  ajVeuModal();
  if (fil) { snprintf(de, sizeof de, i18n("%d fileiras"), v); snprintf(para, sizeof para, i18n("%d fileiras"), nv); }
  else { uxValorTexto(riscoFolhaOp, v, de, sizeof de); uxValorTexto(riscoFolhaOp, nv, para, sizeof para); }
  ajConfirmacao(fil ? "Tela inicial · Fileiras" : uxCaminho(riscoFolhaOp), 1, riscoFolhaTitulo(), i18n(riscoFolhaTexto()),
                de, para, "Continuar", "Cancelar", riscoFolhaFoco == 0);
}

// VINCULO (Trakt/Simkl) como ilha modal: o QR real (src/qr.c) num ladrilho
// branco, endereco e codigo grandes, e o "aguardando" com o ponto que respira
// da ilha do relogio. O codigo curto e o endereco continuam ao lado do QR:
// a pagina de ativacao pede o codigo depois de aberta.
#ifdef AJUSTES_TESTE
static int ajVinculoTeste;
#endif
static void desenhaVinculo(const char *servico, const char *codigo,
                           const char *endereco, const char *falha, int esperando) {
  GfxRect m = { 410, 230, 1100, 620 };
  float x, y;
  char t[96];
  ajIlha(m, 36, 1, 1);
  x = m.x + 56 + 356 + 56; y = m.y + 50;
  ajKicker("Contas e serviços", x, y, 1);
  y += 22 + 4;
  snprintf(t, sizeof t, i18n("Conectar %s"), servico);
  { TxtLinha l = ajTxtC(TXT_ILHA_TITULO, t, m.x + m.w - 56 - x); txt_desenhar(l, x, y); y += 48; }
  if (falha && falha[0]) {
    TxtLinha l = txt_linha_corta(TXT_AJ_NOME, falha, 236, 108, 108, 255, m.x + m.w - 56 - x);
    txt_desenhar(l, x, y + 30);
    { AjDica d[2] = { { "OK", "Tentar de novo" }, { "Voltar", "Fechar" } };
      ajDicasLinha(d, 2, x, m.y + m.h - 50 - 30, 1); }
    return;
  }
  if (!codigo || !codigo[0]) {
    TxtLinha l = ajTxt(TXT_AJ_SUB, "Preparando o código…");
    txt_desenhar_alpha(l, x, y + 30, 0.6f);
    return;
  }
  qrVinTex(endereco);
  { GfxRect tile = { m.x + 56, m.y + (m.h - 356) * 0.5f, 356, 356 };
    gfx_cor(tile, 28.0f / 356, 1, 1, 1, 1);
    if (texQrVin) {
      gfx_tex_aspect_atual = 0.0f;
      gfx_rect((GfxRect){ tile.x + 14, tile.y + 14, 328, 328 }, texQrVin, GFX_SNAP, 0, 0, 0, 0, 0, 0, 0, 1);
    } }
  y += 30;
  { TxtLinha l = ajTxt(TXT_AJ_TEXTO, "Aponte a câmera do celular, ou abra:"); txt_desenhar_alpha(l, x, y, 0.6f); y += l.h + 6; }
  { char e[200]; const char *p = endereco ? strstr(endereco, "://") : NULL;
    snprintf(e, sizeof e, "%s", p ? p + 3 : endereco ? endereco : "-");
    { TxtLinha l = ajTxtC(TXT_ILHA_TITULO, e, m.x + m.w - 56 - x); txt_desenhar(l, x, y); y += l.h + 26; } }
  { TxtLinha l = ajTxt(TXT_AJ_TEXTO, "e informe o código:"); txt_desenhar_alpha(l, x, y, 0.6f); y += l.h + 4; }
  txt_tracking(TXT_AJ_NUM64, codigo, 243, 242, 239, x, y, 1.0f, 14.0f);
  { float by = m.y + m.h - 50 - 30;
    AjDica d[1] = { { "Voltar", "Fechar" } };
    float dw = ajDicasLargura(d, 1);
    if (esperando) {
      float ar, ag, ab, p = ajustes_animacoes_reduzidas() ? 1.0f
                          : 0.55f + 0.45f * sinf((float)SDL_GetTicks() * (2.0f * 3.14159265f / 1200.0f));
      TxtLinha l = ajTxt(TXT_AJ_ESTADO, "Aguardando a autorização…");
      ajustes_acento(&ar, &ag, &ab);
      gfx_cor((GfxRect){ x - 6, by + 15 - 11, 22, 22 }, 0.5f, ar, ag, ab, 0.22f * p);
      gfx_cor((GfxRect){ x, by + 10, 10, 10 }, 0.5f, ar, ag, ab, p);
      txt_desenhar_alpha(l, x + 20, by + (30 - l.h) * 0.5f, 0.62f);
    }
    ajDicasLinha(d, 1, m.x + m.w - 56 - dw, by, 1); }
}
static int ajVinculoAtivo(void) {
  TraEstado ta = traktauth_estado();
  SmkEstado sa = simklauth_estado();
#ifdef AJUSTES_TESTE
  if (ajVinculoTeste) return 1;
#endif
  return ta == TRA_PEDINDO || ta == TRA_AGUARDANDO || ta == TRA_ERRO ||
         sa == SMK_PEDINDO || sa == SMK_AGUARDANDO || sa == SMK_ERRO;
}

// --- A TELA ---------------------------------------------------------------------
static int ajModalAberto(void) {
  return (uxEditor && !ajEditandoLinha(uxOp)) || riscoFolha || ajVinculoAtivo() || teclado_aberto() ||
         registro_envio_aberto();
}
static void ajDesenharTela(void) {
  int modal = ajModalAberto();
  ajFundo();
  if (filAberta) return;   // a folha de fileiras ocupa a tela (desenhada depois)
  if (modal) gfx_opacidade_grupo = 0.16f;
  ajDesenharIndice();
  if (uxIndice < 2) ajDesenharEspecial();
  else { ajDesenharFolha(); ajDesenharInspetor(); }
  gfx_opacidade_grupo = 1.0f;
  if (modal && !(uxEditor && !ajEditandoLinha(uxOp)) && !riscoFolha && !teclado_aberto()) ajVeuModal();
  if (!uxEditor && uxAviso[0]) {
    GfxRect fr = ajFolhaR();
    TxtLinha t = ajTxtC(TXT_ILHA_GENERO, uxAviso, fr.w - 88);
    txt_desenhar_alpha(t, fr.x + 44, fr.y + fr.h - 30, 0.7f);
  }
  ajDesenharModais();
  if (registro_envio_aberto()) registro_envio_desenhar();
}

// --- ADDONS (addonsui.c) COMO UMA CATEGORIA DE AJUSTES ------------------------------
// A tela de addons no mesmo arranjo: indice com Contas aberta, a folha com os
// addons (rosto neutro, o que fornecem embaixo do nome, o chip Ligado) e o
// inspetor com o que o manifesto declara.
static const char *ajAddonFornece(int i) {
  static char b[96];
  if (!addons_sondado(i)) return i18n("conferindo o manifesto…");
  b[0] = 0;
  if (addons_fornece(i, ADD_CATALOGO)) strcat(b, i18n("Catálogo"));
  if (addons_fornece(i, ADD_STREAM)) { if (b[0]) strcat(b, " · "); strcat(b, i18n("Fontes")); }
  if (addons_fornece(i, ADD_LEGENDA)) { if (b[0]) strcat(b, " · "); strcat(b, i18n("Legendas")); }
  if (!b[0]) snprintf(b, sizeof b, "%s", i18n("não fornece nada que este app use"));
  return b;
}
static void ajRosto(const char *nome, float x, float y, float lado, float raio, float vidA, float tinta, int estilo) {
  char ini[8] = "?";
  TxtLinha l;
  if (nome && nome[0]) {
    unsigned char c0 = (unsigned char)nome[0];
    int nb = c0 >= 0xF0 ? 4 : c0 >= 0xE0 ? 3 : c0 >= 0xC0 ? 2 : 1;
    snprintf(ini, sizeof ini, "%.*s", nb, nome);
    if (ini[0] >= 'a' && ini[0] <= 'z') ini[0] = (char)(ini[0] - 32);
  }
  ajNeutro((GfxRect){ x, y, lado, lado }, raio, vidA, 0.141f, 0.149f, 0.173f, 1);
  l = ajTxt((TxtEstilo)estilo, ini);
  txt_desenhar_alpha(l, x + (lado - l.w) * 0.5f, y + (lado - l.h) * 0.5f, tinta);
}
void ajustes_desenhar_addons(int foco) {
  int n = addons_n(), i, lig = 0, salvoIdx = uxIndice, salvoFoco = focoIndice;
  GfxRect fr, ir;
  float topo, base = AJ_BASE, y, alvo;
  char sub[96];
  montarTela();
  for (i = 0; i < n; i++) if (addons_ativo(i)) lig++;
  ajFundo();
  uxIndice = uxSecaoDe(AJ_ADDONS) + 2; focoIndice = 0;
  ajDesenharIndice();
  uxIndice = salvoIdx; focoIndice = salvoFoco;
  fr = ajFolhaR(); ir = ajInspR();
  ajIlha(fr, 36, 1, 0);
  snprintf(sub, sizeof sub, i18n("%d de %d ligados nesta TV"), lig, n);
  topo = ajCabecalhoFolha(fr, "Contas e serviços", "Addons", n ? sub : "Nenhum addon nesta conta.", 0, 1);
  if (foco >= n) foco = n - 1;
  alvo = foco * 84.0f - (base - topo - 80.0f) * 0.5f;
  if (alvo > n * 84.0f - (base - topo)) alvo = n * 80.0f - (base - topo);
  if (alvo < 0) alvo = 0;
  gfx_recorte(fr.x, topo, fr.w, base - topo);
  y = topo - alvo;
  for (i = 0; i < n; i++, y += 84) {
    GfxRect r = { fr.x + 18, y, fr.w - 36, 80 };
    float a = ajMascara(y + 40, topo, base);
    int ligado = addons_ativo(i), f = i == foco;
    TxtLinha t, c;
    if (y + 80 < topo || y > base) continue;
    if (f) ajFocoLinha(r, 22, 1, a);
    ajRosto(addons_nome(i), r.x + 22, y + 16, 48, 14, 0.09f, 0.85f * a, TXT_ILHA_INICIAL);
    { float cw = ajChipLiga(r.x + r.w - 22, y + 40, ligado, a);
      t = ajTxtC(TXT_AJ_ROTULO, addons_nome(i), r.w - 22 - 64 - cw - 40);
      c = ajTxtC(TXT_ILHA_GENERO, ajAddonFornece(i), r.w - 22 - 64 - cw - 40); }
    txt_desenhar_alpha(t, r.x + 22 + 48 + 16, y + 40 - (t.h + 4 + c.h) * 0.5f, (f ? 1.0f : 0.86f) * a);
    txt_desenhar_alpha(c, r.x + 22 + 48 + 16, y + 40 - (t.h + 4 + c.h) * 0.5f + t.h + 4, 0.42f * a);
  }
  gfx_sem_recorte();
  ajIlha(ir, 36, 1, 0);
  if (n > 0) {
    float x = ir.x + 32, w = ir.w - 64, py = ir.y + 32, ix = x + 26;
    static const int REC[3] = { ADD_CATALOGO, ADD_STREAM, ADD_LEGENDA };
    static const char *const NOME[3] = { "Catálogo", "Fontes", "Legendas" };
    const char *base0 = addons_base(foco);
    char host[160] = "";
    float h = 26 + 64 + 24 + 18 + 12 + 44 + 22 + 2 * 42.5f + 22;
    if (base0) {
      const char *p = strstr(base0, "://");
      p = p ? p + 3 : base0;
      snprintf(host, sizeof host, "%.*s", (int)strcspn(p, "/"), p);
    }
    ajNeutro((GfxRect){ x, py, w, h }, 22, 0.04f, 0.106f, 0.110f, 0.129f, 1);
    py += 26;
    ajRosto(addons_nome(foco), ix, py, 64, 20, 0.10f, 1, TXT_AJ_TIT28);
    { TxtLinha nm = ajTxtC(TXT_AJ_NOME, addons_nome(foco), w - 52 - 80);
      TxtLinha st = ajTxt(TXT_ILHA_GENERO, addons_sondado(foco) ? "manifesto conferido" : "conferindo o manifesto…");
      float ty = py + 32 - (nm.h + 4 + st.h) * 0.5f;
      txt_desenhar(nm, ix + 80, ty);
      txt_desenhar_alpha(st, ix + 80, ty + nm.h + 4, 0.55f); }
    py += 64 + 24;
    ajKicker("Fornece", ix, py, 1);
    py += 18 + 12;
    { float cx = ix;
      for (i = 0; i < 3; i++) {
        int sim = addons_fornece(foco, REC[i]);
        TxtLinha l = ajTxt(TXT_ILHA_GENERO, NOME[i]);
        float cw = 16 + 18 + 10 + l.w + 16;
        ajNeutro((GfxRect){ cx, py, cw, 44 }, 22, 0.08f, AJ_CHIP_SOL, 1);
        ajIcone(sim ? "aj_check" : "aj_x", cx + 16, py + 13, 18, sim ? 0.85f : 0.4f);
        txt_desenhar_alpha(l, cx + 16 + 18 + 10, py + (44 - l.h) * 0.5f, sim ? 0.85f : 0.4f);
        cx += cw + 10;
      } }
    py += 44 + 22;
    py += ajMeta(i18n("Nesta TV"), i18n(addons_ativo(foco) ? "Ligado" : "Desligado"), ix, py, w - 52, 1) - 4;
    ajMeta(i18n("Endereço"), host, ix, py, w - 52, 1);
    { float yy = ir.y + 32 + h + 26, tx = x + 6, tw = w - 12;
      ajIcone("aj_puzzle", tx, yy - 1, 20, 0.45f);
      ajCaps(TXT_MINI, "Ligar ou desligar", 2.1f, tx + 30, yy, 0.45f);
      yy += 28;
      yy += ajBloco(TXT_AJ_INSP, addons_nome(foco), tx, yy, tw, 38.4f, 1, 2) + 12;
      ajBloco(TXT_AJ_SUB, "A lista de addons da sua conta, para ligar e desligar cada um nesta TV.", tx, yy, tw, 28.5f, 0.62f, 5); }
  }
  { AjDica d[2] = { { "OK", "Ligar ou desligar" }, { "Voltar", "Ajustes" } };
    ajDicas(d, 2, ir.x + 32, ir.y + ir.h - 32, ir.w - 64, 1); }
}

// --- KIT DAS ILHAS PARA AS TELAS VIZINHAS (diagnostico.c, livetvdiag.c) -----------
// As mesmas pecas de Ajustes, para o diagnostico, o teste de velocidade e o
// diagnostico da Live TV ornarem com a tela de onde saem. Ver ajustes.h.
void ajustes_ui_fundo(void) { ajFundo(); }
void ajustes_ui_arte(int n) { ajArteFundoN = n; }
void ajustes_ui_ilha(GfxRect r, float raioPx, int modal) { ajIlha(r, raioPx, 1, modal); }
void ajustes_ui_veu(void) { ajVeuModal(); }
float ajustes_ui_kicker(const char *s, float x, float y, float a) { return ajKicker(s, x, y, a); }
float ajustes_ui_meta(const char *k, const char *v, float x, float y, float w) { return ajMeta(k, v, x, y, w, 1); }
float ajustes_ui_ef(const char *icone, const char *s, float x, float y, float w) {
  float h = ajEf(icone, s, x, y, w, 1);
  // O alerta das telas vizinhas e AMBAR (aviso de consumo de dados).
  if (!strcmp(icone, "aj_triangle-alert"))
    gfx_icone((GfxRect){ x + 16, y + 16, 20, 20 }, icone, 0.910f, 0.722f, 0.290f, 1);
  return h;
}
float ajustes_ui_botao(const char *rot, const char *icone, float x, float y, int foco) { return ajBotao(rot, icone, x, y, foco, 1); }
float ajustes_ui_dicas(const char *const *teclas, const char *const *rotulos, int n, float x, float y, int desenha) {
  AjDica d[6];
  int i;
  if (n > 6) n = 6;
  for (i = 0; i < n; i++) { d[i].k = teclas[i]; d[i].l = rotulos[i]; }
  if (!desenha) return ajDicasLargura(d, n);
  return ajDicasLinha(d, n, x, y, 1);
}
void ajustes_ui_neutro(GfxRect r, float raioPx, float vidA) { ajNeutro(r, raioPx, vidA, 0.106f, 0.110f, 0.129f, 1); }
void ajustes_ui_foco_linha(GfxRect r, float raioPx) { ajFocoLinha(r, raioPx, 1, 1); }
// "Rotulo ........ a → b unidade" com a barra de cada valor na mesma escala.
float ajustes_ui_antes_depois(const char *rot, int a, int b, const char *unid, int max,
                              float x, float y, float w, int compacto, float cr, float cg, float cb) {
  float pad = compacto ? 9.0f : 14.0f, ar, ag, ab;
  char t[96];
  TxtLinha lr = ajTxtC(TXT_AJ_ESTADO, rot, w * 0.6f), lv;
  ajustes_acento(&ar, &ag, &ab);
  if (cr >= 0) { ar = cr; ag = cg; ab = cb; }
  gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.07f);
  y += pad;
  txt_desenhar_alpha(lr, x, y, 0.5f);
  snprintf(t, sizeof t, "%d %s", b, i18n(unid));
  lv = txt_linha(TXT_AJ_ESTADO, t, AJ_TX);
  txt_desenhar_alpha(lv, x + w - lv.w, y, 0.9f);
  if (a != b) {
    char s[32];
    TxtLinha la;
    snprintf(s, sizeof s, "%d → ", a);
    la = ajTxt(TXT_AJ_ESTADO, s);
    txt_desenhar_alpha(la, x + w - lv.w - la.w, y, 0.45f);
  }
  y += lr.h + (compacto ? 7.0f : 10.0f);
  if (max <= 0) max = 1;
  gfx_cor((GfxRect){ x, y, w, 6 }, 0.5f, 1, 1, 1, 0.07f);
  if (a != b) {
    gfx_cor((GfxRect){ x, y, w * (b > max ? 1.0f : (float)b / max), 6 }, 0.5f, ar, ag, ab, 0.85f);
    gfx_cor((GfxRect){ x, y, w * (float)(a < b ? a : b) / max, 6 }, 0.5f, 0.953f, 0.949f, 0.937f, 0.4f);
  } else gfx_cor((GfxRect){ x, y, w * (a > max ? 1.0f : (float)a / max), 6 }, 0.5f, 0.953f, 0.949f, 0.937f, 0.28f);
  return pad + lr.h + (compacto ? 7.0f : 10.0f) + 6 + pad * 0.5f;
}
// O grafico de pressao da memoria (os ultimos 2 min) sem o painel em volta.
void ajustes_ui_grafico_memoria(float x, float y, float w, float h) {
  long hist[120], teto = tex_orcamento_bytes();
  int nh = tex_historico(hist, 120), i;
  float pw = w / 120.0f;
#ifdef AJUSTES_TESTE
  if (ajMemFixa) {
    teto = 240L << 20; nh = 120;
    for (i = 0; i < 120; i++) {
      double v = i < 30 ? .30 + i * .009 + sin(i * .9) * .01 : i < 52 ? .58 + (i - 30) * .0165 + sin(i * 1.3) * .012
               : i < 74 ? (i % 5 < 3 ? .91 : .80) + sin(i) * .015 : .66 - (i - 74) * .0008 + sin(i * .7) * .012;
      hist[i] = (long)(v * teto);
    }
  }
#endif
  if (teto <= 0) teto = 1;
  gfx_cor((GfxRect){ x, y, w, h }, 12.0f / h, 0, 0, 0, 0.18f);
  for (i = 0; i < nh; i++) {
    float v = (float)hist[i] / (float)teto, hh, cx, r2, g2, b2;
    if (v > 1) v = 1;
    hh = h * v;
    if (hh < 1) continue;
    corPressao(v, &r2, &g2, &b2);
    cx = x + w - (float)(nh - i) * pw;
    gfx_cor((GfxRect){ cx, y + h - hh, pw - 1.2f, hh }, 0, r2, g2, b2, 0.38f);
    gfx_cor((GfxRect){ cx, y + h - hh, pw - 1.2f, 2.5f }, 0, r2, g2, b2, 1);
  }
  { static const float F[2] = { 0.7f, 0.9f };
    static const char *const N[2] = { "70%", "90%" };
    int k;
    for (k = 0; k < 2; k++) {
      float ly = y + h * (1 - F[k]), dx;
      TxtLinha l = ajTxt(TXT_AJ_MINI12, N[k]);
      for (dx = 0; dx < w; dx += 10) gfx_cor((GfxRect){ x + dx, ly, 4, 1 }, 0, 1, 1, 1, 0.22f);
      txt_desenhar_alpha(l, x + 8, ly + 4, 0.45f);
    } }
}
#ifdef AJUSTES_TESTE
void ajustes_teste_mem_fixa(int liga) { ajMemFixa = liga; }
#endif
