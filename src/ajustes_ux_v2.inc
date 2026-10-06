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
static float aj2V1, aj2N2, aj2V2;     // degraus 1 e 2 (molas); aj2N1 mora na geometria (desenho.inc)
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
// Medidas do indice (04/10): a linha, o vao entre linhas e o respiro entre grupos
// se ajustam a altura da ilha (aj2MenuMedir) — a 80% tudo cabe sem rolar; a 100% ou
// mais ficam nas medidas cheias e a lista rola.
#define AJ2_MENU_LN    aj2Ln
#define AJ2_MENU_GAP   aj2Gap
#define AJ2_MENU_CHIP  62.0f   // as tres pilulas do alto (Buscar, Diferentes do padrao, Avancadas); era 56
#define AJ2_MENU_CHIP_GAP 8.0f // vao entre as fileiras de pilulas, quando elas quebram
#define AJ2_MENU_TOPOL aj2ChipsTopo()   // onde comeca a lista: pilulas + 10 + fio 1 + 10
#define AJ2_MENU_GT    30.0f   // titulo de grupo (Telas, Assistir, Conta e sistema)
#define AJ2_MENU_GT_EX aj2GtEx // respiro extra antes do 2o e do 3o grupo
#define AJ2_MENU_FADE  aj2Fade
#define AJ2_MENU_ICO   34.0f
#define AJ2_MENU_ICO_GAP 18.0f
#define AJ2_MENU_RESERVA 80.0f // o que a linha aberta (descricao de 2 linhas) soma a uma fechada
#define AJ2_NMENU (AJ_MAX_SECOES + 2)
static float aj2MH[AJ2_NMENU], aj2MV[AJ2_NMENU], aj2MDesc[AJ2_NMENU], aj2MTempo[AJ2_NMENU];
static float aj2MRol, aj2MRolV, aj2SupY, aj2SupYv, aj2SupH, aj2SupHv;
static int aj2MIni;
static float aj2Ln = 58.0f, aj2Gap = 3.0f, aj2GtEx = 18.0f, aj2Fade = 70.0f;

// --- AS TRES PILULAS DO ALTO ----------------------------------------------------------
// Buscar, Diferentes do padrao e Avancadas. O ROTULO NUNCA SOME: a largura de
// cada uma vem do texto traduzido, medido na fonte da vez (a Montserrat da TV
// e mais larga que a Inter do Mac), e o arranjo se adapta nesta ordem:
//   1. as tres numa fileira, se cabem;
//   2. Buscar sozinha em cima e as duas de filtro embaixo;
//   3. Buscar + Diferentes em cima e Avancadas embaixo;
//   4. uma por fileira.
// Se nem sozinha a pilula cabe na largura da ilha, o corpo do texto desce um
// degrau (28 > 26 > 25) e, por ultimo, o rotulo e cortado com reticencias —
// cortado, nunca escondido. Com mais de uma fileira, as pilulas de cada
// fileira se esticam ate a largura da ilha (sem "dente" a direita).
// Antes (ate 05/10): fileira unica, e a terceira virava so o icone quando nao
// cabia — em ingles com Montserrat nao cabia nunca (captura do dono).
// Cada pilula tem a sua cor, tirada dos acentos do app (TEMA_ACENTO: Oceano,
// Ambar, Violeta): o `fill` tinge o fundo e a `marca` (a versao clara do
// matiz, feita para o escuro) pinta o icone. Nao dependem do acento escolhido.
#define AJ2_CHIP_ICO 28.0f
static const unsigned char AJ2_CHIP_TEMA[3] = { 2, 5, 3 };   // Oceano, Ambar, Violeta
static struct {
  float x[3], w[3];        // posicao (relativa ao miolo da ilha) e largura abertas
  int lin[3], linhas;      // a fileira de cada uma
  TxtEstilo e;
  int idioma, fonte, pronto;
} aj2Chips = { { 0 }, { 0 }, { 0, 0, 0 }, 1, TXT_V2_SEG, -1, -1, 0 };
static const char *aj2ChipRotulo(int c) {
  return i18n(c == 0 ? "Buscar" : c == 1 ? "Diferentes do padrão" : "Avançadas");
}
static float aj2ChipNatural(int c, TxtEstilo e) {
  return 20.0f + AJ2_CHIP_ICO + 10.0f + (float)txt_largura(e, aj2ChipRotulo(c)) + 22.0f;
}
static void aj2ChipsMedir(void) {
  static const TxtEstilo E[3] = { TXT_ILHA_NOME_L, TXT_V2_LN_B, TXT_V2_SEG };
  static const signed char ARR[4][3] = { { 0, 0, 0 }, { 0, 1, 1 }, { 0, 0, 1 }, { 0, 1, 2 } };
  const float livre = AJ2_MENU_W - 2 * AJ2_MENU_PAD, vao = 10.0f;
  float nat[3];
  int k, a, c, achou = 0;
  if (aj2Chips.pronto && aj2Chips.idioma == ajustes_idioma() && aj2Chips.fonte == valor[AJ_FONTE_UI]) return;
  for (k = 0; k < 3 && !achou; k++) {
    for (c = 0; c < 3; c++) nat[c] = aj2ChipNatural(c, E[k]);
    for (a = 0; a < 4 && !achou; a++) {
      float soma[3] = { 0, 0, 0 };
      int n[3] = { 0, 0, 0 }, cabe = 1, l;
      // No corpo grande so valem os arranjos de ate duas fileiras; tres fileiras
      // e o ultimo recurso, ja no corpo menor.
      if (a == 3 && k < 2) continue;
      for (c = 0; c < 3; c++) { l = ARR[a][c]; soma[l] += nat[c] + (n[l] ? vao : 0); n[l]++; }
      for (l = 0; l < 3; l++) if (soma[l] > livre + 0.5f) cabe = 0;
      if (!cabe && !(a == 3 && k == 2)) continue;
      aj2Chips.e = E[k]; aj2Chips.linhas = ARR[a][2] + 1;
      for (l = 0; l < 3; l++) {
        float x = 0, folga = livre - soma[l];
        for (c = 0; c < 3; c++) {
          float w;
          if (ARR[a][c] != l) continue;
          // Estica so quando quebrou (fileira unica fica a esquerda, como era);
          // a que nao cabe nem sozinha fica na largura da ilha e corta o texto.
          w = nat[c] + (aj2Chips.linhas > 1 && n[l] ? folga / (float)n[l] : 0.0f);
          if (w > livre) w = livre;
          aj2Chips.lin[c] = l; aj2Chips.x[c] = x; aj2Chips.w[c] = w;
          x += w + vao;
        }
      }
      achou = 1;
    }
  }
  for (c = 0; c < 3; c++) uxChipLinha[c] = aj2Chips.lin[c];
  aj2Chips.idioma = ajustes_idioma(); aj2Chips.fonte = valor[AJ_FONTE_UI]; aj2Chips.pronto = 1;
}
// Altura do cabecalho do indice (as pilulas e o fio), que empurra a lista.
static float aj2ChipsTopo(void) {
  aj2ChipsMedir();
  return (float)aj2Chips.linhas * AJ2_MENU_CHIP + (float)(aj2Chips.linhas - 1) * AJ2_MENU_CHIP_GAP + 21.0f;
}
// f = 1: medidas cheias (58 / 3 / 18); f = 0: as compactas (50 / 2 / 10);
// f = -0,35: o piso (47 / 1,65 / 7).
// R6 (04/10): o indice estava "enorme" — linha, icone e corpo menores.
static float aj2MenuNecessario(float f, int nLin) {
  float ln = 50.0f + 8.0f * f, gap = 2.0f + f, ex = 10.0f + 8.0f * f;
  return AJ2_MENU_TOPOL + 3.0f * AJ2_MENU_GT + 2.0f * ex + (float)nLin * (ln + gap) + AJ2_MENU_RESERVA;
}
static void aj2MenuMedir(void) {
  GfxRect m = aj2MenuR();
  float janela = m.h - 2 * AJ2_MENU_PAD, f = 1.0f, n0, n1;
  int nLin = nSecoes > 0 ? nSecoes : 11;
  n0 = aj2MenuNecessario(0.0f, nLin); n1 = aj2MenuNecessario(1.0f, nLin);
  aj2Fade = 70.0f;
  // Com as pilulas em duas fileiras o cabecalho cresceu ~76: a linha pode
  // descer ate 47 (f = -0,35) antes de a lista passar a rolar — a 100% o
  // indice continua inteiro na tela, com as pilulas sempre a vista.
  if (n1 <= janela) aj2Fade = 0.0f;
  else if (n1 > n0 && (janela - n0) / (n1 - n0) >= -0.35f) { f = (janela - n0) / (n1 - n0); aj2Fade = 0.0f; }
  aj2Ln = 50.0f + 8.0f * f; aj2Gap = 2.0f + f; aj2GtEx = 10.0f + 8.0f * f;
}
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
  float w = AJ2_MENU_W - 2 * AJ2_MENU_PAD - 2 * 28.0f - AJ2_MENU_ICO - AJ2_MENU_ICO_GAP;
  float h = txt_bloco(TXT_V2_26, aj2MenuDesc(j), 0, 0, 0, 0, -4000, w, 32.4f, 0, 2);
  if (h < 32.4f) h = 32.4f;
  return AJ2_MENU_LN - 2.0f + h + 14.0f;
}
static int aj2MenuFocado(void) { return focoIndice ? uxIndice : -1; }
// Os tres grupos do indice: so a apresentacao. As categorias seguem a ordem de
// TELA[] (reordenada em ajustes_ux_tela.inc: Telas = 0-4, Assistir = 5-8, o
// resto em Conta e sistema); nada aqui mexe em valor[] nem em CHAVE[].
static int aj2Grupo(int s) { return s < AJS_REPRODUCAO ? 0 : s < AJS_CONTAS ? 1 : 2; }
static int aj2GrupoIni(int s) { return s == 0 || aj2Grupo(s) != aj2Grupo(s - 1); }
static const char *aj2GrupoNome(int g) { return g == 0 ? "Telas" : g == 1 ? "Assistir" : "Conta e sistema"; }
static int aj2ContaOpcoes(int sec) {
  int i, n = 0;
  for (i = secIni[sec] + 1; i < secFim(sec); i++) if (TELA[i].tipo == IT_OPC) n++;
  return n;
}
static float aj2MenuAltura(int j) { return j < 2 ? AJ2_MENU_CHIP : AJ2_MENU_LN; }
// y do alto da linha j relativo ao topo do conteudo (sem a rolagem). Cada
// pilula fica na sua fileira (aj2Chips).
static float aj2MenuY(int j) {
  float y;
  int k, g;
  if (j < 2) { aj2ChipsMedir(); return (float)aj2Chips.lin[j] * (AJ2_MENU_CHIP + AJ2_MENU_CHIP_GAP); }
  y = AJ2_MENU_TOPOL;
  for (k = 2; k < j; k++) y += aj2MH[k] + AJ2_MENU_GAP;
  g = aj2Grupo(j - 2);
  return y + (float)(g + 1) * AJ2_MENU_GT + (float)g * AJ2_MENU_GT_EX;
}
static void aj2MenuAtualizar(float dt) {
  int j, n = nSecoes + 2, f = aj2MenuFocado(), red = ajustes_animacoes_reduzidas();
  GfxRect m = aj2MenuR();
  float janela = m.h - 2 * AJ2_MENU_PAD, alvo;
  if (n > AJ2_NMENU) n = AJ2_NMENU;
  aj2MenuMedir();
  for (j = 0; j < n; j++) {
    float alvoH = j < 2 ? AJ2_MENU_CHIP : j == f ? aj2MenuAberta(j) : AJ2_MENU_LN;
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
    float ty = aj2MenuY(alvoJ), th = (f >= 0 && alvoJ == f && alvoJ >= 2) ? aj2MenuAberta(alvoJ) : aj2MenuAltura(alvoJ);
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
  float a;
  if (AJ2_MENU_FADE <= 0.5f) return yc < base ? 1.0f : 0.0f;
  a = (base - yc) / AJ2_MENU_FADE;
  return a < 0 ? 0 : a > 1 ? 1 : a;
}
static void aj2DesenharMenu(const Aj2Pose *p) {
  GfxRect m = aj2MenuR();
  int j, n = nSecoes + 2, f = aj2MenuFocado();
  float topo = m.y + AJ2_MENU_PAD, base = m.y + m.h - AJ2_MENU_PAD;
  float ar, ag, ab, grupo = gfx_opacidade_grupo;
  if (n > AJ2_NMENU) n = AJ2_NMENU;
  if (p->mA <= 0.003f) return;
  aj2MenuMedir();
  ajustes_acento(&ar, &ag, &ab);
  gfx_opacidade_grupo = grupo * p->mA;
  gfx_transformar(m.x, m.y + m.h * 0.5f, p->mS, p->mDx, 0);
  ajIlha(m, 40.0f, 1, 0);
  gfx_recorte(m.x, topo, m.w, base - topo);
  // A superficie de foco (vidro 14%, solido #2B2D34 com sombra).
  if (f >= 2) {
    GfxRect r = { m.x + AJ2_MENU_PAD, topo + aj2SupY - aj2MRol, m.w - 2 * AJ2_MENU_PAD, aj2SupH };
    if (ajVid()) gfx_cor(r, 38.0f / r.h, 1, 1, 1, 0.14f);
    else {
      gfx_rect((GfxRect){ r.x - 10, r.y + 2, r.w + 20, r.h + 26 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f, 0, 0, 0, 0.30f);
      gfx_cor(r, 38.0f / r.h, 0.169f, 0.176f, 0.204f, 1);
    }
    gfx_cor(r, 38.0f / r.h, ar, ag, ab, 0.14f);   // um toque leve do acento
  }
  // As tres pilulas do alto (aj2Chips). Recolhido (rotulos a 0) viram so o
  // icone, cada uma no comeco da sua fileira, para caberem na coluna de
  // icones; a terceira some junto com os rotulos.
  { float rot = p->mRot < 0 ? 0 : p->mRot > 1 ? 1 : p->mRot;
    float altura = AJ2_MENU_TOPOL - 21.0f, fio = aj2MenuMascara(topo - aj2MRol + altura + 10.0f, base);
    float xl[3] = { 0, 0, 0 };   // o x corrente de cada fileira (as pilulas encolhem com `rot`)
    int c;
    for (c = 0; c < 3 && n >= 2; c++) {
      const AcentoFixo *cor = &TEMA_ACENTO[AJ2_CHIP_TEMA[c]];
      float fr = ((cor->fill >> 16) & 255) / 255.0f, fg = ((cor->fill >> 8) & 255) / 255.0f, fb = (cor->fill & 255) / 255.0f;
      float mr = ((cor->marca >> 16) & 255) / 255.0f, mg = ((cor->marca >> 8) & 255) / 255.0f, mb = (cor->marca & 255) / 255.0f;
      int l = aj2Chips.lin[c];
      float cy = topo - aj2MRol + (float)l * (AJ2_MENU_CHIP + AJ2_MENU_CHIP_GAP);
      float mkc = aj2MenuMascara(cy + AJ2_MENU_CHIP * 0.5f, base), ac = c == 2 ? rot : 1.0f;
      float wc = AJ2_MENU_CHIP, w = wc + (aj2Chips.w[c] - wc) * rot, cx = m.x + AJ2_MENU_PAD + xl[l];
      int sel = c == uxIndice, fc = c == 2 ? focoIndice && uxChipAv : sel && focoIndice && !uxChipAv;
      int lig_ = c == 2 && lig(AJ_AVANCADAS);
      // LIGADA: a busca/diferencas aberta (a entrada do indice selecionada) ou
      // as avancadas ligadas. E o estado que fica quando o foco sai.
      int ativa = c == 2 ? lig_ : sel;
      GfxRect r = { cx, cy, w, AJ2_MENU_CHIP };
      xl[l] += w + 10.0f;
      if (c == 2 && rot <= 0.003f) continue;   // recolhido: a terceira nao cabe na coluna
      if (mkc > 0.003f) {
        float mc = mkc * ac, raio = AJ2_MENU_CHIP * 0.5f;
        // Foco = preenchimento e elevacao (sem contorno): a sombra, o fundo
        // quase cheio na cor da pilula e o icone branco. Ligada sem foco =
        // o fundo tingido a meio caminho. Em repouso = so um veu da cor.
        if (fc) gfx_rect((GfxRect){ r.x - 10, r.y + 2, r.w + 20, r.h + 26 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f, 0, 0, 0, 0.34f * mc);
        ajNeutro(r, raio, 0.10f, AJ_CHIP_SOL, mc);
        gfx_cor(r, raio / r.h, fr, fg, fb, (fc ? 0.78f : ativa ? 0.42f : 0.20f) * mc);
        { const char *ic = c == 0 ? "aj_search" : c == 1 ? "aj_rotate-ccw-clock" : lig_ ? "aj_eye" : "aj_eye-off";
          GfxRect ri = { cx + (wc - AJ2_CHIP_ICO) * 0.5f + (20.0f - (wc - AJ2_CHIP_ICO) * 0.5f) * rot,
                         cy + (AJ2_MENU_CHIP - AJ2_CHIP_ICO) * 0.5f, AJ2_CHIP_ICO, AJ2_CHIP_ICO };
          if (fc) gfx_icone(ri, ic, 1, 1, 1, mc);
          else gfx_icone(ri, ic, mr, mg, mb, (ativa ? 1.0f : 0.92f) * mc); }
        if (rot > 0.003f && w > wc + 4.0f) {
          // O estado das avancadas por extenso ("Ligado"/"Desligado") a direita,
          // quando a pilula esticada tem lugar para ele sem apertar o rotulo.
          float tx = cx + 20.0f + AJ2_CHIP_ICO + 10.0f, tmax = w - (20.0f + AJ2_CHIP_ICO + 10.0f) - 22.0f;
          if (c == 2) {
            TxtLinha st = ajTxt(TXT_V2_24, lig_ ? "Ligado" : "Desligado");
            if (tmax - (float)txt_largura(aj2Chips.e, aj2ChipRotulo(c)) >= st.w + 18.0f) {
              txt_desenhar_alpha(st, cx + w - 22.0f - st.w, cy + (AJ2_MENU_CHIP - st.h) * 0.5f, (lig_ || fc ? 0.92f : 0.6f) * mc * rot);
              tmax -= st.w + 18.0f;
            }
          }
          { TxtLinha tc = ajTxtC(aj2Chips.e, aj2ChipRotulo(c), tmax);
            txt_desenhar_alpha(tc, tx, cy + (AJ2_MENU_CHIP - tc.h) * 0.5f, ((fc || ativa) ? 1.0f : 0.9f) * mc * rot); }
        }
      }
    }
    if (fio > 0.003f) gfx_cor((GfxRect){ m.x + AJ2_MENU_PAD + 24.0f, topo - aj2MRol + altura + 10.0f, m.w - 2 * AJ2_MENU_PAD - 48.0f, 1 }, 0, 1, 1, 1, 0.08f * fio);
  }
  for (j = 0; j < n; j++) {
    float y = topo + aj2MenuY(j) - aj2MRol, h = aj2MH[j];
    int foco = j == f, ativa = j == uxIndice;
    float rx = m.x + AJ2_MENU_PAD, mk, al, lc;
    if (j < 2) continue;
    if (y > base || y + h < topo) continue;
    if (aj2GrupoIni(j - 2)) {
      float ty = y - AJ2_MENU_GT + 6.0f, mt = aj2MenuMascara(ty + 10.0f, base);
      if (p->mRot > 0.003f && mt > 0.003f) ajKickerAcento(aj2GrupoNome(aj2Grupo(j - 2)), rx + 28.0f, ty, mt * p->mRot);
    }
    mk = aj2MenuMascara(y + (h > AJ2_MENU_LN + 1 ? AJ2_MENU_LN * 0.5f + 1.0f : h * 0.5f), base);
    if (mk <= 0.003f) continue;
    al = (foco || ativa) ? 1.0f : 0.66f;
    // o rotulo fica no alto da linha que abre: centro 38 fechada, 39 aberta
    lc = y + (h > AJ2_MENU_LN + 1 ? AJ2_MENU_LN * 0.5f + (h - AJ2_MENU_LN) / (aj2MenuAberta(j) - AJ2_MENU_LN + 0.001f) : h * 0.5f);
    if (ativa && !foco) gfx_cor((GfxRect){ rx + 9.0f, lc - 4.0f, 8, 8 }, 0.5f, ar, ag, ab, mk);
    ajIcone(aj2MenuIcone(j), rx + 28.0f, lc - AJ2_MENU_ICO * 0.5f, AJ2_MENU_ICO, al * mk);
    if (p->mRot > 0.003f) {
      TxtLinha t, cn;
      char qn[16];
      snprintf(qn, sizeof qn, "%d", aj2ContaOpcoes(j - 2));
      cn = ajTxt(TXT_V2_26, qn);
      { float livre = AJ2_MENU_W - 2 * AJ2_MENU_PAD - 28 - AJ2_MENU_ICO - AJ2_MENU_ICO_GAP - 20 - cn.w - 16;
        TxtEstilo e = ativa ? TXT_V2_MENU_B : TXT_V2_MENU;
        // Nome que nao cabe inteiro (idiomas longos): um corpo menor antes do
        // corte com reticencias.
        if ((float)txt_largura(e, aj2MenuTitulo(j)) > livre) e = TXT_V2_28;
        t = ajTxtC(e, aj2MenuTitulo(j), livre); }
      txt_desenhar_alpha(cn, m.x + m.w - AJ2_MENU_PAD - 28.0f - cn.w, lc - cn.h * 0.5f, 0.5f * mk * p->mRot);
      txt_desenhar_alpha(t, rx + 28 + AJ2_MENU_ICO + AJ2_MENU_ICO_GAP, lc - t.h * 0.5f, al * mk * p->mRot);
      if (aj2MDesc[j] > 0.003f && h > AJ2_MENU_LN + 4) {
        float dy = (1.0f - aj2MDesc[j]) * 6.0f;
        ajBloco(TXT_V2_26, aj2MenuDesc(j), rx + 28 + AJ2_MENU_ICO + AJ2_MENU_ICO_GAP, y + AJ2_MENU_LN - 2.0f + dy,
                m.w - 2 * AJ2_MENU_PAD - 2 * 28.0f - AJ2_MENU_ICO - AJ2_MENU_ICO_GAP, 32.4f, 0.7f * aj2MDesc[j] * mk * p->mRot, 2);
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
    // A imagem da categoria preenche o quadro de cima (16:9, arte real de ponta
    // a ponta); embaixo, a descricao com a contagem a direita, os grupos em
    // chips e as dicas no rodape — o mesmo raio e o mesmo vao da camada 2.
    int sec = uxIndice - 2, i, nOp = 0, nAv = 0, nl = 1;
    float ph = w * 9.0f / 16.0f, numW, descW, descH, chipsH, abaixo, fimUtil, cx, cy;
    const float CH = 46.0f, CG = 10.0f, LH = 34.0f;
    const char *desc = sec < (int)(sizeof SECAO_AJUDA / sizeof *SECAO_AJUDA) ? SECAO_AJUDA[sec] : TELA[secIni[sec]].sub;
    char q[16], t[96];
    TxtLinha ln, lt;
    for (i = secIni[sec] + 1; i < secFim(sec); i++)
      if (TELA[i].tipo == IT_OPC) { nOp++; if (uxAvancada(TELA[i].op)) nAv++; }
    snprintf(q, sizeof q, "%d", nOp);
    snprintf(t, sizeof t, nAv == 1 ? i18n("opções · %d avançada") : i18n("opções · %d avançadas"), nAv);
    ln = ajTxt(TXT_V2_NUM, q); lt = ajTxt(TXT_V2_24, t);
    numW = ln.w + 10.0f + lt.w;
    descW = w - numW - 40.0f;
    descH = txt_bloco(TXT_V2_26, desc, 0, 0, 0, x, -4000, descW, LH, 0, 2);
    if (descH < ln.h) descH = ln.h;
    // chips: mede as linhas antes de desenhar
    cx = x;
    for (i = secIni[sec] + 1; i < secFim(sec); i++) {
      float bw;
      if (TELA[i].tipo != IT_ROT) continue;
      bw = 20 + ajTxt(TXT_V2_24, TELA[i].titulo).w + 20;
      if (cx > x && cx + bw > x + w) { cx = x; nl++; }
      cx += bw + CG;
    }
    chipsH = cx == x ? 0 : nl * CH + (nl - 1) * CG;
    abaixo = 24.0f + descH + (chipsH > 0 ? 18.0f + chipsH : 0);
    fimUtil = r.y + r.h - 40.0f - 40.0f - 24.0f;   // acima das dicas
    if (y + ph + abaixo > fimUtil) ph = fimUtil - abaixo - y;
    ajCenaTroca(AJ_TROCA_RESUMO, sec, -1, x, y, w, ph);
    y += ph + 24.0f;
    ajBloco(TXT_V2_26, desc, x, y, descW, LH, 0.66f, 2);
    { float base = y + ln.h * 0.80f, nx = x + w - numW;
      txt_desenhar(ln, nx, y - 4.0f);
      txt_desenhar_alpha(lt, nx + ln.w + 10.0f, base - lt.h * 0.80f - 4.0f, 0.55f); }
    y += descH + 18.0f;
    cx = x; cy = y;
    for (i = secIni[sec] + 1; i < secFim(sec); i++) {
      TxtLinha c;
      float bw;
      if (TELA[i].tipo != IT_ROT) continue;
      c = ajTxt(TXT_V2_24, TELA[i].titulo);
      bw = 20 + c.w + 20;
      if (cx > x && cx + bw > x + w) { cx = x; cy += CH + CG; }
      ajNeutro((GfxRect){ cx, cy, bw, CH }, CH * 0.5f, 0.08f, AJ_CHIP_SOL, 1);
      txt_desenhar_alpha(c, cx + 20, cy + (CH - c.h) * 0.5f, 0.85f);
      cx += bw + CG;
    }
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
  float cheia = w * 9.0f / 16.0f;
  if (h >= cheia - 0.5f) return ajVisualCena(sec, -1, x, y, w);
  // Sem altura para o 16:9 (Ajustes grandes): o quadro encolhe por cima e por
  // baixo, a cena fica no meio.
  gfx_recorte(x, y, w, h);
  ajVisualCena(sec, -1, x, y - (cheia - h) * 0.5f, w);
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
// Cor de destaque (acentos-mockup.html, quadro 1): a PREVIA VIVA da cor — o
// cartao de um titulo com o que a cor pinta (marcas, botao em foco, chip
// ligado, pagina ativa, a luz do aviso) — e, embaixo do texto de ajuda, as
// medidas que ela tem de passar (aj2CorMedidas). A cor e a PENDENTE: o
// inspetor troca valor[AJ_TEMA] enquanto desenha (ajDesenharInspetor).
static void aj2Cor8(const float c[3], int *r, int *g, int *b) {
  *r = (int)(c[0] * 255.0f + 0.5f); *g = (int)(c[1] * 255.0f + 0.5f); *b = (int)(c[2] * 255.0f + 0.5f);
}
static float aj2PreviaCor(float x, float y, float w) {
  // O cartao do mockup (540x300) na largura do inspetor, a 2:1: a ilha da v2 e
  // mais baixa que a do mockup das cores, e as medidas tem de caber embaixo.
  float h = w * 0.5f, s = h / 300.0f, ac[3], mc[3], hd[3], tk;
  int r8, g8, b8;
  GfxRect q = { x, y, w, h };
  const char *arte = ajFundoArte();
  ajustes_acento(&ac[0], &ac[1], &ac[2]);
  ajustes_acento_marca(&mc[0], &mc[1], &mc[2]);
  ajustes_acento_hdr(&hd[0], &hd[1], &hd[2]);
  tk = ajustes_acento_tinta(NULL, NULL, NULL);
#define PX(v) (x + (v) * s)
#define PY(v) (y + (v) * s)
  gfx_recorte(x, y, w, h);
  if (!ajImg(arte, q, 22.0f * s, 1)) gfx_cor(q, 22.0f * s / h, 0.10f, 0.105f, 0.12f, 1);
  // veu: linear-gradient(90deg, rgba(8,8,10,.92), rgba(8,8,10,.35) 75%)
  gfx_cor(q, 22.0f * s / h, 0.031f, 0.031f, 0.039f, 0.35f);
  gfx_veu_css((GfxRect){ x, y, w * 0.75f, h }, 2, 0, 1.0f, 0.877f);
  ajCaps(TXT_AJ_CAPS13, "Filme · 2026 · 119 min", 1.9f, PX(26), PY(20), 0.5f);
  ajLogo(ajArte(AJA_LOGO, ajArteFundoN), PX(24), PY(44), 250.0f * s, 78.0f * s, 1);
  { char up[64]; float mx = PX(26);
    static const char *const M[3] = { "4K", "DOLBY VISION", "DOLBY ATMOS" };
    int k;
    for (k = 0; k < 3; k++) {
      const float *c = k < 2 ? mc : hd;
      aj2Cor8(c, &r8, &g8, &b8);
      snprintf(up, sizeof up, "%s", M[k]);
      mx += txt_tracking(TXT_AJ_CAPS13, up, r8, g8, b8, mx, PY(132), 1, 1.3f) + 12.0f * s;
    } }
  // botao em foco
  { int ti = (int)(tk * 255.0f + 0.5f);
    TxtLinha l = txt_linha(TXT_G20B, i18n("Reproduzir"), ti, ti, ti, 255);
    float bh = 46.0f * s, bw = 16.0f * s + 18.0f * s + 8.0f * s + l.w + 20.0f * s;
    ajPilulaAcento((GfxRect){ PX(26), PY(166), bw, bh }, bh * 0.5f, 1);
    gfx_icone((GfxRect){ PX(26) + 16.0f * s, PY(166) + (bh - 18.0f * s) * 0.5f, 18.0f * s, 18.0f * s }, "pl_play-f", tk, tk, tk, 1);
    txt_desenhar(l, PX(26) + 42.0f * s, PY(166) + (bh - l.h) * 0.5f);
    gfx_cor((GfxRect){ PX(26) + bw + 14.0f * s, PY(166), bh, bh }, 0.5f, 1, 1, 1, 0.12f);
    gfx_cor((GfxRect){ PX(26) + bw + 14.0f * s + bh + 8.0f * s, PY(166), bh, bh }, 0.5f, 1, 1, 1, 0.12f); }
  // chip ligado + pagina ativa
  { TxtLinha l;
    float ch = 38.0f * s, cw;
    aj2Cor8(mc, &r8, &g8, &b8);
    l = txt_linha(TXT_G16B, i18n("Só MP4"), r8, g8, b8, 255);
    cw = 15.0f * s + 7.0f * s + 8.0f * s + l.w + 15.0f * s;
    gfx_cor((GfxRect){ PX(26), PY(232), cw, ch }, 0.5f, 0.055f, 0.059f, 0.071f, 0.7f);
    gfx_cor((GfxRect){ PX(26), PY(232), cw, ch }, 0.5f, ac[0], ac[1], ac[2], 0.22f);
    gfx_cor((GfxRect){ PX(26) + 15.0f * s, PY(232) + (ch - 7.0f * s) * 0.5f, 7.0f * s, 7.0f * s }, 0.5f, mc[0], mc[1], mc[2], 1);
    txt_desenhar(l, PX(26) + 30.0f * s, PY(232) + (ch - l.h) * 0.5f);
    { float dx = PX(26) + cw + 22.0f * s; int k;
      for (k = 0; k < 4; k++) {
        float dw = (k == 1 ? 18.0f : 6.0f) * s;
        if (k == 1) gfx_cor((GfxRect){ dx, PY(248), dw, 6.0f * s }, 0.5f, mc[0], mc[1], mc[2], 1);
        else gfx_cor((GfxRect){ dx, PY(248), dw, 6.0f * s }, 0.5f, 0.953f, 0.949f, 0.937f, 0.3f);
        dx += dw + 7.0f * s;
      } } }
  // a ilha do aviso, com a luz do acento
  { TxtLinha l = txt_linha(TXT_G16B, i18n("Começa em 5 min"), 243, 242, 239, 255);
    float ih = 44.0f * s, iw = 8.0f * s + 30.0f * s + 10.0f * s + l.w + 14.0f * s;
    GfxRect il = { x + w - 18.0f * s - iw, PY(18), iw, ih };
    gfx_cor(il, 0.5f, 0.055f, 0.059f, 0.071f, 0.82f);
    gfx_cor((GfxRect){ il.x + 8.0f * s, il.y + 7.0f * s, 30.0f * s, 30.0f * s }, 0.5f, ac[0], ac[1], ac[2], 0.24f);
    gfx_icone((GfxRect){ il.x + 15.0f * s, il.y + 14.0f * s, 16.0f * s, 16.0f * s }, "aj_bell", mc[0], mc[1], mc[2], 1);
    txt_desenhar(l, il.x + 48.0f * s, il.y + (ih - l.h) * 0.5f); }
#undef PX
#undef PY
  gfx_sem_recorte();
  return h;
}
// As MEDIDAS da cor (o "tok" do mockup): pilula, pilula contra a ilha, marca
// no escuro, chip ligado e "HDR" de grupo, cada uma com a razao de contraste
// e a meta (verde passa, vermelho nao); embaixo, a familia e o OKLCH.
static void aj2Num(char *d, size_t n, const char *fmt, float v) {
  char *p;
  snprintf(d, n, fmt, v);
  if (ajustes_idioma() == IDIOMA_PT || ajustes_idioma() == IDIOMA_PTPT)
    for (p = d; *p; p++) if (*p == '.') *p = ',';
}
static float aj2CorMedidas(float x, float y, float w, float fim, float a) {
  static const float ILHA[3] = { 18 / 255.0f, 19 / 255.0f, 22 / 255.0f };
  float ac[3], mc[3], hd[3], ch[3], tk, tinta[3], lab[3], y0 = y;
  int k;
  char hx[5][12], cr[16], cap[160];
  struct { const char *rot; const float *sw; const char *hex; float c, meta; int aro; } L[5];
  ajustes_acento(&ac[0], &ac[1], &ac[2]);
  ajustes_acento_marca(&mc[0], &mc[1], &mc[2]);
  ajustes_acento_hdr(&hd[0], &hd[1], &hd[2]);
  tk = ajustes_acento_tinta(NULL, NULL, NULL);
  tinta[0] = tinta[1] = tinta[2] = tk;
  if (tk < 0.5f) { tinta[0] = ILHA[0]; tinta[1] = ILHA[1]; tinta[2] = ILHA[2]; }
  for (k = 0; k < 3; k++) ch[k] = ILHA[k] + (ac[k] - ILHA[k]) * 0.22f;
  { const float *C[5] = { ac, ILHA, mc, ch, hd };
    for (k = 0; k < 5; k++) { int r8, g8, b8; aj2Cor8(C[k], &r8, &g8, &b8); snprintf(hx[k], sizeof hx[k], "#%02x%02x%02x", r8, g8, b8); } }
  L[0].rot = tk > 0.5f ? "Pílula · tinta branca" : "Pílula · tinta escura"; L[0].sw = ac; L[0].hex = hx[0];
  L[0].c = corviva_contraste(ac, tinta); L[0].meta = 4.5f; L[0].aro = 0;
  L[1].rot = "Pílula contra a ilha"; L[1].sw = ac; L[1].hex = hx[1]; L[1].c = corviva_contraste(ac, ILHA); L[1].meta = 3.0f; L[1].aro = 1;
  L[2].rot = "Marca no escuro"; L[2].sw = mc; L[2].hex = hx[2]; L[2].c = corviva_contraste(mc, ILHA); L[2].meta = 7.0f; L[2].aro = 0;
  L[3].rot = "Chip ligado (22%)"; L[3].sw = ch; L[3].hex = hx[3]; L[3].c = corviva_contraste(mc, ch); L[3].meta = 4.5f; L[3].aro = 0;
  L[4].rot = "\"HDR\" do grupo"; L[4].sw = hd; L[4].hex = hx[4]; L[4].c = corviva_contraste(hd, ILHA); L[4].meta = 4.5f; L[4].aro = 0;
  for (k = 0; k < 5 && y + 42.0f <= fim; k++) {
    float yc = y + 21.0f;
    TxtLinha t = ajTxtC(TXT_V2_24, L[k].rot, w - 260.0f), hxl, crl;
    int ok = L[k].c >= L[k].meta;
    gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.07f * a);
    if (L[k].aro) gfx_cor((GfxRect){ x - 4, yc - 15, 30, 30 }, 0.5f, 0.953f, 0.949f, 0.937f, 0.2f * a);
    if (L[k].aro) gfx_cor((GfxRect){ x - 3, yc - 14, 28, 28 }, 0.5f, ILHA[0], ILHA[1], ILHA[2], a);
    gfx_cor((GfxRect){ x, yc - 11, 22, 22 }, 0.5f, L[k].sw[0], L[k].sw[1], L[k].sw[2], a);
    txt_desenhar_alpha(t, x + 34, yc - t.h * 0.5f, 0.5f * a);
    aj2Num(cr, sizeof cr, "%.1f:1", L[k].c);
    crl = txt_linha(TXT_V2_SEG, cr, ok ? 110 : 240, ok ? 210 : 110, ok ? 140 : 110, 255);
    txt_desenhar_alpha(crl, x + w - crl.w, yc - crl.h * 0.5f, a);
    hxl = ajTxt(TXT_V2_24, L[k].hex);
    txt_desenhar_alpha(hxl, x + w - 96.0f - hxl.w, yc - hxl.h * 0.5f, 0.85f * a);
    y += 42.0f;
  }
  if (y + 40.0f <= fim) {
    char Ls[16], Cs[16];
    float Cc, hh;
    corviva_srgb_para_oklab(ac, lab);
    Cc = sqrtf(lab[1] * lab[1] + lab[2] * lab[2]);
    hh = atan2f(lab[2], lab[1]) * 57.29578f;
    if (hh < 0) hh += 360.0f;
    aj2Num(Ls, sizeof Ls, "%.2f", lab[0]);
    aj2Num(Cs, sizeof Cs, "%.3f", Cc);
    snprintf(cap, sizeof cap, "%s · OKLCH L %s · C %s · %.0f°", i18n(tk > 0.5f ? "Profundo" : "Claro"), Ls, Cs, hh);
    { TxtLinha c = ajTxtC(TXT_V2_24, cap, w);
      gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.07f * a);
      txt_desenhar_alpha(c, x, y + 14.0f, 0.42f * a); }
    y += 40.0f + 14.0f;
  }
  return y - y0;
}

// --- A ANIMACAO ---------------------------------------------------------------------
static void aj2Atualizar(float dt) {
  int deg = aj2Degrau(), red = ajustes_animacoes_reduzidas();
  float a1 = deg >= 1 ? 1.0f : 0.0f, a2 = deg >= 2 ? 1.0f : 0.0f;
  ajMovAtualizar(dt);
  ajCenaTrocaAtualizar(dt);
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
