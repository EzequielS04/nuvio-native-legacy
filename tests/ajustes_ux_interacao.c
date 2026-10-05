// Exercita o contrato do controle com persistência real numa pasta descartável.
#include "../src/ajustes.c"
#include <assert.h>
#include <unistd.h>

static void key(SDL_Keycode k) {
  SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  ajustes_evento(&e);
}
static char *arquivo(void) {
  char path[700]; long n; FILE *f; char *b;
  snprintf(path, sizeof path, "%s/ajustes.txt", dirAjustes);
  f = fopen(path, "rb"); assert(f);
  fseek(f, 0, SEEK_END); n = ftell(f); rewind(f);
  b = calloc((size_t)n + 1, 1); assert(b);
  assert(fread(b, 1, n, f) == (size_t)n); fclose(f); return b;
}
static void igual(const char *antes) { char *depois = arquivo(); assert(!strcmp(antes, depois)); free(depois); }
static void abrir(int op) { focarOpcao(op); key(SDLK_RETURN); assert(uxEditor); }

int main(void) {
  char dir[1024], bad[1400]; char *antes;
  int original, foco, n, i;
  snprintf(dir, sizeof dir, "%s/nuvio-ajustes-interacao-XXXXXX", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
  assert(mkdtemp(dir)); assert(SDL_Init(SDL_INIT_TIMER) == 0);
  assert(setenv("NUVIO_DADOS", dir, 1) == 0);
  dados_iniciar(dir); assert(!strcmp(dados_dir(), dir)); snprintf(dirAjustes, sizeof dirAjustes, "%s", dir);
  ajustes_iniciar(); assert(gravar());

  // Glass and outline are reachable without advanced settings, even when off.
  assert(!uxAvancada(AJ_VIDRO) && !uxAvancada(AJ_VIDRO_CONTORNO));
  original = valor[AJ_VIDRO]; valor[AJ_VIDRO] = 1;
  for (i = 0; i < AJ_N_TELA; i++) {
    if (TELA[i].tipo != IT_OPC ||
        (TELA[i].op != AJ_VIDRO && TELA[i].op != AJ_VIDRO_CONTORNO)) continue;
    valor[AJ_AVANCADAS] = 1;   // global advanced toggle Desligado
    assert(visivel(i) && focavel(i));
  }
  assert(inativa(AJ_VIDRO_CONTORNO));
  assert(uxRequisito(AJ_VIDRO_CONTORNO) == AJ_VIDRO);
  assert(strstr(ajudaOpcao(AJ_VIDRO_CONTORNO), "Ative a interface de vidro"));
  antes = arquivo(); abrir(AJ_VIDRO_CONTORNO);
  assert(uxEditor == 2); igual(antes);
  key(SDLK_ESCAPE); igual(antes); free(antes);
  valor[AJ_VIDRO] = original;

  // Interruptor (Ligado/Desligado): OK troca e grava na hora, sem editor nem
  // confirmacao; sem ajuste de risco, nunca abre o aviso.
  valor[AJ_RELOGIO] = 0; assert(gravar()); assert(ehInterruptor(AJ_RELOGIO));
  focarOpcao(AJ_RELOGIO); antes = arquivo();
  key(SDLK_RETURN);
  assert(!uxEditor && !uxAvisoRisco && !uxRestaurar && valor[AJ_RELOGIO] == 1);
  { char *depois = arquivo(); assert(strcmp(antes, depois)); free(depois); } free(antes);
  assert(strstr(uxAviso, "salvo"));
  key(SDLK_RETURN); assert(!uxEditor && valor[AJ_RELOGIO] == 0);
  // Falha de disco: o valor anterior fica e o aviso diz que nao salvou.
  { char falho[1400]; snprintf(falho, sizeof falho, "%s/ausente", dir); snprintf(dirAjustes, sizeof dirAjustes, "%s", falho);
    key(SDLK_RETURN); assert(valor[AJ_RELOGIO] == 0 && !uxEditor); assert(strstr(uxAviso, "salvar"));
    snprintf(dirAjustes, sizeof dirAjustes, "%s", dir); }
  // Interruptor inativo (depende de outro) continua abrindo o aviso de requisito.
  valor[AJ_VIDRO] = 1; focarOpcao(AJ_VIDRO_CONTORNO); key(SDLK_RETURN); assert(uxEditor == 2); key(SDLK_ESCAPE);
  valor[AJ_VIDRO] = 0;
  focarOpcao(AJ_RELOGIO);

  // #238: opcao inativa explica a dependencia DELA, nunca a da profundidade.
  { int i0 = valor[AJ_FONTE_MANUAL], r0 = valor[AJ_RELOGIO], s0 = valor[AJ_SEEKR_LIGADO];
    int ops[] = { AJ_SAIDA_PLAYER, AJ_SEEKR_FITA, AJ_SEEKR_AJUSTE };
    valor[AJ_FONTE_MANUAL] = 0; valor[AJ_RELOGIO] = 1; valor[AJ_SEEKR_LIGADO] = 1;  // 0 = ligado (lig)
    for (i = 0; i < (int)(sizeof ops / sizeof ops[0]); i++) {
      assert(inativa(ops[i]));
      assert(!strstr(ajudaOpcao(ops[i]), "profundidade"));
    }
    assert(!inativa(AJ_FONTE_PRAZO));   /* #238: sem dependencia, nem da profundidade nem de Escolher a fonte */
    valor[AJ_FONTE_MANUAL] = 1; assert(!inativa(AJ_FONTE_PRAZO)); valor[AJ_FONTE_MANUAL] = 0;
    assert(strstr(ajudaOpcao(AJ_SAIDA_PLAYER), "Relógio na tela"));
    assert(strstr(ajudaOpcao(AJ_SEEKR_FITA), "Miniaturas na barra de tempo"));
    valor[AJ_FONTE_MANUAL] = i0; valor[AJ_RELOGIO] = r0; valor[AJ_SEEKR_LIGADO] = s0;
    for (i = 0; i < AJ_N; i++)
      if (inativa(i) && strstr(ajudaOpcao(i), "profundidade"))
        assert(!ajustes_profundidade());
  }

  // Foco e rascunho (escolha, nao interruptor) não alteram nem o valor nem o arquivo; Back cancela.
  valor[AJ_HOME_LAYOUT] = 0; assert(gravar());
  original = valor[AJ_HOME_LAYOUT]; antes = arquivo(); abrir(AJ_HOME_LAYOUT);
  key(SDLK_DOWN); assert(uxPendente != original); assert(valor[AJ_HOME_LAYOUT] == original); igual(antes);
  SDL_Event repetida = {0}; repetida.type = SDL_KEYDOWN;
  repetida.key.keysym.sym = SDLK_RETURN; repetida.key.repeat = 1;
  ajustes_evento(&repetida); assert(uxEditor && valor[AJ_HOME_LAYOUT] == original); igual(antes);
  key(SDLK_ESCAPE); assert(!uxEditor); assert(valor[AJ_HOME_LAYOUT] == original); igual(antes); free(antes);
  abrir(AJ_HOME_LAYOUT); key(SDLK_DOWN); key(SDLK_RETURN);
  assert(!uxEditor && valor[AJ_HOME_LAYOUT] != original);
  antes = arquivo(); key(SDLK_LEFT); assert(focoIndice); igual(antes); free(antes);

  // Número só aplica o candidato final, sem gravar cada passo.
  original = valor[AJ_LARGURA_DP]; antes = arquivo(); abrir(AJ_LARGURA_DP);
  key(SDLK_RIGHT); key(SDLK_RIGHT); key(SDLK_RIGHT);
  assert(valor[AJ_LARGURA_DP] == original); igual(antes); free(antes);
  key(SDLK_RETURN); assert(valor[AJ_LARGURA_DP] == original + 3 * OPCOES[AJ_LARGURA_DP].passo);

  // Eixo horizontal, limites, cancelamento e estado visual continuo.
  abrir(AJ_LARGURA_DP); original = valor[AJ_LARGURA_DP];
  key(SDLK_LEFT); assert(uxPendente == original-OPCOES[AJ_LARGURA_DP].passo);
  key(SDLK_DOWN); assert(uxRodape); assert(uxPendente == original-OPCOES[AJ_LARGURA_DP].passo);
  key(SDLK_UP); assert(!uxRodape);
  for (i=0;i<1000;i++) key(SDLK_LEFT);
  assert(uxPendente == OPCOES[AJ_LARGURA_DP].min);
  for (i=0;i<1000;i++) key(SDLK_RIGHT);
  assert(uxPendente == OPCOES[AJ_LARGURA_DP].max);
  key(SDLK_ESCAPE); assert(valor[AJ_LARGURA_DP] == original);
  valor[AJ_ANIM] = 0;
  ajMovPronto[AJ_RELOGIO] = 0; valor[AJ_RELOGIO] = 0;
  assert(ajMov(AJ_RELOGIO,0) == 0);
  valor[AJ_RELOGIO] = 1; ajMovAtualizar(0.016f);
  assert(ajMovValor[AJ_RELOGIO] > 0 && ajMovValor[AJ_RELOGIO] < 1);
  float meio = ajMovValor[AJ_RELOGIO];
  valor[AJ_RELOGIO] = 0; ajMovAtualizar(0.016f);
  assert(ajMovValor[AJ_RELOGIO] < meio && ajMovValor[AJ_RELOGIO] > 0);
  valor[AJ_ANIM] = 1; valor[AJ_RELOGIO] = 1; ajMovAtualizar(0.016f);
  assert(ajMovValor[AJ_RELOGIO] == 1); valor[AJ_ANIM] = 0;

  // Diferenças permanecem estáveis após restaurar; confirmação começa em Cancelar.
  uxListarDiferencas(); n = uxNDifs; assert(n >= 2);
  for (i = 0; i < uxNDifs && uxDifs[i] != AJ_LARGURA_DP; i++) { }
  assert(i < uxNDifs);
  uxIndice = 1; focoIndice = 0; uxDifFoco = i; foco = i;
  key(SDLK_RETURN); key(SDLK_DOWN); key(SDLK_RETURN);
  assert(uxRestaurar && !uxConfirmar); antes = arquivo();
  key(SDLK_RETURN); assert(uxEditor && !uxRestaurar); igual(antes); free(antes);
  key(SDLK_RETURN); assert(uxRestaurar); key(SDLK_LEFT); key(SDLK_RETURN);
  assert(!uxEditor && valor[AJ_LARGURA_DP] == valorPadrao[AJ_LARGURA_DP]);
  assert(uxIndice == 1 && uxDifFoco == foco && uxNDifs == n);

  // Falha de disco deixa o editor aberto e preserva o valor anterior.
  valor[AJ_HOME_LAYOUT] = 0; abrir(AJ_HOME_LAYOUT); original = valor[AJ_HOME_LAYOUT]; key(SDLK_DOWN);
  snprintf(bad, sizeof bad, "%s/ausente", dir); snprintf(dirAjustes, sizeof dirAjustes, "%s", bad);
  key(SDLK_RETURN); assert(uxEditor && valor[AJ_HOME_LAYOUT] == original); assert(strstr(uxAviso, "salvar"));
  snprintf(dirAjustes, sizeof dirAjustes, "%s", dir); key(SDLK_ESCAPE);

  // Mudança externa durante edição exige rever o candidato antes de aplicar.
  valor[AJ_HOME_LAYOUT] = 0; abrir(AJ_HOME_LAYOUT); key(SDLK_DOWN); valor[AJ_HOME_LAYOUT] = 2; assert(gravar());
  key(SDLK_RETURN); assert(uxEditor && uxPendente == 2); assert(strstr(uxAviso, "outra tela")); key(SDLK_ESCAPE);

  // Avançado encontrado pela busca é revelado e alcançável, sem ciclos extras.
  ajustes_abrir_opcao(AJ_TEX_MB); ajustes_iniciar();
  assert(focoOp == AJ_TEX_MB && !focoIndice && visivel(focoItem));
  assert(lig(AJ_AVANCADAS));   // the deep link turned the global toggle on
  key(SDLK_ESCAPE); assert(ajustes_pediu_busca() == 2); assert(!ajustes_pediu_busca());
  assert(!uxVeioBusca);
  ajustes_abrir_opcao(AJ_TEX_MB); ajustes_iniciar(); assert(uxVeioBusca);
  ajustes_encerrar(); ajustes_iniciar(); assert(!uxVeioBusca);
  focarOpcao(AJ_TEX_MB);
  key(SDLK_LEFT); while (uxIndice > 0) key(SDLK_UP);
  key(SDLK_RETURN); assert(ajustes_pediu_busca()); assert(!ajustes_pediu_busca());

  // Dependência: abrir requisito e voltar retorna à opção original.
  valor[AJ_HERO_TRAILER] = 1; abrir(AJ_HERO_TRAILER_ESPERA); assert(uxEditor == 2);
  key(SDLK_RETURN); assert(focoOp == AJ_HERO_TRAILER && !uxEditor);
  key(SDLK_ESCAPE); assert(focoOp == AJ_HERO_TRAILER_ESPERA && !focoIndice);

  abrir(AJ_HERO_TRAILER_ESPERA); key(SDLK_RETURN); key(SDLK_DOWN);
  assert(uxRetornarOp == -1); key(SDLK_ESCAPE); assert(focoIndice);

  // Categoria lembra a última opção; esconder avançados nunca deixa foco oculto.
  focarOpcao(AJ_TEX_MB); int sec = secAtual;
  focarSecao(0); focarSecao(sec); assert(focoOp == AJ_TEX_MB);
  { int ti = focoItem; (void)sec;
    // Global toggle chip (top of the index): RIGHT from "Diferentes", OK flips it.
    focoIndice = 1; uxIndice = 1; key(SDLK_RIGHT); assert(uxChipAv && focoIndice);
    key(SDLK_RETURN); assert(!lig(AJ_AVANCADAS) && !visivel(ti) && !focavel(ti));
    { char *f = arquivo(); assert(strstr(f, "avancadasLocal 1")); free(f); }
    key(SDLK_RETURN); assert(lig(AJ_AVANCADAS) && visivel(ti));
    key(SDLK_LEFT); assert(!uxChipAv);
    // Advanced stays hidden in EVERY category while off, visible in every one while on.
    valor[AJ_AVANCADAS] = 1; assert(!visivel(ti));
    for (i = 0; i < AJ_N_TELA; i++) if (TELA[i].tipo == IT_OPC && uxAvancada(TELA[i].op)) assert(!visivel(i));
    valor[AJ_AVANCADAS] = 0;
    for (i = 0; i < AJ_N_TELA; i++) if (TELA[i].tipo == IT_OPC && uxAvancada(TELA[i].op)) assert(visivel(i) || TELA[i].op == AJ_ICONE_APP);
    focoIndice = 1; }   // focus on the index: the row rests (no hover)
  // Top pills in several rows (uxChipLinha, measured by the draw): Up/Down change
  // row, Left/Right stay inside it.
  { focoIndice = 1; uxChipAv = 0; uxChipLinha[0] = 0; uxChipLinha[1] = 1; uxChipLinha[2] = 1;
    uxIndice = 0; key(SDLK_RIGHT); assert(uxIndice == 0 && !uxChipAv);          // Buscar alone in its row
    key(SDLK_DOWN); assert(uxIndice == 1 && !uxChipAv);                         // down to Diferentes
    key(SDLK_RIGHT); assert(uxChipAv);                                          // same row as Avancadas
    key(SDLK_UP); assert(!uxChipAv && uxIndice == 0);                           // up to Buscar
    uxChipLinha[0] = 0; uxChipLinha[1] = 0; uxChipLinha[2] = 1;
    uxIndice = 1; uxChipAv = 0; key(SDLK_RIGHT); assert(!uxChipAv && uxIndice == 1);   // Avancadas is a row below
    key(SDLK_DOWN); assert(uxChipAv);
    key(SDLK_UP); assert(!uxChipAv && uxIndice == 0);
    uxChipLinha[0] = uxChipLinha[1] = uxChipLinha[2] = 0; uxIndice = 1; focoIndice = 1; }
  valor[AJ_ANIM] = 1; ajustes_atualizar(1.0f, SDL_GetTicks());
  assert(animItem[focoItem] == 0);

  // Memória: uma confirmação para o valor final; cancelar não salva.
  valor[AJ_ITENS_FILEIRA] = 0; abrir(AJ_ITENS_FILEIRA); key(SDLK_DOWN); key(SDLK_DOWN);
  antes = arquivo(); key(SDLK_RETURN); assert(uxAvisoRisco && !uxConfirmar); igual(antes);
  key(SDLK_ESCAPE); key(SDLK_ESCAPE); assert(valor[AJ_ITENS_FILEIRA] == 0); igual(antes); free(antes);
  abrir(AJ_ITENS_FILEIRA); key(SDLK_DOWN); key(SDLK_RETURN);
  key(SDLK_LEFT); key(SDLK_RETURN); assert(!uxEditor && valor[AJ_ITENS_FILEIRA] == 1);

  // Toda opcao visivel tem uma escolha explicita no mapa visual.
  for (i=0;i<AJ_N_TELA;i++) if (TELA[i].tipo == IT_OPC)
    assert(strcmp(ajVisualIcone(TELA[i].op), "aj_settings-2"));

  // Reabrir a tela preserva valores confirmados e não confirma rascunhos.
  int confirmado = valor[AJ_RELOGIO]; ajustes_encerrar(); ajustes_iniciar();
  assert(valor[AJ_RELOGIO] == confirmado);

  // Sair da tela descarta rascunho (Spotlight / troca de tela / fechamento).
  abrir(AJ_HOME_LAYOUT); original = valor[AJ_HOME_LAYOUT]; key(SDLK_DOWN);
  ajustes_encerrar(); assert(!uxEditor && valor[AJ_HOME_LAYOUT] == original);
  char path[700]; snprintf(path, sizeof path, "%s/ajustes.txt", dir); unlink(path);
  snprintf(path, sizeof path, "%s/ajustes-locais.txt", dir); unlink(path);
  rmdir(dir); SDL_Quit();
  puts("ajustes_ux_interacao: cenários OK"); return 0;
}
