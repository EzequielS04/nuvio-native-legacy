// Ver dobra.h.
#include "dobra.h"

// Versalete do IPA e do bloco fonetico, na ordem do alfabeto. O Unicode nao
// tem os 26 num bloco so: vem de U+1D00 (fonetico), U+02xx (IPA) e U+A7xx
// (latim estendido-D), e ainda falta um de verdade (X, que ninguem usa).
static const struct { unsigned short cp; char c; } VERSALETE[] = {
  { 0x1D00, 'A' }, { 0x0299, 'B' }, { 0x1D04, 'C' }, { 0x1D05, 'D' },
  { 0x1D07, 'E' }, { 0xA730, 'F' }, { 0x0262, 'G' }, { 0x029C, 'H' },
  { 0x026A, 'I' }, { 0x1D0A, 'J' }, { 0x1D0B, 'K' }, { 0x029F, 'L' },
  { 0x1D0D, 'M' }, { 0x0274, 'N' }, { 0x1D0F, 'O' }, { 0x1D18, 'P' },
  { 0x01EB, 'Q' }, { 0x0280, 'R' }, { 0xA731, 'S' }, { 0x1D1B, 'T' },
  { 0x1D1C, 'U' }, { 0x1D20, 'V' }, { 0x1D21, 'W' }, { 0x028F, 'Y' },
  { 0x1D22, 'Z' },
};

char nv_dobra_estilizada(unsigned long cp) {
  unsigned i;
  if (cp < 0x80) return 0;
  for (i = 0; i < sizeof VERSALETE / sizeof *VERSALETE; i++)
    if (VERSALETE[i].cp == cp) return VERSALETE[i].c;
  // Sobrescrito e subscrito: ⁰ ⁴-⁹ e ₀-₉ (¹ ² ³ sao do Latin-1, que a fonte tem).
  if (cp == 0x2070) return '0';
  if (cp >= 0x2074 && cp <= 0x2079) return (char)('4' + (cp - 0x2074));
  if (cp >= 0x2080 && cp <= 0x2089) return (char)('0' + (cp - 0x2080));
  // Circuladas: Ⓐ-Ⓩ, ⓐ-ⓩ, ①-⑨.
  if (cp >= 0x24B6 && cp <= 0x24CF) return (char)('A' + (cp - 0x24B6));
  if (cp >= 0x24D0 && cp <= 0x24E9) return (char)('a' + (cp - 0x24D0));
  if (cp >= 0x2460 && cp <= 0x2468) return (char)('1' + (cp - 0x2460));
  // Largura cheia: ！ a ～ espelham o ASCII de 0x21 a 0x7E.
  if (cp >= 0xFF01 && cp <= 0xFF5E) return (char)(cp - 0xFEE0);
  // Alfanumericos matematicos: treze estilos de 52 letras (A-Z, a-z) a partir
  // de U+1D400, e cinco de 10 digitos a partir de U+1D7CE. Os "buracos" do
  // bloco (letras que ja existiam em outro lugar, como o ℎ) nao sao atribuidos
  // e nao aparecem em texto real; cair na letra certa ali e inofensivo.
  if (cp >= 0x1D400 && cp <= 0x1D6A3) {
    unsigned off = (unsigned)((cp - 0x1D400) % 52);
    return off < 26 ? (char)('A' + off) : (char)('a' + off - 26);
  }
  if (cp >= 0x1D7CE && cp <= 0x1D7FF) return (char)('0' + (cp - 0x1D7CE) % 10);
  return 0;
}
