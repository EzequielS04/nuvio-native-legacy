#include "badges.h"
#include "js.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "ajustes.h"
#include "idioma.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
static const char *ids[]={"r-4k","r-1080","r-720","r-sd","q-remux","q-bluray","q-webdl","q-webrip","q-seadex","v-dv","v-hdr10plus","v-hdr10","v-hdr","v-hlg","v-imax-enhanced","v-imax","v-sdr","a-atmos-dv","a-atmos","a-truehd-dv","a-truehd","a-dtsx","a-dtshdma","a-dtshd","a-dts","a-dd-dv","a-ddp","a-dd","c-71","c-51","co-x265","co-x264","co-av1","p-netflix","p-prime","p-appletv","p-disney","p-max","p-hulu","p-peacock","p-paramount","p-crave","p-crunchyroll"};
#define NB (sizeof ids/sizeof ids[0])
static struct {char image[700],name[64];} art[NB];
static uint64_t bit(const char *id){for(size_t i=0;i<NB;i++)if(!strcmp(id,ids[i]))return UINT64_C(1)<<i;return 0;}
static int token(const char *s,const char *t){size_t n=strlen(t);for(const char *p=s;(p=strstr(p,t));p++)if((p==s||!isalnum((unsigned char)p[-1]))&&!isalnum((unsigned char)p[n]))return 1;return 0;}
uint64_t badges_detectar(const char *metadata) {
  char s[4096];size_t n=0;for(;metadata&&metadata[n]&&n<sizeof s-1;n++)s[n]=(char)tolower((unsigned char)metadata[n]);s[n]=0;
  uint64_t m=0;
#define HAS(t) token(s,t)
#define ADD(id) (m|=bit(id))
  int dv=HAS("dv")||HAS("dovi")||HAS("dolbyvision")||HAS("dolby vision")||HAS("dolby.vision")||HAS("dolby-vision")||HAS("dolby_vision");
  int atmos=HAS("atmos"),thd=HAS("truehd")||HAS("true-hd")||HAS("true hd");
  int ddp=HAS("ddp")||strstr(s,"ddp5")||strstr(s,"ddp7")||HAS("dd+")||HAS("eac3")||HAS("eac-3")||HAS("e-ac-3");
  int dd=HAS("ac3")||HAS("ac-3")||HAS("dd5.1")||HAS("dd2.0");
  if((HAS("4k")||HAS("2160p")||HAS("2160")||HAS("uhd"))&&!HAS("1080p")&&!HAS("720p"))ADD("r-4k");
  if(HAS("1080p")||HAS("1080i")||HAS("1080"))ADD("r-1080");
  if(HAS("720p")||HAS("720"))ADD("r-720");
  if(HAS("remux"))ADD("q-remux");else if(HAS("bluray")||HAS("blu-ray"))ADD("q-bluray");
  if(HAS("web-dl")||HAS("webdl")||HAS("web.dl")||HAS("web dl"))ADD("q-webdl");
  if(HAS("webrip")||HAS("web-rip")||HAS("web.rip"))ADD("q-webrip");
  if(HAS("seadex"))ADD("q-seadex");
  if(HAS("hdr10+")||HAS("hdr10plus")||HAS("hdr10p"))ADD("v-hdr10plus");
  else if(HAS("hdr10"))ADD("v-hdr10");else if(HAS("hlg"))ADD("v-hlg");else if(HAS("hdr"))ADD("v-hdr");
  if(HAS("imax enhanced")||HAS("imax.enhanced"))ADD("v-imax-enhanced");else if(HAS("imax"))ADD("v-imax");
  if(HAS("sdr"))ADD("v-sdr");
  if(atmos)ADD(dv?"a-atmos-dv":"a-atmos");
  else if(thd)ADD(dv?"a-truehd-dv":"a-truehd");
  else if(ddp||dd)ADD(dv?"a-dd-dv":ddp?"a-ddp":"a-dd");
  else if(dv)ADD("v-dv");
  if(HAS("dts:x")||HAS("dts-x")||HAS("dts.x"))ADD("a-dtsx");
  else if(HAS("dts-hd ma")||HAS("dts-hd.ma")||HAS("dts.hd.ma")||HAS("dts-ma"))ADD("a-dtshdma");
  else if(HAS("dts-hd")||HAS("dts.hd"))ADD("a-dtshd");else if(HAS("dts"))ADD("a-dts");
  if(HAS("7.1")||HAS("7.0"))ADD("c-71");else if(HAS("5.1")||HAS("5.0"))ADD("c-51");
  if(HAS("hevc")||HAS("x265")||HAS("h265")||HAS("h.265"))ADD("co-x265");
  if(HAS("avc")||HAS("x264")||HAS("h264")||HAS("h.264"))ADD("co-x264");if(HAS("av1"))ADD("co-av1");
  if(HAS("netflix")||HAS("nflx"))ADD("p-netflix");
  if(HAS("amzn")||HAS("amazon")||HAS("prime video")||HAS("primevideo"))ADD("p-prime");
  if(HAS("atvp")||HAS("atv+")||HAS("apple tv")||HAS("appletv"))ADD("p-appletv");
  if(HAS("dsnp")||HAS("disney"))ADD("p-disney");if(HAS("hmax")||HAS("hbo")||HAS("max"))ADD("p-max");
  if(HAS("hulu"))ADD("p-hulu");if(HAS("pcok")||HAS("peacock"))ADD("p-peacock");
  if(HAS("pmtp")||HAS("paramount"))ADD("p-paramount");if(HAS("crave"))ADD("p-crave");
  if(HAS("crunchyroll"))ADD("p-crunchyroll");
#undef HAS
#undef ADD
  return m;
}
// Provedores comecam em p-netflix; a conta sai do nome e nao de um 31 cravado,
// que quebrou em silencio quando entrou o v-hlg no meio da tabela.
uint64_t badges_provedor(const char *name){
  static uint64_t dePartida;
  if(!dePartida){uint64_t b=bit("p-netflix");dePartida=~UINT64_C(0)<<__builtin_ctzll(b);}
  return badges_detectar(name)&dePartida;
}
// Marca do HDR que a FONTE anuncia (nao o que o painel ativou), ou -1. Trocou
// badges_fonte_hdr, que devolvia a frase "Fonte HDR10+" em texto.
int badges_fonte_hdr_marca(uint64_t mask) {
  if (mask & bit("v-hdr10plus")) return FMT_HDR10P;
  if (mask & bit("v-hdr10")) return FMT_HDR10;
  if (mask & bit("v-hlg")) return FMT_HLG;
  if (mask & bit("v-hdr")) return FMT_HDR;
  return -1;
}

// --- MARCA DE FORMATO, UMA SO PORTA (29/09/2026) -----------------------------
//
// Pedido do dono: "vamos usar as logos sempre que formos referenciar HDR,
// HDR10+, Dolby Vision, Dolby Atmos, etc." Antes cada tela escrevia a palavra
// (a folha de fontes "Dolby Vision · Atmos", o player "Dolby Vision", "HDR10",
// os Ajustes "DV"/"ATMOS") e so a fileira de badges da folha usava a arte.
//
// As artes sao as mesmas de badges/: brancas, forma no ALFA. Por isso o
// desenho vai sempre por GFX_MARCA, que tinge com a cor pedida — segue a tinta
// do foco, o vidro e o realce sem arquivo novo.
static const struct { const char *id; const char *texto; } FORMATOS[FMT_N] = {
  [FMT_4K]={"r-4k","4K"},[FMT_1080]={"r-1080","1080p"},[FMT_720]={"r-720","720p"},
  // SD nao vinha no pacote de marcas: r-sd.webp (29/09/2026) foi desenhada na
  // mesma caixa do 4K (altura de glifo 119 em 194, topo em 37), para a fileira
  // de resolucoes ter um corpo so. badges_detectar NAO a acende: "sd" solto no
  // nome de uma fonte e ruido demais; so quem sabe a resolucao (o guia, o
  // player) pede FMT_SD.
  [FMT_SD]={"r-sd","SD"},
  [FMT_SDR]={"v-sdr","SDR"},[FMT_HDR]={"v-hdr","HDR"},[FMT_HDR10]={"v-hdr10","HDR10"},
  [FMT_HDR10P]={"v-hdr10plus","HDR10+"},[FMT_HLG]={"v-hlg","HLG"},
  [FMT_DV]={"v-dv","Dolby Vision"},[FMT_ATMOS]={"a-atmos","Dolby Atmos"},
  [FMT_DTS]={"a-dts","DTS"},[FMT_DTSX]={"a-dtsx","DTS:X"},[FMT_DTSHD]={"a-dtshd","DTS-HD"},
  [FMT_TRUEHD]={"a-truehd","Dolby TrueHD"},[FMT_DD]={"a-dd","Dolby Digital"},
  [FMT_DDP]={"a-ddp","Dolby Digital+"},[FMT_IMAX]={"v-imax","IMAX"},
  [FMT_IMAX_ENH]={"v-imax-enhanced","IMAX Enhanced"},[FMT_AV1]={"co-av1","AV1"},
  [FMT_HEVC]={"co-x265","HEVC"},[FMT_AVC]={"co-x264","AVC"},[FMT_REMUX]={"q-remux","Remux"},
};
static int indiceFormato(FormatoMarca f) {
  if ((int)f < 0 || f >= FMT_N) return -1;
  for (size_t i = 0; i < NB; i++) if (!strcmp(ids[i], FORMATOS[f].id)) return (int)i;
  return -1;
}
int marca_resolucao(const char *nome) {
  if (!nome || !nome[0]) return -1;
  if (!strcasecmp(nome, "4K") || !strcasecmp(nome, "UHD") || !strcasecmp(nome, "2160p")) return FMT_4K;
  if (!strcasecmp(nome, "1080p") || !strcasecmp(nome, "FHD")) return FMT_1080;
  if (!strcasecmp(nome, "720p") || !strcasecmp(nome, "HD")) return FMT_720;
  if (!strcasecmp(nome, "SD")) return FMT_SD;
  return -1;
}
const char *marca_formato_nome(FormatoMarca f) {
  return ((int)f >= 0 && f < FMT_N) ? FORMATOS[f].texto : "";
}
// Largura sem desenhar, para o chamador alinhar a direita ou centralizar. Com a
// textura ainda nao decodificada tex_aspecto vale 0 e a largura cai no palpite
// (2,2 x altura): o quadro seguinte corrige, e a marca aparece no lugar certo.
float marca_formato_largura(FormatoMarca f, float altura) {
  int i = indiceFormato(f);
  float asp;
  if (i < 0) return 0.0f;
  if (!art[i].image[0]) return (float)txt_largura(TXT_MINI, FORMATOS[f].texto);
  asp = tex_aspecto(art[i].image);
  if (asp <= 0.0f) { tex_obter_larg(art[i].image, 128); asp = 2.2f; }
  { float w = altura * asp; return w > altura * 4.6f ? altura * 4.6f : w; }
}
float marca_formato(FormatoMarca f, float x, float y, float altura,
                    float r, float g, float b, float a) {
  int i = indiceFormato(f);
  GLuint t;
  float asp;
  if (i < 0) return 0.0f;
  if (art[i].image[0]) {
    // Decodifica na largura de uso: 128 cobre as marcas de linha, e as largas
    // (DTS-HD a 44 px de caixa passa de 180) pedem 256 para nao borrar.
    float w = altura * 4.6f;
    t = tex_obter_larg(art[i].image, altura * 4.16f > 128.0f ? 256.0f : 128.0f);
    asp = tex_aspecto(art[i].image);
    if (t && asp > 0.0f) {
      w = altura * asp;
      if (w > altura * 4.6f) w = altura * 4.6f;
      gfx_rect((GfxRect){ x, y + (altura - w / asp) * 0.5f, w, w / asp }, t,
               GFX_MARCA, 0, 0, 0, 0, r, g, b, a);
      return w;
    }
  }
  // SEM ARTE (pacote sem badges/, ou ainda decodificando): o nome em texto, na
  // mesma cor. Melhor a palavra que um buraco — e some assim que a textura vem.
  { TxtLinha l = txt_linha(TXT_MINI, FORMATOS[f].texto, (int)(r * 255.0f), (int)(g * 255.0f),
                           (int)(b * 255.0f), 255);
    txt_desenhar_alpha(l, x, y + (altura - (float)l.h) * 0.5f, a);
    return (float)l.w; }
}
void badges_carregar(const char *dir) {
  char path[700];snprintf(path,sizeof path,"%s/badges/index.json",dir);FILE *f=fopen(path,"rb");if(!f)return;
  char body[24000];size_t n=fread(body,1,sizeof body-1,f);body[n]=0;fclose(f);
  for(const char *p=js_array(body,NULL,"badges");p;p=js_prox(js_fim(p))){char id[48],name[64],file[128];const char *e=js_fim(p);
    js_texto(p,e,"id",id,sizeof id);js_texto(p,e,"name",name,sizeof name);js_texto(p,e,"image",file,sizeof file);
    if(strchr(file,'/')||strstr(file,".."))continue;
    for(size_t i=0;i<NB;i++)if(!strcmp(ids[i],id)){snprintf(art[i].image,sizeof art[i].image,"%s/badges/%s",dir,file);snprintf(art[i].name,sizeof art[i].name,"%s",name);}
  }
}
// FILEIRA DE BADGES, clara ou escura. A escura existe porque desde 16/09 a
// linha SELECIONADA e uma superficie CLARA (ver o comentario do preenchimento
// em streams.c): arte branca sobre linha branca nao se ve. Relato do dono:
// "nas fontes as badges inferiores ... nao mudaram de cor para preto quando
// selecionadas".
//
// DA PARA TINGIR porque as 42 artes de badges/ sao MONOCROMATICAS BRANCAS — a
// forma mora no alfa e o RGB e 255 em todas (conferido arquivo por arquivo).
// Entao o escuro vai por GFX_MARCA, que pega a forma do alfa e a cor de uCor.
//
// O CLARO CONTINUA EM GFX_TEXTO de proposito. Os dois modos dao o mesmo pixel
// para arte branca opaca, mas divergem na borda reduzida (o GFX_TEXTO filtra o
// RGB junto, o GFX_MARCA nao), e o claro e o que detail.c, home.c e vertudo.c
// ja desenham hoje. Trocar o modo deles nao e o defeito relatado.
static float fileira(uint64_t mask,float x,float y,float maxW,float h,float a,int escuro) {
  float start=x;
  // A TINTA ESCURA E A SUPERFICIE DE REPOUSO da linha nao selecionada
  // (.135,.135,.14 em streams.c). Sobre a linha clara ela e a tinta que o app
  // ja usa, e nao um cinza inventado so para esta fileira.
  float cr=escuro?.135f:1.f,cg=escuro?.135f:1.f,cb=escuro?.14f:1.f;
  int t1=escuro?26:225,t2=escuro?28:228,t3=escuro?34:235;
  for(size_t i=0;i<NB;i++)if((mask&(UINT64_C(1)<<i))&&art[i].image[0]) {
    GLuint t=tex_obter_larg(art[i].image,128);float aspect=tex_aspecto(art[i].image),w=aspect>0?h*aspect:80;
    if(w>144)w=144;if(x+w>start+maxW)break;
    if(t&&aspect>0){float height=w/aspect;gfx_rect((GfxRect){x,y+(h-height)*.5f,w,height},t,escuro?GFX_MARCA:GFX_TEXTO,0,0,0,0,cr,cg,cb,a);}
    else {TxtLinha l=txt_linha_corta(TXT_MINI,art[i].name,t1,t2,t3,255,w);txt_desenhar_alpha(l,x,y+(h-l.h)*.5f,a);}
    x+=w+14;
  }return x-start;
}
float badges_desenhar(uint64_t mask,float x,float y,float maxW,float h,float a) {
  return fileira(mask,x,y,maxW,h,a,0);
}
float badges_desenhar_escura(uint64_t mask,float x,float y,float maxW,float h,float a) {
  return fileira(mask,x,y,maxW,h,a,1);
}

// ROTULO COM A PALAVRA DO FORMATO TROCADA PELA MARCA ("Sem HDR" -> "Sem" +
// logo). O rotulo e traduzido ANTES de procurar a palavra: "Ohne HDR", "Без HDR"
// e "No HDR" saem do mesmo caminho, sem cravar a lingua. Sem a palavra na
// traducao, ou sem espaco, cai no texto puro.
static int partesRotulo(const char *rotulo, FormatoMarca f, char *ante, size_t na,
                        char *depois, size_t nd) {
  const char *t = i18n(rotulo), *nome = marca_formato_nome(f), *p;
  size_t ln = strlen(nome);
  ante[0] = depois[0] = 0;
  if (!ln || !(p = strstr(t, nome))) return 0;
  snprintf(ante, na, "%.*s", (int)(p - t), t);
  snprintf(depois, nd, "%s", p + ln);
  // Os espacos ao redor da palavra viram o respiro da marca (6 px).
  { size_t k = strlen(ante); while (k && ante[k - 1] == ' ') ante[--k] = 0; }
  { char *q = depois; while (*q == ' ') q++; memmove(depois, q, strlen(q) + 1); }
  return 1;
}
float marca_rotulo_largura(TxtEstilo est, const char *rotulo, FormatoMarca f, float altura) {
  char a[96], d[96];
  float w = 0.0f;
  if (!partesRotulo(rotulo, f, a, sizeof a, d, sizeof d))
    return (float)txt_linha(est, rotulo, 0, 0, 0, 255).w;
  if (a[0]) w += (float)txt_linha(est, a, 0, 0, 0, 255).w + 6.0f;
  w += marca_formato_largura(f, altura);
  if (d[0]) w += (float)txt_linha(est, d, 0, 0, 0, 255).w + 6.0f;
  return w;
}
float marca_rotulo(TxtEstilo est, const char *rotulo, FormatoMarca f, float x, float y,
                   float altura, int tinta, float a) {
  char an[96], de[96];
  float k = (float)tinta / 255.0f, x0 = x;
  if (!partesRotulo(rotulo, f, an, sizeof an, de, sizeof de)) {
    TxtLinha l = txt_linha(est, rotulo, tinta, tinta, tinta, 255);
    txt_desenhar_alpha(l, x, y, a);
    return (float)l.w;
  }
  {
    TxtLinha ref = txt_linha(est, an[0] ? an : de[0] ? de : "H", tinta, tinta, tinta, 255);
    float cy = y + (float)ref.h * 0.5f;
    if (an[0]) { txt_desenhar_alpha(ref, x, y, a); x += (float)ref.w + 6.0f; }
    x += marca_formato(f, x, cy - altura * 0.5f, altura, k, k, k, a);
    if (de[0]) {
      TxtLinha l = txt_linha(est, de, tinta, tinta, tinta, 255);
      x += 6.0f;
      txt_desenhar_alpha(l, x, y, a);
      x += (float)l.w;
    }
  }
  return x - x0;
}

// --- SELOS DE TEXTO ----------------------------------------------------------
float badge_largura(const char *texto) {
  TxtLinha l = txt_linha(TXT_CAPTION2, texto, 0, 0, 0, 255);
  return (float)l.w + BADGE_PADX * 2.0f;
}
float badge_desenhar(float x, float y, const char *texto, BadgeEstilo estilo, float a) {
  float fr = 0.10f, fg = 0.11f, fb = 0.13f, fa = 0.85f;
  int c = 235;
  TxtLinha l;
  GfxRect p;
  switch (estilo) {
    case BADGE_APAGADO: c = 170; break;
    case BADGE_REALCE:
      // 18 % da cor de realce sobre o painel escuro: le como "aceso" sem
      // disputar com a linha em foco, que e realce cheio.
      ajustes_acento(&fr, &fg, &fb); fa = 0.18f; break;
    case BADGE_REALCE_CHEIO:
      c = (int)(ajustes_acento_tinta(&fr, &fg, &fb) * 255.0f + 0.5f); fa = 1.0f; break;
    case BADGE_SOBRE_REALCE: {
      float t = ajustes_acento_tinta(NULL, NULL, NULL);
      fr = fg = fb = t; fa = 0.12f; c = ajustes_tinta_foco2(); break; }
    default: break;
  }
  l = txt_linha(TXT_CAPTION2, texto, c, c, c, 255);
  p.x = x; p.y = y; p.w = (float)l.w + BADGE_PADX * 2.0f; p.h = BADGE_H;
  gfx_cor(p, 0.5f, fr, fg, fb, fa * a);
  txt_desenhar_alpha(l, x + BADGE_PADX, y + (BADGE_H - (float)l.h) * 0.5f, a);
  return p.w;
}
static void notaTexto(char *dst, size_t n, int nota) {
  // Separador decimal pelo idioma: "8,4" em portugues, "8.4" em ingles — em
  // ingles a virgula le como milhar interrompido (ver recomenda.c).
  snprintf(dst, n, ajustes_idioma_ingles() ? "%d.%d" : "%d,%d", nota / 10, nota % 10);
}
float badge_imdb_largura(int nota) {
  char t[8]; TxtLinha l;
  if (nota <= 0) return 0.0f;
  notaTexto(t, sizeof t, nota);
  l = txt_linha(TXT_CAPTION2, t, 0, 0, 0, 255);
  return BADGE_IMDB_W + BADGE_IMDB_GAP + (float)l.w;
}
float badge_imdb(float x, float y, int nota, int escuro, float a) {
  char t[8]; TxtLinha l, lm; GfxRect m;
  int c = escuro ? ajustes_tinta_foco() : 235;
  if (nota <= 0) return 0.0f;
  notaTexto(t, sizeof t, nota);
  l = txt_linha(TXT_CAPTION2, t, c, c, c, 255);
  m.x = x; m.y = y; m.w = BADGE_IMDB_W; m.h = BADGE_H;
  // #F5C518 e o amarelo da marca; o texto e preto porque e assim que ela
  // existe — nao segue o realce nem a tinta da linha.
  gfx_cor(m, BADGE_IMDB_R / BADGE_H, 0.961f, 0.773f, 0.094f, a);
  lm = txt_linha(TXT_MINI, "IMDb", 10, 10, 10, 255);
  txt_desenhar_alpha(lm, m.x + (m.w - (float)lm.w) * 0.5f, m.y + (m.h - (float)lm.h) * 0.5f, a);
  txt_desenhar_alpha(l, x + BADGE_IMDB_W + BADGE_IMDB_GAP, y + (BADGE_H - (float)l.h) * 0.5f, a);
  return BADGE_IMDB_W + BADGE_IMDB_GAP + (float)l.w;
}
