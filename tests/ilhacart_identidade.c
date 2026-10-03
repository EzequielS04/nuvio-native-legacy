// Estado real de ilhacart + ilha, com identidade/catalogo/timer dublados.
// Nenhum renderer, conta, disco, URL ou backend real. A inclusao da ilha
// permite conferir suas copias de transicao alem do contrato do modal.
#include "../src/ilha.c"
#include "ilhacart.h"
#include "catalogo.h"
#include "agenda.h"
#include "recomenda.h"
#include <assert.h>

static Uint32 agora = 1000;
static int logada = 1, perfil = 1, vistos;
static const char *conta = "fixture-a";
static CatItem titulo;
int anim_politica_reduzida;
Uint32 SDL_GetTicks(void) { return agora; }
int sessao_logada(void) { return logada; }
const char *sessao_usuario(void) { return conta; }
int perfis_ativo(void) { return perfil; }
int ajustes_relogio_ligado(void) { return 1; }
int ajustes_animacoes_reduzidas(void) { return 0; }
GLuint tex_obter_larg_qualquer(const char *c, float l) { (void)c; (void)l; return 0; }
const char *i18n(const char *s) { return s; }
int recomenda_ativo(void) { return 0; }
int recomenda_feed_n(void) { return 0; }
int recomenda_feed_item(int i, RecEvento *saida) { (void)i; (void)saida; return 0; }
int home_retorno_vale(int idx, double pos, double dur) {
  return idx == 0 && dur > 1 && pos / dur >= .01 && pos / dur < .9;
}
const CatItem *cat_item(int idx) { return idx == 0 ? &titulo : NULL; }
int cat_n_episodios(int idx) { (void)idx; return 0; }
const CatEp *cat_episodio(int idx, int ep) { (void)idx; (void)ep; return NULL; }
int cat_indice_por_imdb(const char *id) { return !strcmp(id, titulo.imdb) ? 0 : -1; }
const AgItem *agenda_registro(const char *id) { (void)id; return NULL; }
int agenda_dias(const char *d) { (void)d; return 0; }
int avisos_estreia_pendente(char *id, size_t ti, char *imdb, size_t tm) {
  (void)id; (void)ti; (void)imdb; (void)tm; return 0;
}
void avisos_marcar_visto(const char *id) { (void)id; vistos++; }

static IlhaCartao criar(int t, int e) {
  snprintf(titulo.imdb, sizeof titulo.imdb, "fixture-titulo");
  snprintf(titulo.titulo, sizeof titulo.titulo, "Titulo ficticio");
  snprintf(titulo.tipo, sizeof titulo.tipo, "%s", t ? "series" : "movie");
  ilhacart_player_saiu(0, 600, 3600, t, e);
  assert(temCartao[ILHA_VIVO]);
  IlhaCartao c = cartoes[ILHA_VIVO];
  assert(ilhacart_vivo_vale(&c));
  return c;
}
static void abrirModal(int qual) {
  relogioQuer = 1; cartaoVez = qual;
  mostra = M_CARTAO; mostraQual = qual; mostraC = cartoes[qual]; conteudoA = 1;
  assert(ilha_modal_abrir()); modalT = 1;
}
static void pedirRetomar(void) {
  SDL_Event ev = {0}; ev.type = SDL_KEYDOWN; ev.key.keysym.sym = SDLK_RETURN;
  assert(ilha_evento(&ev));
}
static void conferirRemovido(const IlhaCartao *antigo) {
  assert(!ilhacart_vivo_vale(antigo) && !temCartao[ILHA_VIVO]);
  assert(!cartoes[ILHA_VIVO].imdb[0] && !mostraC.imdb[0]);
  assert(!ilha_modal_visivel() && !modalC.imdb[0]);
  assert(!ilha_minimizando() && !vooArte[0] && !vooCapa[0]);
  assert(!ilha_pediu(NULL, NULL));
}
int main(void) {
  IlhaCartao c = criar(0, 0), nova;
  for (int i = 0; i < 600; i++) ilhacart_validar_identidade();
  assert(ilhacart_vivo_vale(&c));
  puts("ok mesma identidade (inclusive servidor indisponivel) conserva cartao");

  // O backend retido vence em 2 min; este cartao continua com a regra de
  // 30 min sem tecla, inclusive a fronteira estrita que ja existia.
  ilhacart_atualizar(agora + 120001, NULL); assert(ilhacart_vivo_vale(&c));
  ilhacart_atualizar(agora + 1800000, NULL); assert(ilhacart_vivo_vale(&c));
  ilhacart_atualizar(agora + 1800001, NULL); assert(!temCartao[ILHA_VIVO]);
  puts("ok regra de 30 min preservada e independente da retencao de 2 min");

  // Pilula ociosa/inativa pode ter deixado o modal e um pedido ja copiados.
  // A troca de conta deve invalidar essas copias mesmo sem temCartao/Vivo.
  c = criar(0, 0); abrirModal(ILHA_VIVO); pedirRetomar();
  ilhacart_atualizar(agora + 1800001, NULL); assert(!temCartao[ILHA_VIVO]);
  assert(ilhacart_vivo_vale(&c)); // a copia aberta ainda e desta identidade
  conta = "fixture-b"; ilhacart_validar_identidade(); conferirRemovido(&c);
  conta = "fixture-a";
  puts("ok conta mudou apos ociosidade: modal/pedido/arte antigos tambem saem");

  c = criar(0, 0); abrirModal(ILHA_VIVO); pedirRetomar(); perfil = 2;
  ilhacart_player_saiu(-1, 600, 3600, 0, 0); conferirRemovido(&c);
  perfil = 1;
  puts("ok saida invalida apos troca tambem confere identidade antiga");

  c = criar(0, 0);
  abrirModal(ILHA_VIVO); pedirRetomar();
  assert(pedido == ILHA_PEDIU_TOCAR); assert(ilha_minimizar(NULL));
  ilha_avisar("fixture-aviso", ILHA_OK, "", "Aviso ficticio", 5000, 0);
  IlhaCartao ep = {0}; snprintf(ep.chave, sizeof ep.chave, "estreia:fixture");
  snprintf(ep.imdb, sizeof ep.imdb, "fixture-estreia"); ilha_cartao(ILHA_ESTREIA, &ep);
  perfil = 2; ilhacart_validar_identidade(); conferirRemovido(&c);
  assert(temCartao[ILHA_ESTREIA] && ilha_tem("fixture-aviso") && !vistos);
  nova = criar(0, 0); assert(!ilhacart_vivo_vale(&c)); assert(strcmp(c.chave, nova.chave));
  puts("ok perfil A-B apaga cartao/modal/pedido/voo; mesmo IMDb nao revive copia antiga");

  c = criar(1, 3); abrirModal(ILHA_VIVO); pedirRetomar();
  perfil = 1; ilhacart_validar_identidade(); conferirRemovido(&c);
  nova = criar(1, 3); assert(!ilhacart_vivo_vale(&c) && ilhacart_vivo_vale(&nova));
  puts("ok mesmo IMDb/episodio entre perfis conserva somente nova instancia");

  abrirModal(ILHA_VIVO); pedirRetomar(); logada = 0;
  ilhacart_validar_identidade(); conferirRemovido(&nova);
  ilhacart_player_saiu(0, 600, 3600, 1, 3); assert(!temCartao[ILHA_VIVO]);
  logada = 1; conta = "fixture-b"; c = criar(1, 3);
  assert(!ilhacart_vivo_vale(&nova));
  conta = "fixture-a"; ilhacart_validar_identidade(); conferirRemovido(&c);
  puts("ok logout/login A-B e mudanca direta de conta nao reutilizam cartao");

  criar(0, 0); abrirModal(ILHA_ESTREIA); pedirRetomar();
  assert(pedidoQual == ILHA_ESTREIA);
  // Reabre para conferir tambem o modal da estreia, alem de seu pedido.
  abrirModal(ILHA_ESTREIA); perfil = 2; ilhacart_validar_identidade();
  assert(ilha_modal_visivel() && !strcmp(modalC.imdb, ep.imdb));
  assert(!strcmp(mostraC.imdb, ep.imdb) && temCartao[ILHA_ESTREIA]);
  assert(ilha_pediu(&c, &perfil) == ILHA_PEDIU_TOCAR && perfil == ILHA_ESTREIA);
  assert(!strcmp(c.imdb, ep.imdb) && ilha_tem("fixture-aviso") && !vistos);
  puts("ok estreia/modal/pedido e aviso preservados na invalidacao de ILHA_VIVO");
  puts("ilhacart_identidade: tudo ok"); return 0;
}
