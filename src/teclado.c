// Ver teclado.h para por que esta modal existe e o que ela NAO tenta ser.
#include "teclado.h"
#include "escala.h"
#include "gfx.h"
#include "text.h"
#include "anim.h"
#include "layout.h"
#include "ajustes.h"
#include "idioma.h"
#include "sistexto.h"
#include "ponteiro.h"
#include "celbotao.h"
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

// DIGITAR PELO CELULAR (celbotao.h): era um painel fixo de 440 px a direita
// da modal, com o QR sempre a vista. Virou o MESMO botao do Spotlight e da
// Busca, ao lado do campo (depois do Falar): um so gesto em todo o app, a
// modal volta a ser centrada e da largura da grade, e o servidor so sobe
// quando a pessoa pede (OK no botao), nao a cada teclado aberto.
#define TE_X        ((NV_TELA_W - TE_W) * 0.5f)


static const char *ALFABETO = "abcdefghijklmnopqrstuvwxyz0123456789";

// fileira -1 = a BARRA do campo: coluna 0 o campo (OK chama o teclado da TV),
// coluna 1 o Falar (os dois so com sistexto.h), coluna 2 o celular.
static int   aberto, fileira, coluna;
static float animBarra[3];
// Coluna de caractere de onde o foco desceu para apagar/limpar/pronto. Sem ela,
// subir de "pronto" (coluna 2) numa grade de 13 caia no 'c', a dez teclas de
// onde a pessoa estava; com ela, volta para a mesma tecla.
static int   colunaAntes;
static float anim, focoAnim[TE_FILEIRAS_MAX][TE_COLS_MAX];
static const char *alfabetoAtual = NULL;   // NULL = o padrao
static int   nFileiras = TE_FILEIRAS_PAD;
static int   nCols = TE_COLS;

// LOGIN POR E-MAIL (#216, decisao do dono): quem entra por e-mail e senha e
// justamente quem nao tem celular a mao — se tivesse, logava pelo QR. Nesses
// campos o botao "Digitar pelo celular" some.
static int semCel;
static int celOk(void) { return celb_disponivel() && !semCel; }

static float gradeW(void) {
  return (float)nCols * TE_TECLA + (float)(nCols - 1) * TE_GAP;   // 504 com 6
}
// O campo perde a largura do botao Falar onde ha voz.
#define TE_MIC_D (TE_CY - 12.0f)
static float campoW(void) {
  return gradeW() - (st_voz_disponivel() ? TE_MIC_D + 14.0f : 0.0f) -
         (celOk() ? TE_MIC_D + 14.0f : 0.0f);
}
// A barra existe como destino do foco se tiver algum alvo.
static int temBarra(void) { return st_ime_disponivel() || celOk(); }

// A altura da modal depende de quantas fileiras o alfabeto pediu, entao as tres
// medidas que dela dependem viraram funcao. Continuam sendo a mesma conta.
static float gradeH(void) {
  return (float)nFileiras * TE_TECLA + (float)(nFileiras - 1) * TE_GAP;
}
// GLASS UI (mockup de Ajustes, quadro "teclado", 03/10): a modal virou uma
// ilha de DUAS COLUNAS para caber a 3 m sem rolar — a esquerda o titulo, a
// dica, o campo e os modos (teclado da TV, falar, celular); a direita a grade
// de 74 px e a fileira apagar / limpar / concluir, de 56.
#define TE_ILHA_PX   52.0f
#define TE_ILHA_PY   48.0f
#define TE_COL_GAP   56.0f
#define TE_EXTRA_H   56.0f
static float teEsqW(void) {
  float w = 1340.0f - 2 * TE_ILHA_PX - TE_COL_GAP - gradeW();
  float teto = NV_TELA_W - 80.0f - 2 * TE_ILHA_PX - TE_COL_GAP - gradeW();
  if (w > teto) w = teto;
  if (w < 420.0f) w = 420.0f;
  return w;
}
static float teW(void) { return 2 * TE_ILHA_PX + teEsqW() + TE_COL_GAP + gradeW(); }
static float teX(void) { return (NV_TELA_W - teW()) * 0.5f; }
static float teGradeX(void) { return teX() + TE_ILHA_PX + teEsqW() + TE_COL_GAP; }
static float teH(void) {
  float grade = (float)(nFileiras - 1) * TE_PASSO - TE_GAP + 20.0f + TE_EXTRA_H;
  // A coluna da esquerda tem altura propria (titulo, dica, campo, modos e
  // as dicas na base): com um alfabeto curto (hexadecimal, 3 fileiras) a
  // grade sozinha deixaria as dicas em cima do texto.
  float esq = 22 + 48 + 10 + 57 + 30 + 76 + 22 + 55 + 16 + 50 + 40 + 30;
  return 2 * TE_ILHA_PY + (grade > esq ? grade : esq);
}
static float teY(void) { return (NV_TELA_H - teH()) * 0.5f; }
static const char *alfa(void) { return alfabetoAtual ? alfabetoAtual : ALFABETO; }
static char  texto[TECLADO_LONGO + 1];
static int   n, maxN, resultado;
static char  tituloAtual[96], dicaAtual[160];
static int   celRecebido;
// SENHA (#216): o campo mostra pontos, nunca o texto. A modal fica na tela e
// a tela vira foto.
static int   mascarar, tipoIme, ehSenha;
// ATALHOS DE E-MAIL (#216): uma fileira de teclas que digitam o pedaco
// inteiro. Com o D-pad, "@gmail.com" sao dez teclas a menos.
static const char *ATALHOS_EMAIL[] = { ".com", "@gmail.com", "@hotmail.com", "@outlook.com" };
#define TE_N_ATALHOS ((int)(sizeof ATALHOS_EMAIL / sizeof ATALHOS_EMAIL[0]))
static int   nAtalhos;
// Fileiras: as de caractere, a de atalhos (so no e-mail) e a de
// apagar/limpar/(mostrar)/pronto, sempre a ultima.
static int fileirasChar(void) { return nFileiras - 1 - (nAtalhos ? 1 : 0); }
static int ehChar(int f) { return f >= 0 && f < fileirasChar(); }
static int ehAtalho(int f) { return nAtalhos && f == fileirasChar(); }
static int colunaPronto(void) { return ehSenha ? 3 : 2; }
static void abrirImeAgora(void);
void teclado_tipo(int tipo) {
  tipoIme = tipo == TECLADO_TIPO_EMAIL ? ST_IME_EMAIL : tipo == TECLADO_TIPO_SENHA ? ST_IME_SENHA : ST_IME_TEXTO;
  ehSenha = tipo == TECLADO_TIPO_SENHA;
  mascarar = ehSenha;
  semCel = tipo == TECLADO_TIPO_EMAIL || tipo == TECLADO_TIPO_SENHA;
  if (tipo == TECLADO_TIPO_EMAIL) {
    // 13 colunas (a-m / n-z / 0-9@._ / -+) + atalhos: 6 fileiras, a altura
    // do teclado padrao, em vez de 8 de 6 colunas.
    int letras = (int)strlen(alfa());
    nCols = TE_COLS_LONGO;
    nAtalhos = TE_N_ATALHOS;
    nFileiras = (letras + nCols - 1) / nCols + 2;
    if (nFileiras > TE_FILEIRAS_MAX) nFileiras = TE_FILEIRAS_MAX;
  }
  // Onde o teclado do sistema abre sozinho (Android), ele ja vem aberto: quem
  // toca no campo de e-mail quer o teclado do aparelho, nao a grade.
  if (semCel && aberto && st_ime_disponivel() && st_abre_sozinho()) {
    fileira = -1; coluna = 0;
    abrirImeAgora();
  }
}
void teclado_mascarar(int liga) { mascarar = liga != 0; }
int  teclado_mascarado(void) { return mascarar; }

const char *teclado_alfabeto(void) { return ALFABETO; }
int teclado_aberto(void) { return aberto; }
int teclado_foco_campo(void) { return fileira < 0 ? coluna + 1 : 0; }
const char *teclado_texto(void) { return texto; }
void teclado_esquecer(void) { volatile char *p = texto; size_t k = sizeof texto; while (k--) *p++ = 0; n = 0; }

int teclado_resultado(void) {
  int r = resultado;
  resultado = TECLADO_NADA;
  return r;
}

// O contexto da modal ("Contas e serviços · Chaves"), consumido pela proxima
// abertura: quem chama poe antes de teclado_abrir_com, e a seguinte nasce sem.
// Vale por um instante: quem pos e nao abriu nao contamina a proxima modal.
static char kickerPend[120], kickerAtual[120];
static Uint32 kickerQuando;
void teclado_contexto(const char *kicker) {
  snprintf(kickerPend, sizeof kickerPend, "%s", kicker ? kicker : "");
  kickerQuando = SDL_GetTicks();
}
#ifdef AJUSTES_TESTE
void teclado_teste_texto(const char *t) { snprintf(texto, sizeof texto, "%s", t); n = (int)strlen(texto); }
void teclado_teste_foco(int f, int c) { fileira = f; coluna = c; }
#endif

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
  snprintf(kickerAtual, sizeof kickerAtual, "%s", SDL_GetTicks() - kickerQuando < 1000u ? kickerPend : "");
  kickerPend[0] = 0;
  memset(focoAnim, 0, sizeof focoAnim);
  animBarra[0] = animBarra[1] = animBarra[2] = 0.0f;
  celRecebido = 0;
  mascarar = 0; tipoIme = ST_IME_TEXTO; ehSenha = 0; semCel = 0; nAtalhos = 0;
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
  celb_fechar_dono(CELB_TECLADO);
  resultado = r;
  st_fechar(ST_TECLADO);
}

static void abrirImeAgora(void) { st_ime_tipo(tipoIme); st_ime_abrir(ST_TECLADO, texto, maxN); }
static void okBarra(void) {
  if (coluna == 2) { st_fechar(ST_TECLADO); celb_abrir(CELB_TECLADO, tituloAtual); }
  else if (coluna == 1) st_voz_iniciar(ST_TECLADO);
  else { st_ime_tipo(tipoIme); st_ime_abrir(ST_TECLADO, texto, maxN); }
}
static void focarBarra(int c, int b) { (void)b; fileira = -1; coluna = c; }
static void focarTecla(int f, int c) {
  if (!aberto || f < 0 || f >= nFileiras) return;
  if (!ehChar(f) && ehChar(fileira)) colunaAntes = coluna;
  fileira = f; coluna = c;
}


static int colunasDe(int f) {
  int n;
  if (f >= nFileiras - 1) return ehSenha ? 4 : 3;   // apagar / limpar / (mostrar) / pronto
  if (ehAtalho(f)) return nAtalhos;
  // A ULTIMA FILEIRA DE CARACTERE PODE SER PARCIAL: um alfabeto de 39 enche
  // seis colunas em seis fileiras e deixa tres na setima. Sem isto o foco
  // entraria em celula vazia e "digitaria" o byte depois do fim da string.
  n = (int)strlen(alfa()) - f * nCols;
  return n > nCols ? nCols : (n > 0 ? n : 1);
}

static GfxRect retangulo(int f, int c) {
  GfxRect r;
  r.y = teY() + TE_ILHA_PY + (float)f * TE_PASSO;
  r.h = TE_TECLA;
  if (ehChar(f)) {
    r.x = teGradeX() + (float)c * TE_PASSO;
    r.w = TE_TECLA;
  } else if (f == nFileiras - 1) {
    // apagar / limpar / (mostrar) / CONCLUIR: o ultimo e 1,3 vez os outros.
    int k = colunasDe(f), i;
    float unid = (gradeW() - (float)(k - 1) * TE_GAP) / ((float)(k - 1) + 1.3f);
    r.y = teY() + TE_ILHA_PY + (float)(nFileiras - 1) * TE_PASSO - TE_GAP + 20.0f;
    r.h = TE_EXTRA_H;
    r.x = teGradeX();
    for (i = 0; i < c; i++) r.x += unid + TE_GAP;
    r.w = c == k - 1 ? unid * 1.3f : unid;
  } else {
    int k = colunasDe(f);
    r.w = (gradeW() - (float)(k - 1) * TE_GAP) / (float)k;
    r.x = teGradeX() + (float)c * (r.w + TE_GAP);
  }
  return r;
}

// O rotulo da ultima fileira. As tres teclas sao o unico ponto da modal com
// palavra em vez de caractere, e por isso as tres estao na tabela de i18n.
static const char *rotuloExtra(int c) {
  if (c == 0) return "apagar";
  if (c == 1) return "limpar";
  if (ehSenha && c == 2) return mascarar ? "mostrar" : "ocultar";
  return "Concluir";
}

static void aplicar(void) {
  if (ehChar(fileira)) {
    if (n < maxN) { texto[n++] = alfa()[fileira * nCols + coluna]; texto[n] = 0; }
    return;
  }
  if (ehAtalho(fileira)) {
    const char *a = ATALHOS_EMAIL[coluna < nAtalhos ? coluna : 0];
    // "@gmail.com" depois de um "@" ja digitado nao duplica a arroba.
    if (a[0] == '@' && strchr(texto, '@')) a++;
    if (n + (int)strlen(a) <= maxN) { memcpy(texto + n, a, strlen(a) + 1); n += (int)strlen(a); }
    return;
  }
  if (coluna == 0) { if (n > 0) texto[--n] = 0; return; }
  if (coluna == 1) { n = 0; texto[0] = 0; return; }
  if (ehSenha && coluna == 2) { mascarar = !mascarar; return; }
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
    // DIREITA anda campo -> Falar -> Celular, o que existir aqui.
    else if (k == SDLK_RIGHT && coluna == 0 && st_voz_disponivel()) coluna = 1;
    else if (k == SDLK_RIGHT && coluna < 2 && celOk()) coluna = 2;
    else if (k == SDLK_LEFT && coluna == 2 && st_voz_disponivel()) coluna = 1;
    else if (k == SDLK_LEFT && coluna >= 1 && st_ime_disponivel()) coluna = 0;
    else if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && !e->key.repeat) okBarra();
    return;
  }
  if (k == SDLK_UP) {
    // Cima da primeira fileira: o campo, onde ha teclado do sistema; sem ele
    // (LG, Samsung), direto o botao do celular.
    if (fileira == 0 && temBarra()) {
      colunaAntes = coluna; fileira = -1; coluna = st_ime_disponivel() ? 0 : 2;
      return;
    }
    if (fileira > 0) {
      if (!ehChar(fileira) && ehChar(fileira - 1)) coluna = colunaAntes;
      fileira--;
    }
    if (coluna >= colunasDe(fileira)) coluna = colunasDe(fileira) - 1;
    return;
  }
  if (k == SDLK_DOWN) {
    if (fileira + 1 < nFileiras) {
      if (ehChar(fileira) && !ehChar(fileira + 1)) colunaAntes = coluna;
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
    if (celb_pegar(CELB_TECLADO, t, sizeof t)) {
      // O texto do celular e o VALOR INTEIRO do campo, como o do teclado da
      // TV: passa pelo mesmo filtro de alfabeto. Nao confirma sozinho — a
      // pessoa ve o que chegou, e o foco vai para "pronto": um OK salva.
      st_fechar(ST_TECLADO);
      definirDoSistema(t);
      memset(t, 0, sizeof t);
      celRecebido = 1;
      fileira = nFileiras - 1; coluna = colunaPronto();
    }
    r = st_ler(ST_TECLADO, t, sizeof t);
    if (r == ST_PEDE_TECLADO) { coluna = 0; st_ime_tipo(tipoIme); st_ime_abrir(ST_TECLADO, texto, maxN); }
    else if (r == ST_TEXTO || r == ST_FIM) {
      definirDoSistema(t);
      // "Concluir" no teclado da TV e o "pronto" da modal; o fim da FALA nao —
      // a pessoa confere o que o reconhecedor entendeu antes de enviar.
      if (r == ST_FIM && !voz && n > 0) fechar(TECLADO_PRONTO);
    }
  }
  for (c = 0; c < 3; c++)
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

// Material da ilha (o mesmo de ajustes_ux_desenho.inc): vidro a 86% com a
// luz do canto, solido #15161A; foco de tecla/chip = cheio no acento.
static void teNeutro(GfxRect r, float raioPx, float vid, float sr, float sg, float sb, float a) {
  if (ajustes_vidro()) gfx_cor(r, raioPx / r.h, 1, 1, 1, vid * a);
  else gfx_cor(r, raioPx / r.h, sr, sg, sb, a);
}
static void teAcento(GfxRect r, float raioPx, float k, float a) {
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  gfx_rect((GfxRect){ r.x - 12, r.y - 2, r.w + 24, r.h + 26 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f, ar, ag, ab, 0.35f * k * a);
  gfx_cor(r, raioPx / r.h, ar, ag, ab, k * a);
}
static float teCaps(const char *s, float x, float y, float a) {
  char up[200];
  idioma_maiusc_em(ajustes_idioma(), up, sizeof up, i18n(s));
  return txt_tracking(TXT_MINI, up, 243, 242, 239, x, y, 0.45f * a, 2.1f);
}
static void teDica(float *x, float y, const char *k, const char *l, float a) {
  TxtLinha tk = txt_linha(TXT_AJ_KBD, k, 243, 242, 239, 255), tl = txt_linha(TXT_ILHA_APOIO, l, 243, 242, 239, 255);
  float kw = tk.w + 18.0f < 34.0f ? 34.0f : tk.w + 18.0f;
  teNeutro((GfxRect){ *x, y, kw, 30 }, 15, 0.09f, 0.141f, 0.149f, 0.173f, a);
  txt_desenhar_alpha(tk, *x + (kw - tk.w) * 0.5f, y + (30 - tk.h) * 0.5f, 0.82f * a);
  txt_desenhar_alpha(tl, *x + kw + 9, y + (30 - tl.h) * 0.5f, 0.45f * a);
  *x += kw + 9 + tl.w + 20;
}

static void teDesenhar(Uint32 agora);
// O teclado fica em 1080p em qualquer "Tamanho da interface": em 100% ele ja
// ocupa a largura da tela (ver a conta no alto), e ampliado nao caberia.
void teclado_desenhar(Uint32 agora) {
  ESCALA_REAL_INI();
  teDesenhar(agora);
  ESCALA_REAL_FIM();
}
static void teDesenhar(Uint32 agora) {
  float a = anim_suave(anim), dy, x, y, ew;
  int f, c, i;
  if (anim < 0.01f) return;
  dy = (1.0f - a) * 36.0f;
  if (aberto && ponteiro_ativo()) {
    ponteiro_camada();
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, NULL, 0, 0);
  }
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0, 0, 0, (ajustes_vidro() ? 0.40f : 0.42f) * anim);
  { GfxRect p = { teX(), teY() + dy, teW(), teH() };
    float raio = 36.0f / p.h;
    gfx_rect((GfxRect){ p.x - 20, p.y - 6, p.w + 40, p.h + 46 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f, 0, 0, 0, 0.40f * a);
    if (ajustes_vidro()) {
      gfx_cor(p, raio, 0.055f, 0.059f, 0.071f, 0.86f * a);
      gfx_luz_canto(p, raio, p.w * 0.22f, -p.h * 0.40f, p.h * 0.62f, 1, 1, 1, 0.10f * a);
    } else gfx_cor(p, raio, 0.082f, 0.086f, 0.102f, a); }

  // COLUNA DA ESQUERDA
  x = teX() + TE_ILHA_PX; ew = teEsqW();
  y = teY() + dy + TE_ILHA_PY;
  if (kickerAtual[0]) { teCaps(kickerAtual, x, y + 2, a); y += 22.0f; }
  { TxtLinha t = txt_linha_corta(TXT_ILHA_TITULO, tituloAtual, 243, 242, 239, 255, ew);
    txt_desenhar_alpha(t, x, y, a); y += 48.0f; }
  if (dicaAtual[0]) {
    y += 10.0f;
    y += txt_bloco(TXT_AJ_SUB, dicaAtual, 243, 242, 239, x, y, ew, 28.5f, 0.58f * a, 3);
  }
  y += 30.0f;
  { float ar, ag, ab;
    ajustes_acento(&ar, &ag, &ab);
    // O QUE FOI DIGITADO: caixas quando cada uma cabe o glifo (codigos
    // curtos), senao o campo de texto com cursor.
    if (!mascarar && (ew - (float)(maxN - 1) * TE_CGAP) / (float)maxN >= TE_CX_MIN) {
      float bw = (ew - (float)(maxN - 1) * TE_CGAP) / (float)maxN, bx = x;
      if (bw > TE_CX) bw = TE_CX;
      for (i = 0; i < maxN; i++) {
        GfxRect b = { bx, y, bw, TE_CY };
        teNeutro(b, 20, i < n ? 0.16f : 0.05f, i < n ? 0.20f : 0.12f, i < n ? 0.205f : 0.125f, i < n ? 0.23f : 0.145f, a);
        if (i < n) {
          char ch[2] = { texto[i], 0 };
          TxtLinha t = txt_linha(TXT_TITULO2, ch, 243, 242, 239, 255);
          txt_desenhar_alpha(t, b.x + (b.w - t.w) * 0.5f, b.y + (b.h - t.h) * 0.5f, a);
        }
        bx += bw + TE_CGAP;
      }
      y += TE_CY;
    } else {
      GfxRect campo = { x, y, ew, 76.0f };
      float tx = campo.x + 24.0f, cursorX = tx, larg;
      char q[48];
      TxtLinha lq;
      teNeutro(campo, 24, 0.07f, 0.125f, 0.129f, 0.153f, a);
      if (animBarra[0] > 0.01f) teNeutro(campo, 24, 0.07f * animBarra[0], 0.17f, 0.176f, 0.204f, animBarra[0] * a);
      if (st_ime_disponivel() && ponteiro_ativo()) ponteiro_alvo(campo.x, campo.y, campo.w, campo.h, focarBarra, NULL, 0, 0);
      snprintf(q, sizeof q, n == 1 ? i18n("%d caractere") : i18n("%d caracteres"), n);
      lq = txt_linha(TXT_ILHA_APOIO, q, 243, 242, 239, 255);
      txt_desenhar_alpha(lq, campo.x + campo.w - 24 - lq.w, campo.y + (campo.h - lq.h) * 0.5f, 0.4f * a);
      larg = campo.w - 48.0f - lq.w - 20.0f;
      if (n) {
        char pontos[TECLADO_LONGO * 3 + 1];
        TxtLinha t;
        float ox;
        if (mascarar) {
          int i2;
          for (i2 = 0; i2 < n && i2 < TECLADO_LONGO; i2++) memcpy(pontos + i2 * 3, "\xE2\x80\xA2", 3);
          pontos[i2 * 3] = 0;
        }
        t = txt_linha(TXT_AJ_INSP, mascarar ? pontos : texto, 243, 242, 239, 255);
        ox = t.w > larg ? t.w - larg : 0.0f;
        gfx_recorte(tx, campo.y, larg, campo.h);
        txt_desenhar_alpha(t, tx - ox, campo.y + (campo.h - t.h) * 0.5f, a);
        gfx_sem_recorte();
        cursorX = tx + (t.w - ox);
      }
      { float op = ajustes_animacoes_reduzidas() ? 0.95f : (((agora / 500) % 2) ? 0.35f : 0.95f);
        gfx_cor((GfxRect){ cursorX + 3.0f, campo.y + 20.0f, 2.0f, 36.0f }, 0.5f, ar, ag, ab, op * a); }
      y += 76.0f;
    } }
  // MODOS: teclado da TV, falar e celular, num segmentado (o que existir).
  { const char *rot[3]; int col[3], k = 0;
    if (st_ime_disponivel()) { rot[k] = "Teclado da TV"; col[k++] = 0; }
    if (st_voz_disponivel()) { rot[k] = "Falar"; col[k++] = 1; }
    if (celOk()) { rot[k] = "Digitar pelo celular"; col[k++] = 2; }
    if (k) {
      float sx = x, sw = 10.0f, h = 55.0f, ih = 45.0f;
      y += 22.0f;
      for (i = 0; i < k; i++) sw += txt_linha(TXT_AJ_SEG, rot[i], 0, 0, 0, 255).w + 40.0f + (i ? 4.0f : 0.0f);
      teNeutro((GfxRect){ sx, y, sw, h }, h * 0.5f, 0.06f, 0.114f, 0.118f, 0.137f, a);
      sx += 5.0f;
      for (i = 0; i < k; i++) {
        int foco = aberto && fileira < 0 && coluna == col[i];
        int ti = foco ? ajustes_tinta_foco() : 243;
        TxtLinha t = txt_linha(TXT_AJ_SEG, rot[i], ti, ti, ti, 255);
        GfxRect r = { sx, y + 5.0f, t.w + 40.0f, ih };
        if (aberto && ponteiro_ativo()) ponteiro_alvo(r.x, r.y, r.w, r.h, focarBarra, NULL, col[i], 0);
        if (foco) teAcento(r, ih * 0.5f, animBarra[col[i]] > 0.5f ? 1.0f : animBarra[col[i]] * 2.0f, a);
        else if (col[i] == 2 && celRecebido) teNeutro(r, ih * 0.5f, 0.14f, 0.204f, 0.212f, 0.243f, a);
        txt_desenhar_alpha(t, r.x + 20, r.y + (ih - t.h) * 0.5f, (foco ? 1.0f : 0.55f) * a);
        sx += r.w + 4.0f;
      }
      y += h;
      if (celOk()) {
        y += 16.0f;
        txt_bloco(TXT_ILHA_GENERO, "Pelo celular, o texto chega aqui para você conferir antes de concluir.",
                  243, 242, 239, x, y, ew, 25.0f, 0.45f * a, 2);
      }
    } }
  // Dicas na base da coluna.
  { const char *av = st_dono() == ST_TECLADO ? st_aviso() : "";
    float dx = x, by = teY() + dy + teH() - TE_ILHA_PY - 30.0f;
    if (av[0]) {
      TxtLinha t = txt_linha_corta(TXT_ILHA_GENERO, av, 240, 196, 140, 255, ew);
      txt_desenhar_alpha(t, x, by + (30 - t.h) * 0.5f, a);
    } else if (celRecebido && fileira == nFileiras - 1) {
      TxtLinha t = txt_linha_corta(TXT_ILHA_GENERO, "Recebido do celular. Confira e aperte Concluir.", 243, 242, 239, 255, ew);
      txt_desenhar_alpha(t, x, by + (30 - t.h) * 0.5f, 0.62f * a);
    } else if (fileira < 0) {
      teDica(&dx, by, "OK", coluna == 2 ? "Digitar pelo celular" : coluna == 1 ? "Falar" : "Teclado da TV", a);
      teDica(&dx, by, "↓", "Teclado", a);
      teDica(&dx, by, "Voltar", "Cancelar", a);
    } else {
      teDica(&dx, by, "Setas", "Navegar", a);
      teDica(&dx, by, "OK", "Digitar", a);
      teDica(&dx, by, "Voltar", "Cancelar", a);
    } }

  // COLUNA DA DIREITA: a grade.
  for (f = 0; f < nFileiras; f++) {
    for (c = 0; c < colunasDe(f); c++) {
      float k = focoAnim[f][c];
      GfxRect base = retangulo(f, c);
      int extra = f == nFileiras - 1, concluir = extra && c == colunasDe(f) - 1;
      float esc = 1.0f + (extra ? 0.0f : TE_ESCALA * k);
      GfxRect t;
      const char *s;
      char ch[2];
      int tom;
      base.y += dy;
      if (aberto && ponteiro_ativo()) ponteiro_alvo(base.x, base.y, base.w, base.h, focarTecla, NULL, f, c);
      t.w = base.w * esc; t.h = base.h * esc;
      t.x = base.x - (t.w - base.w) * 0.5f;
      t.y = base.y - (t.h - base.h) * 0.5f;
      if (extra) teNeutro(t, t.h * 0.5f, 0.08f, 0.141f, 0.149f, 0.173f, a);
      else teNeutro(t, 20.0f * esc, 0.07f, 0.125f, 0.129f, 0.153f, a);
      if (k > 0.01f) teAcento(t, extra ? t.h * 0.5f : 20.0f * esc, k, a);
      if (ehAtalho(f)) s = ATALHOS_EMAIL[c];
      else if (ehChar(f)) {
        ch[0] = alfa()[f * nCols + c]; ch[1] = 0;
        s = ch;
        if (ch[0] == ' ') s = i18n("espaço");
      } else s = rotuloExtra(c);
      // A cor do texto em DEGRAU (a chave do cache de linhas de text.c).
      tom = k >= 0.5f ? ajustes_tinta_foco() : 243;
      { TxtLinha l = txt_linha(extra ? TXT_AJ_SEG : (!ehChar(f) || s != ch ? TXT_AJ_ESTADO : TXT_AJ_TIT28),
                               s, tom, tom, tom, 255);
        const char *ic = extra ? (c == 0 ? "aj_delete" : concluir ? "aj_check" : NULL) : NULL;
        float iw = ic ? 22.0f + 10.0f : 0.0f, lx = t.x + (t.w - l.w - iw) * 0.5f;
        if (ic) gfx_icone((GfxRect){ lx, t.y + (t.h - 22) * 0.5f, 22, 22 }, ic, tom / 255.0f, tom / 255.0f, tom / 255.0f, (k >= 0.5f ? 1.0f : 0.85f) * a);
        txt_desenhar_alpha(l, lx + iw, t.y + (t.h - l.h) * 0.5f, (k >= 0.5f ? 1.0f : extra ? 0.85f : 0.9f) * a); }
    }
  }
}
