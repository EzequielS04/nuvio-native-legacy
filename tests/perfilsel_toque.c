// Selecionar perfil por toque usa a mesma regra de PIN do controle remoto.
#include <assert.h>
#include "../src/perfilsel.c"

static ContaPerfil ps[2] = {{.indice = 1}, {.indice = 2, .temPin = 1}};
static int selecionado = 1, total = 2;
const ContaPerfil *perfis_item(int i) { return i >= 0 && i < total ? &ps[i] : NULL; }
int perfis_n(void) { return total; }
PerfilAcao perfis_acao(int i) { return ps[i].temPin ? PERFIL_ACAO_PIN : PERFIL_ACAO_ENTRAR; }
void perfis_definir_ativo(int i) { selecionado = i; }
int perfis_verificar_pin(int i, const char *p) { (void)i; (void)p; return 0; }
SyncEstado sync_estado(void) { return SYNC_FALHOU; }
int main(void) {
  foco = 0; pinDe = -1; preparando = verificando = concluido = 0;
  perfilFocar(1, 2); assert(foco == 1);
  perfilAtivar(1, 2); assert(pinDe == 1 && selecionado == 1 && !concluido);
  perfilAtivar(0, 1); assert(selecionado == 1 && pinDe == 1 && !concluido);
  pinFocar(0, 1); eventoPin(SDLK_RETURN); assert(!strcmp(pin, "1"));
  pinFocar(PS_PIN_ZERO, 1); eventoPin(SDLK_RETURN); assert(!strcmp(pin, "10"));
  pinFocar(PS_PIN_APAGAR, 1); eventoPin(SDLK_RETURN); assert(!strcmp(pin, "1"));
  verificando = 1; int anterior = pinFoco;
  pinFocar(3, 1); assert(pinFoco == anterior);
  verificando = 0; eventoPin(SDLK_ESCAPE); eventoPin(SDLK_ESCAPE); assert(pinDe == -1);
  preparando = 1; perfilAtivar(0, 1); assert(!concluido);
  preparando = 0; perfilAtivar(0, 2); assert(!concluido); // alvo antigo/identidade trocada
  perfilAtivar(0, 1); assert(concluido && selecionado == 1);
  repetir = 0; perfisRetentar(0, 0); assert(!repetir);
  total = 0; perfisRetentar(0, 0); assert(repetir);
  puts("perfilsel toque: PIN, bloqueio durante preparo, identidade e retentativa ok");
}
