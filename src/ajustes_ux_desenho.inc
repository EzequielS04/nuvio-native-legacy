// Settings temporarily shows explanations without illustrative artwork.
// Keep the public resource hook compatible with startup and existing callers.
void ajustes_recursos(const char *dirArte) { (void)dirArte; }
static const char *uxResumo(int op) {
  if (op == AJ_HOME_LAYOUT) return i18n("Escolha como o destaque e as fileiras se organizam na Tela inicial.");
  return ajudaOpcao(op);
}
static void uxDesenharCabecalho(void) {
  if (ajustes_home_layout() != HOME_LAYOUT_DINAMICA) {
    TxtLinha tit = txt_linha(TXT_TITULO2, "Ajustes", 244, 245, 248, 255);
    txt_desenhar(tit, AJ_IDX_X, NV_MARGEM_Y + 2);
    TxtLinha sub = txt_linha(TXT_CAPTION, "Do seu jeito, em cada detalhe.", 165, 173, 189, 255);
    txt_desenhar(sub, AJ_IDX_X + 2, NV_MARGEM_Y + 78);
  }
  const ContaPerfil *p = perfis_item_ativo();
  if (p) {
    float direito = NV_TELA_W - NV_LEGACY_CONTENT_RIGHT;
    TxtLinha nome = txt_linha_corta(TXT_BODY, p->nome, 220, 225, 236, 255, 300);
    TxtLinha escopo = txt_linha(TXT_CAPTION2, "Conta/perfil", 142, 153, 174, 255);
    txt_desenhar(nome, direito - nome.w, NV_MARGEM_Y + 24);
    txt_desenhar(escopo, direito - escopo.w, NV_MARGEM_Y + 64);
  }
}
static void uxDesenharRodape(void) {
  float y = AJ_BASE + 32, w = NV_TELA_W - NV_LEGACY_CONTENT_RIGHT - AJ_IDX_X;
  gfx_cor((GfxRect){AJ_IDX_X, y, w, 1}, 0, 0.14f, 0.16f, 0.20f, 1);
  const char *a = focoIndice ? i18n("↑ ↓   Escolher categoria") : i18n("↑ ↓   Navegar");
  const char *b = focoIndice ? i18n("OK ou →   Entrar na categoria") : uxCabecalho ? i18n("OK   Mostrar ou ocultar avançados") : i18n("OK   Abrir");
  const char *c = uxVeioBusca && !focoIndice ? i18n("Voltar   Resultados da busca") : focoIndice ? i18n("Voltar   Sair dos ajustes") : i18n("← ou Voltar   Categorias");
  if (uxIndice == 0) b = i18n("OK   Buscar nos ajustes desta TV");
  TxtLinha t = txt_linha(TXT_CAPTION2, a, 157, 169, 190, 255);
  txt_desenhar(t, AJ_IDX_X, y + 22);
  t = txt_linha(TXT_CAPTION2, b, 157, 169, 190, 255);
  txt_desenhar(t, AJ_IDX_X + 370, y + 22);
  t = txt_linha(TXT_CAPTION2, c, 157, 169, 190, 255);
  txt_desenhar(t, AJ_IDX_X + w - t.w, y + 22);
}
static void uxDesenharInspetor(void) {
  float x = AJ_LISTA_X + AJ_LISTA_W + 48, y = AJ_TOPO;
  float w = NV_TELA_W - NV_LEGACY_CONTENT_RIGHT - x;
  int categoria = focoIndice || uxCabecalho;
  int op = categoria ? -1 : focoOp;
  if (w < 200) return;
  y += 8;
  if (!categoria && op == AJ_ICONE_APP) {
    iconeapp_desenhar(iconeapp_atual(), (GfxRect){x, y, 112, 112}, 1, 1.0f);
    y += 136;
  }
  const char *titulo = categoria ? TELA[secIni[secAtual]].titulo : rotuloOpcao(op);
  y += txt_bloco(TXT_CALLOUT, titulo, 239, 241, 246, x, y, w, 36, 1, 2) + 18;
  y += txt_bloco(TXT_CAPTION, categoria ? SECAO_AJUDA[secAtual] : uxResumo(op),
                 185, 194, 209, x, y, w, 34, 1, 7);
  if (!categoria && uxTemPadrao(op)) {
    char padrao[160];
    uxValorTexto(op, uxValorPadrao(op), padrao, sizeof padrao);
    y += 28;
    gfx_cor((GfxRect){x, y, w, 1}, 0, 0.17f, 0.19f, 0.23f, 1);
    y += 24;
    TxtLinha rot = txt_linha(TXT_CAPTION2, "Vale para", 135, 147, 167, 255);
    TxtLinha val = txt_linha_corta(TXT_CAPTION2, uxEscopo(op), 208, 216, 230, 255, w - rot.w - 24);
    txt_desenhar(rot, x, y); txt_desenhar(val, x + w - val.w, y);
    y += 40;
    rot = txt_linha(TXT_CAPTION2, "Padrão", 135, 147, 167, 255);
    val = txt_linha_corta(TXT_CAPTION2, padrao, 208, 216, 230, 255, w - rot.w - 24);
    txt_desenhar(rot, x, y); txt_desenhar(val, x + w - val.w, y);
  }
}

// Superfícies de edição e diferenças: alto contraste, sem blur extra de tela.
static void uxTexto(const char *s, float x, float y, float w, int claro) {
  TxtLinha t = txt_linha_corta(TXT_BODY, s, claro, claro, claro, 255, w);
  txt_desenhar(t, x, y);
}
static void uxDesenharAviso(float x, float y, float w) {
  if (!uxAviso[0]) return;
  txt_bloco(TXT_CAPTION, uxAviso, 213, 221, 231, x, y, w, 30, 1, 2);
}
static void uxDesenharAvancados(float y) {
  char t[80];
  int n = 0, i;
  for (i = secIni[secAtual] + 1; i < secFim(secAtual); i++)
    if (TELA[i].tipo == IT_OPC && uxAvancada(TELA[i].op)) n++;
  snprintf(t, sizeof t, uxAvancados[secAtual] ? i18n("Visíveis · %d") : i18n("Ocultos · %d"), n);
  GfxRect r = { AJ_LISTA_X + AJ_LISTA_W - 154, y + 4, 154, 68 };
  int c = uxCabecalho ? AJ_TEXTO_ESCURO : 181;
  if (uxCabecalho) desenhaSuperficie(r, 0.14f, 1, 1);
  else {
    if (uxAvancados[secAtual]) gfx_cor(r, 0.14f, 0.12f, 0.16f, 0.23f, 1);
    gfx_anel(r, 0.14f, 1, 0.22f, 0.25f, 0.31f, 1);
  }
  TxtLinha l = txt_linha_corta(TXT_CAPTION2, "Avançados", c, c, c, 255, r.w - 16);
  txt_desenhar(l, r.x + (r.w - l.w) / 2, r.y + 7);
  l = txt_linha_corta(TXT_CAPTION2, t, c, c, c, 255, r.w - 16);
  txt_desenhar(l, r.x + (r.w - l.w) / 2, r.y + 36);
}
static const char *uxCaminho(int op) {
  int i;
  for (i = 0; i < AJ_N_TELA; i++)
    if (TELA[i].tipo == IT_OPC && TELA[i].op == op) return TELA[secIni[secDoItem[i]]].titulo;
  return "Ajustes";
}
static void uxDesenharEspecial(void) {
  float x = AJ_LISTA_X + AJ_PAD, y = AJ_TOPO;
  float w = NV_TELA_W - NV_LEGACY_CONTENT_RIGHT - x;
  uxTexto(uxIndice == 0 ? "Buscar ajuste" : "Diferentes do padrão", x, y, w, 245);
  y += 64;
  if (uxIndice == 0) {
    txt_bloco(TXT_BODY, "Encontre pelo nome ou pelo que quer mudar. Experimente “legenda”, “trailer” ou “TV lenta”.",
              183, 186, 194, x, y, w - 100, 40, 1, 3);
    uxTexto("OK   Buscar nos ajustes desta TV", x, y + 170, w, 232);
    return;
  }
  txt_bloco(TXT_CAPTION, "Valores diferentes da configuração de fábrica. Podem ter vindo da conta ou de outra versão do app.",
            183, 186, 194, x, y, w, 32, 1, 2);
  y += 92;
  if (!uxNDifs) {
    uxTexto("Tudo está no padrão", x, y + 32, w, 238);
    uxTexto("Seus serviços e credenciais não entram nesta lista.", x, y + 96, w, 170);
  } else {
    int primeiro = uxDifFoco > 3 ? uxDifFoco - 3 : 0;
    int i;
    for (i = primeiro; i < uxNDifs && i < primeiro + 5; i++) {
      char atual[160], padrao[160], detalhe[450];
      int op = uxDifs[i], foco = !focoIndice && i == uxDifFoco;
      GfxRect r = { x, y, w, 108 };
      int c = foco ? AJ_TEXTO_ESCURO : 238;
      desenhaSuperficie(r, 0.12f, foco ? 1 : 0, 1);
      uxValorTexto(op, uxValorAtual(op), atual, sizeof atual);
      uxValorTexto(op, uxValorPadrao(op), padrao, sizeof padrao);
      snprintf(detalhe, sizeof detalhe, i18n("%s · %s · Padrão: %s"), i18n(uxCaminho(op)), i18n(uxEscopo(op)), padrao);
      uxTexto(rotuloOpcao(op), x + 24, y + 14, w - 360, c);
      uxTexto(atual, x + w - 328, y + 14, 300, c);
      TxtLinha d = txt_linha_corta(TXT_CAPTION, detalhe, foco ? AJ_TEXTO_ESCURO2 : 167,
                                   foco ? AJ_TEXTO_ESCURO2 : 170, foco ? AJ_TEXTO_ESCURO2 : 180, 255, w - 48);
      txt_desenhar(d, x + 24, y + 60);
      y += 116;
    }
    char total[80]; snprintf(total, sizeof total, i18n("%d de %d · OK para revisar"), uxDifFoco + 1, uxNDifs);
    uxTexto(total, x, AJ_BASE - 48, w, 169);
  }
  uxDesenharAviso(x, AJ_BASE - 4, w);
}
static void uxDesenharEditor(void) {
  if (!uxEditor) { uxDesenharAviso(AJ_LISTA_X + AJ_PAD, AJ_BASE + 4, AJ_LISTA_W); return; }
  const float w = 1020;
  float h = uxRestaurar || uxAvisoRisco ? 700 : 740;
  if (uxEditor == 1 && !uxRestaurar && !uxAvisoRisco) {
    h = 850;
    if (OPCOES[uxOp].tipo == OP_NUMERO) h = 640;
    else { int n = nValores(uxOp); if (n < 6) h -= (6 - n) * 70.0f; }
  }
  float x = (NV_TELA_W - w) * 0.5f, y = (NV_TELA_H - h) * 0.5f;
  char padrao[160], cabec[320];
  gfx_cor((GfxRect){0, 0, NV_TELA_W, NV_TELA_H}, 0, 0, 0, 0, 0.80f);
  gfx_cor((GfxRect){x, y, w, h}, 28.0f / h, 0.082f, 0.106f, 0.149f, 1);
  gfx_anel((GfxRect){x, y, w, h}, 28.0f / h, 1, 0.22f, 0.26f, 0.34f, 1);
  txt_bloco(TXT_HEADLINE, rotuloOpcao(uxOp), 245, 246, 248, x + 48, y + 38, w - 96, 44, 1, 2);
  uxValorTexto(uxOp, uxValorPadrao(uxOp), padrao, sizeof padrao);
  snprintf(cabec, sizeof cabec, uxTemPadrao(uxOp) ? i18n("%s · Padrão: %s") : "%s", i18n(uxEscopoValor(uxOp, uxRestaurar ? uxValorPadrao(uxOp) : uxPendente)), padrao);
  uxTexto(cabec, x + 48, y + 118, w - 96, 168);
  if (uxRestaurar || uxAvisoRisco) {
    const char *titulo = uxAvisoRisco ? "Confirmar uso de mais memória" : "Restaurar este ajuste?";
    uxTexto(titulo, x + 48, y + 202, w - 96, 243);
    char msg[500];
    if (uxAvisoRisco)
      snprintf(msg, sizeof msg, "%s", i18n("Mais itens e fileiras usam memória e rede. Se o app fechar durante a avaliação automática, ele pode voltar ao valor anterior."));
    else
      snprintf(msg, sizeof msg, i18n("O valor voltará para “%s”. %s"), padrao,
        !strcmp(uxEscopoValor(uxOp, uxValorPadrao(uxOp)), "Conta/perfil") ? i18n("Esta preferência participa dos ajustes do perfil. Nenhum serviço será desconectado.") : i18n("A mudança vale só nesta TV. Nenhum serviço será desconectado."));
    txt_bloco(TXT_BODY, msg, 191, 195, 205, x + 48, y + 262, w - 96, 38, 1, 5);
    GfxRect a = {x + 48, y + h - 138, (w - 120) / 2, 64}, b = {x + w / 2 + 12, y + h - 138, (w - 120) / 2, 64};
    desenhaSuperficie(a, 0.17f, uxConfirmar ? 1 : 0, 1);
    desenhaSuperficie(b, 0.17f, uxConfirmar ? 0 : 1, 1);
    int ca = uxConfirmar ? AJ_TEXTO_ESCURO : 224, cb = uxConfirmar ? 224 : AJ_TEXTO_ESCURO;
    TxtLinha ta = txt_linha(TXT_BODY, uxAvisoRisco ? "Aplicar" : "Restaurar", ca, ca, ca, 255);
    TxtLinha tb = txt_linha(TXT_BODY, "Cancelar", cb, cb, cb, 255);
    txt_desenhar(ta, a.x + (a.w - ta.w) / 2, a.y + (a.h - ta.h) / 2);
    txt_desenhar(tb, b.x + (b.w - tb.w) / 2, b.y + (b.h - tb.h) / 2);
  } else if (uxEditor == 2) {
    txt_bloco(TXT_BODY, ajudaOpcao(uxOp), 205, 210, 220, x + 48, y + 220, w - 96, 38, 1, 6);
    if (uxRequisito(uxOp) >= 0)
      uxTexto("OK   Ver o ajuste necessário", x + 48, y + h - 262, w - 96, 241);
    uxTexto("Voltar   Fechar", x + 48, y + h - 204, w - 96, 180);
  } else if (OPCOES[uxOp].tipo == OP_NUMERO) {
    const Opcao *o = &OPCOES[uxOp];
    char v[160], salvo[160], intervalo[240];
    float parte = o->max > o->min ? (float)(uxPendente - o->min) / (o->max - o->min) : 0;
    uxValorTexto(uxOp, uxPendente, v, sizeof v);
    uxValorTexto(uxOp, uxOriginal, salvo, sizeof salvo);
    GfxRect r = { x + 48, y + 210, w - 96, 148 };
    desenhaSuperficie(r, 0.10f, uxRodape ? 0 : 1, 1);
    int cor = uxRodape ? 238 : AJ_TEXTO_ESCURO;
    uxTexto(v, r.x + 28, r.y + 26, r.w - 56, cor);
    snprintf(intervalo, sizeof intervalo, i18n("Salvo: %s · De %d a %d%s"), salvo, o->min, o->max, o->sufixo ? o->sufixo : "");
    uxTexto(intervalo, x + 48, y + 374, w - 96, 170);
    gfx_cor((GfxRect){r.x + 28, r.y + 110, r.w - 56, 4}, 0.5f, cor / 255.0f, cor / 255.0f, cor / 255.0f, 0.25f);
    if (parte > 0) gfx_cor((GfxRect){r.x + 28, r.y + 110, (r.w - 56) * parte, 4}, 0.5f, cor / 255.0f, cor / 255.0f, cor / 255.0f, 1);
    uxTexto("↑ ↓   Ajustar · OK   Confirmar · Voltar   Cancelar", x + 48, y + h - 205, w - 96, 170);
  } else {
    const Opcao *o = &OPCOES[uxOp];
    int numero = o->tipo == OP_NUMERO;
    int passo = numero && o->passo > 0 ? o->passo : 1;
    int n = numero ? (o->max - o->min) / passo + 1 : nValores(uxOp);
    int idx = numero ? (uxPendente - o->min) / passo : uxPendente;
    int primeiro = idx > 2 ? idx - 2 : 0, i;
    if (primeiro + 6 > n) primeiro = n > 6 ? n - 6 : 0;
    for (i = primeiro; i < n && i < primeiro + 6; i++) {
      int v = numero ? o->min + i * passo : i, foco = !uxRodape && i == idx;
      char t[160];
      GfxRect r = { x + 48, y + 207 + (i - primeiro) * 70, w - 96, 64 };
      uxValorTexto(uxOp, v, t, sizeof t);
      desenhaSuperficie(r, 0.18f, foco ? 1 : 0, 1);
      float textoX = r.x + 24;
      if (uxOp == AJ_ICONE_APP) {
        iconeapp_desenhar(v, (GfxRect){r.x + 16, r.y + 6, 52, 52}, 1, 1.0f);
        textoX = r.x + 86;
      }
      uxTexto(t, textoX, r.y + 14, r.w - (textoX - r.x) - 166, foco ? AJ_TEXTO_ESCURO : 225);
      if (v == uxOriginal)
        uxTexto("✓ Salvo", r.x + r.w - 158, r.y + 14, 140, foco ? AJ_TEXTO_ESCURO2 : 184);
    }
    uxTexto("↑ ↓   Escolher · OK   Confirmar · Voltar   Cancelar", x + 48, y + h - 205, w - 96, 170);
  }
  if (!uxRestaurar && !uxAvisoRisco && uxTemPadrao(uxOp)) {
    GfxRect r = {x + w - 396, y + h - 148, 348, 64};
    gfx_cor((GfxRect){x + 48, r.y - 16, w - 96, 1}, 0, 0.19f, 0.24f, 0.32f, 1);
    if (uxRodape) desenhaSuperficie(r, 0.18f, 1, 1);
    uxTexto("→   Restaurar padrão…", r.x + 24, r.y + 14, r.w - 48, uxRodape ? AJ_TEXTO_ESCURO : 191);
  }
  uxDesenharAviso(x + 48, y + h - 68, w - 96);
}
