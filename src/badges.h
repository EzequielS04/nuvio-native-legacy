#ifndef NV_BADGES_H
#define NV_BADGES_H
#include <stdint.h>
#include "text.h"
void badges_carregar(const char *dir);
uint64_t badges_detectar(const char *metadata);
uint64_t badges_provedor(const char *name);

// --- MARCA DE FORMATO (29/09/2026) -------------------------------------------
// HDR, Dolby Vision, Atmos, DTS, 4K...: onde o app cita um formato de video ou
// de audio como ROTULO (selo, linha de meta, titulo de opcao), desenha a marca
// e nao a palavra. Em frase corrida de ajuda o texto continua texto.
typedef enum {
  FMT_4K = 0, FMT_1080, FMT_720, FMT_SDR, FMT_HDR, FMT_HDR10, FMT_HDR10P, FMT_HLG,
  FMT_DV, FMT_ATMOS, FMT_DTS, FMT_DTSX, FMT_DTSHD, FMT_TRUEHD, FMT_DD, FMT_DDP,
  FMT_IMAX, FMT_IMAX_ENH, FMT_AV1, FMT_HEVC, FMT_AVC, FMT_REMUX, FMT_N
} FormatoMarca;
// Desenha a marca com o TOPO em y e `altura` de caixa (a arte fica centrada na
// caixa; centre a caixa na linha do texto vizinho: y = centroDoTexto - altura/2).
// Tinge com (r,g,b) 0..1 e alfa `a`; sem a arte, escreve o nome. Devolve a
// largura ocupada.
float marca_formato(FormatoMarca f, float x, float y, float altura,
                    float r, float g, float b, float a);
float marca_formato_largura(FormatoMarca f, float altura);
const char *marca_formato_nome(FormatoMarca f);
// Rotulo cuja palavra de formato vira a marca: "Sem HDR" -> "Sem" [HDR].
// `rotulo` e a chave em portugues (traduzida aqui); `y` e o TOPO da linha de
// texto e a marca fica centrada nela. Devolve a largura desenhada.
float marca_rotulo(TxtEstilo est, const char *rotulo, FormatoMarca f, float x, float y,
                   float altura, int tinta, float a);
float marca_rotulo_largura(TxtEstilo est, const char *rotulo, FormatoMarca f, float altura);
// Marca do HDR que a FONTE anuncia (nao o que o painel ativou), ou -1.
int badges_fonte_hdr_marca(uint64_t mask);
float badges_desenhar(uint64_t mask,float x,float y,float maxW,float height,float alpha);
// A MESMA fileira em tinta ESCURA, para desenhar SOBRE superficie clara
// (linha selecionada). Ver a nota em badges.c.
float badges_desenhar_escura(uint64_t mask,float x,float y,float maxW,float height,float alpha);

// --- SELOS DE TEXTO (a TABELA UNICA de badges, 21/09/2026) -------------------
//
// "Novo", "Up next", "1h 46min restantes", "14", "Na biblioteca", "Filme":
// cada um era desenhado com numeros proprios (34, 32, 30 e 28 px de altura,
// raios 0,22/0,18/0,5, fundos 0.13 a 0.16). Um selo e sempre a MESMA coisa:
// PILULA de BADGE_H, TXT_CAPTION2 (21/400), folga BADGE_PADX. So a COR muda,
// e muda por ESTILO, nunca por valor cravado no chamador:
//
//   BADGE_NEUTRO        fundo 0.10/0.11/0.13 a 85 %, texto 235 — meta comum
//   BADGE_APAGADO       o mesmo fundo, texto 170 — estado ausente/negativo
//                       ("Fora da biblioteca", "Nao assistido")
//   BADGE_REALCE        realce a 18 % com texto 235 — estado positivo
//                       ("Na biblioteca", "Assistido")
//   BADGE_REALCE_CHEIO  realce cheio com a tinta de ajustes_tinta_foco —
//                       destaque de verdade ("Novo")
//   BADGE_SOBRE_REALCE  para selo DENTRO de uma linha em foco (superficie na
//                       cor de realce): tinta a 12 % e texto ajustes_tinta_foco2
//
// O IMDb e MARCA e nao selo: amarelo #F5C518 com "IMDb" preto em TXT_MINI, e
// a nota em texto logo depois. badge_imdb devolve a largura do par.
typedef enum {
  BADGE_NEUTRO = 0, BADGE_APAGADO, BADGE_REALCE, BADGE_REALCE_CHEIO,
  BADGE_SOBRE_REALCE
} BadgeEstilo;
#define BADGE_H      28.0f
#define BADGE_PADX   12.0f
#define BADGE_GAP    10.0f    // entre selos lado a lado
#define BADGE_IMDB_W 50.0f    // a marca amarela; a nota vem depois
#define BADGE_IMDB_R 5.0f
#define BADGE_IMDB_GAP 8.0f
// Largura que badge_desenhar vai ocupar (mesma conta), para ancorar pela
// direita ou cortar o texto ao lado.
float badge_largura(const char *texto);
// Desenha o selo com o canto superior esquerdo em (x, y) e devolve a largura.
float badge_desenhar(float x, float y, const char *texto, BadgeEstilo estilo, float a);
// Marca do IMDb + nota (centesimos, "8,4" / "8.4" pelo idioma). `escuro` = a
// nota em tinta escura (sobre superficie clara). 0 de largura com nota <= 0.
float badge_imdb(float x, float y, int nota, int escuro, float a);
float badge_imdb_largura(int nota);
#endif
