// PROTECAO DE OLED: esmaecer a interface parada, e o brilho da interface do player.
//
// DOIS ASSUNTOS, UM ARQUIVO, porque os dois existem pelo mesmo motivo: pixel
// aceso e parado queima o OLED (a C9 do dono), e a interface do Nuvio e feita
// de coisas paradas (barra, logo, relogio, cartazes).
//
// 1. ESMAECER QUANDO PARADO. Fora da reproducao (Home, titulo, Ajustes, menus,
//    E o player com o video PAUSADO), depois de N minutos sem tecla a tela vai
//    a um veu preto de 60% (estagio 1) e, mais NO_FIM_MS depois, a 92% com um
//    relogio pequeno que anda devagar (estagio 2): nada parado fica aceso.
//    Qualquer tecla acorda NA HORA e e CONSUMIDA (nao age). Com video tocando de
//    verdade nunca esmaece. E uma passada so de tela cheia no fim do quadro.
//
// 2. BRILHO DA INTERFACE NO PLAYER. Multiplica a COR (nao o alfa: forma e
//    legibilidade ficam) do que o player desenha por cima do video; mais um
//    degrau automatico quando a barra fica parada com o filme tocando.
//
// A maquina de estados e as contas sao puras (sem SDL nem GL) para o teste.
#ifndef ESMAECER_H
#define ESMAECER_H

// Escolhas do ajuste (indices de V_ESMAECER): 0 desligado, 1 = 2 min, 2 = 5 min, 3 = 10 min.
#define ESM_ESCOLHAS 4
// Estagios.
#define ESM_ACESO   0
#define ESM_VEU     1   // ~60% preto
#define ESM_ESCURO  2   // ~92% preto + relogio andando
#define ESM_ALFA_VEU    0.60f
#define ESM_ALFA_ESCURO 0.92f
// Tempo entre o estagio 1 e o 2.
#define ESM_NO_FIM_MS   (3u * 60u * 1000u)
// Velocidade do esmaecer (alfa por segundo): 0 -> 60% em 1,5 s.
#define ESM_VELOCIDADE  0.40f

// Minutos ate o estagio 1 para a escolha (0 = nunca).
int  esmaecer_minutos(int escolha);
// PURA: estagio para `ocioMs` sem tecla.
int  esmaecer_estagio_para(unsigned ocioMs, int escolha);
float esmaecer_alfa_do_estagio(int estagio);

// Estado. `agora` em ms (SDL_GetTicks).
void  esmaecer_reiniciar(void);
void  esmaecer_escolha(int escolha);
// Chamar para CADA evento de pessoa (tecla, ponteiro). `consumivel` = o evento
// age se passar (KEYDOWN, clique). Devolve 1 se o evento deve ser engolido (a
// primeira tecla que acorda a tela, e a repeticao dela logo depois).
int   esmaecer_entrada(unsigned agora, int consumivel);
// Uma vez por quadro. `reproduzindo` = video tocando de verdade agora.
void  esmaecer_quadro(unsigned agora, float dt, int reproduzindo);
int   esmaecer_estagio(void);     // estagio ALVO atual
float esmaecer_veu(void);         // alfa corrente do veu (0..0,92)
int   esmaecer_apagado(void);     // 1 = no estagio 2 e o veu ja chegou la
// 1 uma vez quando o estagio 2 comeca e 1 uma vez ao sair dele, para a
// plataforma soltar/segurar o "manter tela ligada". Consome o aviso.
int   esmaecer_mudou_escuro(int *escuro);

// BRILHO DO PLAYER. `escolha` indexa V_BRILHO_PLAYER (100/80/65/50%).
float esmaecer_brilho_base(int escolha);
// Fator final: base e, com o filme tocando e a interface parada ha mais de
// BRILHO_AUTO_MS, um degrau a mais, com transicao suave. Nunca abaixo de 0,40.
#define BRILHO_AUTO_MS     2000u
#define BRILHO_AUTO_RAMPA  1500u
#define BRILHO_AUTO_PASSO  0.85f
float esmaecer_brilho_osd(int escolha, unsigned parado_ms, int tocando);

#ifndef ESMAECER_SEM_GFX
// O veu (e o relogio andando no estagio 2), no FIM do quadro.
void esmaecer_desenhar(unsigned agora);
#endif

#endif
