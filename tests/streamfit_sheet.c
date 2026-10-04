// The actual production source sheet with only platform boundaries stubbed.
// No SDL initialization, drawing, network request, resolver or TV benchmark.
#include "streams.h"
static uint64_t fakeNow = UINT64_C(1700000000000);
static uint64_t fixtureNow(void) { return fakeNow; }
#define streamfit_agora_ms fixtureNow
#include "../src/streams.c"
#undef streamfit_agora_ms
#include <assert.h>

Uint32 SDL_GetTicks(void) { return (Uint32)fakeNow; }
float gfx_escala_ui(void) { return 1; }
const char *i18n(const char *s) { return s; }
int debrid_ativo(void) { return 0; }
int p2p_ativo(void) { return 0; }
int selospacote_ativo(void) { return -1; }
unsigned selospacote_versao(void) { return 1; }
int selospacote_casar(const char *const *p,int n,unsigned short *o,int m) { (void)p;(void)n;(void)o;(void)m;return 0; }
const char *ajustes_qualidade(void) { return "Automatica"; }
int ajustes_fonte_texto_addon(void) { return 0; }
int video_pode_forcar_sdr(void) { return 0; }
void video_forcar_sdr(void) {}
int player_aberto(void) { return 0; }
void ondever_apps_atualizar(void) {}
int ondever_n(const char *id) { (void)id;return 2; }
int ondever_item(const char *id,int ix,OndeVer *o) { (void)id;(void)ix;(void)o;return 0; }
int ondever_abrir(const char *nome) { (void)nome;return ONDE_INFO; }
TxtLinha txt_linha(TxtEstilo estilo,const char *s,int r,int g,int b,int a) {
  (void)estilo;(void)r;(void)g;(void)b;(void)a;TxtLinha l={0};l.w=(int)strlen(s)*8;return l;
}
static Stream source(int id,long mb,uint64_t exact,int h) {
  Stream s={0};s.altura=h;s.mp4=1;s.fileIdx=-1;s.tamanhoMB=mb;s.tamanhoBytes=exact;
  snprintf(s.url,sizeof s.url,"https://media.invalid/%d.mp4",id);
  snprintf(s.provedor,sizeof s.provedor,"Addon");snprintf(s.rotulo,sizeof s.rotulo,"Source %d",id);
  snprintf(s.arquivo,sizeof s.arquivo,"file-%d.mp4",id);return s;
}
static uint64_t sizeAt(int kbps) { return (uint64_t)kbps*1000*7200/8; }
static void expect(const int *expected,int count) {
  assert(nOrdem==count);for(int i=0;i<count;i++) assert(ordem[i]==expected[i]);
}
int main(void) {
  Stream sources[5]={source(0,50000,sizeAt(50000),2160),source(1,10000,sizeAt(10000),2160),
    source(2,10000,sizeAt(10000),2160),source(3,70000,0,2160),source(4,80000,sizeAt(50000),1080)};
  int speed[8]={20000,20000,20000,20000,20000,20000,20000,20000};
  streamfit_limpar();stream_definir_alvo("tt1");stream_fit_duracao("tt1",7200,SF_DUR_METADATA);
  stream_definir_lista(sources,5);stream_folha_abrir();
  const int original[]={0,3,1,2,4};expect(original,5);
  assert(stream_fit_folha_estado(0,NULL)==SF_DESCONHECIDA);
  streamfit_rede(1);assert(streamfit_diagnostico(1,"https://media.invalid/probe",speed,8,fakeNow)==8);
  montar(automaticaDaFolha());expect(original,5); // open sheet freezes unknown
  stream_folha_abrir();const int learned[]={3,1,2,0,4};expect(learned,5);
  assert(stream_fit_folha_estado(0,NULL)==SF_PESADA && stream_fit_folha_estado(3,NULL)==SF_DESCONHECIDA);
  assert(stream_n()==5 && stream_folha_n()==5 && stream_automatico()==0); // raw autoplay policy unchanged
  int focused=2;foco=linhaDe(focused);grupo=1;
  fakeNow++;int faster[8]={200000,200000,200000,200000,200000,200000,200000,200000};
  assert(streamfit_diagnostico(1,"https://media.invalid/probe",faster,8,fakeNow)==8);
  stream_fit_duracao("tt1",14400,SF_DUR_MEDIA); // cannot mutate an open sheet
  montar(automaticaDaFolha());expect(learned,5);assert(filtrado(foco)==focused);
  Stream late=source(5,20000,sizeAt(40000),2160);
  stream_lista_acrescentar(&late,1,0);
  stream_folha_atualizar(.016f,0);
  const int incremental[]={3,1,2,0,5,4};expect(incremental,6);
  assert(filtrado(foco)==focused && stream_fit_folha_estado(5,NULL)==SF_PESADA);
  // A service tab never enters the source partition or source-index space.
  filtro=-1;montar(automaticaDaFolha());const int services[]={-2,-3};expect(services,2);
  filtro=0;montar(automaticaDaFolha());expect(incremental,6);
  // Complete-list replacement keeps focus identity and uses the same photo.
  stream_atualizar_lista(sources,5);assert(filtrado(foco)==focused);expect(learned,5);
  stream_folha_abrir();assert(stream_fit_folha_estado(0,NULL)==SF_ADEQUADA);
  // Real media duration has precedence over delayed metadata.
  stream_fit_duracao("tt1",1,SF_DUR_METADATA);stream_folha_abrir();assert(fitFotoSeg==14400);
  stream_definir_alvo("tt2:1:1");stream_definir_lista(sources,5);stream_folha_abrir();
  assert(stream_fit_folha_estado(0,NULL)==SF_DESCONHECIDA); // episode has no runtime
  stream_fit_duracao("tt2:1:1",NAN,SF_DUR_MEDIA);stream_folha_abrir();assert(fitFotoSeg==0);
  streamfit_rede(2);stream_folha_abrir();assert(stream_fit_folha_estado(0,NULL)==SF_DESCONHECIDA);
  // The separate PRIMEIRA queue keeps the add-on's order and one probe.
  long points[3]={1,99,99};int queue[3];unsigned char excluded[3]={0};
  assert(fonteauto_fila(FONTEAUTO_PRIMEIRA,3,-1,points,NULL,excluded,3,queue)==3);
  assert(queue[0]==0 && queue[1]==1 && queue[2]==2 && fonteauto_tentativas(FONTEAUTO_PRIMEIRA,9)==1);
  stream_definir_lista(NULL,0);free(ordem);free(linhaY);free(linhaH);free(grupoTmp);free(grupoFitTmp);free(fitResultados);free(fitClasses);
  puts("streamfit sheet: PASS (stable groups, raw identities, freeze, incremental/focus, OndeVer, first-source)");return 0;
}
