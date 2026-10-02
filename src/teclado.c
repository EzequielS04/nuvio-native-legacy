// Ver teclado.h para por que esta modal existe e o que ela NAO tenta ser.
#include "teclado.h"
#include "gfx.h"
#include "text.h"
#include "anim.h"
#include "layout.h"
#include "ajustes.h"
#include "idioma.h"
#include "sistexto.h"
#include "ponteiro.h"
#include "celular.h"
#include "qr.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// AS MESMAS MEDIDAS DA GRADE DA BUSCA (busca.c): tecla de 74 com vao de 12, e
// o mesmo crescimento de 10% no foco. Nao e coincidencia nem copia — e o
// tamanho ja medido para uma tecla que se acerta com o D-pad a tres metros, e
// duas grades do mesmo app com teclas de tamanhos diferentes leem como dois
// aplicativos.
#define TE_TECLA     74.0f
#define TE_GAP       12.0f
#define TE_COLS       6
// ALFABETO LONGO GANHA COLUNAS, NAO FILEIRAS (#88). O de usuario/senha Xtream
// tem 73 simbolos: em 6 colunas pedia 13 fileiras, o teto e 7 de caractere
// (8 com apagar/limpar/pronto = 1080 px de modal, a tela inteira), e tudo
// depois do indice 42 — 'Q' em diante, TODOS os digitos — ficava fora da grade,
// nem desenhado nem alcancavel. O usuario fotografou o teclado cortado em "P".
//
// 13 e metade do alfabeto latino: a grade le a-m / n-z / A-M / N-Z /
// 0-9._- / @!#$%&*+=, cada fileira uma metade de algo que a pessoa ja conhece,
// e cabe em 6 fileiras — a MESMA altura do teclado padrao (994 px). Largura:
// 13 x 74 + 12 x 12 = 1106 de grade, 1194 de modal, folga de 363 de cada lado
// em 1920. Rolagem foi descartada: com tudo a vista nao ha "tem mais embaixo"
// para a pessoa adivinhar, e cada tecla a mais de distancia no D-pad custa o
// mesmo que numa grade rolada.
//
// So entra acima de 42 (7 x 6): padrao (36), portal (39) e MAC (17) continuam
// em 6 colunas, pixel por pixel o layout de antes.
#define TE_COLS_LONGO 13
// Teto de colunas: alfabeto que nem em 13 x 7 coubesse (91+) alarga ate aqui,
// 20 x 74 + 19 x 12 = 1708 + 88 = 1796 < 1920. Acima de 140 simbolos o resto
// fica de fora, como ficava antes — nenhum chamador chega perto.
#define TE_COLS_MAX  20
// TETO das fileiras de caractere + a de apagar/limpar/pronto. O alfabeto
// PADRAO ocupa 6 (36 caracteres); o do portal IPTV precisa de ponto, dois
// pontos e hifen alem de a-z0-9, e nao cabe em 36. Quem abre escolhe o
// alfabeto, e a modal se ajusta — as fileiras de verdade sao `nFileiras`.
#define TE_FILEIRAS_MAX 8
#define TE_FILEIRAS_PAD 7          // 6 de a-z0-9 + 1 de apagar/limpar/pronto
#define TE_PASSO     (TE_TECLA + TE_GAP)

#define TE_ESCALA     0.10f

// Caixa de um caractere digitado. 6 delas com vao de 12 cabem nos 504 da
// grade de 6 colunas, e a caixa por caractere e o que torna o codigo DITAVEL ao telefone:
// separada, ninguem confunde "rn" com "m" nem conta letra errada.
#define TE_CX        76.0f
#define TE_CY        92.0f
#define TE_CGAP      12.0f
// Piso de largura da caixa por caractere: abaixo disto o glifo de TXT_TITULO2
// nao cabe e as letras se sobrepoem. Medido na captura do dono com maxN=24,
// onde a conta dava 9,5 px por caixa.
#define TE_CX_MIN    44.0f
#define TE_CAMPO_PAD 18.0f

#define TE_PAD       44.0f
// TITULO (46) + DICA em ate duas linhas (2 x 28) + CAIXAS (92) + folgas.
//
// ERA 176, DE CABECA, e a primeira captura mostrou o resultado: as caixas do
// que foi digitado nasciam ACIMA da linha de dica e as duas se sobrepunham. A
// altura do cabecalho de uma modal nao se estima — ela e a soma do que esta
// dentro dela.
#define TE_DICA_Y    (TE_PAD + 50.0f)
#define TE_DICA_H     56.0f
#define TE_CAIXA_Y   (TE_DICA_Y + TE_DICA_H + 10.0f)
// 44 DE FOLGA ATE A GRADE, e nao 26: as caixas do que foi digitado tem a
// mesma largura das teclas, e coladas nelas a primeira captura leu como uma
// SETIMA FILEIRA do teclado em vez de "o que voce ja digitou".
#define TE_CAB       (TE_CAIXA_Y + TE_CY + 44.0f)
#define TE_RODAPE    64.0f
#define TE_W        (gradeW() + TE_PAD * 2.0f)

// DIGITAR PELO CELULAR (celular.h): painel a DIREITA da modal, com o QR e o
// endereco. A modal anda para a esquerda para o par ficar centrado; numa grade
// tao larga que o par nao caiba (20 colunas, nenhum chamador hoje), sem painel.
#define TE_CEL_W    400.0f
#define TE_CEL_GAP   24.0f
#define TE_CEL_QR   300.0f
static int celAtivo;   // o painel existe nesta abertura
#define TE_X        ((NV_TELA_W - TE_W - (celAtivo ? TE_CEL_GAP + TE_CEL_W : 0.0f)) * 0.5f)


static const char *ALFABETO = "abcdefghijklmnopqrstuvwxyz0123456789";

// fileira -1 = a BARRA do campo (so com teclado/voz do sistema, sistexto.h):
// coluna 0 o campo (OK chama o teclado da TV), coluna 1 o Falar.
static int   aberto, fileira, coluna;
static float animBarra[2];
// Coluna de caractere de onde o foco desceu para apagar/limpar/pronto. Sem ela,
// subir de "pronto" (coluna 2) numa grade de 13 caia no 'c', a dez teclas de
// onde a pessoa estava; com ela, volta para a mesma tecla.
static int   colunaAntes;
static float anim, focoAnim[TE_FILEIRAS_MAX][TE_COLS_MAX];
static const char *alfabetoAtual = NULL;   // NULL = o padrao
static int   nFileiras = TE_FILEIRAS_PAD;
static int   nCols = TE_COLS;

static float gradeW(void) {
  return (float)nCols * TE_TECLA + (float)(nCols - 1) * TE_GAP;   // 504 com 6
}
// O campo perde a largura do botao Falar onde ha voz.
#define TE_MIC_D (TE_CY - 12.0f)
static float campoW(void) { return gradeW() - (st_voz_disponivel() ? TE_MIC_D + 14.0f : 0.0f); }

// A altura da modal depende de quantas fileiras o alfabeto pediu, entao as tres
// medidas que dela dependem viraram funcao. Continuam sendo a mesma conta.
static float gradeH(void) {
  return (float)nFileiras * TE_TECLA + (float)(nFileiras - 1) * TE_GAP;
}
static float teH(void) { return TE_CAB + gradeH() + TE_RODAPE + TE_PAD; }
static float teY(void) { return (NV_TELA_H - teH()) * 0.5f; }
static const char *alfa(void) { return alfabetoAtual ? alfabetoAtual : ALFABETO; }
static char  texto[TECLADO_LONGO + 1];
static int   n, maxN, resultado;
static char  tituloAtual[96], dicaAtual[160];
static int   celRecebido;

const char *teclado_alfabeto(void) { return ALFABETO; }
int teclado_aberto(void) { return aberto; }
int teclado_foco_campo(void) { return fileira < 0 ? coluna + 1 : 0; }
const char *teclado_texto(void) { return texto; }

int teclado_resultado(void) {
  int r = resultado;
  resultado = TECLADO_NADA;
  return r;
}

void teclado_abrir(const char *titulo, const char *dica, int max) {
  teclado_abrir_com(titulo, dica, max, NULL, NULL);
}

void teclado_abrir_com(const char *titulo, const char *dica, int max,
                       const char *alfabeto, const char *inicial) {
  int letras;
  alfabetoAtual = (alfabeto && *alfabeto) ? alfabeto : NULL;
  letras = (int)strlen(alfa());
  // Colunas: 6, ou 13 quando 7 fileiras de 6 nao bastam (ver TE_COLS_LONGO),
  // ou o que fizer caber em 7 fileiras, ate TE_COLS_MAX.
  nCols = TE_COLS;
  if (letras > (TE_FILEIRAS_MAX - 1) * TE_COLS) {
    nCols = TE_COLS_LONGO;
    if (letras > (TE_FILEIRAS_MAX - 1) * nCols)
      nCols = (letras + TE_FILEIRAS_MAX - 2) / (TE_FILEIRAS_MAX - 1);
    if (nCols > TE_COLS_MAX) nCols = TE_COLS_MAX;
  }
  // Fileiras de caractere = quantas o alfabeto pede, arredondando para cima,
  // mais a de apagar/limpar/pronto. O teto existe porque `focoAnim` e vetor
  // fixo e porque uma grade mais alta que isto nao cabe na tela.
  nFileiras = (letras + nCols - 1) / nCols + 1;
  if (nFileiras > TE_FILEIRAS_MAX) nFileiras = TE_FILEIRAS_MAX;
  if (nFileiras < 2) nFileiras = 2;
  aberto = 1;
  fileira = 0; coluna = 0; colunaAntes = 0;
  resultado = TECLADO_NADA;
  maxN = max > 0 && max <= TECLADO_LONGO ? max : TECLADO_MAX;
  // TEXTO INICIAL: editar um portal ja cadastrado nao pode obrigar a redigitar
  // o endereco inteiro. Cortado em maxN, nunca truncado no meio de nada porque
  // o alfabeto e de um byte por caractere.
  snprintf(texto, sizeof texto, "%s", inicial ? inicial : "");
  texto[maxN] = 0;
  n = (int)strlen(texto);
  snprintf(tituloAtual, sizeof tituloAtual, "%s", titulo ? titulo : "");
  snprintf(dicaAtual,   sizeof dicaAtual,   "%s", dica   ? dica   : "");
  memset(focoAnim, 0, sizeof focoAnim);
  animBarra[0] = animBarra[1] = 0.0f;
  // O servidor so vive enquanto a modal esta aberta (fechar() o derruba).
  celRecebido = 0;
  celAtivo = 0;
  if (celular_disponivel() && gradeW() + TE_PAD * 2.0f + TE_CEL_GAP + TE_CEL_W <= NV_TELA_W - 48.0f)
    celAtivo = celular_abrir(tituloAtual);
}

// O texto do sistema passa pelo ALFABETO da modal: o codigo de pareamento e
// a-z0-9, o MAC e 0-9a-f — caixa alta vira baixa quando so a baixa existe, e
// o que nao existe nele (acento, emoji) fica de fora.
static void definirDoSistema(const char *t) {
  const char *a = alfa();
  int w = 0;
  for (; *t && w < maxN; t++) {
    unsigned char c = (unsigned char)*t;
    if (c >= 0x80 || !c) continue;
    if (strchr(a, c)) texto[w++] = (char)c;
    else if (isupper(c) && strchr(a, tolower(c))) texto[w++] = (char)tolower(c);
    else if (islower(c) && strchr(a, toupper(c))) texto[w++] = (char)toupper(c);
  }
  texto[w] = 0;
  n = w;
}

static void fechar(int r) {
  aberto = 0;
  celular_fechar();
  resultado = r;
  st_fechar(ST_TECLADO);
}

static void okBarra(void) {
  if (coluna == 1) st_voz_iniciar(ST_TECLADO);
  else st_ime_abrir(ST_TECLADO, texto, maxN);
}
static void focarBarra(int c, int b) { (void)b; fileira = -1; coluna = c; }

static int colunasDe(int f) {
  int n;
  if (f >= nFileiras - 1) return 3;         // apagar / limpar / pronto
  // A ULTIMA FILEIRA DE CARACTERE PODE SER PARCIAL: um alfabeto de 39 enche
  // seis colunas em seis fileiras e deixa tres na setima. Sem isto o foco
  // entraria em celula vazia e "digitaria" o byte depois do fim da string.
  n = (int)strlen(alfa()) - f * nCols;
  return n > nCols ? nCols : (n > 0 ? n : 1);
}

static GfxRect retangulo(int f, int c) {
  GfxRect r;
  r.y = teY() + TE_CAB + (float)f * TE_PASSO;
  r.h = TE_TECLA;
  if (f < nFileiras - 1) {
    r.x = TE_X + TE_PAD + (float)c * TE_PASSO;
    r.w = TE_TECLA;
  } else {
    r.w = (gradeW() - 2.0f * TE_GAP) / 3.0f;
    r.x = TE_X + TE_PAD + (float)c * (r.w + TE_GAP);
  }
  return r;
}

// O rotulo da ultima fileira. As tres teclas sao o unico ponto da modal com
// palavra em vez de caractere, e por isso as tres estao na tabela de i18n.
static const char *rotuloExtra(int c) {
  return c == 0 ? "apagar" : (c == 1 ? "limpar" : "pronto");
}

static void aplicar(void) {
  if (fileira < nFileiras - 1) {
    if (n < maxN) { texto[n++] = alfa()[fileira * nCols + coluna]; texto[n] = 0; }
    return;
  }
  if (coluna == 0) { if (n > 0) texto[--n] = 0; return; }
  if (coluna == 1) { n = 0; texto[0] = 0; return; }
  // "pronto" com o campo vazio nao e uma confirmacao de nada: quem chega ali
  // sem digitar quis olhar o teclado, e fechar a modal com resultado PRONTO
  // mandaria a tela de amigos tentar vincular uma string vazia.
  if (n < 1) return;
  fechar(TECLADO_PRONTO);
}

void teclado_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (aberto && st_evento(e)) return;   // teclado da TV: valor inteiro por sistexto
  if (!aberto || e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      k == SDLK_DELETE || e->key.keysym.scancode == NV_SCANCODE_BACK) {
    fechar(TECLADO_CANCELOU);
    return;
  }
  if (fileira < 0) {
    if (k == SDLK_DOWN) { fileira = 0; coluna = colunaAntes < colunasDe(0) ? colunaAntes : 0; }
    else if (k == SDLK_RIGHT && coluna == 0 && st_voz_disponivel()) coluna = 1;
    else if (k == SDLK_LEFT && coluna == 1) coluna = 0;
    else if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && !e->key.repeat) okBarra();
    return;
  }
  if (k == SDLK_UP) {
    // Cima da primeira fileira: o campo, onde ha teclado do sistema.
    if (fileira == 0 && st_ime_disponivel()) { colunaAntes = coluna; fileira = -1; coluna = 0; return; }
    if (fileira > 0) {
      if (fileira == nFileiras - 1) coluna = colunaAntes;
      fileira--;
    }
    if (coluna >= colunasDe(fileira)) coluna = colunasDe(fileira) - 1;
    return;
  }
  if (k == SDLK_DOWN) {
    if (fileira + 1 < nFileiras) {
      if (fileira + 1 == nFileiras - 1) colunaAntes = coluna;
      fileira++;
    }
    if (coluna >= colunasDe(fileira)) coluna = colunasDe(fileira) - 1;
    return;
  }
  if (k == SDLK_LEFT)  { if (coluna > 0) coluna--; return; }
  if (k == SDLK_RIGHT) { if (coluna + 1 < colunasDe(fileira)) coluna++; return; }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) {
    if (e->key.repeat) return;   // manter OK apertado nao digita a mesma letra
    aplicar();
    return;
  }
}

void teclado_atualizar(float dt, Uint32 agora) {
  int f, c;
  (void)agora;
  if (aberto) {
    char t[TECLADO_LONGO + 1];
    int voz = st_estado() == ST_OUVINDO || st_estado() == ST_PERMISSAO || st_estado() == ST_VOZ_SISTEMA;
    int r;
    if (celAtivo && celular_pegar(t, sizeof t)) {
      // O texto do celular e o VALOR INTEIRO do campo, como o do teclado da
      // TV: passa pelo mesmo filtro de alfabeto. Nao confirma sozinho — a
      // pessoa ve o que chegou, e o foco vai para "pronto": um OK salva.
      st_fechar(ST_TECLADO);
      definirDoSistema(t);
      memset(t, 0, sizeof t);
      celRecebido = 1;
      fileira = nFileiras - 1; coluna = 2;
    }
    r = st_ler(ST_TECLADO, t, sizeof t);
    if (r == ST_PEDE_TECLADO) { coluna = 0; st_ime_abrir(ST_TECLADO, texto, maxN); }
    else if (r == ST_TEXTO || r == ST_FIM) {
      definirDoSistema(t);
      // "Concluir" no teclado da TV e o "pronto" da modal; o fim da FALA nao —
      // a pessoa confere o que o reconhecedor entendeu antes de enviar.
      if (r == ST_FIM && !voz && n > 0) fechar(TECLADO_PRONTO);
    }
  }
  for (c = 0; c < 2; c++)
    animBarra[c] = anim_mola(animBarra[c], (aberto && fileira < 0 && coluna == c) ? 1.0f : 0.0f,
                             dt, NV_MOLA_FOCO);
  if (!aberto && anim < 0.002f) { anim = 0.0f; return; }
  anim = ajustes_animacoes_reduzidas()
           ? (aberto ? 1.0f : 0.0f)
           : anim_mola(anim, aberto ? 1.0f : 0.0f, dt, NV_MOLA_TELA);
  for (f = 0; f < nFileiras; f++)
    for (c = 0; c < nCols; c++) {
      float alvo = (aberto && f == fileira && c == coluna) ? 1.0f : 0.0f;
      focoAnim[f][c] = ajustes_animacoes_reduzidas()
        ? alvo : anim_mola(focoAnim[f][c], alvo, dt, NV_MOLA_FOCO);
    }
}

// --- painel "Digitar pelo celular" ---------------------------------------------
// QR como TEXTURA (como login.c): a versao 3-4 tem ~1000 modulos, e um retangulo
// por modulo por quadro custaria mais que a modal inteira. NEAREST: modulo
// borrado e o jeito mais rapido de a camera nao ler.
static GLuint texCel;
static char   texCelDe[96];
static void celTextura(const char *u) {
  Qr q;
  int lado, x, y;
  unsigned char *px;
  if (!u[0] || (texCel && !strcmp(texCelDe, u))) return;
  if (!qr_gerar(&q, u)) return;
  lado = q.lado + 4;
  px = (unsigned char *)malloc((size_t)lado * lado * 3);
  if (!px) return;
  memset(px, 255, (size_t)lado * lado * 3);
  for (y = 0; y < q.lado; y++)
    for (x = 0; x < q.lado; x++)
      if (qr_modulo(&q, x, y)) {
        size_t i = ((size_t)(y + 2) * lado + (x + 2)) * 3;
        px[i] = px[i + 1] = px[i + 2] = 0;
      }
  if (!texCel) glGenTextures(1, &texCel);
  glBindTexture(GL_TEXTURE_2D, texCel);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, lado, lado, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  free(px);
  snprintf(texCelDe, sizeof texCelDe, "%s", u);
}

static void desenharCelular(float dy, float a) {
  float px = TE_X + TE_W + TE_CEL_GAP, py = teY() + dy, ph = teH();
  float x = px + 36.0f, w = TE_CEL_W - 72.0f, y = py + TE_PAD;
  int est = celular_estado();
  const char *u = celular_url();
  gfx_cor((GfxRect){ px, py, TE_CEL_W, ph }, 24.0f / ph, 0.075f, 0.078f, 0.088f, 0.99f * a);
  { TxtLinha t = txt_linha(TXT_HEADLINE, "Digitar pelo celular", 245, 248, 255, 255);
    txt_desenhar_alpha(t, x, y, a);
    y += t.h + 14.0f; }
  if (celRecebido || est == CEL_RECEBIDO) {
    float ar, ag, ab;
    ajustes_acento(&ar, &ag, &ab);
    y += 40.0f;
    { TxtLinha t = txt_linha(TXT_HEADLINE, "Recebido do celular",
                             (int)(ar * 255), (int)(ag * 255), (int)(ab * 255), 255);
      txt_desenhar_alpha(t, x, y, a);
      y += t.h + 16.0f; }
    txt_bloco(TXT_BODY, "Confira o texto e aperte Pronto.", 190, 194, 204, x, y, w, 36.0f, a, 3);
    return;
  }
  if (est != CEL_ESPERANDO || !u[0]) {
    txt_bloco(TXT_BODY, est == CEL_FALHOU
                ? "Endereço bloqueado por tentativas erradas. Feche e abra o teclado de novo."
                : "O endereço expirou. Feche e abra o teclado de novo.",
              190, 194, 204, x, y + 40.0f, w, 36.0f, a, 4);
    return;
  }
  txt_bloco(TXT_CAPTION2, "Aponte a câmera do celular para o código e cole o texto na página.",
            160, 164, 175, x, y, w, 28.0f, a * 0.9f, 3);
  y += 3 * 28.0f + 18.0f;
  celTextura(u);
  if (texCel) {
    float q = TE_CEL_QR, qx = px + (TE_CEL_W - q) * 0.5f;
    gfx_cor((GfxRect){ qx - 14.0f, y - 14.0f, q + 28.0f, q + 28.0f }, 0.06f, 1.0f, 1.0f, 1.0f, a);
    gfx_tex_aspect_atual = 0.0f;
    gfx_rect((GfxRect){ qx, y, q, q }, texCel, GFX_SNAP, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, a);
    y += q + 40.0f;
  }
  // O ENDERECO EM TEXTO, para quem nao tem camera: numa linha se couber, senao
  // quebrado antes do token (a URL nao tem espaco para o txt_bloco quebrar).
  { TxtLinha t = txt_linha(TXT_CAPTION, u, 220, 224, 232, 255);
    if (t.w <= w) txt_desenhar_alpha(t, px + (TE_CEL_W - t.w) * 0.5f, y, a);
    else {
      const char *barra = strrchr(u, '/');
      char l1[96];
      TxtLinha t2;
      snprintf(l1, sizeof l1, "%.*s", (int)(barra - u), u);
      t = txt_linha(TXT_CAPTION, l1, 220, 224, 232, 255);
      t2 = txt_linha(TXT_CAPTION, barra, 220, 224, 232, 255);
      txt_desenhar_alpha(t, px + (TE_CEL_W - t.w) * 0.5f, y, a);
      txt_desenhar_alpha(t2, px + (TE_CEL_W - t2.w) * 0.5f, y + t.h + 4.0f, a);
    } }
  txt_bloco(TXT_CAPTION2, "Mesma rede Wi-Fi da TV. Vale por 5 minutos e um envio.",
            140, 144, 155, x, py + ph - TE_PAD - 56.0f, w, 28.0f, a * 0.86f, 2);
}

void teclado_desenhar(Uint32 agora) {
  float a = anim_suave(anim), dy, x, y;
  int f, c, i;
  if (anim < 0.01f) return;
  dy = (1.0f - a) * 36.0f;

  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0, 0, 0, 0.76f * anim);
  { GfxRect p = { TE_X, teY() + dy, TE_W, teH() };
    gfx_cor(p, 24.0f / teH(), 0.075f, 0.078f, 0.088f, 0.99f * a); }
  if (celAtivo) desenharCelular(dy, a);

  x = TE_X + TE_PAD;
  y = teY() + dy + TE_PAD;
  { TxtLinha t = txt_linha(TXT_HEADLINE, tituloAtual, 245, 248, 255, 255);
    txt_desenhar_alpha(t, x, y, a); }
  // EM BLOCO: a dica nao cabe numa linha de 504px em portugues, e na captura
  // ela saiu terminando em "...que ele te...".
  if (dicaAtual[0])
    txt_bloco(TXT_CAPTION2, dicaAtual, 160, 164, 175,
              x, teY() + dy + TE_DICA_Y, gradeW(), 28.0f, a * 0.9f, 2);

  // O QUE FOI DIGITADO — em CAIXAS ou em LINHA, e quem decide e a conta, nao
  // quem chamou.
  //
  // Uma caixa por caractere so funciona enquanto a caixa couber o glifo. Com
  // maxN = 4 (o codigo do amigo) cada caixa tem 117 px e a fileira de casas
  // vazias diz "faltam tres" sem precisar de frase nenhuma. Com maxN = 24 (a
  // busca de listas publicas) a mesma conta da 9,5 px por caixa, e o glifo de
  // TXT_TITULO2 tem mais de 30: as letras se sobrepunham umas nas outras e o
  // dono fotografou o resultado — tres "a" viraram uma mancha em cima de uma
  // cerca de barrinhas.
  //
  // Entao: caixa so quando ela cabe o glifo (TE_CX_MIN), senao CAMPO DE TEXTO
  // com cursor, que e a forma certa para texto livre de qualquer tamanho. E a
  // fileira de casas vazias nao faz falta aqui — numa busca nao ha numero de
  // caracteres a completar.
  { float ar, ag, ab, cw = campoW();
    GfxRect zona = { TE_X + TE_PAD, teY() + dy + TE_CAIXA_Y, cw, TE_CY };
    ajustes_acento(&ar, &ag, &ab);
    // FOCO NO CAMPO (cima da primeira fileira, so com teclado do sistema).
    if (animBarra[0] > 0.01f)
      gfx_vidro_aro((GfxRect){ zona.x - 8, zona.y - 8, zona.w + 16, zona.h + 16 }, NV_RAIO_CARD, 2.5f,
                    ar, ag, ab, 0.95f * animBarra[0] * a);
    if (st_ime_disponivel() && ponteiro_ativo())
      ponteiro_alvo(zona.x, zona.y, zona.w, zona.h, focarBarra, NULL, 0, 0);
    // FALAR, ao lado do campo.
    if (st_voz_disponivel()) {
      float d = TE_MIC_D, mx = zona.x + zona.w + 14.0f, my = zona.y + (zona.h - d) * 0.5f;
      int ouve = st_dono() == ST_TECLADO && (st_estado() == ST_OUVINDO || st_estado() == ST_PERMISSAO ||
                                              st_estado() == ST_VOZ_SISTEMA);
      float k = ouve ? 1.0f : animBarra[1];
      if (ponteiro_ativo()) ponteiro_alvo(mx, my, d, d, focarBarra, NULL, 1, 0);
      if (k > 0.01f)
        gfx_rect((GfxRect){ mx - 12, my - 12, d + 24, d + 24 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
                 ar, ag, ab, (ouve ? 0.25f + 0.4f * st_nivel() : 0.3f) * k * a);
      gfx_cor((GfxRect){ mx, my, d, d }, 0.5f, anim_mistura(0.18f, ar, k), anim_mistura(0.185f, ag, k),
              anim_mistura(0.205f, ab, k), a);
      { int t = k > 0.5f ? ajustes_tinta_foco() : 220;
        gfx_icone((GfxRect){ mx + d * 0.27f, my + d * 0.27f, d * 0.46f, d * 0.46f }, "aj_mic",
                  t / 255.0f, t / 255.0f, t / 255.0f, a); }
    } }
  if ((campoW() - (float)(maxN - 1) * TE_CGAP) / (float)maxN >= TE_CX_MIN) {
    float bw = (campoW() - (float)(maxN - 1) * TE_CGAP) / (float)maxN;
    float bx, by = teY() + dy + TE_CAIXA_Y;
    if (bw > TE_CX) bw = TE_CX;
    bx = TE_X + TE_PAD + (campoW() - ((float)maxN * bw + (float)(maxN - 1) * TE_CGAP)) * 0.5f;
    for (i = 0; i < maxN; i++) {
      GfxRect b = { bx, by, bw, TE_CY };
      char ch[2];
      // 0.055 do menor lado, como NV_RAIO_CARD: o raio do gfx_cor e FRACAO,
      // nao pixel, e um 12 aqui viraria uma pilula.
      // A CHEIA BEM MAIS CLARA QUE A VAZIA, pelo mesmo motivo: com 0,13
      // contra 0,09 de uma tecla em repouso, cheia e vazia eram a mesma
      // mancha a tres metros.
      gfx_cor(b, NV_RAIO_CARD, 1.0f, 1.0f, 1.0f, (i < n ? 0.22f : 0.04f) * a);
      if (i < n) {
        TxtLinha t;
        ch[0] = texto[i]; ch[1] = 0;
        t = txt_linha(TXT_TITULO2, ch, 246, 248, 255, 255);
        txt_desenhar_alpha(t, b.x + (b.w - t.w) * 0.5f,
                           b.y + (b.h - t.h) * 0.5f, a);
      }
      bx += bw + TE_CGAP;
    }
  } else {
    GfxRect campo = { TE_X + TE_PAD, teY() + dy + TE_CAIXA_Y,
                      campoW(), TE_CY };
    float tx = campo.x + TE_CAMPO_PAD, cursorX = tx;
    gfx_cor(campo, NV_RAIO_CARD, 1.0f, 1.0f, 1.0f, 0.07f * a);
    if (n) {
      TxtLinha t = txt_linha(TXT_TITULO2, texto, 246, 248, 255, 255);
      // TEXTO MAIS LARGO QUE O CAMPO ROLA PELO FIM, nao pelo comeco: quem
      // digita precisa ver a ultima letra que apertou, nao a primeira.
      float larg = campo.w - TE_CAMPO_PAD * 2.0f;
      float ox = t.w > larg ? t.w - larg : 0.0f;
      gfx_recorte(campo.x + TE_CAMPO_PAD, campo.y,
                  larg, campo.h);
      txt_desenhar_alpha(t, tx - ox, campo.y + (campo.h - t.h) * 0.5f, a);
      gfx_sem_recorte();
      cursorX = tx + (t.w - ox);
    }
    // CURSOR SEM PISCA-PISCA quando as animacoes estao reduzidas — piscar e
    // movimento, e a regra vale aqui como vale no resto do app.
    { float op = ajustes_animacoes_reduzidas()
                   ? 0.85f
                   : 0.35f + 0.5f * (((agora / 500) % 2) ? 0.0f : 1.0f);
      GfxRect cur = { cursorX + 3.0f, campo.y + 22.0f, 3.0f, campo.h - 44.0f };
      if (cur.x > campo.x + campo.w - TE_CAMPO_PAD)
        cur.x = campo.x + campo.w - TE_CAMPO_PAD;
      gfx_cor(cur, 0.5f, 0.95f, 0.96f, 0.99f, op * a); }
  }

  for (f = 0; f < nFileiras; f++) {
    for (c = 0; c < colunasDe(f); c++) {
      float k = focoAnim[f][c];
      GfxRect base = retangulo(f, c);
      float esc = 1.0f + TE_ESCALA * k;
      GfxRect t;
      const char *s;
      char ch[2];
      int tom;
      base.y += dy;
      t.w = base.w * esc; t.h = base.h * esc;
      t.x = base.x - (t.w - base.w) * 0.5f;
      t.y = base.y - (t.h - base.h) * 0.5f;
      // INVERTE no foco, como a grade da busca: a tres metros, numa grade de
      // 39 alvos iguais, a inversao e o unico contraste que se ve de relance.
      gfx_cor(t, NV_RAIO_CARD, 1.0f, 1.0f, 1.0f, anim_mistura(0.09f, 1.0f, k) * a);
      if (f < nFileiras - 1) {
        ch[0] = alfa()[f * nCols + c]; ch[1] = 0;
        s = ch;
        // A TECLA DE ESPACO (busca do guia) diz o que e: uma tecla vazia
        // pareceria quebrada. "␣" nao existe na fonte da interface.
        if (ch[0] == ' ') s = i18n("espaço");
      } else {
        s = rotuloExtra(c);
      }
      // A COR DO TEXTO EM DEGRAU e nao interpolada: ela faz parte da chave do
      // cache de linhas de text.c, e uma cor por quadro em 39 teclas estoura o
      // orcamento de rasterizacao — e ai a tecla sai SEM GLIFO (a nota longa
      // esta em ctxmenu.c). O degrau cai em k=0,5, onde o fundo esta a 0,55 de
      // luminancia e as duas cores ainda sao legiveis.
      tom = k >= 0.5f ? 26 : 236;
      // "espaco" no corpo de uma letra (TITULO3) passava das bordas da tecla
      // de 74 px e cobria a vizinha; a palavra vai no corpo de legenda.
      { TxtLinha l = txt_linha(f >= nFileiras - 1 ? TXT_BODY
                               : (s != ch ? TXT_CAPTION2 : TXT_TITULO3),
                               s, tom, tom, tom, 255);
        txt_desenhar_alpha(l, t.x + (t.w - l.w) * 0.5f,
                           t.y + (t.h - l.h) * 0.5f, a); }
    }
  }

  { const char *av = st_dono() == ST_TECLADO ? st_aviso() : "";
    const char *d = av[0] ? av
      : fileira < 0 ? (coluna == 1 ? "OK Falar   Baixo Teclado   Voltar Cancelar"
                                   : "OK Teclado da TV   Baixo Teclado   Voltar Cancelar")
      : st_ime_disponivel() ? "Setas Navegar   OK Digitar   Cima Teclado da TV"
      : "Setas Navegar   OK Digitar   Voltar Cancelar";
    TxtLinha t = txt_linha(TXT_CAPTION2, d, av[0] ? 240 : 155, av[0] ? 196 : 159, av[0] ? 140 : 169, 255);
    txt_desenhar_alpha(t, x, teY() + dy + teH() - TE_PAD - t.h, a * 0.86f); }
}
