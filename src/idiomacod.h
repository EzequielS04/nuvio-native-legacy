// Codigos dos idiomas da interface e os pedacos de texto que dependem so do
// idioma (mes abreviado, caixa alta em UTF-8). Sem dependencia de ajustes.h:
// cwordem.c e noticias.c sao compilados isolados por testes e nao podem puxar
// a biblioteca grafica so por causa de um numero.
//
// O NUMERO E O QUE FICA GRAVADO em ajustes.txt (AJ_IDIOMA): 0 e 1 sao os dois
// idiomas de sempre, e ficaram onde estavam para nao trocar o idioma de quem ja
// escolheu. Novos entram no FIM; nunca reordenar.
#ifndef NV_IDIOMACOD_H
#define NV_IDIOMACOD_H

#include <stddef.h>

enum { IDIOMA_PT = 0, IDIOMA_EN = 1, IDIOMA_RO = 2, IDIOMA_UK = 3, IDIOMA_RU = 4,
       IDIOMA_FR = 5, IDIOMA_DE = 6, IDIOMA_ES = 7, IDIOMA_N = 8 };

// Codigo ISO 639-1 do idioma da interface ("pt", "en", "ro", "uk", "ru", "fr",
// "de", "es"). E o mesmo que o Trakt poe em `language` de um comentario, e o
// prefixo de "pt-BR"/"ru-RU" que o TMDB recebe — por isso mora aqui, ao lado
// dos codigos, e nao em cada consumidor.
static inline const char *idioma_iso(int idioma) {
  switch (idioma) {
    case IDIOMA_EN: return "en";
    case IDIOMA_RO: return "ro";
    case IDIOMA_UK: return "uk";
    case IDIOMA_RU: return "ru";
    case IDIOMA_FR: return "fr";
    case IDIOMA_DE: return "de";
    case IDIOMA_ES: return "es";
    default:        return "pt";
  }
}

// Mes abreviado, minusculo, m0 = 0..11. Em ingles a capitalizacao e a do
// idioma ("Sep"); nos demais e minuscula ("set", "sep", "вер"), como no uso
// corrente de cada lingua.
static inline const char *idioma_mes_curto(int idioma, int m0) {
  static const char *EN[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
  static const char *PT[] = { "jan","fev","mar","abr","mai","jun","jul","ago","set","out","nov","dez" };
  static const char *RO[] = { "ian","feb","mar","apr","mai","iun","iul","aug","sep","oct","nov","dec" };
  static const char *UK[] = { "січ","лют","бер","кві","тра","чер","лип","сер","вер","жов","лис","гру" };
  static const char *RU[] = { "янв","фев","мар","апр","мая","июн","июл","авг","сен","окт","ноя","дек" };
  // fr e de: o ponto faz parte da abreviacao ("21 janv.", "21. Jan." fica sem o
  // ponto do dia porque a data curta e "dia mes"); mes que ja e curto nao leva.
  static const char *FR[] = { "janv.","févr.","mars","avr.","mai","juin","juil.","août","sept.","oct.","nov.","déc." };
  static const char *DE[] = { "Jan.","Feb.","März","Apr.","Mai","Juni","Juli","Aug.","Sept.","Okt.","Nov.","Dez." };
  static const char *ES[] = { "ene","feb","mar","abr","may","jun","jul","ago","sep","oct","nov","dic" };
  if (m0 < 0 || m0 > 11) return "";
  switch (idioma) {
    case IDIOMA_EN: return EN[m0];
    case IDIOMA_RO: return RO[m0];
    case IDIOMA_UK: return UK[m0];
    case IDIOMA_RU: return RU[m0];
    case IDIOMA_FR: return FR[m0];
    case IDIOMA_DE: return DE[m0];
    case IDIOMA_ES: return ES[m0];
    default:        return PT[m0];
  }
}

// Caixa alta em UTF-8 para as escritas que a interface usa: ASCII, Latin-1
// (acentos do portugues, do frances, do alemao e do espanhol; o "ß" fica como
// esta, porque "SS" mudaria o tamanho), o "œ" do frances, o romeno (a com breve, s e t com virgula embaixo e as
// variantes com cedilha que teclados antigos ainda geram) e o cirilico do
// russo e do ucraniano. toupper() byte a byte trocaria o primeiro byte de
// "ação" ou "мая" por lixo. TODA a conversao preserva o tamanho em bytes, e e
// isso que permite mexer no buffer sem medir antes. Devolve o tamanho escrito.
static inline size_t idioma_maiusc(char *dst, size_t tam, const char *s) {
  size_t i = 0, o = 0;
  if (!tam) return 0;
  dst[0] = 0;
  if (!s) return 0;
  while (s[i] && o + 2 < tam) {
    unsigned char c = (unsigned char)s[i], d = (unsigned char)s[i + 1];
    unsigned char a = c, b = d;
    int n = 1;
    if (c < 0x80) { if (c >= 'a' && c <= 'z') a = (unsigned char)(c - 32); }
    else if (d) {
      n = 2;
      if (c == 0xC3 && d >= 0xA0 && d <= 0xBE && d != 0xB7) b = (unsigned char)(d - 0x20);
      else if (c == 0xC4 && d == 0x83) b = 0x82;                     /* ă */
      else if (c == 0xC5 && d == 0x93) b = 0x92;                     /* œ */
      else if (c == 0xC5 && (d == 0x9F || d == 0xA3)) b = (unsigned char)(d - 1); /* ş ţ */
      else if (c == 0xC8 && (d == 0x99 || d == 0x9B)) b = (unsigned char)(d - 1); /* ș ț */
      else if (c == 0xD0 && d >= 0xB0 && d <= 0xBF) b = (unsigned char)(d - 0x20);
      else if (c == 0xD1 && d >= 0x80 && d <= 0x8F) { a = 0xD0; b = (unsigned char)(d + 0x20); }
      else if (c == 0xD1 && (d == 0x91 || d == 0x94 || d == 0x96 || d == 0x97)) {
        a = 0xD0; b = (unsigned char)(d - 0x10); }                    /* ё є і ї */
      else if (c == 0xD2 && d == 0x91) b = 0x90;                     /* ґ */
    }
    dst[o++] = (char)a;
    if (n == 2) dst[o++] = (char)b;
    i += (size_t)n;
    /* bytes 3 e 4 de uma sequencia longa passam crus no laco seguinte: nenhum
       deles cai nas faixas acima, porque sao bytes de continuacao (0x80-0xBF). */
  }
  dst[o] = 0;
  return o;
}

#endif
