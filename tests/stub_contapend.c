// Jornal da conta (contapend.c) desligado, para os testes que linkam sync.c
// sem conta de verdade. O jornal tem teste proprio: tests/contapend.sh.
long long contapend_agora_ms(void) { return 0; }
int  contapend_aplicar_local(void) { return 0; }
int  contapend_enviar(void) { return 0; }
void contapend_esquecer(void) {}
int  contapend_pendentes(void) { return 0; }
void contapend_podar(long long desdeMs) { (void)desdeMs; }
