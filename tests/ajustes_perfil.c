// AJUSTES POR PERFIL NESTA TV (ajustes_perfil_guardar/_restaurar/_esquecer).
//
// A troca de perfil guarda os ajustes do perfil que sai e devolve os do que
// entra (sync_trocar_perfil). O que este teste prova, sem SDL na tela:
//   1. a copia leva o que e DO PERFIL e volta igual;
//   2. o que e DESTE APARELHO (superficie 4K) nao entra na copia: trocar de
//      perfil nunca muda o que descreve a TV;
//   3. perfil sem copia devolve 0 e nao mexe em nada (quem chama decide partir
//      do principal);
//   4. o logout apaga as copias.
#include "../src/ajustes.c"
#include <assert.h>
#include <unistd.h>

int main(void) {
  char dir[] = "/tmp/nuvio-aj-perfil-XXXXXX";
  assert(mkdtemp(dir));
  setenv("NUVIO_DADOS", dir, 1);
  dados_iniciar(dir);
  ajustes_dir(dir);
  // Sem ajustes.txt, o ajuste novo nasce LIGADO (V_LIGA: 0 = Ligado).
  assert(ajustes_addons_do_principal() == 1);

  // Perfil 1: destaque desligado, 4K pedido.
  valor[AJ_HERO] = 1;
  valor[AJ_RESOLUCAO] = 1;
  valor[AJ_ADDONS_PRINCIPAL] = 0;
  ajustes_perfil_guardar(1);

  // O perfil 2 mexe nas duas coisas.
  valor[AJ_HERO] = 0;
  valor[AJ_RESOLUCAO] = 0;
  valor[AJ_ADDONS_PRINCIPAL] = 1;
  ajustes_perfil_guardar(2);

  // Volta ao 1: o destaque dele volta, a TV continua como esta.
  assert(ajustes_perfil_restaurar(1) == 1);
  assert(valor[AJ_HERO] == 1);
  assert(valor[AJ_RESOLUCAO] == 0);
  assert(valor[AJ_ADDONS_PRINCIPAL] == 1);   // ajuste desta TV, nao do perfil

  // E o 2 de novo.
  assert(ajustes_perfil_restaurar(2) == 1);
  assert(valor[AJ_HERO] == 0);

  // Perfil nunca usado aqui: nada muda.
  valor[AJ_HERO] = 1;
  assert(ajustes_perfil_restaurar(3) == 0);
  assert(valor[AJ_HERO] == 1);

  // A copia nao carrega linha de aparelho nem chave local com "-".
  { char *t = dados_ler("ajustes-p1.txt");
    assert(t);
    assert(!strstr(t, CHAVE[AJ_RESOLUCAO]));
    assert(!strstr(t, CHAVE[AJ_ADDONS_PRINCIPAL]));
    assert(!strstr(t, "\n-") && t[0] != '-');
    assert(strstr(t, CHAVE[AJ_HERO]));
    free(t); }

  // Logout: nenhuma copia sobra.
  ajustes_perfil_esquecer();
  assert(ajustes_perfil_restaurar(1) == 0);
  assert(ajustes_perfil_restaurar(2) == 0);

  puts("ajustes_perfil: tudo ok");
  return 0;
}
