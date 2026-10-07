#ifndef NV_ARRANQUE_H
#define NV_ARRANQUE_H
// RASTRO DO ARRANQUE (so webOS). #317 e #211: duas LG UK65xx (2018, webOS 4.0)
// "fecham ao abrir", sem um byte de log. O log normal so existe depois de
// main() e o tratador de queda (queda.c) so e armado depois do SDL_Init, entao
// uma morte antes disso, ou dentro de um construtor estatico do binario,
// nao deixava nada. Aqui:
//   - um construtor de prioridade 101 (antes de qualquer outro) arma o tratador
//     de queda num arquivo de /tmp e grava a primeira etapa;
//   - main.c anota cada etapa seguinte (uma escrita de poucos bytes);
//   - a abertura seguinte, se a anterior nao passou do primeiro quadro, poe no
//     log "[arranque] sessao anterior parou em <etapa>" e traduz o relato.
// Falha de LOADER (lib faltando, instrucao ilegal antes de qualquer codigo
// nosso) continua sem rastro possivel: so a saida do binario no SSH mostra.
void arranque_etapa(const char *nome);
// Chamar logo depois do freopen do log, ainda no inicio de main().
void arranque_relatar(void);
#endif
