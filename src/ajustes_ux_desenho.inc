// AJUSTES NO GLASS UI — ILHAS (mockup aprovado pelo dono em 03/10/2026,
// "perfeito": ajustes-mockup.html, DESIGN.md). Tres ilhas sobre a arte do
// titulo: categorias (o ponto no acento marca a categoria aberta), a lista da
// categoria e o inspetor com a previa DE VERDADE da opcao em foco — arte do
// catalogo, marcas de formato e os graficos que a versao anterior tinha, no
// lugar da foto fixa "Exemplo ilustrativo".
//
// Medidas: as do CSS do mockup, em px de 1080p. Os nomes seguem as classes de
// la (ilha, idx, folha, ln, tg, seg, insp, dicas) para que a comparacao lado a
// lado ache cada peca.

#include "home.h"
// A linha e o inspetor do "Guia de uso" moram com o guia (ajustes_ux_guia.inc).
static void guiaInspetorAjustes(GfxRect ir);
static int guiaTotal(void);
static char uxDirArte[1024];
void ajustes_recursos(const char *dirArte) {
  snprintf(uxDirArte, sizeof uxDirArte, "%s", dirArte ? dirArte : "");
}
static const char *uxResumo(int op) {
  if (op == AJ_HOME_LAYOUT && uxIndice == 1) return i18n("Escolha como o destaque e as fileiras se organizam na Tela inicial.");
  return ajudaOpcao(op);
}
static const char *uxCaminho(int op) {
  int i;
  for (i = 0; i < AJ_N_TELA; i++)
    if (TELA[i].tipo == IT_OPC && TELA[i].op == op) return TELA[secIni[secDoItem[i]]].titulo;
  return "Ajustes";
}
static int uxSecaoDe(int op) {
  int i;
  for (i = 0; i < AJ_N_TELA; i++)
    if (TELA[i].tipo == IT_OPC && TELA[i].op == op) return secDoItem[i];
  return 0;
}
// O bloco (ROT) da opcao, "" sem bloco.
static const char *uxBloco(int op) {
  int i, j;
  for (i = 0; i < AJ_N_TELA; i++)
    if (TELA[i].tipo == IT_OPC && TELA[i].op == op) {
      for (j = i; j > secIni[secDoItem[i]]; j--)
        if (TELA[j].tipo == IT_ROT) return TELA[j].titulo;
      return "";
    }
  return "";
}

// --- ARTE ---------------------------------------------------------------------
// A arte das previas: a do CATALOGO quando ele ja chegou (os titulos da conta,
// os mesmos da Home), e a amostra embarcada (deploy/app/art) antes disso. `n`
// e o indice da amostra que o mockup usa; com catalogo vira o titulo n.
enum { AJA_FUNDO = 0, AJA_POSTER, AJA_LOGO };
static int ajArteFundoN = 12;   // titulo atras da tela (ajustes_teste_arte)
static const char *ajArte(int tipo, int n) {
  static char buf[8][1100];
  static int k;
  char *b;
  if (n < 0) n = 0;
  if (cat_n() > 0) {
    const CatItem *c = cat_item(n % cat_n());
    if (c) {
      const char *s = tipo == AJA_POSTER ? c->poster : tipo == AJA_LOGO ? c->logo : c->backdrop;
      if (s[0]) return s;
      if (tipo == AJA_LOGO) return "";
    }
  }
  if (!uxDirArte[0]) return "";
  k = (k + 1) % 8;
  b = buf[k];
  snprintf(b, sizeof buf[0], "%s/%s%02d.%s", uxDirArte,
           tipo == AJA_POSTER ? "poster/" : tipo == AJA_LOGO ? "logo/" : "", n % 40,
           tipo == AJA_LOGO ? "png" : "jpg");
  return b;
}
// Arte em "cover" num retangulo de cantos `raioPx`. Devolve 1 se desenhou.
static int ajImg(const char *c, GfxRect r, float raioPx, float a) {
  GLuint t;
  if (!c || !c[0] || r.w < 1.0f || r.h < 1.0f || a <= 0.003f) return 0;
  t = tex_obter_larg(c, r.w > r.h * 1.78f ? r.w : r.h * 1.78f);
  if (!t) {
    gfx_cor(r, raioPx / r.h, 0.16f, 0.165f, 0.18f, 0.6f * a);
    return 0;
  }
  gfx_tex_aspect_atual = tex_aspecto(c);
  gfx_rect(r, t, GFX_ARTE, 0, 0, 0, raioPx / r.h, 1, 1, 1, a);
  gfx_tex_aspect_atual = 0;
  return 1;
}
// Logo do titulo inteiro dentro de maxW x maxH, alinhado a esquerda e em cima.
static float ajLogo(const char *c, float x, float y, float maxW, float maxH, float a) {
  GLuint t;
  float asp, w, h;
  if (!c || !c[0]) return 0.0f;
  t = tex_obter_larg(c, maxW);
  if (!t) return 0.0f;
  asp = tex_aspecto(c);
  if (asp <= 0.0f) asp = 3.0f;
  w = maxW; h = w / asp;
  if (h > maxH) { h = maxH; w = h * asp; }
  gfx_tex_aspect_atual = asp;
  gfx_rect((GfxRect){ x, y, w, h }, t, GFX_ARTE, 0, 0, 0, 0, 1, 1, 1, a);
  gfx_tex_aspect_atual = 0;
  return w;
}

// --- MATERIAL -------------------------------------------------------------------
static int ajVid(void) { return ajustes_vidro(); }
// Ilha: sombra curta, miolo (vidro 80% / solido #15161A) e a luz larga do canto
// de cima, sem contorno. `modal` = miolo a 86%, para a ilha por cima de outra.
static void ajIlha(GfxRect r, float raioPx, float a, int modal) {
  float raio = raioPx / r.h;
  if (a <= 0.003f) return;
  gfx_rect((GfxRect){ r.x - 20, r.y - 6, r.w + 40, r.h + 46 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
           0, 0, 0, (ajVid() ? 0.36f : 0.45f) * a);
  if (ajVid()) {
    gfx_cor(r, raio, 0.055f, 0.059f, 0.071f, (modal ? 0.86f : 0.80f) * a);
    gfx_luz_canto(r, raio, r.w * 0.22f, -r.h * 0.40f, r.h * 0.62f, 1, 1, 1, 0.10f * a);
  } else {
    gfx_cor(r, raio, 0.082f, 0.086f, 0.102f, a);
  }
}
// Superficie de FOCO de linha: um degrau mais clara (vidro 12%, solido #2B2D34).
static void ajFocoLinha(GfxRect r, float raioPx, float f, float a) {
  if (f <= 0.01f) return;
  if (ajVid()) gfx_cor(r, raioPx / r.h, 1, 1, 1, 0.12f * f * a);
  else {
    gfx_rect((GfxRect){ r.x - 10, r.y + 2, r.w + 20, r.h + 26 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f, 0, 0, 0, 0.30f * f * a);
    gfx_cor(r, raioPx / r.h, 0.169f, 0.176f, 0.204f, f * a);
  }
}
// Neutro claro sobre a ilha: branco a `vidA` no vidro, o cinza opaco no solido.
static void ajNeutro(GfxRect r, float raioPx, float vidA, float sr, float sg, float sb, float a) {
  if (ajVid()) gfx_cor(r, raioPx / r.h, 1, 1, 1, vidA * a);
  else gfx_cor(r, raioPx / r.h, sr, sg, sb, a);
}
#define AJ_CHIP_SOL 0.141f, 0.149f, 0.173f   // #24262C
// Pilula cheia no acento (botao/chip/tecla em foco), com a luz do acento.
static void ajPilulaAcento(GfxRect r, float raioPx, float a) {
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  gfx_rect((GfxRect){ r.x - 14, r.y - 2, r.w + 28, r.h + 30 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f, ar, ag, ab, 0.32f * a);
  gfx_cor(r, raioPx / r.h, ar, ag, ab, a);
}
static int ajTinta(void) { return ajustes_tinta_foco(); }
static void ajAcento8(int *r, int *g, int *b) {
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  *r = (int)(ar * 255.0f + 0.5f); *g = (int)(ag * 255.0f + 0.5f); *b = (int)(ab * 255.0f + 0.5f);
}
// O branco do texto do mockup (#F3F2EF). A opacidade vai no DESENHO
// (txt_desenhar_alpha), nao na rasterizacao: uma textura por opacidade
// encheria o cache de linhas.
#define AJ_TX 243, 242, 239, 255
static TxtLinha ajTxt(TxtEstilo e, const char *s) { return txt_linha(e, s, AJ_TX); }
static TxtLinha ajTxtC(TxtEstilo e, const char *s, float maxW) { return txt_linha_corta(e, s, AJ_TX, maxW); }
static TxtLinha ajTxtT(TxtEstilo e, const char *s) { int t = ajTinta(); return txt_linha(e, s, t, t, t, 255); }
// Caixa alta espacada (kicker): i18n antes, maiusculas (latim + acentos) e tracking.
static float ajCaps(TxtEstilo e, const char *s, float track, float x, float y, float al) {
  char up[200];
  size_t k;
  snprintf(up, sizeof up, "%s", i18n(s));
  for (k = 0; up[k]; k++)
    if (up[k] >= 'a' && up[k] <= 'z') up[k] = (char)(up[k] - 32);
    else if ((unsigned char)up[k] == 0xC3 && up[k + 1] && (unsigned char)up[k + 1] >= 0xA0 && (unsigned char)up[k + 1] <= 0xBE)
      { up[k + 1] = (char)((unsigned char)up[k + 1] - 0x20); k++; }
  return txt_tracking(e, up, 243, 242, 239, x, y, al, track);
}
// Kicker padrao: 15/700, tracking .14em, branco a 45%. Devolve a largura.
static float ajKicker(const char *s, float x, float y, float a) {
  return ajCaps(TXT_MINI, s, 2.1f, x, y, 0.45f * a);
}
// Icone Lucide na cor do texto do mockup.
static void ajIcone(const char *nome, float x, float y, float lado, float al) {
  gfx_icone((GfxRect){ x, y, lado, lado }, nome, 0.953f, 0.949f, 0.937f, al);
}
static void ajIconeT(const char *nome, float x, float y, float lado, float a) {
  float t = ajTinta() / 255.0f;
  gfx_icone((GfxRect){ x, y, lado, lado }, nome, t, t, t, a);
}
// Bloco de texto com a cor do mockup e o alfa no desenho.
static float ajBloco(TxtEstilo e, const char *s, float x, float y, float w, float lh, float al, int maxL) {
  return txt_bloco(e, s, 243, 242, 239, x, y, w, lh, al, maxL);
}

// --- FUNDO ----------------------------------------------------------------------
// A arte do titulo atras de Ajustes (o modo "Arte" do DESIGN.md §6) com o veu
// do mockup, preto ~50%. Chapado e nao degrade: veu de cor unica nao faz faixa
// no painel de 8 bits.
static void ajFundo(void) {
  // O titulo em foco na Home quando ha catalogo; a amostra n sem ele.
  HomeItem hi;
  const char *c = (cat_n() > 0 && home_item_focado(&hi) && hi.indice >= 0)
                    ? ajArte(AJA_FUNDO, hi.indice) : ajArte(AJA_FUNDO, ajArteFundoN);
  if (!ajImg(c, (GfxRect){ 0, 0, NV_VTELA_W, NV_VTELA_H }, 0, 1.0f))
    gfx_cor((GfxRect){ 0, 0, NV_VTELA_W, NV_VTELA_H }, 0, NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1);
  gfx_cor((GfxRect){ 0, 0, NV_VTELA_W, NV_VTELA_H }, 0, 0, 0, 0, 0.50f);
}

// --- GEOMETRIA ------------------------------------------------------------------
#define AJI_TOPO        112.0f   // indice: logo abaixo da ilha do relogio
#define AJI_MARGEM       40.0f   // folha e inspetor: 40 das bordas
#define AJI_IDX_W       360.0f
#define AJI_INSP_X     1276.0f
#define AJI_LN_H         76.0f
#define AJI_LN_GAP        4.0f
static float ajX0(void) { return 48.0f + ajustes_rail_largura_fixa(); }
static float ajFolhaX(void) { return ajX0() + AJI_IDX_W + 24.0f; }
// TAMANHO DA INTERFACE (escala.h): na tela virtual menor as tres ilhas nao
// cabem com as larguras de 1080p. A folha fica com pelo menos AJI_FOLHA_MIN (o
// segmentado de 460 e o rotulo ao lado) e a proporcao de 1080p (820:604); o
// inspetor fica com o resto, e SAI quando o resto e menor que AJI_INSP_MIN —
// a folha ocupa o lugar dele. Em 100% e a conta fixa de sempre (1276).
#define AJI_FOLHA_MIN  600.0f
#define AJI_INSP_MIN   360.0f
static float ajInspX(void) {
  float total, folha;
  if (gfx_escala_ui() == 1.0f) return AJI_INSP_X;
  total = NV_VTELA_W - AJI_MARGEM - ajFolhaX() - 24.0f;
  folha = total * (820.0f / 1424.0f);
  if (folha < AJI_FOLHA_MIN) folha = AJI_FOLHA_MIN;
  if (total - folha < AJI_INSP_MIN) return NV_VTELA_W;   // sem inspetor
  return ajFolhaX() + folha + 24.0f;
}
static int ajTemInspetor(void) { return ajInspX() < NV_VTELA_W; }
static float ajFolhaW(void) {
  return ajTemInspetor() ? ajInspX() - 24.0f - ajFolhaX() : NV_VTELA_W - AJI_MARGEM - ajFolhaX();
}
static GfxRect ajFolhaR(void) { return (GfxRect){ ajFolhaX(), AJI_MARGEM, ajFolhaW(), NV_VTELA_H - 2 * AJI_MARGEM }; }
static GfxRect ajInspR(void) { return (GfxRect){ ajInspX(), AJI_MARGEM, NV_VTELA_W - AJI_MARGEM - ajInspX(), NV_VTELA_H - 2 * AJI_MARGEM }; }

// --- CLASSIFICACAO DAS LINHAS -----------------------------------------------------
enum { AJL_TOG = 0, AJL_ESC, AJL_NUM, AJL_ACAO, AJL_LER };
static int ajTipo(int op) {
  if (ehInterruptor(op)) return AJL_TOG;
  switch (OPCOES[op].tipo) {
    case OP_NUMERO: return AJL_NUM;
    case OP_ACAO: return AJL_ACAO;
    case OP_LEITURA: return AJL_LER;
    default: return AJL_ESC;
  }
}
// O valor i no segmentado: traduzido e sem o aposto entre parenteses
// ("Dinâmica (Apple TV)" vira "Dinâmica"; o nome inteiro fica no inspetor).
static const char *ajRotSeg(int op, int i) {
  static char b[3][96];
  char *p;
  snprintf(b[i % 3], sizeof b[0], "%s", i18n(OPCOES[op].valores[i]));
  p = strstr(b[i % 3], " (");
  if (p && p != b[i % 3]) *p = 0;
  return b[i % 3];
}
// Escolha que cabe na linha: ate tres valores curtos viram o segmentado.
static int ajSegmentada(int op) {
  int i, n;
  float w = 10.0f;
  if (op < 0 || ajTipo(op) != AJL_ESC || op == AJ_LEG_LINGUA || op == AJ_AUD_LINGUA) return 0;
  n = nValores(op);
  if (n < 2 || n > 3 || !OPCOES[op].valores) return 0;
  for (i = 0; i < n; i++) w += (float)txt_largura(TXT_AJ_SEG, ajRotSeg(op, i)) + 44.0f;
  // Folha mais estreita (escala.h): o segmentado so fica na linha se o nome
  // da opcao cabe ao lado dele; senao a escolha abre no editor.
  if (gfx_escala_ui() != 1.0f &&
      w + (float)txt_largura(TXT_AJ_ROTULO, rotuloOpcao(op)) + 96.0f > ajFolhaW()) return 0;
  return w < 460.0f;
}
// O editor desta opcao acontece NA LINHA (segmentado, interruptor, cor), e nao
// no modal. Vindo da lista de diferencas a linha nao esta na tela: la e modal.
static int ajEmLinha(int op) {
  if (op < 0 || uxIndice < 2) return 0;
  return ajTipo(op) == AJL_TOG || ajSegmentada(op) || op == AJ_TEMA;
}
static int ajEditandoLinha(int op) {
  return uxEditor == 1 && !uxRestaurar && !uxAvisoRisco && uxOp == op && ajEmLinha(op);
}
// A linha da cor de destaque abre a regua de amostras quando tem o foco.
#define AJ_REGUA_H 118.0f
static float ajLinhaH(int i) {
  int op = TELA[i].op;
  float h = AJI_LN_H;
  if (inativa(op) && uxRequisito(op) >= 0) h = 96.0f;
  if (op == AJ_GUIA) h = 80.0f;   // a linha do guia leva o subtitulo
  if (op == AJ_TEMA && i == focoItem && !focoIndice && uxIndice >= 2) h += AJ_REGUA_H;
  return h;
}

// --- INDICE (ilha de categorias) -------------------------------------------------
static void ajDesenharIndice(void) {
  GfxRect ilha = { ajX0(), AJI_TOPO, AJI_IDX_W, NV_VTELA_H - AJI_TOPO - AJI_MARGEM };
  float y = ilha.y + 14.0f;
  int j, indRecorte = 0;
  float indFim = 0.0f;
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  ajIlha(ilha, 32.0f, 1, 0);
  // TELA VIRTUAL MENOR (escala.h): as categorias nao cabem acima do rodape do
  // perfil. A lista vira uma janela recortada que acompanha a categoria
  // escolhida; em 100% tudo cabe e nada disso acontece.
  { float topoL = ilha.y + 8.0f, fimL = ilha.y + ilha.h - 14.0f - 6.0f - 21.0f - 20.0f - 12.0f;
    float tot = (float)(nSecoes + 2) * 62.0f + 21.0f + 14.0f;
    if (gfx_escala_ui() != 1.0f && tot > fimL - topoL) {
      float jan = fimL - topoL, sy = 14.0f + uxIndice * 62.0f + (uxIndice >= 2 ? 21.0f : 0.0f) + 29.0f;
      float off = sy - jan * 0.5f;
      if (off > tot - jan) off = tot - jan;
      if (off < 0.0f) off = 0.0f;
      y -= off;
      indRecorte = 1; indFim = fimL;
      gfx_recorte(ilha.x, topoL, ilha.w, jan);
    } }
  for (j = 0; j < nSecoes + 2; j++) {
    int sel = j == uxIndice, foco = sel && focoIndice;
    const char *titulo = j == 0 ? "Buscar ajuste" : j == 1 ? "Diferentes do padrão" : TELA[secIni[j - 2]].titulo;
    const char *icone = j == 0 ? "aj_search" : j == 1 ? "aj_rotate-ccw-clock" : TELA[secIni[j - 2]].icone;
    GfxRect r;
    float al = (foco || sel) ? 1.0f : 0.66f;
    if (j == 2) {
      gfx_cor((GfxRect){ ilha.x + 32.0f, y + 8.0f, ilha.w - 64.0f, 1 }, 0, 1, 1, 1, 0.08f);
      y += 21.0f;
    }
    r = (GfxRect){ ilha.x + 14.0f, y, ilha.w - 28.0f, 58.0f };
    // Na janela recortada so entram categorias inteiras: meia linha cortada
    // pelo recorte parece defeito, nao rolagem.
    if (indRecorte && (r.y < ilha.y + 7.0f || r.y + 58.0f > indFim + 1.0f)) { y += 62.0f; continue; }
    if (foco) ajNeutro(r, 29.0f, 0.14f, 0.169f, 0.176f, 0.204f, 1);
    else if (sel) gfx_cor((GfxRect){ r.x + 7.0f, r.y + 26.5f, 5, 5 }, 0.5f, ar, ag, ab, 1);
    ajIcone(icone, r.x + 20.0f, r.y + 16.0f, 26.0f, al);
    { TxtLinha t = ajTxtC(TXT_AJ_ITEM, titulo, r.w - 82.0f);
      txt_desenhar_alpha(t, r.x + 62.0f, r.y + (58.0f - t.h) * 0.5f, al); }
    y += 62.0f;
  }
  if (indRecorte) gfx_sem_recorte();
  // Rodape: quem esta usando (as preferencias "Conta/perfil" sao dele).
  { const ContaPerfil *p = perfis_item_ativo();
    const char *nome = p && p->nome[0] ? p->nome : NULL;
    char ini[8], nb[80];
    float cy = ilha.y + ilha.h - 14.0f - 6.0f - 21.0f;
    float ax = ilha.x + 30.0f;
    float cr = 0.486f, cg = 0.361f, cb = 1.0f;
    TxtLinha l, n, s;
    if (nome) {
      unsigned char c0 = (unsigned char)nome[0];
      int nb0 = c0 >= 0xF0 ? 4 : c0 >= 0xE0 ? 3 : c0 >= 0xC0 ? 2 : 1;
      snprintf(ini, sizeof ini, "%.*s", nb0, nome);
      if (ini[0] >= 'a' && ini[0] <= 'z') ini[0] = (char)(ini[0] - 32);
      snprintf(nb, sizeof nb, "%s", nome);
    } else {
      snprintf(ini, sizeof ini, "%d", perfis_ativo());
      snprintf(nb, sizeof nb, i18n("Perfil %d"), perfis_ativo());
    }
    if (p && p->corHex[0] == '#' && strlen(p->corHex) >= 7) {
      unsigned v = (unsigned)strtoul(p->corHex + 1, NULL, 16);
      cr = ((v >> 16) & 255) / 255.0f; cg = ((v >> 8) & 255) / 255.0f; cb = (v & 255) / 255.0f;
    }
    gfx_cor((GfxRect){ ax, cy - 20.0f, 40, 40 }, 0.5f, cr, cg, cb, 1);
    l = txt_linha(TXT_ILHA_INICIAL, ini, 255, 255, 255, 255);
    txt_desenhar(l, ax + (40 - l.w) * 0.5f, cy - l.h * 0.5f);
    n = txt_linha_corta(TXT_AJ_SEG, nb, AJ_TX, ilha.w - 110.0f);
    s = ajTxt(TXT_ILHA_APOIO, "Conta/perfil");
    { float ty = cy - (n.h + 2 + s.h) * 0.5f;
      txt_desenhar(n, ax + 54.0f, ty);
      txt_desenhar_alpha(s, ax + 54.0f, ty + n.h + 2, 0.45f); } }
}

// --- PECAS DA LINHA ---------------------------------------------------------------
static void ajChevron(float xDir, float yc, float al) {
  ajIcone("aj_chevron-right", xDir - 24.0f, yc - 12.0f, 24.0f, al);
}
// Interruptor: o chip "Ligado" no acento a 22% (e estado), "Desligado" neutro.
static float ajChipLiga(float xDir, float yc, int ligado, float a) {
  TxtLinha t;
  float w, ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  if (ligado) {
    t = txt_linha(TXT_AJ_CHIP, "Ligado", (int)(ar * 255), (int)(ag * 255), (int)(ab * 255), 255);
    w = 18 + 8 + 9 + t.w + 18;
    gfx_cor((GfxRect){ xDir - w, yc - 20, w, 40 }, 0.5f, ar, ag, ab, 0.22f * a);
    gfx_cor((GfxRect){ xDir - w + 18, yc - 4, 8, 8 }, 0.5f, ar, ag, ab, a);
    txt_desenhar_alpha(t, xDir - w + 35, yc - t.h * 0.5f, a);
  } else {
    t = ajTxt(TXT_AJ_CHIP, "Desligado");
    w = 18 + t.w + 18;
    ajNeutro((GfxRect){ xDir - w, yc - 20, w, 40 }, 20, 0.07f, AJ_CHIP_SOL, a);
    txt_desenhar_alpha(t, xDir - w + 18, yc - t.h * 0.5f, 0.5f * a);
  }
  return w;
}
// Segmentado: trilho 6% (solido #1D1E23), selecionado 14% (#34363E); editando,
// o item que as setas movem fica cheio no acento, como botao em foco.
static float ajSegLargura(const char *const *rot, const char *const *num, int n) {
  float w = 10.0f;
  int i;
  for (i = 0; i < n; i++) {
    w += (float)txt_largura(TXT_AJ_SEG, i18n(rot[i])) + 40.0f + (i ? 4.0f : 0.0f);
    if (num && num[i]) w += 9.0f + (float)txt_largura(TXT_ILHA_GENERO, num[i]);
  }
  return w;
}
static float ajSeg(const char *const *rot, const char *const *num, int n, int sel, int ed,
                   float x, float yc, float h, float a) {
  float w = ajSegLargura(rot, num, n), ih = h - 10.0f;
  int i;
  ajNeutro((GfxRect){ x, yc - h * 0.5f, w, h }, h * 0.5f, 0.06f, 0.114f, 0.118f, 0.137f, a);
  x += 5.0f;
  for (i = 0; i < n; i++) {
    int ac = i == sel && ed;
    TxtLinha t = ac ? ajTxtT(TXT_AJ_SEG, rot[i]) : ajTxt(TXT_AJ_SEG, rot[i]), q = { 0 };
    float iw = t.w + 40.0f, al = 0.55f;
    if (num && num[i]) { q = ac ? ajTxtT(TXT_ILHA_GENERO, num[i]) : ajTxt(TXT_ILHA_GENERO, num[i]); iw += 9.0f + q.w; }
    if (ac) { ajPilulaAcento((GfxRect){ x, yc - ih * 0.5f, iw, ih }, ih * 0.5f, a); al = 1; }
    else if (i == sel) { ajNeutro((GfxRect){ x, yc - ih * 0.5f, iw, ih }, ih * 0.5f, 0.14f, 0.204f, 0.212f, 0.243f, a); al = 1; }
    txt_desenhar_alpha(t, x + 20, yc - t.h * 0.5f, al * a);
    if (q.w) txt_desenhar_alpha(q, x + 20 + t.w + 9, yc - q.h * 0.5f + 1, (ac ? 0.6f : 0.35f) * a);
    x += iw + 4.0f;
  }
  return w;
}
// Trilho do numero: onde o valor fica no intervalo e o risco do padrao.
static void ajTrilho(float x, float yc, float w, float frac, float fracPad, int foco, float a) {
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  if (frac < 0) frac = 0;
  if (frac > 1) frac = 1;
  gfx_cor((GfxRect){ x, yc - 2, w, 4 }, 0.5f, 1, 1, 1, 0.14f * a);
  if (frac * w >= 1.0f) {
    if (foco) gfx_cor((GfxRect){ x, yc - 2, w * frac, 4 }, 0.5f, ar, ag, ab, a);
    else gfx_cor((GfxRect){ x, yc - 2, w * frac, 4 }, 0.5f, 0.953f, 0.949f, 0.937f, 0.55f * a);
  }
  if (fracPad >= 0) gfx_cor((GfxRect){ x + w * fracPad - 1, yc - 7, 2, 14 }, 0.5f, 0.953f, 0.949f, 0.937f, 0.35f * a);
}
static float ajFracao(int op, int v) {
  const Opcao *o = &OPCOES[op];
  return o->max > o->min ? (float)(v - o->min) / (float)(o->max - o->min) : 0.0f;
}
// O padrao que a tela MOSTRA. O limite de fileiras mora em fileiras.c e nao
// tem "Restaurar padrao" (uxTemPadrao), mas o risco do 7 no trilho ainda diz
// onde o valor de fabrica fica.
static int ajPadraoVisual(int op) {
  if (uxTemPadrao(op)) return uxValorPadrao(op);
  if (op == AJ_FIL_LIMITE) return FIL_LIMITE_PADRAO;
  return -1;
}
// Pressao da memoria de imagens: usado sobre o teto.
static void corPressao(float t, float *r, float *g, float *b);
#ifdef AJUSTES_TESTE
static int ajMemFixa;   // capturas: os numeros do mockup, sem depender do cache
#endif
static void ajMemUso(long *bytes, long *teto) {
  *bytes = 0; *teto = tex_orcamento_bytes();
  tex_estatisticas(NULL, NULL, bytes, NULL, NULL);
#ifdef AJUSTES_TESTE
  if (ajMemFixa) { *bytes = (long)(151.4 * 1048576); *teto = 240L << 20; }
#endif
}
static float ajPressao(void) {
  long bytes, teto;
  ajMemUso(&bytes, &teto);
  if (teto <= 0) teto = 1;
  return (float)bytes / (float)teto;
}
// O estado curto de uma acao leva o ponto no acento quando e um VINCULO ativo.
static int ajEstadoLigado(int op) {
  return (op == AJ_TRAKT && traktauth_estado() == TRA_LIGADO) ||
         (op == AJ_SIMKL && simklauth_estado() == SMK_LIGADO);
}
static const char *ajEstadoAcao(int op) {
  static char b[64];
  if (op == AJ_FIL_ORDEM) {
    int home = fil_n_na_home(), fila = fil_n_fila();
    if (fila) snprintf(b, sizeof b, i18n("%d na Home · %d na fila"), home, fila);
    else snprintf(b, sizeof b, i18n("%d na Home"), home);
    return b;
  }
  if (op == AJ_GUIA) { snprintf(b, sizeof b, i18n("%d recursos"), guiaTotal()); return b; }
  return textoLeitura(op);
}

// --- A LINHA ----------------------------------------------------------------------
// So a linha em foco ganha superficie. Cada tipo tem a sua cauda: interruptor
// = chip; escolha = valor + seta, ou o segmentado na linha; numero = valor +
// trilho + seta; acao = estado + seta; leitura = valor apagado, sem seta.
static void ajDesenharLinha(int item, float y, float f, float a) {
  int op = TELA[item].op, tipo = ajTipo(op);
  float h = ajLinhaH(item);
  GfxRect fr = ajFolhaR();
  GfxRect ln = { fr.x + 18.0f, y, fr.w - 36.0f, h };
  int desligada = inativa(op), emFoco = f > 0.5f, ed = ajEditandoLinha(op);
  int sub = desligada && uxRequisito(op) >= 0;
  float aT = a * (desligada ? 0.45f : 1.0f);
  float xDir = ln.x + ln.w - 22.0f, yc = y + ((sub || op == AJ_GUIA) ? h : AJI_LN_H) * 0.5f;
  if (h > AJI_LN_H + 30) yc = y + 28.0f;   // linha que abre: o rotulo fica no alto
  float cauda = 0;
  if (a <= 0.004f) return;
  ajFocoLinha(ln, 22.0f, f, a);
  if ((tipo == AJL_TOG && ed) || (ajSegmentada(op) && emFoco && !desligada)) {
    const char *rot[3];
    int n = nValores(op), i;
    float w;
    if (n > 3) n = 3;
    for (i = 0; i < n; i++) rot[i] = ajRotSeg(op, i);
    w = ajSegLargura(rot, NULL, n);
    cauda = ajSeg(rot, NULL, n, ed ? uxPendente : valor[op], ed, ln.x + ln.w - 22.0f - w + 0.0f, yc, 55.0f, a);
  } else if (tipo == AJL_TOG) {
    cauda = ajChipLiga(xDir, yc, lig(op), aT);
  } else if (tipo == AJL_ESC || tipo == AJL_NUM) {
    const char *v = textoValor(op);
    float x = xDir;
    if (!desligada) { ajChevron(x, yc, (emFoco ? 0.7f : 0.32f) * aT); x -= 24.0f + 12.0f; }
    if (tipo == AJL_NUM) {
      int pad = ajPadraoVisual(op);
      ajTrilho(x - 150.0f, yc, 150.0f, ajFracao(op, valor[op]), pad >= 0 ? ajFracao(op, pad) : -1.0f, emFoco, aT);
      x -= 150.0f + 12.0f;
    }
    { int fv = marcaDoValor(op, v);
      if (fv >= 0) {
        float mw = marca_formato_largura((FormatoMarca)fv, 36.0f);
        marca_formato((FormatoMarca)fv, x - mw, yc - 18.0f, 36.0f, 1, 1, 1, (emFoco ? 0.85f : 0.6f) * aT);
        x -= mw;
      } else {
        TxtLinha t = ajTxtC(TXT_AJ_VALOR, v, 300.0f);
        txt_desenhar_alpha(t, x - t.w, yc - t.h * 0.5f, (emFoco ? 0.85f : 0.55f) * aT);
        x -= t.w;
        if (op == AJ_TEMA) {
          float sr, sg, sb;
          ajustes_acento(&sr, &sg, &sb);
          gfx_cor((GfxRect){ x - 10 - 22, yc - 11, 22, 22 }, 0.5f, sr, sg, sb, aT);
          x -= 32.0f;
        }
      } }
    cauda = xDir - x;
  } else if (tipo == AJL_ACAO) {
    const char *st = ajEstadoAcao(op);
    float x = xDir;
    if (!desligada) { ajChevron(x, yc, (emFoco ? 0.7f : 0.32f) * aT); x -= 24.0f + 12.0f; }
    if (st && st[0]) {
      TxtLinha t = ajTxtC(TXT_AJ_18, st, 380.0f);
      txt_desenhar_alpha(t, x - t.w, yc - t.h * 0.5f, 0.55f * aT);
      x -= t.w;
      if (ajEstadoLigado(op)) {
        float ar, ag, ab;
        ajustes_acento(&ar, &ag, &ab);
        gfx_cor((GfxRect){ x - 9 - 8, yc - 4, 8, 8 }, 0.5f, ar, ag, ab, aT);
        x -= 17.0f;
      }
    }
    cauda = xDir - x;
  } else {
    float x = xDir;
    char vb[96];
    const char *v = textoValor(op);
    if (op == AJ_ESPACO) {
      float t = ajPressao(), pr, pg, pb;
      long bytes, teto;
      ajMemUso(&bytes, &teto);
      if (t > 1) t = 1;
      corPressao(t, &pr, &pg, &pb);
      gfx_cor((GfxRect){ x - 120, yc - 3, 120, 6 }, 0.5f, 1, 1, 1, 0.12f * aT);
      if (t * 120 > 1) gfx_cor((GfxRect){ x - 120, yc - 3, 120 * t, 6 }, 0.5f, pr, pg, pb, 0.85f * aT);
      x -= 132.0f;
      snprintf(vb, sizeof vb, i18n("%d de %d MB"), (int)(bytes / 1048576), (int)(teto / 1048576));
      v = vb;
    }
    { TxtLinha t = ajTxtC(TXT_AJ_VALOR, v, 380.0f);
      txt_desenhar_alpha(t, x - t.w, yc - t.h * 0.5f, (emFoco ? 0.85f : 0.42f) * aT);
      x -= t.w; }
    cauda = xDir - x;
  }
  // ROTULO, marca do formato, ponto de "diferente do padrao", "AVANCADO".
  { float tx = ln.x + 22.0f, maxW = ln.w - 44.0f - cauda - 20.0f, tyc = yc;
    TxtLinha r = ajTxtC(TXT_AJ_ROTULO, rotuloOpcao(op), maxW);
    float rx;
    if (sub || op == AJ_GUIA) tyc = y + h * 0.5f - 12.0f;
    if (op == AJ_GUIA) {
      TxtLinha s = ajTxtC(TXT_ILHA_GENERO, "Tudo o que o app faz, por assunto", maxW);
      txt_desenhar_alpha(s, tx, tyc + r.h * 0.5f + 4, 0.42f * aT);
    }
    txt_desenhar_alpha(r, tx, tyc - r.h * 0.5f, (emFoco ? 1.0f : 0.86f) * aT);
    rx = tx + r.w;
    { int fm = marcaDaOpcao(op);
      if (fm >= 0) {
        float mw = marca_formato_largura((FormatoMarca)fm, 40.0f);
        if (rx + 12 + mw < tx + maxW) {
          marca_formato((FormatoMarca)fm, rx + 12, tyc - 20, 40, 1, 1, 1, (emFoco ? 0.85f : 0.55f) * aT);
          rx += 12 + mw;
        }
      } }
    if (uxDiferente(op) && rx + 19 < tx + maxW) {
      gfx_cor((GfxRect){ rx + 12, tyc - 3.5f, 7, 7 }, 0.5f, 0.953f, 0.949f, 0.937f, 0.45f * aT);
      rx += 19;
    }
    // "NOVO" no acento (e estado) ate a pessoa abrir o registro uma vez.
    if (op == AJ_VER_REGISTRO && !registro_ja_aberto()) {
      int r8, g8, b8;
      float w = ajCaps(TXT_AJ_CAPS13, "Novo", 1.56f, -1, 0, 0);
      ajAcento8(&r8, &g8, &b8);
      if (rx + 16 + w < tx + maxW) {
        char up[40];
        idioma_maiusc_em(ajustes_idioma(), up, sizeof up, i18n("Novo"));
        txt_tracking(TXT_AJ_CAPS13, up, r8, g8, b8, rx + 16, tyc - 8, aT, 1.56f);
      }
    }
    if (uxAvancada(op)) {
      float w = ajCaps(TXT_AJ_CAPS13, "Avançado", 1.56f, -1, 0, 0);
      if (rx + 16 + w < tx + maxW) ajCaps(TXT_AJ_CAPS13, "Avançado", 1.56f, rx + 16, tyc - 8, 0.38f * aT);
    }
    if (sub) {
      char t[200];
      TxtLinha s;
      snprintf(t, sizeof t, emFoco ? i18n("Depende de: %s · OK mostra") : i18n("Depende de: %s"),
               i18n(rotuloOpcao(uxRequisito(op))));
      s = ajTxtC(TXT_ILHA_GENERO, t, maxW);
      txt_desenhar_alpha(s, tx, tyc + r.h * 0.5f + 4, 0.42f * aT);
    }
  }
  // A REGUA DA COR DE DESTAQUE: os doze temas da conta e os quatro dinamicos.
  if (op == AJ_TEMA && h > AJI_LN_H + 1) {
    int sel = valor[op], i, alvo = ed ? uxPendente : sel;
    float sx = ln.x + 22.0f, sy = y + 74.0f;
    for (i = 0; i < AJ_N_TEMAS; i++) {
      float s = (ed && i == alvo) ? 1.2f : 1.0f, d = 44.0f * s;
      float cx = sx + i * 56.0f + 22.0f, cy = sy + 22.0f;
      float cr = TEMA_ACENTO[i].r, cg = TEMA_ACENTO[i].g, cb = TEMA_ACENTO[i].b;
      float lum = 0.2126f * cr + 0.7152f * cg + 0.0722f * cb;
      if (s > 1.0f) gfx_rect((GfxRect){ cx - d * 0.5f - 12, cy - d * 0.5f - 2, d + 24, d + 30 }, 0, GFX_SOMBRA, 1, 0, 0, 0.5f, 0, 0, 0, 0.55f * a);
      gfx_cor((GfxRect){ cx - d * 0.5f, cy - d * 0.5f, d, d }, 0.5f, cr, cg, cb, a);
      if (i == sel) {
        float k = lum > 0.5f ? 0.05f : 1.0f;
        gfx_icone((GfxRect){ cx - 11 * s, cy - 11 * s, 22 * s, 22 * s }, "aj_check", k, k, k, a);
      }
    }
    sy = y + 134.0f;
    { float px = sx + ajCaps(TXT_AJ_CAPS13, "Dinâmicas", 1.82f, sx, sy + 12, 0.45f * a) + 18.0f;
      static const int ARTE[4] = { 7, 12, 15, 0 };
      static const char *const NOME[4] = { "Dinâmica", "Estilizada", "Gradiente", "Imersiva" };
      for (i = 0; i < 4; i++) {
        int k = AJ_TEMA_DINAMICA + i, foc = ed && alvo == k;
        TxtLinha t = foc ? ajTxtT(TXT_MINI, NOME[i]) : ajTxt(TXT_MINI, NOME[i]);
        float w = 44 + t.w + 16;
        GfxRect p = { px, sy, w, 40 };
        if (foc) ajPilulaAcento(p, 20, a);
        else ajNeutro(p, 20, 0.07f, AJ_CHIP_SOL, a);
        ajImg(ajArte(AJA_FUNDO, ARTE[i]), (GfxRect){ px + 6, sy + 6, 28, 28 }, 14, a);
        if (k == sel) gfx_icone((GfxRect){ px + 11, sy + 11, 18, 18 }, "aj_check", 1, 1, 1, a);
        txt_desenhar_alpha(t, px + 44, sy + (40 - t.h) * 0.5f, (foc ? 1.0f : 0.75f) * a);
        px += w + 10.0f;
      } }
  }
}

// Cabecalho de grupo (ROT): nome forte, contagem a direita, fio de 1 px.
static int ajContaGrupo(int item) {
  int i, n = 0;
  for (i = item + 1; i < secFim(secDoItem[item]) && TELA[i].tipo == IT_OPC; i++)
    if (visivel(i)) n++;
  return n;
}
static void ajDesenharGrupo(int item, float y, float a) {
  GfxRect fr = ajFolhaR();
  float x = fr.x + 18.0f, w = fr.w - 36.0f;
  char q[16];
  TxtLinha t, c;
  if (a <= 0.004f) return;
  snprintf(q, sizeof q, "%d", ajContaGrupo(item));
  t = ajTxt(TXT_AJ_SECAO, TELA[item].titulo);
  c = ajTxt(TXT_ILHA_GENERO, TELA[item].sub ? TELA[item].sub : q);
  txt_desenhar_alpha(t, x + 22, y + 14 - t.h * 0.5f + 0.0f, a);
  txt_desenhar_alpha(c, x + w - 22 - c.w, y + 16 - c.h * 0.5f, 0.38f * a);
  gfx_cor((GfxRect){ x, y + 37, w, 1 }, 0, 1, 1, 1, 0.08f * a);
}

// O chip "Avançados · ocultos N" do cabecalho da folha. Devolve a largura.
static int ajNAvancadas(void) {
  int n = 0, i;
  for (i = secIni[secAtual] + 1; i < secFim(secAtual); i++)
    if (TELA[i].tipo == IT_OPC && uxAvancada(TELA[i].op)) n++;
  return n;
}
static float ajChipAvancados(float xDir, float y, int foco, float a) {
  char t[80];
  int n = ajNAvancadas(), vis = uxAvancados[secAtual];
  TxtLinha r, q;
  float w, x;
  if (!n) return 0;
  snprintf(t, sizeof t, vis ? i18n("visíveis · %d") : i18n("ocultos · %d"), n);
  r = foco ? ajTxtT(TXT_AJ_SEG, "Avançados") : ajTxt(TXT_AJ_SEG, "Avançados");
  q = foco ? ajTxtT(TXT_AJ_VALOR, t) : ajTxt(TXT_AJ_VALOR, t);
  w = 22 + 22 + 10 + r.w + 8 + q.w + 22;
  x = xDir - w;
  if (foco) ajPilulaAcento((GfxRect){ x, y, w, 56 }, 28, a);
  else ajNeutro((GfxRect){ x, y, w, 56 }, 28, 0.08f, AJ_CHIP_SOL, a);
  if (foco) ajIconeT(vis ? "aj_eye" : "aj_eye-off", x + 22, y + 17, 22, a);
  else ajIcone(vis ? "aj_eye" : "aj_eye-off", x + 22, y + 17, 22, 0.85f * a);
  txt_desenhar_alpha(r, x + 54, y + (56 - r.h) * 0.5f, (foco ? 1.0f : 0.85f) * a);
  txt_desenhar_alpha(q, x + 54 + r.w + 8, y + (56 - q.h) * 0.5f, (foco ? 0.65f : 0.45f) * a);
  return w;
}

// Cabecalho da folha: kicker + titulo 40/700 + sub. Devolve onde a lista comeca.
static float ajCabecalhoFolha(GfxRect fr, const char *kick, const char *titulo, const char *sub,
                              float reservaDir, float a) {
  float x = fr.x + 44.0f, y = fr.y + 38.0f;
  ajKicker(kick, x, y + 1, a);
  y += 22.0f;
  { TxtLinha t = ajTxtC(TXT_ILHA_TITULO, titulo, fr.w - 88.0f - reservaDir);
    txt_desenhar_alpha(t, x, y + (46.0f - t.h) * 0.5f, a); }
  y += 46.0f + 8.0f;
  if (sub && sub[0]) {
    float hh = ajBloco(TXT_AJ_SUB, sub, x, y + 2, fr.w - 84.0f, 26.6f, 0.55f * a, 2);
    y += hh > 26.6f ? hh : 26.6f;
  }
  return y + 18.0f;
}

// --- DICAS ---------------------------------------------------------------------------
typedef struct { const char *k, *l; } AjDica;
static float ajDicasLinha(const AjDica *d, int n, float x, float y, float a) {
  int i;
  float x0 = x;
  for (i = 0; i < n; i++) {
    TxtLinha k = ajTxt(TXT_AJ_KBD, d[i].k), l = ajTxt(TXT_ILHA_GENERO, d[i].l);
    float kw = k.w + 18.0f < 34.0f ? 34.0f : k.w + 18.0f;
    ajNeutro((GfxRect){ x, y, kw, 30 }, 15, 0.09f, AJ_CHIP_SOL, a);
    txt_desenhar_alpha(k, x + (kw - k.w) * 0.5f, y + (30 - k.h) * 0.5f, 0.82f * a);
    txt_desenhar_alpha(l, x + kw + 9, y + (30 - l.h) * 0.5f, 0.5f * a);
    x += kw + 9 + l.w + 20.0f;
  }
  return x - x0 - 20.0f;
}
static float ajDicasLargura(const AjDica *d, int n) {
  float w = 0;
  int i;
  for (i = 0; i < n; i++) {
    float kw = (float)txt_largura(TXT_AJ_KBD, i18n(d[i].k)) + 18.0f;
    if (kw < 34.0f) kw = 34.0f;
    w += kw + 9 + (float)txt_largura(TXT_ILHA_GENERO, i18n(d[i].l)) + (i ? 20.0f : 0.0f);
  }
  return w;
}
// Rodape de dicas de uma ilha: fio de 7% e as teclas, com o fundo em yBase.
static void ajDicas(const AjDica *d, int n, float x, float yBase, float w, float a) {
  gfx_cor((GfxRect){ x, yBase - 30.0f - 18.0f, w, 1 }, 0, 1, 1, 1, 0.07f * a);
  ajDicasLinha(d, n, x + 6.0f, yBase - 30.0f, a);
}

// Linhas "rotulo ........ valor" com fio de 7% em cima (o .meta do mockup).
static float ajMeta(const char *k, const char *v, float x, float y, float w, float a) {
  TxtLinha lk = ajTxt(TXT_AJ_ESTADO, k);
  TxtLinha lv = ajTxtC(TXT_AJ_ESTADO, v, w - lk.w - 24);
  gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.07f * a);
  txt_desenhar_alpha(lk, x, y + 13, 0.45f * a);
  txt_desenhar_alpha(lv, x + w - lv.w, y + 13, 0.85f * a);
  return 13 + 20.5f + 13;
}
// Caixa de "o que muda" (.ef): icone e texto corrido sobre 5% (solido #1B1C21).
static float ajEf(const char *icone, const char *s, float x, float y, float w, float a) {
  float tw = w - 32 - 20 - 12, h;
  int linhas = 0;
  { // altura: mede quebrando sem desenhar (alfa 0)
    h = txt_bloco(TXT_AJ_ESTADO, s, 0, 0, 0, x, -4000, tw, 24.65f, 0, 6); (void)linhas; }
  ajNeutro((GfxRect){ x, y, w, h + 28 }, 18, 0.05f, 0.106f, 0.110f, 0.129f, a);
  ajIcone(icone, x + 16, y + 16, 20, 0.55f * a);
  ajBloco(TXT_AJ_ESTADO, s, x + 16 + 20 + 12, y + 14, tw, 24.65f, 0.72f * a, 6);
  return h + 28;
}
#include "ajustes_ux_previa.inc"
#include "ajustes_ux_ilha.inc"
#include "ajustes_ux_fileiras.inc"
#include "ajustes_ux_guia.inc"
