// Cartao de ATUALIZACAO — "saiu a 1.0.54, e isto e o que mudou".
//
// POR QUE EXISTE: o app se instala a mao (.ipk pelo Homebrew Channel, .wgt
// assinado pelo proprio usuario) e nao ha loja que avise. Quem instalou a
// 1.0.50 continua nela ate ler o GitHub por conta propria — e os relatos de
// defeito ja corrigido ("still doing the same thing") sao em parte isso.
//
// DE ONDE VEM: a release mais recente do repositorio, pela API publica do
// GitHub (60 consultas por hora por IP sem token — uma por abertura do app
// nao chega perto). `tag_name` diz a versao, `body` sao as notas em Markdown.
// As notas sao mostradas como texto: titulos "##" viram linhas de secao,
// "- **x**" vira "• x", o resto da marcacao cai. A secao "## Notes" (como
// instalar) e o que vem depois dela nao entram: e boilerplate de toda release.
//
// UMA VEZ POR VERSAO: o arquivo-marca guarda a tag mostrada. Nova release,
// nova tag, novo cartao; a mesma nao volta.
//
// O irmao e novidades.c: mesmo cartao central, mesma regra de fechamento.
#include "atualizacao.h"
#include "dados.h"
#include "rede.h"
#include "gfx.h"
#include "botoes.h"
#include "text.h"
#include "anim.h"
#include "layout.h"
#include "idioma.h"
#include "ajustes.h"
#ifdef NV_ANDROID
#include "android.h"
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NV_VERSAO
#define NV_VERSAO "dev"
#endif

// ===================== AUTO-ATUALIZACAO DO .tpk (SO NV_TPK) =====================
// Numa TV Samsung o Nuvio nativo E a libnuvio.so (UI, catalogo, player, sync); o
// host .NET quase nunca muda. Este bloco deixa o app se atualizar SEM o usuario
// reinstalar o .tpk: quando a release anexa uma libnuvio.so mais nova para esta
// ABI, baixamos por HTTPS, CONFERIMOS o sha256 e so entao ENCENAMOS o arquivo em
// data/ (staging). O host, no proximo arranque, memfd-carrega a .so encenada em
// vez da empacotada (Program.cs / Program40.cs).
//
// SEGURANCA: isto carrega CODIGO NATIVO REMOTO. A unica barreira e o sha256, e
// ele vem do MESMO lugar da checagem de versao — o campo `digest` do anexo no
// JSON da release do GitHub, por HTTPS (AT_URL). E o mesmo campo ja usado para o
// ipkHash da LG. Nunca carregamos em processo aqui: so encenamos; quem carrega
// e o host, no proximo arranque, e so depois de reconferir o hash do arquivo
// encenado. Em QUALQUER duvida o host apaga o staging e volta para a empacotada.
// No Android o mesmo SHA-256 confere o APK baixado (auto-atualizacao do .apk).
#if defined(NV_TPK) || defined(NV_ANDROID)
#include <stdint.h>
#include <unistd.h>
#include <strings.h>

// SHA-256 compacto (FIPS 180-4). So para conferir o anexo baixado; nao ha
// sha256 alcancavel sob NV_TPK (a libcrypto do aparelho nao e garantida).
typedef struct { uint32_t h[8]; uint64_t n; unsigned char b[64]; size_t k; } At_Sha;
static void at_sha_bloco(At_Sha *s, const unsigned char *p) {
  static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };
  uint32_t w[64], a,b,c,d,e,f,g,h; int i;
  for (i = 0; i < 16; i++)
    w[i] = (uint32_t)p[i*4]<<24 | (uint32_t)p[i*4+1]<<16 | (uint32_t)p[i*4+2]<<8 | p[i*4+3];
  for (i = 16; i < 64; i++) {
    uint32_t s0 = (w[i-15]>>7|w[i-15]<<25) ^ (w[i-15]>>18|w[i-15]<<14) ^ (w[i-15]>>3);
    uint32_t s1 = (w[i-2]>>17|w[i-2]<<15) ^ (w[i-2]>>19|w[i-2]<<13) ^ (w[i-2]>>10);
    w[i] = w[i-16] + s0 + w[i-7] + s1;
  }
  a=s->h[0];b=s->h[1];c=s->h[2];d=s->h[3];e=s->h[4];f=s->h[5];g=s->h[6];h=s->h[7];
  for (i = 0; i < 64; i++) {
    uint32_t S1 = (e>>6|e<<26) ^ (e>>11|e<<21) ^ (e>>25|e<<7);
    uint32_t ch = (e&f) ^ (~e&g);
    uint32_t t1 = h + S1 + ch + K[i] + w[i];
    uint32_t S0 = (a>>2|a<<30) ^ (a>>13|a<<19) ^ (a>>22|a<<10);
    uint32_t maj = (a&b) ^ (a&c) ^ (b&c);
    uint32_t t2 = S0 + maj;
    h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
  }
  s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;s->h[4]+=e;s->h[5]+=f;s->h[6]+=g;s->h[7]+=h;
}
static void at_sha_init(At_Sha *s) {
  s->h[0]=0x6a09e667;s->h[1]=0xbb67ae85;s->h[2]=0x3c6ef372;s->h[3]=0xa54ff53a;
  s->h[4]=0x510e527f;s->h[5]=0x9b05688c;s->h[6]=0x1f83d9ab;s->h[7]=0x5be0cd19;
  s->n=0;s->k=0;
}
static void at_sha_up(At_Sha *s, const unsigned char *p, size_t n) {
  s->n += (uint64_t)n * 8;
  while (n) {
    size_t take = 64 - s->k; if (take > n) take = n;
    memcpy(s->b + s->k, p, take); s->k += take; p += take; n -= take;
    if (s->k == 64) { at_sha_bloco(s, s->b); s->k = 0; }
  }
}
static void at_sha_fim(At_Sha *s, char *hex65) {
  unsigned char len[8]; int i;
  uint64_t bits = s->n;
  unsigned char um = 0x80;
  for (i = 0; i < 8; i++) len[7-i] = (unsigned char)(bits >> (i*8));
  at_sha_up(s, &um, 1);
  while (s->k != 56) { unsigned char z = 0; at_sha_up(s, &z, 1); }
  at_sha_up(s, len, 8);
  for (i = 0; i < 8; i++)
    snprintf(hex65 + i*8, 9, "%08x", s->h[i]);
}
// sha256 hex de um buffer inteiro em memoria -> hex65 (64 chars + NUL).
static void at_sha256_hex(const unsigned char *buf, size_t n, char *hex65) {
  At_Sha s; at_sha_init(&s); at_sha_up(&s, buf, n); at_sha_fim(&s, hex65);
}
#endif

#define AT_URL   "https://api.github.com/repos/iqui27/nuvio-native-legacy/releases/latest"
#define AT_ARQ   "atualizacao-vista.txt"
#define AT_PAGINA "https://github.com/iqui27/nuvio-native-legacy/releases"
#define AT_APPID  "space.nuvio.native.legacy"
#define AT_LUNA_PUB "/usr/bin/luna-send-pub"
#define AT_LOG_INST "/tmp/nuvio-instalar.log"
#define AT_HB_DIR   "/media/developer/apps/usr/palm/applications/org.webosbrew.hbchannel"

#define AT_W        1240.0f
#define AT_H         760.0f
#define AT_X        ((NV_TELA_W - AT_W) * 0.5f)
#define AT_Y        ((NV_TELA_H - AT_H) * 0.5f)
#define AT_PAD        64.0f
#define AT_ABRIR_MS   280.0f
#define AT_FECHAR_MS  160.0f
// Linhas de NOTAS guardadas (secao, paragrafo ou item). Era 14, e com as notas
// sem rolagem isso nao importava: o cartao ja cortava antes. Agora a area das
// notas rola, entao o teto so protege o buffer.
#define AT_LINHAS_MAX  48
#define AT_LEADING    34.0f
// RODAPE FIXO: os botoes (ou o endereco da pagina) moram nos ultimos
// AT_RODAPE_H px do cartao e as notas NUNCA descem ate la. Ver atualizacao_desenhar.
#define AT_RODAPE_H  132.0f
// Um aperto de cima/baixo rola tres linhas de notas.
#define AT_PASSO     (AT_LEADING * 3.0f)
// Linhas por item. Com rolagem nao ha por que cortar um item no meio; o teto
// so impede que um paragrafo gigante vire uma parede.
#define AT_ITEM_LINHAS 8

static SDL_mutex *mtx;
static SDL_Thread *fio;
static int disparado, pronto, aberto, mostrado;
static float entrada;
static char tagNova[32];          // "1.0.54", vazio se nao ha nada mais novo
static char notas[6144];          // texto ja limpo, linhas separadas por \n
// URL do .ipk da release. Vazia quando a release nao anexou um (ou quando este
// alvo nao sabe instalar, e ai nem se procura).
static char ipkUrl[512];
static char ipkHash[80];          // sha256 em hex; vazio quando a release nao diz
#ifdef NV_TPK
// Anexo libnuvio.so desta ABI (auto-atualizacao do .tpk). soUrl/soHash vem do
// JSON da release; soVer e a tag (== tagNova) que essa .so entrega.
static char soUrl[512];
static char soHash[80];
static char soVer[32];
#endif
#ifdef NV_ANDROID
// Anexo "Nuvio-<versao>-android.apk" da release (auto-atualizacao no Android).
// apkPerm: o sistema pediu a permissao de instalar apps desta fonte; o cartao
// avisa e deixa tentar de novo.
static char apkUrl[512];
static char apkHash[80];
static char apkVer[32];
static int  apkPerm;
#endif

// INSTALAR DE DENTRO DO APP so existe no webOS, e a razao e de plataforma:
// aqui o app roda como ROOT (webosbrew) e alcanca o luna-send, que e quem fala
// com o appInstallService. No Tizen o .wgt vive num runtime de navegador
// isolado, sem API para instalar widget — la o cartao continua sendo so o
// aviso. No Mac nao ha o que instalar.
//
// A CAPTURA PRECISA DO CASO DA LG RODANDO NO MAC. Este cartao so existe quando
// ha versao nova, e o ramo com botoes e barra so existe onde ha instalador —
// ou seja, fotografa-lo de verdade exigiria segurar uma release, uma TV e o
// Homebrew Channel ao mesmo tempo. NV_AT_INSTALA e o unico jeito de o harness
// alcancar esse ramo; ele NAO e definido por nenhum build de produto (ver
// tools/env.sh e tools/arm.sh), so por tests/atualizacao_shot.sh.
#if defined(NV_AT_INSTALA)
#define AT_INSTALA NV_AT_INSTALA
#elif !defined(__EMSCRIPTEN__) && !defined(__APPLE__) && !defined(NV_TPK) && !defined(NV_ANDROID)
#define AT_INSTALA 1
#else
#define AT_INSTALA 0
#endif

enum { AT_PARADO = 0, AT_BAIXANDO, AT_INSTALANDO, AT_PRONTO, AT_FALHOU };
// Quanto o instalador ja andou (0..100) e em que passo ele esta. Os dois saem
// do log do proprio servico, que publica `progress` e `statusText` a cada
// volta — sem isso a tela ficaria com uma frase parada por dois minutos, que e
// exatamente o tempo em que a pessoa acha que travou.
static float instPct;
static char  instPasso[48];
static int estado;
// .so nova ENCENADA nesta sessao (so NV_TPK). O host escolhe a lib no arranque
// de um processo NOVO, e na Samsung "sair" nem sempre acaba o processo: a TV o
// guarda e a reabertura retoma o antigo (#184, S90D Tizen 9: cinco arranques
// seguidos na 1.5.4 com a 1.6.0 ja encenada). Por isso o cartao oferece
// "Reiniciar agora", que encerra o app de verdade (SDL_QUIT -> fim do main ->
// o host sai e a vigia dele forca o _exit).
static int soEncenada;
static int foco;                  // 0 = "Atualizar agora", 1 = "Depois"
// ROLAGEM DAS NOTAS. Relato de mackojanko (Samsung Tizen 6.0, 1.4.5): "quando
// o aviso aparece nao consigo descer e nao vejo o botao de atualizar". As
// notas da 1.4.4 e da 1.4.5 sao longas, e o laco de desenho so parava de COMECAR itens perto do
// rodape — o ultimo item, com ate 3 linhas, descia por cima dos botoes (e, no
// Tizen, por cima do endereco e do "OK para fechar"), e cima/baixo nao faziam
// nada. Agora o rodape e fixo, as notas ficam recortadas na area de cima e
// cima/baixo rolam essa area. `rolarAlvo` e para onde o controle mandou,
// `rolar` e onde o desenho esta (anda ate o alvo); `notasH` e a altura do texto
// inteiro medida no quadro anterior e `vistaH` a da janela visivel.
static float rolar, rolarAlvo, notasH, vistaH;
static SDL_Thread *fioInst;

const char *atualizacao_nova(void) { return tagNova; }
int atualizacao_aberta(void) { return aberto; }

// "1.0.54" > "1.0.53"? Compara numero a numero; o que nao e numero vale 0.
static int maisNova(const char *a, const char *b) {
  while (*a || *b) {
    long na = strtol(a, (char **)&a, 10), nb = strtol(b, (char **)&b, 10);
    if (na != nb) return na > nb;
    if (*a == '.') a++;
    if (*b == '.') b++;
    if (!*a && !*b) break;
    if ((*a && *a != '.' && (*a < '0' || *a > '9')) ||
        (*b && *b != '.' && (*b < '0' || *b > '9'))) break;
  }
  return 0;
}

// Copia o valor da string JSON `chave` decodificando escapes de verdade —
// js_texto troca \n por espaco, e aqui a QUEBRA DE LINHA e a estrutura das
// notas. \uXXXX vira espaco (emoji nas notas nao merece decodificador UTF-16).
static int textoJson(const char *corpo, const char *chave, char *dst, size_t tam) {
  char busca[64];
  const char *p;
  size_t k = 0;
  snprintf(busca, sizeof busca, "\"%s\":", chave);
  p = strstr(corpo, busca);
  if (!p) return 0;
  p += strlen(busca);
  while (*p == ' ') p++;
  if (*p != '"') return 0;
  p++;
  while (*p && *p != '"' && k + 1 < tam) {
    if (*p == '\\') {
      p++;
      if (*p == 'n') dst[k++] = '\n';
      else if (*p == 'r' || *p == 't') { /* nada */ }
      else if (*p == 'u') { int q; dst[k++] = ' '; for (q = 0; q < 4 && p[1]; q++) p++; }
      else if (*p) dst[k++] = *p;
      if (*p) p++;
      continue;
    }
    dst[k++] = *p++;
  }
  dst[k] = 0;
  return 1;
}

// O .ipk DENTRO DE assets[]. textoJson acha a PRIMEIRA ocorrencia de uma chave
// e serve para "tag_name"/"body", que sao da raiz; aqui a chave se repete uma
// vez por anexo (o .ipk e o .wgt) e o que decide e o SUFIXO. Por isso o laco:
// varre todas as ocorrencias e fica com a que termina em ".ipk".
// SUFIXO DO ANEXO QUE ESTA BUILD DEVE BAIXAR.
//
// Desde a v1.1.0 a release traz DOIS .ipk — o normal e o "-highcache", que so
// muda o teto de textura (NV_TEX_MB_FIXO). O id do pacote e a versao sao
// IGUAIS nos dois, entao instalar um por cima do outro troca a variante sem
// dizer nada.
//
// E era isso que acontecia: acharIpk devolvia o PRIMEIRO anexo terminado em
// ".ipk", e o GitHub lista em ordem alfabetica, onde "_arm-highcache.ipk" vem
// antes de "_arm.ipk" ('-' e menor que '.'). Ou seja, "Atualizar agora"
// instalava a highcache em TODA LG, inclusive em quem nunca a escolheu.
//
// DOIS NOMES POR VARIANTE, desde 20/09/2026. As releases 1.3.1 e 1.3.2 subiram
// os anexos como "NuvioTV-1.3.N-webos.ipk" / "-webos-highcache.ipk" — nome
// mais legivel, mas "-webos.ipk" nao termina em "_arm.ipk", entao toda LG
// normal ficou SEM o botao "Atualizar agora" por duas versoes (a highcache
// nao sentiu: "-webos-highcache.ipk" ainda termina em "-highcache.ipk"). A
// 1.3.3 volta ao nome de contrato, e este codigo passa a aceitar os dois para
// o proximo nome bonito nao quebrar de novo. A regra que nao muda: a normal
// NUNCA casa com um nome que tenha "highcache".
#ifdef NV_TEX_MB_FIXO
#  define AT_SUFIXO "-highcache.ipk"
#  define AT_SUFIXO2 "-highcache.ipk"
#else
#  define AT_SUFIXO "_arm.ipk"
#  define AT_SUFIXO2 "-webos.ipk"
#endif

static int terminaEm(const char *s, size_t n, const char *sufixo) {
  size_t k = strlen(sufixo);
  if (n >= k && !strncmp(s + n - k, sufixo, k)) return 1;
  k = strlen(AT_SUFIXO2);
  return n >= k && !strncmp(s + n - k, AT_SUFIXO2, k);
}

// O sha256 hex do anexo cuja browser_download_url comeca em `ini`. O campo
// `digest` vem ANTES do browser_download_url dentro do mesmo anexo (ordem do
// JSON do GitHub: ... size, digest, download_count, ..., browser_download_url),
// entao a busca e PARA TRAS a partir da url — ir para frente pegaria o digest do
// anexo SEGUINTE. Escreve "" quando o anexo nao tem digest.
static void hashAntesDe(const char *corpo, const char *ini, char *hash, size_t tamHash) {
  const char *d = NULL, *q = corpo;
  if (!hash || !tamHash) return;
  hash[0] = 0;
  while (q < ini) {
    const char *r = strstr(q, "\"digest\":");
    if (!r || r > ini) break;
    d = r; q = r + 8;
  }
  if (!d) return;
  d = strchr(d + 8, '"');
  if (!d) return;
  d++;
  if (!strncmp(d, "sha256:", 7)) d += 7;   // so o hex interessa
  { const char *e = strchr(d, '"');
    if (e && (size_t)(e - d) < tamHash) { memcpy(hash, d, (size_t)(e - d)); hash[e - d] = 0; } }
}

// `sufixo` obrigatorio. NAO ha reserva para "qualquer .ipk": uma release sem o
// anexo desta variante e motivo para NAO oferecer o botao e mandar a pessoa
// para a pagina — trocar de variante calada e justamente o defeito.
static int acharIpk(const char *corpo, char *dst, size_t tam,
                   char *hash, size_t tamHash, const char *sufixo) {
  const char *p = corpo;
  const char *chave = "\"browser_download_url\":";
  dst[0] = 0;
  if (hash && tamHash) hash[0] = 0;
  while ((p = strstr(p, chave)) != NULL) {
    const char *ini;
    size_t n;
    p += strlen(chave);
    while (*p == ' ') p++;
    if (*p != '"') continue;
    ini = ++p;
    while (*p && *p != '"') p++;
    n = (size_t)(p - ini);
    if (n > 4 && n < tam && terminaEm(ini, n, sufixo)) {
      memcpy(dst, ini, n); dst[n] = 0;
      // O SHA-256 DO MESMO ANEXO, e ele e obrigatorio na pratica: sem ele o
      // instalador do Homebrew Channel compara o hash calculado contra
      // `undefined` e responde `returnValue: false` com "Invalid file
      // checksum" — MEDIDO na C9, e o arquivo ate chegou a ser instalado, o
      // que e pior: sucesso reportado como falha.
      hashAntesDe(corpo, ini, hash, tamHash);
      return 1;
    }
  }
  return 0;
}

#if defined(NV_TPK) || defined(NV_ANDROID)
// Anexo cujo nome termina em `sufixo` (casamento EXATO, sem o AT_SUFIXO2 do
// .ipk) e o sha256 do MESMO anexo (hashAntesDe). Sem digest ignora o anexo: sem
// hash nao ha como confiar em codigo remoto. Serve ao .so do .tpk e ao .apk.
static int acharAnexo(const char *corpo, char *dst, size_t tam, char *hash, size_t tamHash,
                      const char *sufixo) {
  const char *p = corpo;
  const char *chave = "\"browser_download_url\":";
  size_t k = strlen(sufixo);
  dst[0] = 0;
  if (hash && tamHash) hash[0] = 0;
  while ((p = strstr(p, chave)) != NULL) {
    const char *ini;
    size_t n;
    p += strlen(chave);
    while (*p == ' ') p++;
    if (*p != '"') continue;
    ini = ++p;
    while (*p && *p != '"') p++;
    n = (size_t)(p - ini);
    if (n > k && n < tam && !strncmp(ini + n - k, sufixo, k)) {
      char h[80] = "";
      hashAntesDe(corpo, ini, h, sizeof h);
      if (!h[0]) continue;                 // sem digest: nao confiar
      memcpy(dst, ini, n); dst[n] = 0;
      snprintf(hash, tamHash, "%s", h);
      return 1;
    }
  }
  return 0;
}
#endif

#ifdef NV_ANDROID
// Contrato do anexo: "Nuvio-<versao>-android.apk". "-android-debug.apk" e
// "-android-preview.N.apk" NAO terminam em "-android.apk", entao nao casam.
#define AT_APK_SUFIXO "-android.apk"
static int acharApk(const char *corpo, char *dst, size_t tam, char *hash, size_t tamHash) {
  return acharAnexo(corpo, dst, tam, hash, tamHash, AT_APK_SUFIXO);
}
#endif

#ifdef NV_TPK
// O anexo da libnuvio.so DESTA ABI e o seu sha256. Uma release do .tpk anexa UMA
// libnuvio.so (a build de tpk.sh e unica, ARMv7 softfp, compartilhada pelos
// quatro pacotes), com o sufixo AT_SO_SUFIXO. Casa por sufixo EXATO (sem o
// AT_SUFIXO2 do .ipk) e le o digest do MESMO anexo por hashAntesDe. Sem o
// digest, ignora o anexo: sem hash nao ha como confiar em codigo nativo remoto.
// O 4/5 (NV_TPK40) roda a lib SEM TLS, que o carregador ELF exige: baixa o
// anexo proprio. "-tpk-arm.so" nao casa com "-tpk40-arm.so", entao um pacote
// nunca pega a lib do outro.
#ifdef NV_TPK40
#define AT_SO_SUFIXO "-tpk40-arm.so"
#else
#define AT_SO_SUFIXO "-tpk-arm.so"
#endif
static int acharSo(const char *corpo, char *dst, size_t tam, char *hash, size_t tamHash) {
  return acharAnexo(corpo, dst, tam, hash, tamHash, AT_SO_SUFIXO);
}
#endif

// Markdown das notas -> linhas de tela. Devolve em `dst`, linhas por \n.
static void limparNotas(const char *md, char *dst, size_t tam) {
  size_t k = 0;
  const char *p = md;
  int linhas = 0;
  while (*p && k + 2 < tam && linhas < AT_LINHAS_MAX) {
    const char *fim = strchr(p, '\n');
    size_t n = fim ? (size_t)(fim - p) : strlen(p);
    char linha[1024];
    size_t i, j = 0;
    if (n >= sizeof linha) n = sizeof linha - 1;
    memcpy(linha, p, n); linha[n] = 0;
    p = fim ? fim + 1 : p + n;
    // recorta espaco a direita
    while (n > 0 && (linha[n - 1] == ' ' || linha[n - 1] == '\r')) linha[--n] = 0;
    if (!n) continue;
    if (!strncmp(linha, "---", 3)) break;
    if (linha[0] == '#') {
      const char *t = linha;
      while (*t == '#') t++;
      while (*t == ' ') t++;
      // "Notes" e o rodape fixo de toda release: instalar, assinar.
      if (!strncmp(t, "Notes", 5) || !strncmp(t, "Notas", 5)) break;
      j = (size_t)snprintf(linha, sizeof linha, "\x01%s", t);   // \x01 = secao
      if (j >= sizeof linha) j = sizeof linha - 1;
    } else {
      char lim[1024];
      const char *s = linha;
      if (*s == '-' || *s == '*') { s++; while (*s == ' ') s++; j = (size_t)snprintf(lim, sizeof lim, "\xe2\x80\xa2 "); }
      for (i = 0; s[i] && j + 4 < sizeof lim; i++) {
        if (s[i] == '*' || s[i] == '`') continue;
        lim[j++] = s[i];
      }
      lim[j] = 0;
      memcpy(linha, lim, j + 1);
    }
    if (k + j + 1 >= tam) break;
    memcpy(dst + k, linha, j); k += j;
    dst[k++] = '\n';
    linhas++;
  }
  dst[k] = 0;
}

static int fioConsulta(void *arg) {
  char *corpo;
  char tag[48] = "", body[8192] = "";
  (void)arg;
  corpo = rede_baixar(AT_URL, 12);
  if (!corpo) { printf("[atualizacao] sem resposta do GitHub\n"); fflush(stdout); }
  else {
    textoJson(corpo, "tag_name", tag, sizeof tag);
    textoJson(corpo, "body", body, sizeof body);
    // Sem anexo da variante desta build, ipkUrl fica vazio e podeInstalar()
    // devolve 0: o cartao aparece so com a URL da pagina.
    if (AT_INSTALA) acharIpk(corpo, ipkUrl, sizeof ipkUrl, ipkHash, sizeof ipkHash, AT_SUFIXO);
#ifdef NV_TPK
    // Anexo libnuvio.so para a auto-atualizacao do .tpk (staging por hash).
    acharSo(corpo, soUrl, sizeof soUrl, soHash, sizeof soHash);
#endif
#ifdef NV_ANDROID
    acharApk(corpo, apkUrl, sizeof apkUrl, apkHash, sizeof apkHash);
#endif
    free(corpo);
  }
  SDL_LockMutex(mtx);
  if (tag[0]) {
    const char *v = tag[0] == 'v' ? tag + 1 : tag;
    if (maisNova(v, NV_VERSAO)) {
      snprintf(tagNova, sizeof tagNova, "%s", v);
      limparNotas(body, notas, sizeof notas);
#ifdef NV_TPK
      // So a versao mais nova entra: soVer marca a .so encenada e o host a
      // compara com a versao empacotada antes de aplicar.
      snprintf(soVer, sizeof soVer, "%s", v);
#endif
#ifdef NV_ANDROID
      snprintf(apkVer, sizeof apkVer, "%s", v);
#endif
    }
    printf("[atualizacao] instalada %s, no GitHub %s%s\n", NV_VERSAO, v,
           tagNova[0] ? " -- NOVA" : "");
    fflush(stdout);
  }
  pronto = 1;
  SDL_UnlockMutex(mtx);
  return 0;
}

// MANDA O SISTEMA INSTALAR o .ipk da release.
//
// MEDIDO NA C9, e foi o que derrubou a primeira versao disto: o app NAO roda
// como root. O arquivo que ele grava sai com uid 5152, e `/usr/bin/luna-send`
// e `-rwx------ root root` — a chamada morria com "can't execute 'luna-send':
// Permission denied" no log do nohup, depois de ja ter baixado 36 MB. Root
// nesta TV e o que EU tenho por SSH; o app continua no jail.
//
// O que o app alcanca: `/usr/bin/luna-send-pub` (-rwxr-xr-x) e, por ele, o
// servico do Homebrew Channel, que ESTE roda como root e existe justamente
// para instalar ipk. Ele tem os metodos install/uninstall/exec/spawn, e o
// install aceita `ipkUrl` — entao passamos a URL da release direto e nem
// baixamos: quem baixa e ele, sem 36 MB passando pelo nosso heap.
//
// Sem o Homebrew Channel instalado nao ha caminho nenhum, e o cartao volta a
// ser so aviso (ver podeInstalar).
static int fioInstalar(void *arg) {
  char cmd[900];
  (void)arg;
  { char extra[110] = "";
    if (ipkHash[0]) snprintf(extra, sizeof extra, ",\"ipkHash\":\"%s\"", ipkHash);
    snprintf(cmd, sizeof cmd,
      "nohup %s -i -f luna://org.webosbrew.hbchannel.service/install "
      "'{\"ipkUrl\":\"%s\"%s,\"subscribe\":true}' "
      "> %s 2>&1 &", AT_LUNA_PUB, ipkUrl, extra, AT_LOG_INST); }
  printf("[atualizacao] instalando %s\n", ipkUrl);
  fflush(stdout);
  if (system(cmd) != 0) {
    SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx);
    printf("[atualizacao] o instalador nao aceitou o pedido\n"); fflush(stdout);
    return 0;
  }
  // ACOMPANHA O LOG. O servico responde por subscribe e o luna-send vai
  // escrevendo cada resposta no arquivo; ler o arquivo e mais simples e mais
  // robusto do que abrir o barramento aqui — e o arquivo tem poucos KB.
  //
  // Teto de 8 minutos: sao ~36 MB numa TV, e passar disso e sinal de que a
  // resposta nao vem mais. Sem teto o fio ficaria vivo para sempre.
  { Uint32 ate = SDL_GetTicks() + 8 * 60 * 1000;
    while (SDL_GetTicks() < ate) {
      FILE *f = fopen(AT_LOG_INST, "rb");
      SDL_Delay(300);
      if (!f) continue;
      { static char buf[8192];
        size_t n = fread(buf, 1, sizeof buf - 1, f);
        const char *p, *ult;
        float pct = -1.0f;
        char passo[48] = "";
        int fim = 0, erro = 0;
        fclose(f);
        buf[n] = 0;
        // O ARQUIVO CRESCE, entao o que vale e a ULTIMA ocorrencia de cada
        // campo — a primeira e o comeco do download, e ficaria congelada.
        for (p = buf, ult = NULL; (p = strstr(p, "\"progress\":")) != NULL; p += 11) ult = p;
        if (ult) pct = (float)atof(ult + 11);
        for (p = buf, ult = NULL; (p = strstr(p, "\"statusText\":")) != NULL; p += 13) ult = p;
        if (ult) {
          const char *ini = strchr(ult + 13, '"');
          if (ini) {
            const char *e = strchr(++ini, '"');
            size_t k = e ? (size_t)(e - ini) : 0;
            if (k && k < sizeof passo) { memcpy(passo, ini, k); passo[k] = 0; }
          }
        }
        if (strstr(buf, "\"finished\": true") || strstr(buf, "\"finished\":true")) fim = 1;
        if (strstr(buf, "\"errorText\"")) erro = 1;
        SDL_LockMutex(mtx);
        if (pct >= 0.0f) instPct = pct;
        if (passo[0]) snprintf(instPasso, sizeof instPasso, "%s", passo);
        if (fim)       estado = AT_PRONTO;
        else if (erro) estado = AT_FALHOU;
        SDL_UnlockMutex(mtx);
        if (fim || erro) {
          printf("[atualizacao] instalador terminou: %s\n", fim ? "ok" : "falhou");
          fflush(stdout);
          return 0;
        } } } }
  SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx);
  printf("[atualizacao] instalador nao respondeu no prazo\n"); fflush(stdout);
  return 0;
}

// 1 quando existe o que instalar: alvo que sabe, release com .ipk anexado e
// nenhuma instalacao em andamento.
// O Homebrew Channel precisa ESTAR no aparelho: sem ele o servico nao existe e
// o botao prometeria o que nao acontece. Conferido uma vez, no primeiro uso.
static int temInstalador(void) {
  static int visto = -1;
  FILE *f;
#if defined(NV_AT_INSTALA)
  // No harness nao ha Homebrew Channel em disco para achar; quem forcou
  // AT_INSTALA esta dizendo justamente "encene a TV que tem".
  return NV_AT_INSTALA;
#endif
  if (visto >= 0) return visto;
  f = fopen(AT_HB_DIR "/appinfo.json", "r");
  visto = f != NULL;
  if (f) fclose(f);
  return visto;
}

static int podeInstalar(void) {
  return AT_INSTALA && ipkUrl[0] && estado == AT_PARADO && temInstalador();
}

#ifdef NV_TPK
// BAIXA E ENCENA a libnuvio.so nova. NUNCA carrega em processo: grava
// data/libnuvio.staged.so (+ .sha256 e .ver) e o host memfd-carrega no proximo
// arranque. A barreira e o sha256: baixa para data/libnuvio.download, confere o
// hash contra soHash (do digest da release, por HTTPS) e so entao renomeia
// atomicamente. Hash errado, download parcial ou erro de escrita => apaga tudo e
// nao encena nada. Em duvida, o app segue com a .so empacotada.
static int fioBaixarSo(void *arg) {
  char dl[600] = "", so[600] = "", sh[600] = "", vr[600] = "";
  char hex[65] = "", verLocal[32];
  char *buf;
  long n = 0;
  FILE *f;
  int ok = 0;
  (void)arg;
  SDL_LockMutex(mtx); snprintf(verLocal, sizeof verLocal, "%s", soVer); SDL_UnlockMutex(mtx);
  if (!dados_caminho(dl, sizeof dl, "libnuvio.download") ||
      !dados_caminho(so, sizeof so, "libnuvio.staged.so") ||
      !dados_caminho(sh, sizeof sh, "libnuvio.staged.sha256") ||
      !dados_caminho(vr, sizeof vr, "libnuvio.staged.ver")) {
    printf("[atualizacao] sem pasta de dados para encenar a .so\n"); fflush(stdout);
    SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx);
    return 0;
  }
  printf("[atualizacao] baixando libnuvio.so nova (%s)\n", soUrl); fflush(stdout);
  buf = rede_baixar_bin(soUrl, 120, &n);
  if (!buf || n <= 0) {
    printf("[atualizacao] download da .so falhou\n"); fflush(stdout);
    free(buf);
    SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx);
    return 0;
  }
  SDL_LockMutex(mtx); snprintf(instPasso, sizeof instPasso, "Verificando"); SDL_UnlockMutex(mtx);
  at_sha256_hex((const unsigned char *)buf, (size_t)n, hex);
  if (strcasecmp(hex, soHash) != 0) {
    printf("[atualizacao] sha256 NAO confere: baixado %.12s... esperado %.12s...\n", hex, soHash);
    fflush(stdout);
    free(buf);
    SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx);
    return 0;
  }
  // Hash confere: grava o arquivo temporario e renomeia atomicamente. So depois
  // vem o .sha256 e o .ver — se o processo morrer no meio, faltar o .sha256 faz
  // o host ignorar o staging (ele reconfere o hash do arquivo encenado).
  f = fopen(dl, "wb");
  if (f) {
    ok = fwrite(buf, 1, (size_t)n, f) == (size_t)n;
    if (fflush(f) != 0) ok = 0;
    fclose(f);
  }
  free(buf);
  if (!ok) {
    printf("[atualizacao] nao consegui gravar %s\n", dl); fflush(stdout);
    unlink(dl);
    SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx);
    return 0;
  }
  if (rename(dl, so) != 0) {
    printf("[atualizacao] rename para staged falhou\n"); fflush(stdout);
    unlink(dl);
    SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx);
    return 0;
  }
  { char linha[80]; snprintf(linha, sizeof linha, "%s\n", hex); dados_gravar("libnuvio.staged.sha256", linha); (void)sh; }
  { char linha[48]; snprintf(linha, sizeof linha, "%s\n", verLocal); dados_gravar("libnuvio.staged.ver", linha); (void)vr; }
  printf("[atualizacao] libnuvio.so %s encenada; aplica no proximo arranque\n", verLocal);
  fflush(stdout);
  SDL_LockMutex(mtx); estado = AT_PRONTO; soEncenada = 1; SDL_UnlockMutex(mtx);
  return 0;
}

// 1 quando ha uma libnuvio.so nova para encenar e nada em andamento.
static int podeAtualizarTpk(void) {
  return soUrl[0] && soHash[0] && soVer[0] && estado == AT_PARADO;
}
#else
static int podeAtualizarTpk(void) { return 0; }
#endif

#ifdef NV_ANDROID
// BAIXA O APK para <dados>/atualizacao/Nuvio-<v>.apk, CONFERE o sha256 contra o
// digest da release (HTTPS) e entrega ao instalador do sistema (NuvioActivity).
// Hash errado ou download parcial: apaga e nao instala nada. Se o arquivo ja
// esta la com o hash certo (a pessoa foi conceder a permissao e voltou), nao
// baixa de novo. Termina em AT_PARADO quando o instalador abriu (se ela cancelar,
// o cartao deixa tentar outra vez; se confirmar, o sistema mata o app).
static void apkFalhou(void) {
  SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx);
}

static int fioInstalarApk(void *arg) {
  char dir[600] = "", nome[96], rel[128], arq[600] = "", hex[65] = "", ver[32], hash[80];
  char *buf = NULL;
  long n = 0;
  int ok = 0, r;
  FILE *f;
  (void)arg;
  SDL_LockMutex(mtx);
  snprintf(ver, sizeof ver, "%s", apkVer);
  snprintf(hash, sizeof hash, "%s", apkHash);
  apkPerm = 0;
  SDL_UnlockMutex(mtx);
  snprintf(nome, sizeof nome, "Nuvio-%s.apk", ver);
  snprintf(rel, sizeof rel, "atualizacao/%s", nome);
  if (!dados_caminho(dir, sizeof dir, "atualizacao") || !dados_caminho(arq, sizeof arq, rel)) {
    printf("[atualizacao] sem pasta de dados para o APK\n"); fflush(stdout);
    apkFalhou(); return 0;
  }
  mkdir(dir, 0755);
  // Ja baixado e conferido numa tentativa anterior?
  f = fopen(arq, "rb");
  if (f) {
    long t;
    fseek(f, 0, SEEK_END); t = ftell(f); fseek(f, 0, SEEK_SET);
    if (t > 0 && (buf = malloc((size_t)t)) != NULL) {
      if (fread(buf, 1, (size_t)t, f) == (size_t)t) {
        at_sha256_hex((const unsigned char *)buf, (size_t)t, hex);
        ok = strcasecmp(hex, hash) == 0;
      }
      free(buf); buf = NULL;
    }
    fclose(f);
    if (!ok) unlink(arq);
  }
  if (!ok) {
    printf("[atualizacao] baixando o APK (%s)\n", apkUrl); fflush(stdout);
    // TRES TENTATIVAS. Logo depois de um anexo novo o GitHub respondeu 504 por
    // uns minutos (medido em 01/10/2026 com a 1.6.5: o .ipk antigo baixava, o
    // .apk recem-enviado dava 504, e a mesma URL serviu 30 s depois).
    { int tent;
      for (tent = 0; tent < 3; tent++) {
        if (tent) { printf("[atualizacao] tentando de novo em %d s\n", 10 * tent); fflush(stdout); SDL_Delay(10000u * (unsigned)tent); }
        n = 0;
        buf = rede_baixar_bin(apkUrl, 600, &n);
        if (buf && n > 0) break;
        free(buf); buf = NULL;
      } }
    if (!buf || n <= 0) {
      printf("[atualizacao] download do APK falhou\n"); fflush(stdout);
      free(buf); apkFalhou(); return 0;
    }
    SDL_LockMutex(mtx); snprintf(instPasso, sizeof instPasso, "Verificando"); SDL_UnlockMutex(mtx);
    at_sha256_hex((const unsigned char *)buf, (size_t)n, hex);
    if (strcasecmp(hex, hash) != 0) {
      printf("[atualizacao] sha256 do APK NAO confere: baixado %.12s... esperado %.12s...\n", hex, hash);
      fflush(stdout);
      free(buf); apkFalhou(); return 0;
    }
    f = fopen(arq, "wb");
    ok = 0;
    if (f) {
      ok = fwrite(buf, 1, (size_t)n, f) == (size_t)n;
      if (fflush(f) != 0) ok = 0;
      fclose(f);
    }
    free(buf);
    if (!ok) {
      printf("[atualizacao] nao consegui gravar %s\n", arq); fflush(stdout);
      unlink(arq); apkFalhou(); return 0;
    }
  }
  SDL_LockMutex(mtx); snprintf(instPasso, sizeof instPasso, "Install"); SDL_UnlockMutex(mtx);
  // A troca de pacote mata o processo sem saida limpa: grava a despedida "fim"
  // ANTES, para o modo seguro nao contar a atualizacao como queda. A Activity
  // a desfaz se o instalador for cancelado e o app continuar vivo.
  dados_despedida_fim();
  r = android_instalar_apk(arq);
  SDL_LockMutex(mtx);
  if (r == 0) estado = AT_FALHOU;
  else { estado = AT_PARADO; apkPerm = (r == 2); }
  SDL_UnlockMutex(mtx);
  if (r == 2) {   // so a tela de permissao abriu: nada foi instalado, desfaz a despedida
    char d[600];
    if (dados_caminho(d, sizeof d, "despedida.txt")) unlink(d);
  }
  return 0;
}

static int podeAtualizarApk(void) {
  return apkUrl[0] && apkHash[0] && apkVer[0] && estado == AT_PARADO;
}
#else
static int podeAtualizarApk(void) { return 0; }
#endif

// Ha um botao de acao (instalar .ipk na LG, ou encenar .so nova no .tpk)?
static int podeAgir(void) { return podeInstalar() || podeAtualizarTpk() || podeAtualizarApk(); }

// Quanto da para rolar: o que sobra das notas alem da janela. 0 = cabe tudo.
static float rolarMax(void) {
  float m = notasH - vistaH;
  return m > 0.0f ? m : 0.0f;
}

// Cada abertura comeca do topo das notas e com o foco no botao de atualizar.
static void reiniciarVista(void) {
  foco = 0;
  rolar = rolarAlvo = 0.0f;
}

void atualizacao_verificar(void) {
  if (disparado) return;
  disparado = 1;
  if (!mtx) mtx = SDL_CreateMutex();
  fio = SDL_CreateThread(fioConsulta, "nv-atualizacao", NULL);
  if (fio) SDL_DetachThread(fio);
}

void atualizacao_mostrar_se_houver(void) {
  char *visto;
  if (mostrado || aberto) return;
  SDL_LockMutex(mtx);
  if (!pronto || !tagNova[0]) { SDL_UnlockMutex(mtx); return; }
  SDL_UnlockMutex(mtx);
  mostrado = 1;
  visto = dados_ler(AT_ARQ);
  if (visto) {
    int igual = !strncmp(visto, tagNova, strlen(tagNova)) &&
                (visto[strlen(tagNova)] == '\n' || visto[strlen(tagNova)] == 0);
    free(visto);
    if (igual) return;
  }
  aberto = 1;
  reiniciarVista();
}

// Fecha e ANOTA a versao vista: o cartao e uma vez por tag.
static void fechar(void) {
  char s[40];
  aberto = 0;
  snprintf(s, sizeof s, "%s\n", tagNova);
  dados_gravar(AT_ARQ, s);
}

void atualizacao_abrir(void) {
  if (!mtx) return;
  SDL_LockMutex(mtx);
  if (tagNova[0]) { aberto = 1; mostrado = 1; reiniciarVista(); }
  SDL_UnlockMutex(mtx);
}

void atualizacao_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto || e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  // VOLTAR sempre fecha, inclusive durante o download: quem desistiu no meio
  // nao fica preso olhando uma barra. O fio termina sozinho e, se chegar a
  // instalar, o sistema mata o app de qualquer jeito.
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      e->key.keysym.scancode == NV_SCANCODE_BACK) { fechar(); return; }
  // CIMA/BAIXO ROLAM AS NOTAS, em qualquer estado (inclusive baixando): o
  // rodape nao depende delas, entao rolar nunca tira o botao da tela.
  if (k == SDLK_UP || k == SDLK_DOWN) {
    rolarAlvo += k == SDLK_DOWN ? AT_PASSO : -AT_PASSO;
    if (rolarAlvo > rolarMax()) rolarAlvo = rolarMax();
    if (rolarAlvo < 0.0f) rolarAlvo = 0.0f;
    return;
  }
  if (estado == AT_INSTALANDO) return;
  if (estado == AT_PRONTO && soEncenada &&
      (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE)) {
    SDL_Event q;
    printf("[atualizacao] reiniciar agora: encerrando para o host carregar a lib nova\n");
    fflush(stdout);
    memset(&q, 0, sizeof q);
    q.type = SDL_QUIT;
    SDL_PushEvent(&q);
    fechar();
    return;
  }
  if (podeAgir() && (k == SDLK_LEFT || k == SDLK_RIGHT)) {
    foco = k == SDLK_LEFT ? 0 : 1;
    return;
  }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE ||
      k == SDLK_DELETE) {
    if (podeInstalar() && foco == 0) {
      SDL_Thread *t;
      SDL_LockMutex(mtx); estado = AT_INSTALANDO; SDL_UnlockMutex(mtx);
      t = SDL_CreateThread(fioInstalar, "nv-instalar", NULL);
      if (t) { SDL_DetachThread(t); fioInst = t; }
      else { SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx); }
      return;
    }
#ifdef NV_TPK
    if (podeAtualizarTpk() && foco == 0) {
      SDL_Thread *t;
      SDL_LockMutex(mtx); estado = AT_INSTALANDO; SDL_UnlockMutex(mtx);
      t = SDL_CreateThread(fioBaixarSo, "nv-baixar-so", NULL);
      if (t) { SDL_DetachThread(t); fioInst = t; }
      else { SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx); }
      return;
    }
#endif
#ifdef NV_ANDROID
    if (podeAtualizarApk() && foco == 0) {
      SDL_Thread *t;
      SDL_LockMutex(mtx); estado = AT_INSTALANDO; instPct = 0.0f; instPasso[0] = 0; SDL_UnlockMutex(mtx);
      t = SDL_CreateThread(fioInstalarApk, "nv-instalar-apk", NULL);
      if (t) { SDL_DetachThread(t); fioInst = t; }
      else { SDL_LockMutex(mtx); estado = AT_FALHOU; SDL_UnlockMutex(mtx); }
      return;
    }
#endif
    fechar();
  }
}

void atualizacao_atualizar(float dt, Uint32 agora) {
  (void)agora;
  if (!aberto && entrada < 0.002f) { entrada = 0.0f; return; }
  entrada = anim_rampa(entrada, aberto ? 1.0f : 0.0f, dt,
                       aberto ? AT_ABRIR_MS : AT_FECHAR_MS);
  // Rolagem suave, mas curta (~120 ms para chegar): quem segura a seta quer
  // ver o texto andar, nao esperar.
  { float d = rolarAlvo - rolar, f = dt * 1000.0f / 120.0f;
    if (f > 1.0f) f = 1.0f;
    rolar = (d > -0.5f && d < 0.5f) ? rolarAlvo : rolar + d * f; }
}

void atualizacao_desenhar(Uint32 agora) {
  float a = anim_suave(entrada), dy, y, x, w;
  char buf[160];
  const char *p;
  (void)agora;
  if (entrada < 0.002f) return;

  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0, 0, 0, 0.72f * entrada);
  dy = (1.0f - a) * 36.0f;
  // CARTAO FLUTUANTE na "cara nova" (menu.c, 21/09/2026): cantos de 28 px
  // pelo menor lado (a altura), fundo translucido e UMA luz difusa na cor de
  // realce entrando pelo canto superior esquerdo, presa aos cantos do cartao
  // (GFX_LUZ). Com o veu de tela cheia ja pago, a luz e a segunda e ultima
  // camada grande desta tela.
  { GfxRect c = { AT_X, AT_Y + dy, AT_W, AT_H };
    float ar, ag, ab; ajustes_acento(&ar, &ag, &ab);
    gfx_cor(c, 28.0f / AT_H, 0.055f, 0.058f, 0.068f, 0.94f * a);
    gfx_luz_canto(c, 28.0f / AT_H, AT_H * 0.1f, -AT_H * 0.1f, AT_H * 0.65f, ar, ag, ab, 0.22f * a);
    gfx_recorte(AT_X, AT_Y + dy, AT_W, AT_H); }

  x = AT_X + AT_PAD; w = AT_W - 2.0f * AT_PAD;
  y = AT_Y + dy + 56.0f;
  { TxtLinha t = txt_linha(TXT_CAPTION2, i18n("ATUALIZAÇÃO DISPONÍVEL"),
                           150, 154, 165, 255);
    txt_desenhar_alpha(t, x, y, a * 0.92f); }
  y += 30.0f;
  snprintf(buf, sizeof buf, i18n("Nuvio %s"), tagNova);
  { TxtLinha t = txt_linha(TXT_TITULO1, buf, 246, 247, 252, 255);
    txt_desenhar_alpha(t, x, y, a); y += t.h + 6.0f; }
  // A VARIANTE NO CARTAO. Sem isto a pessoa le "Você está na 1.1.2" e vai para
  // uma pagina com dois .ipk sem saber qual e o dela.
#ifdef NV_TEX_MB_FIXO
  snprintf(buf, sizeof buf, i18n("Você está na %s · cache grande"), NV_VERSAO);
#else
  snprintf(buf, sizeof buf, i18n("Você está na %s"), NV_VERSAO);
#endif
  { TxtLinha t = txt_linha(TXT_CAPTION, buf, 176, 180, 190, 255);
    txt_desenhar_alpha(t, x, y, a * 0.9f); y += t.h + 28.0f; }

  // NOTAS, linha a linha; \x01 marca secao. Janela fixa entre o cabecalho e o
  // rodape, recortada: o que nao cabe fica para a rolagem, nunca por cima dos
  // botoes. A altura total sai do proprio desenho e vale para o quadro
  // seguinte (o texto nao muda enquanto o cartao esta aberto).
  { float topo = y, base = AT_Y + dy + AT_H - AT_RODAPE_H, y0;
  vistaH = base - topo;
  if (rolarAlvo > rolarMax()) rolarAlvo = rolarMax();
  if (rolar > rolarMax()) rolar = rolarMax();
  gfx_recorte(AT_X, topo, AT_W, vistaH);
  y = y0 = topo - rolar;
  p = notas;
  while (*p) {
    const char *fim = strchr(p, '\n');
    size_t n = fim ? (size_t)(fim - p) : strlen(p);
    char linha[512];
    if (n >= sizeof linha) n = sizeof linha - 1;
    memcpy(linha, p, n); linha[n] = 0;
    p = fim ? fim + 1 : p + n;
    if (linha[0] == '\x01') {
      y += 10.0f;
      { TxtLinha t = txt_linha(TXT_CALLOUT, i18n(linha + 1), 246, 247, 252, 255);
        txt_desenhar_alpha(t, x, y, a); y += t.h + 10.0f; }
    } else {
      y += txt_bloco(TXT_BODY, linha, 200, 204, 214, x, y, w, AT_LEADING,
                     a * 0.95f, AT_ITEM_LINHAS) + 10.0f;
    }
  }
  notasH = y - y0;
  // DICA DE QUE HA MAIS: o fim da janela some no fundo do cartao (em vez de
  // cortar uma linha ao meio) e uma trilha fina a direita diz onde se esta.
  if (rolarMax() > 0.5f) {
    float ar, ag, ab, tH = vistaH - 16.0f, pH, pY;
    if (rolar < rolarMax() - 0.5f) {
      GfxRect veu = { AT_X, base - 72.0f, AT_W, 72.0f };
      gfx_rect(veu, 0, GFX_VEU_BAIXO, 0, 0, 0, 0.0f, 0.055f, 0.058f, 0.068f, a);
    }
    gfx_recorte(AT_X, AT_Y + dy, AT_W, AT_H);
    ajustes_acento(&ar, &ag, &ab);
    pH = tH * vistaH / notasH;
    if (pH < 40.0f) pH = 40.0f;
    pY = topo + 8.0f + (tH - pH) * (rolar / rolarMax());
    // O raio do SDF e relativo a ALTURA: NV_RAIO_PILL numa trilha em pe vira
    // uma lente. Meia largura sobre a altura e que da a pilula.
    gfx_cor((GfxRect){ AT_X + AT_W - 30.0f, topo + 8.0f, 6.0f, tH },
            3.0f / tH, 1.0f, 1.0f, 1.0f, 0.12f * a);
    gfx_cor((GfxRect){ AT_X + AT_W - 30.0f, pY, 6.0f, pH },
            3.0f / pH, ar, ag, ab, 0.9f * a);
  }
  gfx_recorte(AT_X, AT_Y + dy, AT_W, AT_H); }

  // RODAPE. Onde ha como instalar, ele vira dois botoes; onde nao ha, continua
  // sendo o endereco da pagina, que e a unica coisa util a dizer. Posicao FIXA,
  // abaixo da janela das notas: por mais longas que elas sejam, o botao esta
  // sempre na tela.
  y = AT_Y + dy + AT_H - 96.0f;
  if (estado == AT_PRONTO && soEncenada) {
    const char *rot = i18n("Reiniciar agora");
    TxtLinha t = txt_linha(TXT_CALLOUT, i18n("Pronto. Reinicie o Nuvio para usar a versão nova."),
                           232, 236, 246, 255);
    txt_desenhar_alpha(t, x, y - 58.0f, a);
    { GfxRect b = { x, y, botao_largura(rot, NULL, 1), BOTAO_H_PRIMARIO };
      botao_pilula(b, rot, NULL, 1.0f, 1, 0, a); }
  } else if (estado == AT_INSTALANDO || estado == AT_PRONTO) {
    // BARRA E PORCENTAGEM, e nao uma frase parada. O numero e o passo vem do
    // proprio instalador (progress/statusText); enquanto ele nao disse nada a
    // barra fica vazia em vez de inventar movimento.
    float pct, larg = w * 0.62f;
    char passo[48];
    SDL_LockMutex(mtx);
    pct = estado == AT_PRONTO ? 100.0f : instPct;
    snprintf(passo, sizeof passo, "%s", instPasso);
    SDL_UnlockMutex(mtx);
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    { const char *msg = estado == AT_PRONTO
        ? i18n("Atualizado. Feche e abra o app para usar.")
        : (!strncmp(passo, "Verif", 5) ? i18n("Conferindo o arquivo...")
        : (!strncmp(passo, "Install", 7) ? i18n("Instalando...")
        :  i18n("Baixando a atualização...")));
      TxtLinha t = txt_linha(TXT_CALLOUT, msg, 232, 236, 246, 255);
      txt_desenhar_alpha(t, x, y, a); }
    { GfxRect trilho = { x, y + 46.0f, larg, 10.0f };
      GfxRect cheio  = { x, y + 46.0f, larg * (pct / 100.0f), 10.0f };
      float ar, ag, ab;
      ajustes_acento(&ar, &ag, &ab);
      gfx_cor(trilho, NV_RAIO_PILL, 1.0f, 1.0f, 1.0f, 0.14f * a);
      // Menos de meia altura de barra nao desenha: um retangulo de 2 px com
      // canto arredondado vira um pontinho torto no canto esquerdo.
      if (cheio.w > 12.0f) gfx_cor(cheio, NV_RAIO_PILL, ar, ag, ab, a);
      { char n[16];
        TxtLinha t;
        snprintf(n, sizeof n, "%d%%", (int)(pct + 0.5f));
        t = txt_linha(TXT_CAPTION, n, 200, 204, 214, 255);
        txt_desenhar_alpha(t, x + larg + 20.0f, y + 40.0f, a * 0.95f); } }
  } else if (podeAgir()) {
    const char *rot[2];
    float bx = x;
    int i;
    rot[0] = i18n("Atualizar agora");
    rot[1] = i18n("Depois");
#ifdef NV_ANDROID
    if (apkPerm) {
      TxtLinha t = txt_linha(TXT_CALLOUT, i18n("Permita instalar apps do Nuvio e tente de novo"),
                             232, 236, 246, 255);
      txt_desenhar_alpha(t, x, y - 58.0f, a);
    }
#endif
    // A PILULA DA TABELA (botoes.h): "Atualizar agora" e o primario (72 px,
    // cheio), "Depois" o secundario (56 px, contorno), alinhados pela base.
    for (i = 0; i < 2; i++) {
      int primario = (i == 0);
      float h = primario ? BOTAO_H_PRIMARIO : BOTAO_H_SECUNDARIO;
      GfxRect b = { bx, y + (BOTAO_H_PRIMARIO - h), botao_largura(rot[i], NULL, primario), h };
      botao_pilula(b, rot[i], NULL, i == foco ? 1.0f : 0.0f, primario, 0, a);
      bx += b.w + BOTAO_GAP;
    }
  } else {
    TxtLinha t = txt_linha(TXT_CAPTION,
        estado == AT_FALHOU ? i18n("Não foi possível atualizar por aqui.") : AT_PAGINA,
        176, 180, 190, 255);
    txt_desenhar_alpha(t, x, y + 16.0f, a * 0.9f);
  }
  if (estado != AT_INSTALANDO && estado != AT_PRONTO) {
    int mais = rolarMax() > 0.5f;
    TxtLinha t = txt_linha(TXT_CAPTION2,
        podeAgir()
          ? (mais ? i18n("↑ ↓  Mais notas   ·   Voltar para fechar") : i18n("Voltar para fechar"))
          : (mais ? i18n("↑ ↓  Mais notas   ·   OK para fechar") : i18n("OK para fechar")),
        150, 154, 165, 255);
    txt_desenhar_alpha(t, AT_X + AT_W - AT_PAD - t.w, y + 24.0f, a * 0.85f);
  }
  gfx_sem_recorte();
}
