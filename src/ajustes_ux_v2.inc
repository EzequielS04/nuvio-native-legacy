// AJUSTES V2 — O MENU GRANDE, O RESUMO E A PILHA QUE DESLIZA (ajustes-v2.html,
// inventory-v2.md, aprovado pelo dono em 03/10/2026). Incluido por
// ajustes_ux_desenho.inc, depois das previas e antes da ilha (que desenha a
// lista, o inspetor e os editores com estas pecas).
//
// TRES DEGRAUS: 0 = o menu de categorias sozinho, com o resumo da categoria a
// direita; 1 = OK numa categoria: a lista cresce por cima do menu (que recua
// para a coluna de icones) e o resumo vira o inspetor; 2 = OK numa opcao que
// abre editor: o editor entra do mesmo jeito e a lista recua para a faixa.
// Cada degrau e uma mola de segunda ordem (sem quique) e rampas de alfa; o
// Voltar desfaz com a mola mais rapida. "Animacoes: Reduzidas" = sem deslize
// nem escala, so o fade de 120 ms.

// --- MOLA COM AMORTECIMENTO -----------------------------------------------------------
// A anim_mola2 e criticamente amortecida (zeta 1); a tabela do mockup pede
// zeta 0,86 e 0,9 para a lista e o inspetor. Passos fixos de 1/240 s (semi-
// implicito), estaveis para qualquer dt.
static float aj2Mola(float *v, float x, float alvo, float dt, float w, float z) {
  if (anim_politica_reduzida || ajustes_animacoes_reduzidas()) { *v = 0.0f; return alvo; }
  if (dt > 0.05f) dt = 0.05f;
  while (dt > 0.0f) {
    float h = dt > (1.0f / 240.0f) ? (1.0f / 240.0f) : dt;
    *v += (-2.0f * z * w * *v - w * w * (x - alvo)) * h;
    x += *v * h;
    dt -= h;
  }
  if (fabsf(x - alvo) < 0.0005f && fabsf(*v) < 0.005f) { x = alvo; *v = 0.0f; }
  return x;
}
static float aj2Rampa(float p, float alvo, float dt, float ms) {
  float passo = dt * (1000.0f / (ajustes_animacoes_reduzidas() ? 120.0f : ms));
  if (alvo > p) { p += passo; if (p > alvo) p = alvo; }
  else          { p -= passo; if (p < alvo) p = alvo; }
  return p;
}

// --- O ESTADO DA PILHA ----------------------------------------------------------------
static float aj2N1, aj2V1, aj2N2, aj2V2;     // degraus 1 e 2 (molas)
static float aj2Rot = 1.0f;                  // rotulos do menu (some em 120 ms)
static float aj2Miolo = 1.0f;                // ilha do menu: 1, 0,42, 0,16
static float aj2ListaA, aj2EdA, aj2ResumoA = 1.0f;
static float aj2Insp, aj2VI, aj2InspEspera;  // o inspetor chega 60 ms depois
// Pose: o que o desenho aplica. ajustes_teste_pose fixa uma (o quadro do meio
// da transicao do mockup).
typedef struct {
  float mS, mDx, mA, mRot;             // menu: escala, deslize, alfa, rotulos
  float lS, lDx, lOy, lA, lTxt;        // lista: escala, deslize, origem y, alfa, texto
  float iDx, iA;                       // inspetor
  float rA;                            // resumo (so no degrau 0)
  float eS, eDx, eA;                   // editor
} Aj2Pose;
#ifdef AJUSTES_TESTE
static int aj2PoseFixa;
static Aj2Pose aj2PoseTeste;
#endif
static int aj2EditorPainel(void);
static int aj2Degrau(void) {
  if (focoIndice || uxIndice == 0) return 0;
  return aj2EditorPainel() ? 2 : 1;
}

// --- O MENU ---------------------------------------------------------------------------
// menuV2: ilha de 660 em x 48, y 112, raio 40, padding 18; linhas de 76 (gap
// 4), rotulo 36/500 (600 na ativa), icone 40; a linha em foco abre a
// descricao (24, linha 1,35, ate 2 linhas): padding 18 28 20. Fio de 1 px com
// margem 8 24 depois de "Diferentes do padrao". Esvanece nos 70 px de baixo.
#define AJ2_MENU_PAD   18.0f
#define AJ2_MENU_LN    76.0f
#define AJ2_MENU_FIO   17.0f
#define AJ2_MENU_FADE  70.0f
#define AJ2_NMENU (AJ_MAX_SECOES + 2)
static float aj2MH[AJ2_NMENU], aj2MV[AJ2_NMENU], aj2MDesc[AJ2_NMENU], aj2MTempo[AJ2_NMENU];
static float aj2MRol, aj2MRolV, aj2SupY, aj2SupYv, aj2SupH, aj2SupHv;
static int aj2MIni;
static const char *aj2MenuTitulo(int j) {
  return j == 0 ? "Buscar ajuste" : j == 1 ? "Diferentes do padrão" : TELA[secIni[j - 2]].titulo;
}
static const char *aj2MenuIcone(int j) {
  return j == 0 ? "aj_search" : j == 1 ? "aj_rotate-ccw-clock" : TELA[secIni[j - 2]].icone;
}
static const char *aj2MenuDesc(int j) {
  return j == 0 ? "Pelo nome ou pelo que quer mudar"
       : j == 1 ? "Valores fora da configuração de fábrica" : TELA[secIni[j - 2]].sub;
}
// Altura da linha aberta: 18 + 42 + 6 + linhas x 32,4 + 20.
static float aj2MenuAberta(int j) {
  float w = AJ2_MENU_W - 2 * AJ2_MENU_PAD - 2 * 28.0f - 40.0f - 24.0f;
  float h = txt_bloco(TXT_V2_26, aj2MenuDesc(j), 0, 0, 0, 0, -4000, w, 32.4f, 0, 2);
  if (h < 32.4f) h = 32.4f;
  return 18.0f + 42.0f + 6.0f + h + 20.0f;
}
static int aj2MenuFocado(void) { return focoIndice ? uxIndice : -1; }
// y da linha j relativo ao topo do conteudo (sem a rolagem).
static float aj2MenuY(int j) {
  float y = 0;
  int k;
  for (k = 0; k < j; k++) y += aj2MH[k] + 4.0f;
  if (j >= 2) y += AJ2_MENU_FIO + 4.0f;
  return y;
}
static void aj2MenuAtualizar(float dt) {
  int j, n = nSecoes + 2, f = aj2MenuFocado(), red = ajustes_animacoes_reduzidas();
  GfxRect m = aj2MenuR();
  float janela = m.h - 2 * AJ2_MENU_PAD, alvo;
  if (n > AJ2_NMENU) n = AJ2_NMENU;
  for (j = 0; j < n; j++) {
    float alvoH = j == f ? aj2MenuAberta(j) : AJ2_MENU_LN;
    if (!aj2MIni || red) { aj2MH[j] = alvoH; aj2MV[j] = 0; }
    else aj2MH[j] = anim_mola2(&aj2MV[j], aj2MH[j], alvoH, dt, 20.0f);
    // descricao: sai em 80 ms; entra 90 ms depois, em 140 ms.
    if (j == f) {
      aj2MTempo[j] += dt;
      if (!aj2MIni) aj2MDesc[j] = 1.0f;
      else if (red) aj2MDesc[j] = aj2Rampa(aj2MDesc[j], 1.0f, dt, 80.0f);
      else if (aj2MTempo[j] >= 0.09f) aj2MDesc[j] = aj2Rampa(aj2MDesc[j], 1.0f, dt, 140.0f);
    } else {
      aj2MTempo[j] = 0;
      aj2MDesc[j] = aj2MIni ? aj2Rampa(aj2MDesc[j], 0.0f, dt, 80.0f) : 0.0f;
    }
  }
  // Rolagem: so quando a linha aberta passaria do degrade (ou subiu para fora).
  { int alvoJ = f >= 0 ? f : uxIndice;
    float ty = aj2MenuY(alvoJ), th = (f >= 0 && alvoJ == f) ? aj2MenuAberta(alvoJ) : AJ2_MENU_LN;
    float fim = aj2MenuY(n - 1) + AJ2_MENU_LN;
    alvo = aj2MRol;
    if (ty + th > alvo + janela - AJ2_MENU_FADE) alvo = ty + th - (janela - AJ2_MENU_FADE);
    if (ty < alvo) alvo = ty;
    if (alvo > fim - janela + AJ2_MENU_FADE) alvo = fim - janela + AJ2_MENU_FADE;
    if (alvo < 0) alvo = 0;
    if (!aj2MIni || red) { aj2MRol = alvo; aj2MRolV = 0; }
    else aj2MRol = anim_mola2(&aj2MRolV, aj2MRol, alvo, dt, 16.0f); }
  // A superficie de foco e uma peca so: segue a linha nova com a mesma mola.
  if (f >= 0) {
    float ty = aj2MenuY(f), th = aj2MH[f];
    if (!aj2MIni || red) { aj2SupY = ty; aj2SupH = th; aj2SupYv = aj2SupHv = 0; }
    else {
      aj2SupY = anim_mola2(&aj2SupYv, aj2SupY, ty, dt, 20.0f);
      aj2SupH = anim_mola2(&aj2SupHv, aj2SupH, th, dt, 20.0f);
    }
  }
  aj2MIni = 1;
}
// Alfa do degrade de baixo, medido no centro da peca.
static float aj2MenuMascara(float yc, float base) {
  float a = (base - yc) / AJ2_MENU_FADE;
  return a < 0 ? 0 : a > 1 ? 1 : a;
}
static void aj2DesenharMenu(const Aj2Pose *p) {
  GfxRect m = aj2MenuR();
  int j, n = nSecoes + 2, f = aj2MenuFocado();
  float topo = m.y + AJ2_MENU_PAD, base = m.y + m.h - AJ2_MENU_PAD;
  float ar, ag, ab, grupo = gfx_opacidade_grupo;
  if (n > AJ2_NMENU) n = AJ2_NMENU;
  if (p->mA <= 0.003f) return;
  ajustes_acento(&ar, &ag, &ab);
  gfx_opacidade_grupo = grupo * p->mA;
  gfx_transformar(m.x, m.y + m.h * 0.5f, p->mS, p->mDx, 0);
  ajIlha(m, 40.0f, 1, 0);
  gfx_recorte(m.x, topo, m.w, base - topo);
  // A superficie de foco (vidro 14%, solido #2B2D34 com sombra).
  if (f >= 0) {
    GfxRect r = { m.x + AJ2_MENU_PAD, topo + aj2SupY - aj2MRol, m.w - 2 * AJ2_MENU_PAD, aj2SupH };
    if (ajVid()) gfx_cor(r, 38.0f / r.h, 1, 1, 1, 0.14f);
    else {
      gfx_rect((GfxRect){ r.x - 10, r.y + 2, r.w + 20, r.h + 26 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f, 0, 0, 0, 0.30f);
      gfx_cor(r, 38.0f / r.h, 0.169f, 0.176f, 0.204f, 1);
    }
  }
  for (j = 0; j < n; j++) {
    float y = topo + aj2MenuY(j) - aj2MRol, h = aj2MH[j];
    int foco = j == f, ativa = j == uxIndice;
    float rx = m.x + AJ2_MENU_PAD, mk, al, lc;
    if (j == 2) {
      float fy = y - 4.0f - 8.0f - 1.0f;
      gfx_cor((GfxRect){ m.x + AJ2_MENU_PAD + 24.0f, fy, m.w - 2 * AJ2_MENU_PAD - 48.0f, 1 }, 0, 1, 1, 1,
              0.08f * aj2MenuMascara(fy, base));
    }
    if (y > base || y + h < topo) continue;
    mk = aj2MenuMascara(y + (h > AJ2_MENU_LN + 1 ? 39.0f : h * 0.5f), base);
    if (mk <= 0.003f) continue;
    al = (foco || ativa) ? 1.0f : 0.66f;
    // o rotulo fica no alto da linha que abre: centro 38 fechada, 39 aberta
    lc = y + (h > AJ2_MENU_LN + 1 ? 38.0f + (h - AJ2_MENU_LN) / (aj2MenuAberta(j) - AJ2_MENU_LN + 0.001f) : h * 0.5f);
    if (ativa && !foco) gfx_cor((GfxRect){ rx + 9.0f, lc - 4.0f, 8, 8 }, 0.5f, ar, ag, ab, mk);
    ajIcone(aj2MenuIcone(j), rx + 28.0f, lc - 20.0f, 40.0f, al * mk);
    if (p->mRot > 0.003f) {
      TxtLinha t = ajTxtC(ativa ? TXT_V2_MENU_B : TXT_V2_MENU, aj2MenuTitulo(j), m.w - 2 * AJ2_MENU_PAD - 28 - 40 - 24 - 20);
      txt_desenhar_alpha(t, rx + 28 + 40 + 24, lc - t.h * 0.5f, al * mk * p->mRot);
      if (aj2MDesc[j] > 0.003f && h > AJ2_MENU_LN + 4) {
        float dy = (1.0f - aj2MDesc[j]) * 6.0f;
        ajBloco(TXT_V2_26, aj2MenuDesc(j), rx + 28 + 40 + 24, y + 18 + 42 + 6 + dy,
                m.w - 2 * AJ2_MENU_PAD - 2 * 28.0f - 64.0f, 32.4f, 0.7f * aj2MDesc[j] * mk * p->mRot, 2);
      }
    }
  }
  gfx_sem_recorte();
  gfx_sem_transformar();
  gfx_opacidade_grupo = grupo;
}

// --- DICAS GRANDES ------------------------------------------------------------------
// kbd com o corpo `ek` e altura `kh`, rotulo `el`; vao tecla-rotulo `g1`,
// entre dicas `g2`. Devolve a largura.
static float aj2DicasLinha(const AjDica *d, int n, float x, float yc, TxtEstilo ek, float kh, float kmin,
                           float kpad, TxtEstilo el, float g1, float g2, int desenha, float a) {
  int i;
  float x0 = x;
  for (i = 0; i < n; i++) {
    TxtLinha k = ajTxt(ek, d[i].k), l = ajTxt(el, d[i].l);
    float kw = k.w + 2 * kpad < kmin ? kmin : k.w + 2 * kpad;
    if (desenha) {
      ajNeutro((GfxRect){ x, yc - kh * 0.5f, kw, kh }, kh * 0.5f, 0.09f, AJ_CHIP_SOL, a);
      txt_desenhar_alpha(k, x + (kw - k.w) * 0.5f, yc - k.h * 0.5f, 0.82f * a);
      txt_desenhar_alpha(l, x + kw + g1, yc - l.h * 0.5f, 0.55f * a);
    }
    x += kw + g1 + l.w + g2;
  }
  return x - x0 - g2;
}

// --- O RESUMO DA CATEGORIA (degrau 0) -------------------------------------------------
// A ilha da direita nao repete titulo nem descricao (o menu ja diz): a previa
// real grande, os grupos em chips e a contagem ("32 opções · 7 avançadas").
static float aj2PreviaCategoria(int sec, float x, float y, float w, float h);
static void aj2DesenharResumo(float a) {
  GfxRect r = aj2ResumoR();
  float x = r.x + 40.0f, w = r.w - 80.0f, y = r.y + 40.0f, grupo = gfx_opacidade_grupo;
  AjDica d[3] = { { "↑ ↓", "Categoria" }, { "OK", "Abrir" }, { "Voltar", "Sair dos ajustes" } };
  if (a <= 0.003f) return;
  gfx_opacidade_grupo = grupo * a;
  ajIlha(r, 40.0f, 1, 0);
  if (uxIndice >= 2) {
    int sec = uxIndice - 2, i, nOp = 0, nAv = 0;
    float ph = w * 9.0f / 16.0f, cx, cy, cw, numW;
    char q[16], t[96];
    TxtLinha ln, lt;
    aj2PreviaCategoria(sec, x, y, w, ph);
    y += ph + 26.0f;
    for (i = secIni[sec] + 1; i < secFim(sec); i++)
      if (TELA[i].tipo == IT_OPC) { nOp++; if (uxAvancada(TELA[i].op)) nAv++; }
    snprintf(q, sizeof q, "%d", nOp);
    snprintf(t, sizeof t, nAv == 1 ? i18n("opções · %d avançada") : i18n("opções · %d avançadas"), nAv);
    ln = ajTxt(TXT_V2_NUM, q); lt = ajTxt(TXT_V2_26, t);
    numW = ln.w + 10.0f + lt.w;
    // chips dos grupos (os ROT da categoria), quebrando linha
    cx = x; cy = y; cw = w - numW - 26.0f;
    { int nl = 1;
      for (i = secIni[sec] + 1; i < secFim(sec); i++) {
        TxtLinha c;
        float bw;
        if (TELA[i].tipo != IT_ROT) continue;
        c = ajTxt(TXT_V2_SEG, TELA[i].titulo);
        bw = 22 + c.w + 22;
        if (cx > x && cx + bw > x + cw) { cx = x; cy += 56.0f + 12.0f; nl++; }
        ajNeutro((GfxRect){ cx, cy, bw, 56 }, 28, 0.08f, AJ_CHIP_SOL, 1);
        txt_desenhar_alpha(c, cx + 22, cy + (56 - c.h) * 0.5f, 0.85f);
        cx += bw + 12.0f;
      }
      { float bloco = cx == x && nl == 1 ? 56.0f : (cy - y) + 56.0f, ycn = y + bloco * 0.5f;
        float nx = x + w - numW;
        // alinhados pela base, como o inline do mockup
        float base = ycn + ln.h * 0.32f;
        txt_desenhar(ln, nx, ycn - ln.h * 0.5f);
        txt_desenhar_alpha(lt, nx + ln.w + 10.0f, base - lt.h * 0.80f, 0.55f); } }
  } else if (uxIndice == 1) {
    // Diferentes do padrao: as opcoes fora do padrao, com o valor de agora.
    int i;
    char q[16];
    TxtLinha ln, lt;
    uxListarDiferencas();
    for (i = 0; i < uxNDifs && i < 9; i++) {
      char atual[160];
      TxtLinha a1, l1;
      int op = uxDifs[i];
      uxValorTexto(op, uxValorAtual(op), atual, sizeof atual);
      a1 = ajTxtC(TXT_V2_28, atual, 320);
      l1 = ajTxtC(TXT_V2_ROT, rotuloOpcao(op), w - a1.w - 40);
      txt_desenhar_alpha(l1, x, y + (72 - l1.h) * 0.5f, 0.88f);
      txt_desenhar_alpha(a1, x + w - a1.w, y + (72 - a1.h) * 0.5f, 0.55f);
      if (i + 1 < uxNDifs && i < 8) gfx_cor((GfxRect){ x, y + 72, w, 1 }, 0, 1, 1, 1, 0.07f);
      y += 73.0f;
    }
    if (!uxNDifs) {
      TxtLinha t = ajTxt(TXT_V2_ROT, "Tudo está no padrão");
      txt_desenhar_alpha(t, x, y + 10, 0.88f);
    } else {
      snprintf(q, sizeof q, "%d", uxNDifs);
      ln = ajTxt(TXT_V2_NUM, q);
      lt = ajTxt(TXT_V2_26, uxNDifs == 1 ? "diferente do padrão" : "diferentes do padrão");
      txt_desenhar(ln, x, y + 20);
      txt_desenhar_alpha(lt, x + ln.w + 10, y + 20 + ln.h * 0.82f - lt.h * 0.8f, 0.55f);
    }
  } else {
    // Buscar: a frase de exemplo e o botao, como a tela de busca abre.
    TxtLinha t;
    y += ajBloco(TXT_V2_28, "Encontre pelo nome ou pelo que quer mudar. Experimente “legenda”, “trailer” ou “TV lenta”.",
                 x, y, w, 37.7f, 0.66f, 3) + 26.0f;
    t = ajTxt(TXT_V2_SEG, "Buscar nos ajustes desta TV");
    { GfxRect b = { x, y, 24 + 24 + 10 + t.w + 24, 60 };
      ajNeutro(b, 30, 0.08f, AJ_CHIP_SOL, 1);
      ajIcone("aj_search", b.x + 24, b.y + 18, 24, 0.85f);
      txt_desenhar_alpha(t, b.x + 58, b.y + (60 - t.h) * 0.5f, 0.85f); }
    d[1].l = "Buscar";
  }
  aj2DicasLinha(d, 3, x, r.y + r.h - 40.0f - 20.0f, TXT_V2_KBD20, 40, 44, 9, TXT_V2_26, 10, 28, 1, 1);
  gfx_opacidade_grupo = grupo;
}

// --- AS PREVIAS DA V2 -----------------------------------------------------------------
// A previa grande de cada categoria no resumo (1068 x 601 a 100%). A Tela
// inicial e a home de verdade em miniatura; Cartazes e arte sao tres cartazes
// no tamanho da home; as outras usam a previa da primeira opcao, recortada.
static float aj2PreviaCategoria(int sec, float x, float y, float w, float h) {
  int it = primeiroDaSecao(sec), op = TELA[it].tipo == IT_OPC ? TELA[it].op : -1;
  if (op == AJ_HOME_LAYOUT || sec == 0) { ajPreviaHome(x, y, w); return h; }
  if (familiaPreviaOpcao(op) == AJPV_FOCO || familiaPreviaOpcao(op) == AJPV_CARTAZ || sec == 1) {
    static const int P[3] = { 13, 12, 15 };
    float s = w / 1068.0f, cw = 220 * s, ch = 330 * s, fw = 250 * s, fh = 375 * s, gx = 250 * s;
    int i;
    ajNeutro((GfxRect){ x, y, w, h }, 16, 0.04f, 0.106f, 0.110f, 0.129f, 1);
    for (i = 0; i < 3; i += 2)
      ajImg(ajArte(AJA_POSTER, P[i]), (GfxRect){ x + 190 * s + i * gx, y + 110 * s, cw, ch }, 16 * s, 0.7f);
    gfx_rect((GfxRect){ x + 190 * s + gx - 20, y + 90 * s + 26 * s, fw + 40, fh + 44 }, 0, GFX_SOMBRA, 1, 0, 0, 0.5f, 0, 0, 0, 0.55f);
    ajImg(ajArte(AJA_POSTER, P[1]), (GfxRect){ x + 190 * s + gx, y + 90 * s, fw, fh }, 18 * s, 1);
    return h;
  }
  if (op < 0) return 0;
  gfx_recorte(x, y, w, h);
  ajPrevia(op, x, y, w);
  gfx_sem_recorte();
  return h;
}
// A tela de Ajustes da v2 em miniatura no fundo `modo` (cartoes do Fundo e a
// previa do inspetor): o fundo de verdade e tres ilhas com tracos no lugar do
// texto. Desenhada em escala, sem FBO.
static void aj2MiniFundo(int modo, float x, float y, float w, float a) {
  float s = w / 1920.0f, h = w * 9.0f / 16.0f, rr = w > 300.0f ? 16.0f : 12.0f, ar, ag, ab;
  GfxRect q = { x, y, w, h };
  int i;
  static const float BAR[5] = { 340, 260, 300, 220, 280 };
  ajustes_acento(&ar, &ag, &ab);
  (void)ar; (void)ag; (void)ab;
  gfx_recorte(x, y, w, h);
  fundo_desenhar_modo(modo, q, rr, ajFundoArte(), a);
  // as tres ilhas (menu recuado, lista com uma linha em foco, inspetor)
#define FX(v) (x + (v) * s)
#define FY(v) (y + (v) * s)
  { float va = ajVid() ? 0.80f : 1.0f;
    float r0 = ajVid() ? 0.055f : 0.082f, g0 = ajVid() ? 0.059f : 0.086f, b0 = ajVid() ? 0.071f : 0.102f;
    gfx_cor((GfxRect){ FX(48), FY(112), 640 * s, 928 * s }, 36.0f / 928.0f, r0, g0, b0, va * a);
    gfx_cor((GfxRect){ FX(172), FY(40), 1000 * s, 1000 * s }, 40.0f / 1000.0f, r0, g0, b0, va * a);
    gfx_cor((GfxRect){ FX(1196), FY(40), 684 * s, 1000 * s }, 40.0f / 1000.0f, r0, g0, b0, va * a);
    { TxtEstilo e = ajEstiloPx(52 * s, 1);
      if ((int)e >= 0) { TxtLinha l = ajTxt(e, "Aparência"); txt_desenhar_alpha(l, FX(222), FY(82) + (52 * s * 1.2f - l.h) * 0.5f, a); } }
    for (i = 0; i < 5; i++) {
      float ry = 40 + 42 + 63 + 8 + i * 104;
      if (i == 1) gfx_cor((GfxRect){ FX(222), FY(ry), 900 * s, 96 * s }, 0.29f, 1, 1, 1, 0.12f * a);
      gfx_cor((GfxRect){ FX(252), FY(ry + 37), BAR[i] * s, 22 * s }, 0.5f, 0.953f, 0.949f, 0.937f, 0.45f * a);
    } }
#undef FX
#undef FY
  gfx_sem_recorte();
}
// Cor de destaque na v2 (corPvV2): as doze cores com a escolhida em destaque
// e o que ela pinta — o botao em foco e o chip ligado.
static float aj2PreviaCor(float x, float y, float w) {
  int i, sel = ajEditandoLinha(AJ_TEMA) ? uxPendente : valor[AJ_TEMA], ti = ajTinta();
  float cx = x, cy = y, ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  for (i = 0; i < AJ_N_TEMAS; i++) {
    float cr = TEMA_ACENTO[i].r, cg = TEMA_ACENTO[i].g, cb = TEMA_ACENTO[i].b, d = 50.0f;
    if (cx + 50 > x + w + 0.5f) { cx = x; cy += 64.0f; }
    if (i == sel) {
      float k = 0.2126f * cr + 0.7152f * cg + 0.0722f * cb > 0.5f ? 0.05f : 1.0f;
      d = 57.5f;
      gfx_rect((GfxRect){ cx + 25 - d * 0.5f - 12, cy + 25 - d * 0.5f + 2, d + 24, d + 30 }, 0, GFX_SOMBRA, 1, 0, 0, 0.5f, 0, 0, 0, 0.5f);
      gfx_cor((GfxRect){ cx + 25 - d * 0.5f, cy + 25 - d * 0.5f, d, d }, 0.5f, cr, cg, cb, 1);
      gfx_icone((GfxRect){ cx + 13, cy + 13, 24, 24 }, "aj_check", k, k, k, 1);
    } else gfx_cor((GfxRect){ cx, cy, d, d }, 0.5f, cr, cg, cb, 1);
    cx += 64.0f;
  }
  cy += 50.0f + 22.0f;
  { TxtLinha l = ajTxtT(TXT_V2_SEG, "Reproduzir"), m;
    float bw = 28 + 20 + 10 + l.w + 28, cw;
    ajPilulaAcento((GfxRect){ x, cy, bw, 64 }, 32, 1);
    gfx_rect((GfxRect){ x + 32, cy + 22, 16, 20 }, 0, GFX_PLAY, 0, 0, 0, 0, ti / 255.0f, ti / 255.0f, ti / 255.0f, 1);
    txt_desenhar(l, x + 58, cy + (64 - l.h) * 0.5f);
    m = txt_linha(TXT_G20B, "Só MP4", (int)(ar * 255), (int)(ag * 255), (int)(ab * 255), 255);
    cw = 20 + 8 + 9 + m.w + 20;
    gfx_cor((GfxRect){ x + bw + 14, cy, cw, 52 }, 0.5f, ar, ag, ab, 0.22f);
    gfx_cor((GfxRect){ x + bw + 14 + 20, cy + 22, 8, 8 }, 0.5f, ar, ag, ab, 1);
    txt_desenhar(m, x + bw + 14 + 37, cy + (52 - m.h) * 0.5f); }
  return cy + 64.0f - y;
}

// --- A ANIMACAO ---------------------------------------------------------------------
static void aj2Atualizar(float dt) {
  int deg = aj2Degrau(), red = ajustes_animacoes_reduzidas();
  float a1 = deg >= 1 ? 1.0f : 0.0f, a2 = deg >= 2 ? 1.0f : 0.0f;
  aj2MenuAtualizar(dt);
  if (red) { aj2N1 = a1; aj2N2 = a2; aj2V1 = aj2V2 = 0; }
  else {
    aj2N1 = aj2Mola(&aj2V1, aj2N1, a1, dt, a1 > aj2N1 ? 18.0f : 22.0f, a1 > aj2N1 ? 0.86f : 1.0f);
    aj2N2 = aj2Mola(&aj2V2, aj2N2, a2, dt, a2 > aj2N2 ? 18.0f : 22.0f, a2 > aj2N2 ? 0.86f : 1.0f);
  }
  aj2Rot = aj2Rampa(aj2Rot, deg >= 1 ? 0.0f : 1.0f, dt, 120.0f);
  aj2Miolo = aj2Rampa(aj2Miolo, deg == 0 ? 1.0f : deg == 1 ? 0.42f : 0.16f, dt, 200.0f);
  aj2ListaA = aj2Rampa(aj2ListaA, deg >= 1 ? 1.0f : 0.0f, dt, 160.0f);
  aj2TextoLista = aj2Rampa(aj2TextoLista, deg >= 2 ? 0.0f : 1.0f, dt, 120.0f);
  aj2EdA = aj2Rampa(aj2EdA, deg >= 2 ? 1.0f : 0.0f, dt, 160.0f);
  aj2ResumoA = aj2Rampa(aj2ResumoA, deg == 0 ? 1.0f : 0.0f, dt, 120.0f);
  if (deg >= 1) aj2InspEspera += dt; else aj2InspEspera = 0;
  { float alvo = (deg >= 1 && (red || aj2InspEspera >= 0.06f)) ? 1.0f : 0.0f;
    if (red) aj2Insp = aj2Rampa(aj2Insp, alvo, dt, 120.0f), aj2VI = 0;
    else aj2Insp = aj2Mola(&aj2VI, aj2Insp, alvo, dt, alvo > aj2Insp ? 18.0f : 22.0f, alvo > aj2Insp ? 0.9f : 1.0f); }
}
static Aj2Pose aj2PoseAgora(void) {
  Aj2Pose p;
  int red = ajustes_animacoes_reduzidas();
  float n1 = red ? (aj2Degrau() >= 1) : aj2N1, n2 = red ? (aj2Degrau() >= 2) : aj2N2;
#ifdef AJUSTES_TESTE
  if (aj2PoseFixa) return aj2PoseTeste;
#endif
  p.mS = 1.0f - 0.05f * n1 - 0.05f * n2; p.mDx = -10.0f * n1 - 10.0f * n2;
  p.mA = aj2Miolo; p.mRot = aj2Rot;
  p.lS = 0.92f + 0.08f * n1 - 0.05f * n2; p.lDx = 380.0f * (1.0f - n1) - 30.0f * n2;
  p.lA = aj2ListaA * (1.0f - 0.5f * n2); p.lTxt = aj2TextoLista;
  p.lOy = 0;   // origem: a linha da categoria (ajDesenharTela)
  p.iDx = 160.0f * (1.0f - aj2Insp); p.iA = aj2Insp * (1.0f - aj2EdA);   // o editor cobre o inspetor
  p.rA = aj2ResumoA;
  p.eS = 0.92f + 0.08f * n2; p.eDx = 380.0f * (1.0f - n2); p.eA = aj2EdA;
  if (red) { p.mS = p.lS = p.eS = 1.0f; p.mDx = p.lDx = p.eDx = p.iDx = 0.0f; }
  return p;
}
