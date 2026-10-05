#include "sessao.h"
#include "idioma.h"
#include "nuvem.h"
#include "dados.h"
#include "js.h"
#include "jsw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define ARQ_SESSAO "sessao.txt"
// MEDIDO: a propria resposta do start traz `poll_interval_seconds` (3 no
// servidor de hoje). Este valor e so o padrao de quando ela nao vier — o
// intervalo real vem do servidor, que e quem sabe o custo que ele aguenta.
#define POLL_MS      3000
// Depois disto o codigo do servidor expira de qualquer jeito; continuar
// perguntando so gasta bateria do aparelho e mostra um codigo morto na tela.
#define LOGIN_LIMITE_MS 600000

static char acesso[3000];
static char renovar[3000];
static char sub[80];
static int  anonima;              // 1 quando o token e da sessao anonima
static long expiraEm;             // `exp` do JWT, em segundos

static SesEstado estado = SES_DESLOGADO;
static char codigo[64];
static char urlLogin[400];
static char nonce[64];
static char erro[240];

static unsigned pollMs = POLL_MS;

// Login por e-mail (#216). Os dois textos vivem so ate o pedido sair.
static char emailPend[256], senhaPend[256];
static int  emailPedido;          // pedido guardado ate o fio do QR acabar
static char erroEmail[240];

static pthread_t fio;
static int fioVivo;
static int passoPronto;           // o fio terminou; a proxima etapa pode ir
static unsigned proximoPoll;
static unsigned loginComecouMs;

// ---------------------------------------------------------------- JWT

// base64url -> bytes. So o necessario para ler o payload do JWT: nem valida
// assinatura nem tenta ser geral. Um JWT invalido aqui vira "sem sub e sem
// exp", que os chamadores ja tratam.
static int b64url(const char *s, size_t n, char *dst, size_t tam) {
  static const char *tab =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  unsigned acc = 0;
  int bits = 0;
  size_t i, w = 0;
  for (i = 0; i < n; i++) {
    const char *p = strchr(tab, s[i]);
    if (!p) continue;
    acc = (acc << 6) | (unsigned)(p - tab);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      if (w + 1 >= tam) return 0;
      dst[w++] = (char)((acc >> bits) & 0xFF);
    }
  }
  dst[w] = 0;
  return 1;
}

// Le `sub` e `exp` do payload. Sem isto o app nao sabe de quem e a sessao nem
// quando ela vence, e so descobriria pelo 401 — depois de a operacao ja ter
// falhado uma vez.
static void lerJwt(const char *token) {
  const char *p1, *p2;
  char payload[2200];
  sub[0] = 0;
  expiraEm = 0;
  if (!token || !*token) return;
  p1 = strchr(token, '.');
  if (!p1) return;
  p2 = strchr(p1 + 1, '.');
  if (!p2) return;
  if (!b64url(p1 + 1, (size_t)(p2 - p1 - 1), payload, sizeof payload)) return;
  { const char *fim = payload + strlen(payload);
    js_texto(payload, fim, "sub", sub, sizeof sub);
    expiraEm = (long)js_num(payload, fim, "exp", 0); }
}

// Forma de JWT (tres partes nao vazias, sem espaco): so a FORMA, a assinatura e
// do servidor. Serve para recusar lixo gravado por uma falha no meio do login.
static int jwtValido(const char *t) {
  const char *p1, *p2;
  if (!t || !*t || strpbrk(t, " \t\r\n")) return 0;
  p1 = strchr(t, '.');
  if (!p1 || p1 == t) return 0;
  p2 = strchr(p1 + 1, '.');
  if (!p2 || p2 == p1 + 1 || !p2[1]) return 0;
  return 1;
}

// Folga de 30s, igual a do web: um token que vence durante a requisicao volta
// como 401 e custa a viagem inteira.
static int vencido(void) {
  if (!acesso[0]) return 1;
  if (!expiraEm) return 0;   // sem exp legivel, so o servidor pode dizer
  return expiraEm <= (long)time(NULL) + 30;
}

// FALHA DO SERVIDOR, e nao da pessoa (#215): sem resposta, 429 ou 5xx. Nesses
// casos nada que a TV tem e invalido — o token, o refresh token, o codigo — e
// apagar qualquer um deles transformaria uma queda de horas do servidor numa
// conta perdida neste aparelho.
static int falhaServidor(int st) { return st == 0 || st == 429 || st >= 500; }

// A frase da tela para "o servidor da conta nao respondeu". i18n no FORMATO,
// como o resto deste arquivo: a frase final, com o numero, nunca casaria com
// a chave.
// Sem HTTP (st == 0) a causa e de transporte; o texto da libcurl ("curl 35:
// ...") vai junto, na tela e no log, porque o APK de release nao mostra o stderr
// dela (#223). Sem segredo: so o codigo e a frase da biblioteca.
static void anexarTransporte(char *dst, unsigned tam, int st) {
  const char *t = nuvem_ultimo_erro();
  size_t n = strlen(dst);
  if (st == 0 && t && t[0] && n + 4 < tam) snprintf(dst + n, tam - n, " [%s]", t);
}

static void erroServidor(int st) {
  if (st) snprintf(erro, sizeof erro,
                   i18n("O servidor da conta Nuvio não respondeu (HTTP %d). Tente de novo em alguns minutos."), st);
  else    snprintf(erro, sizeof erro, "%s",
                   i18n("O servidor da conta Nuvio não respondeu. Tente de novo em alguns minutos."));
  anexarTransporte(erro, sizeof erro, st);
}

// ---------------------------------------------------------------- disco

static void gravarSessao(void) {
  char buf[6400];
  snprintf(buf, sizeof buf, "%s\n%s\n%d\n", acesso, renovar, anonima ? 1 : 0);
  dados_gravar(ARQ_SESSAO, buf);
}

static void limpar(void) {
  acesso[0] = renovar[0] = sub[0] = 0;
  anonima = 0;
  expiraEm = 0;
}

// ---------------------------------------------------------------- respostas

// Guarda os tokens de uma resposta de autenticacao. Serve para os tres
// formatos que aparecem: /auth/v1/signup, /auth/v1/token e a funcao de troca —
// todos trazem access_token/refresh_token, uns na raiz, outros dentro de
// "session".
static int guardarTokens(const char *corpo, int ehAnonima) {
  const char *fim, *ses;
  char a[3000], r[3000];
  if (!corpo) return 0;
  fim = corpo + strlen(corpo);
  a[0] = r[0] = 0;
  js_texto(corpo, fim, "access_token", a, sizeof a);
  js_texto(corpo, fim, "refresh_token", r, sizeof r);
  if (!a[0]) {
    ses = strstr(corpo, "\"session\"");
    if (ses) {
      js_texto(ses, fim, "access_token", a, sizeof a);
      js_texto(ses, fim, "refresh_token", r, sizeof r);
    }
  }
  if (!a[0] || !jwtValido(a)) return 0;
  // ATOMICO (#223): o que ja estava (sessao anterior, anonima) so e trocado
  // quando a resposta nova esta inteira; resposta torta nao deixa meia sessao.
  snprintf(acesso, sizeof acesso, "%s", a);
  snprintf(renovar, sizeof renovar, "%s", r);
  anonima = ehAnonima ? 1 : 0;
  lerJwt(acesso);
  gravarSessao();
  return 1;
}

// ---------------------------------------------------------------- anonima

static int sessaoAnonima(void) {
  char *resp;
  int st = 0;
  // Mesmo corpo do web: o `data` marca de onde veio a sessao, e o servidor usa
  // isso nos relatorios dele.
  resp = nuvem_post("/auth/v1/signup",
                    "{\"data\":{\"tv_client\":\"webos\"}}", NULL, &st);
  if (resp && st >= 200 && st < 300 && guardarTokens(resp, 1)) { free(resp); return 1; }
  free(resp);
  // O signup anonimo pode estar desligado no projeto; ai o caminho e o grant
  // dedicado. O web tenta os dois na mesma ordem.
  resp = nuvem_post("/auth/v1/token?grant_type=anonymous", "{}", NULL, &st);
  if (resp && st >= 200 && st < 300 && guardarTokens(resp, 1)) { free(resp); return 1; }
  printf("[sessao] sessao anonima: HTTP %d %s\n", st, nuvem_ultimo_erro());
  if (falhaServidor(st)) erroServidor(st);
  else snprintf(erro, sizeof erro, "sessao anonima recusada (HTTP %d)", st);
  free(resp);
  return 0;
}

// ---------------------------------------------------------------- renovacao

// Status HTTP da ultima renovacao, para chamar() devolver a quem pediu o
// motivo real (um 504 da renovacao nao e um 401 da RPC).
static int stRenovacao;

// 1 = renovado; 0 = recusado de verdade (sessao encerrada); -1 = o servidor
// nao respondeu: a sessao FICA, e a proxima chamada tenta de novo.
static int renovarToken(void) {
  Jsw w;
  char *resp;
  int st = 0, ok;
  stRenovacao = 0;
  if (!renovar[0]) return 0;
  jsw_iniciar(&w);
  jsw_obj_ini(&w);
  jsw_cs(&w, "refresh_token", renovar);
  jsw_obj_fim(&w);
  resp = nuvem_post("/auth/v1/token?grant_type=refresh_token",
                    jsw_texto_final(&w), NULL, &st);
  jsw_livre(&w);
  ok = (resp && st >= 200 && st < 300 && guardarTokens(resp, anonima));
  free(resp);
  stRenovacao = st;
  if (!ok && falhaServidor(st)) {
    // O SERVIDOR CAIU, O TOKEN NAO (#215). Ate aqui qualquer falha da
    // renovacao apagava a sessao do disco: com o servidor da conta em 504 por
    // horas, cada TV cujo token vencesse nesse intervalo saia da conta sozinha
    // — e nao conseguia entrar de novo, porque o login fala com o mesmo
    // servidor. So uma RECUSA (4xx) encerra a sessao.
    printf("[sessao] renovacao sem resposta do servidor (HTTP %d): sessao mantida\n", st);
    nuvem_falhou();
    return -1;
  }
  if (!ok) {
    // Renovacao recusada e o fim da sessao, nao um erro transitorio: insistir
    // com um refresh token invalido devolve 400 para sempre.
    printf("[sessao] renovacao recusada (HTTP %d): saindo\n", st);
    limpar();
    dados_apagar(ARQ_SESSAO);
    estado = SES_DESLOGADO;
  }
  return ok;
}

// ---------------------------------------------------------------- RPC

static char *chamar(const char *caminho, const char *corpo, int *status) {
  char *resp;
  int st = 0;
  if (!nuvem_pronta()) { if (status) *status = 0; return NULL; }
  // Token vencido e servidor sem responder a renovacao: nem tenta a RPC (ela
  // voltaria 401 e pediria outra renovacao). O status e o da renovacao, para
  // quem chamou saber que a falha e do servidor e usar a copia (sync.c).
  if (vencido() && renovar[0] && renovarToken() < 0) {
    if (status) *status = stRenovacao;
    return NULL;
  }
  resp = nuvem_post(caminho, corpo, acesso, &st);
  if (st == 401 && renovar[0]) {
    int rr;
    free(resp);
    rr = renovarToken();
    if (rr <= 0) { if (status) *status = rr < 0 ? stRenovacao : 401; return NULL; }
    resp = nuvem_post(caminho, corpo, acesso, &st);
  }
  if (status) *status = st;
  // 429 tambem freia: e o servidor pedindo menos pedidos, e o ciclo de sync
  // mandaria mais dez em seguida.
  if (!resp || st == 0 || st == 429 || st >= 500) nuvem_falhou();
  else nuvem_ok();
  return resp;
}

char *sessao_rpc(const char *funcao, const char *corpoJson, int *status) {
  char caminho[300];
  if (!funcao || !*funcao) return NULL;
  snprintf(caminho, sizeof caminho, "/rest/v1/rpc/%s", funcao);
  return chamar(caminho, corpoJson, status);
}

char *sessao_tabela(const char *tabela, const char *consulta, int *status) {
  char *resp;
  int st = 0;
  if (!nuvem_pronta()) { if (status) *status = 0; return NULL; }
  if (vencido() && renovar[0] && renovarToken() < 0) {
    if (status) *status = stRenovacao;
    return NULL;
  }
  resp = nuvem_tabela(tabela, consulta, acesso, &st);
  if (st == 401 && renovar[0]) {
    int rr;
    free(resp);
    rr = renovarToken();
    if (rr <= 0) { if (status) *status = rr < 0 ? stRenovacao : 401; return NULL; }
    resp = nuvem_tabela(tabela, consulta, acesso, &st);
  }
  if (status) *status = st;
  if (!resp || st == 0 || st == 429 || st >= 500) nuvem_falhou();
  else nuvem_ok();
  return resp;
}

char *sessao_funcao(const char *nome, const char *corpoJson, int *status) {
  char caminho[300];
  if (!nome || !*nome) return NULL;
  snprintf(caminho, sizeof caminho, "/functions/v1/%s", nome);
  return chamar(caminho, corpoJson, status);
}

// ---------------------------------------------------------------- login

// Passo 1+2, no fio: sessao anonima e pedido do codigo.
static void *fioPedir(void *u) {
  Jsw w;
  char *resp;
  int st = 0;
  (void)u;
#ifdef NV_TPK40
  nv_tpk40_etapa("note sign-in-thread started");   // rastro do Tizen 4/5 (#180)
#endif
  erro[0] = 0;
  codigo[0] = 0;
  urlLogin[0] = 0;

  if (!acesso[0] || (anonima && vencido())) {
    if (!sessaoAnonima()) { estado = SES_ERRO; passoPronto = 1; return NULL; }
  }

  // MEDIDO contra o servidor: `p_redirect_base_url` vazia devolve 400 com
  // "Invalid TV login redirect base URL". Nao ha login sem essa configuracao, e
  // tentar assim mesmo so produziria um erro que a pessoa nao pode resolver.
  if (!nuvem_base_login()[0]) {
    snprintf(erro, sizeof erro, "pacote sem endereco de login configurado");
    estado = SES_ERRO;
    passoPronto = 1;
    return NULL;
  }

  // MEDIDO: o nonce TEM de ser um UUID. Com um identificador proprio, mesmo
  // unico, o servidor devolve 400 "Invalid device nonce".
  dados_uuid(nonce, sizeof nonce);

  jsw_iniciar(&w);
  jsw_obj_ini(&w);
  jsw_cs(&w, "p_device_nonce", nonce);
  jsw_cs(&w, "p_redirect_base_url", nuvem_base_login());
  jsw_cs(&w, "p_device_name", "LG webOS (Nuvio nativo)");
  jsw_obj_fim(&w);
  resp = nuvem_rpc_com("start_tv_login_session", jsw_texto_final(&w), acesso, &st);

  // Servidor antigo recusa o p_device_name; o web repete sem ele. Sem esta
  // segunda tentativa o login simplesmente nao existe nesses projetos.
  if (resp && nuvem_erro_ausente(resp)) {
    free(resp);
    jsw_livre(&w);
    jsw_iniciar(&w);
    jsw_obj_ini(&w);
    jsw_cs(&w, "p_device_nonce", nonce);
    jsw_cs(&w, "p_redirect_base_url", nuvem_base_login());
    jsw_obj_fim(&w);
    resp = nuvem_rpc_com("start_tv_login_session", jsw_texto_final(&w), acesso, &st);
  }
  jsw_livre(&w);

  if (resp && st >= 200 && st < 300) {
    const char *fim = resp + strlen(resp);
    double intervalo;
    js_texto(resp, fim, "code", codigo, sizeof codigo);
    // MEDIDO: a resposta ja traz a URL COMPLETA, com o codigo na query. Montar
    // "base + ?code=" a mao daria o mesmo resultado hoje e quebraria no dia em
    // que o servidor mudar o formato — usar o que ele mandou e de graca.
    js_texto(resp, fim, "web_url", urlLogin, sizeof urlLogin);
    intervalo = js_num(resp, fim, "poll_interval_seconds", 0);
    if (intervalo >= 1.0 && intervalo <= 60.0) pollMs = (unsigned)(intervalo * 1000.0);
  }
  if (!codigo[0]) {
    // O corpo do erro do PostgREST tem a mensagem util ("Invalid device nonce",
    // "Invalid TV login redirect base URL"); jogar fora e ficar so com o numero
    // do HTTP transformaria um defeito de configuracao em mistério.
    char msg[160];
    msg[0] = 0;
    if (resp) js_texto(resp, resp + strlen(resp), "message", msg, sizeof msg);
    // NO LOG, com o corpo: ate aqui a falha do pedido de codigo so ia para a
    // tela, e o #214 ("QR da erro HTTP 400") nao deixou rastro nenhum no
    // registro para dizer QUEM respondeu 400 — o PostgREST (JSON com
    // "message") ou algo na frente dele (Cloudflare responde texto puro,
    // "error code: 502", medido em 02/10). O corpo de erro nao traz segredo.
    printf("[sessao] pedido de codigo: HTTP %d %s: %.160s\n", st, nuvem_ultimo_erro(), resp ? resp : "(sem corpo)");
    if (falhaServidor(st)) erroServidor(st);
    else if (msg[0]) snprintf(erro, sizeof erro, "o servidor recusou: %s", msg);
    else snprintf(erro, sizeof erro,
                  i18n("O servidor da conta Nuvio recusou o pedido do código (HTTP %d). Tente de novo em alguns minutos."), st);
    if (!emailPedido && estado != SES_EMAIL) estado = SES_ERRO;
  } else {
    if (!urlLogin[0])
      snprintf(urlLogin, sizeof urlLogin, "%s?code=%s", nuvem_base_login(), codigo);
    if (!emailPedido && estado != SES_EMAIL) estado = SES_AGUARDANDO;
  }
  free(resp);
  passoPronto = 1;
  return NULL;
}

// Passo 3: pergunta se ja autorizaram; quando sim, troca pelo token.
static void *fioPoll(void *u) {
  Jsw w;
  char *resp;
  int st = 0;
  char status[48];
  (void)u;

  jsw_iniciar(&w);
  jsw_obj_ini(&w);
  jsw_cs(&w, "p_code", codigo);
  jsw_cs(&w, "p_device_nonce", nonce);
  jsw_obj_fim(&w);
  resp = nuvem_rpc_com("poll_tv_login_session", jsw_texto_final(&w), acesso, &st);
  jsw_livre(&w);

  status[0] = 0;
  if (resp && st >= 200 && st < 300)
    js_texto(resp, resp + strlen(resp), "status", status, sizeof status);
  free(resp);

  // "approved"/"authorized"/"ready": o servidor nao publica a lista, entao
  // qualquer coisa que NAO seja pendente/expirado tenta a troca. Uma troca
  // recusada nao custa nada; um estado desconhecido tratado como pendente
  // deixaria o usuario preso numa tela que ja podia ter passado.
  if (status[0] && strcmp(status, "pending") && strcmp(status, "expired") &&
      strcmp(status, "cancelled")) {
    estado = SES_TROCANDO;
    jsw_iniciar(&w);
    jsw_obj_ini(&w);
    jsw_cs(&w, "code", codigo);
    jsw_cs(&w, "device_nonce", nonce);
    jsw_obj_fim(&w);
    resp = nuvem_post("/functions/v1/tv-logins-exchange", jsw_texto_final(&w),
                      acesso, &st);
    jsw_livre(&w);
    if (resp && st >= 200 && st < 300 && guardarTokens(resp, 0)) {
      estado = SES_LOGADO;
      codigo[0] = 0;
      printf("[sessao] logado como %s\n", sub[0] ? sub : "(sem sub)");
    } else if (falhaServidor(st)) {
      // AUTORIZADO, mas o servidor nao respondeu a troca: o codigo continua
      // valendo, entao a TV continua esperando e a proxima volta do poll tenta
      // a troca de novo — a pessoa nao precisa escanear outra vez.
      printf("[sessao] troca do codigo sem resposta do servidor (HTTP %d): tento de novo\n", st);
      estado = SES_AGUARDANDO;
    } else {
      printf("[sessao] troca do codigo recusada (HTTP %d)\n", st);
      snprintf(erro, sizeof erro, i18n("troca de codigo recusada (HTTP %d)"), st);
      estado = SES_ERRO;
    }
    free(resp);
  } else if (!strcmp(status, "expired") || !strcmp(status, "cancelled")) {
    snprintf(erro, sizeof erro, "o codigo expirou");
    estado = SES_ERRO;
  }
  passoPronto = 1;
  return NULL;
}

// ---------------------------------------------------------------- e-mail


static void apagar(volatile char *p, size_t n) { while (n--) *p++ = 0; }

// A FRASE, nunca o corpo cru: o corpo de erro do GoTrue nao traz segredo, mas
// vem em ingles e com codigo interno. Dois formatos convivem: o antigo
// {"error":"invalid_grant","error_description":"Invalid login credentials"} e
// o novo {"error_code":"invalid_credentials","msg":"..."}.
void sessao_email_erro_de(int st, const char *corpo, char *dst, unsigned tam) {
  char cod[80], desc[200];
  const char *fim;
  cod[0] = desc[0] = 0;
  if (corpo) {
    fim = corpo + strlen(corpo);
    js_texto(corpo, fim, "error_code", cod, sizeof cod);
    if (!cod[0]) js_texto(corpo, fim, "error", cod, sizeof cod);
    js_texto(corpo, fim, "error_description", desc, sizeof desc);
    if (!desc[0]) js_texto(corpo, fim, "msg", desc, sizeof desc);
    if (!desc[0]) js_texto(corpo, fim, "message", desc, sizeof desc);
  }
  if (st == 0 || st >= 500) {
    if (st) snprintf(dst, tam, i18n("O servidor da conta Nuvio não respondeu (HTTP %d). Tente de novo em alguns minutos."), st);
    else    snprintf(dst, tam, "%s", i18n("O servidor da conta Nuvio não respondeu. Tente de novo em alguns minutos."));
  } else if (st == 429 || !strcmp(cod, "over_request_rate_limit") ||
             !strcmp(cod, "over_email_send_rate_limit")) {
    snprintf(dst, tam, "%s", i18n("Muitas tentativas. Espere um minuto e tente de novo."));
  } else if (!strcmp(cod, "email_not_confirmed") || strstr(desc, "not confirmed")) {
    snprintf(dst, tam, "%s", i18n("Este e-mail ainda não foi confirmado. Abra o link que a Nuvio mandou e tente de novo."));
  } else if (!strcmp(cod, "invalid_credentials") || !strcmp(cod, "invalid_grant") ||
             strstr(desc, "Invalid login credentials") || st == 400) {
    snprintf(dst, tam, "%s", i18n("E-mail ou senha incorretos."));
  } else {
    snprintf(dst, tam, i18n("O servidor da conta Nuvio recusou o login (HTTP %d)."), st);
  }
}

static void *fioEmail(void *u) {
  Jsw w;
  char *resp;
  int st = 0;
  (void)u;
  jsw_iniciar(&w);
  jsw_obj_ini(&w);
  jsw_cs(&w, "email", emailPend);
  jsw_cs(&w, "password", senhaPend);
  jsw_obj_fim(&w);
  apagar(senhaPend, sizeof senhaPend);
  resp = nuvem_post("/auth/v1/token?grant_type=password", jsw_texto_final(&w), NULL, &st);
  { char *t = (char *)jsw_texto_final(&w); if (t) apagar(t, strlen(t)); }
  jsw_livre(&w);
  apagar(emailPend, sizeof emailPend);
  if (resp && st >= 200 && st < 300 && guardarTokens(resp, 0)) {
    erroEmail[0] = 0;
    codigo[0] = 0;
    estado = SES_LOGADO;
    printf("[sessao] logado por e-mail como %s\n", sub[0] ? sub : "(sem sub)");
  } else {
    // So o status vai para o log: o corpo de erro do GoTrue repete o e-mail
    // em algumas versoes ("User with email x not found").
    printf("[sessao] login por e-mail: HTTP %d %s\n", st, nuvem_ultimo_erro());
    sessao_email_erro_de(st == 200 ? 0 : st, resp, erroEmail, sizeof erroEmail);
    anexarTransporte(erroEmail, sizeof erroEmail, st);
    if (st >= 200 && st < 300)
      snprintf(erroEmail, sizeof erroEmail, "%s", i18n("O servidor da conta Nuvio não respondeu. Tente de novo em alguns minutos."));
    estado = SES_DESLOGADO;
  }
  if (resp) { apagar(resp, strlen(resp)); free(resp); }
  passoPronto = 1;
  return NULL;
}

static void soltar(void *(*rotina)(void *));

void sessao_login_email(const char *email, const char *senha) {
  if (!nuvem_pronta()) {
    snprintf(erroEmail, sizeof erroEmail, "%s", i18n("Este pacote foi montado sem servidor."));
    return;
  }
  if (estado == SES_EMAIL || emailPedido) return;
  snprintf(emailPend, sizeof emailPend, "%s", email ? email : "");
  snprintf(senhaPend, sizeof senhaPend, "%s", senha ? senha : "");
  erroEmail[0] = 0;
  // O QR para aqui: com o estado fora de AGUARDANDO o poll nao e reagendado.
  // Um pedido de codigo ou poll ja em voo termina antes (sessao_passo so
  // solta o fio do e-mail com fioVivo em 0) — dois fios nunca escrevem os
  // tokens ao mesmo tempo.
  codigo[0] = 0;
  estado = SES_EMAIL;
  emailPedido = 1;
  sessao_passo(0);
}

const char *sessao_erro_email(void) { return erroEmail; }

static void soltar(void *(*rotina)(void *)) {
  if (fioVivo) return;
  passoPronto = 0;
  if (pthread_create(&fio, NULL, rotina, NULL) == 0) {
    pthread_detach(fio);
    fioVivo = 1;
  } else {
    snprintf(erro, sizeof erro, "sem fio para falar com o servidor");
    estado = SES_ERRO;
  }
}

void sessao_iniciar(void) {
  char *buf = dados_ler(ARQ_SESSAO);
  char *l1, *l2, *l3;
  if (!buf) return;
  l1 = buf;
  l2 = strchr(l1, '\n');
  if (l2) { *l2++ = 0; l3 = strchr(l2, '\n'); if (l3) *l3++ = 0; } else l3 = NULL;
  snprintf(acesso, sizeof acesso, "%s", l1);
  snprintf(renovar, sizeof renovar, "%s", l2 ? l2 : "");
  anonima = (l3 && atoi(l3) == 1);
  free(buf);
  // SESSAO PELA METADE nunca vira estado (#223): arquivo truncado, token que
  // nao e JWT, ou conta sem `sub`. Descarta de memoria e de disco e cai na tela
  // de login — antes o app subia "logado" sem conta e a tela ficava em branco
  // ate a pessoa limpar os dados.
  lerJwt(acesso);
  if (acesso[0] && (!jwtValido(acesso) || (!anonima && !sub[0]))) {
    printf("[sessao] sessao gravada invalida: descartada, indo para o login\n");
    limpar();
    dados_apagar(ARQ_SESSAO);
    return;
  }
  // Sessao anonima gravada NAO conta como conta: ela existe so como degrau para
  // pedir o codigo, e tratar isso como "logado" faria o app pular a tela de
  // login e depois falhar em todo sync com 401 sem explicar nada.
  if (acesso[0] && !anonima) {
    estado = SES_LOGADO;
    printf("[sessao] sessao restaurada (%s)\n", sub[0] ? sub : "sem sub");
  }
}

SesEstado   sessao_estado(void)   { return estado; }
// Ter conta e uma questao de TOKEN, nao do estado da tela: se a pessoa abre a
// tela de login estando logada e a tentativa falha, o estado vira SES_ERRO mas
// a sessao anterior continua boa. Amarrar isto ao estado deslogaria alguem por
// causa de um erro que nao tocou na sessao dela.
int         sessao_logada(void)   { return acesso[0] != 0 && !anonima; }
const char *sessao_codigo(void)   { return codigo; }
const char *sessao_url_login(void){ return urlLogin; }
const char *sessao_erro(void)     { return erro; }
const char *sessao_usuario(void)  { return sub; }
// "" para sessao anonima de proposito — ver a nota em sessao.h.
const char *sessao_token(void)    { return sessao_logada() ? acesso : ""; }

void sessao_login_comecar(void) {
  if (!nuvem_pronta()) {
    snprintf(erro, sizeof erro, "app sem configuracao de servidor");
    estado = SES_ERRO;
    return;
  }
  if (estado == SES_PEDINDO || estado == SES_AGUARDANDO || estado == SES_TROCANDO ||
      estado == SES_EMAIL || emailPedido) return;
  erro[0] = 0;
  codigo[0] = 0;
  estado = SES_PEDINDO;
  loginComecouMs = 0;
  soltar(fioPedir);
}

void sessao_passo(unsigned agoraMs) {
  if (fioVivo && passoPronto) { fioVivo = 0; passoPronto = 0; }
  if (fioVivo) return;
  if (emailPedido) {
    emailPedido = 0;
    // Um fio do QR que acabou depois do pedido pode ter mexido no estado.
    if (sessao_logada() && estado == SES_LOGADO) {
      apagar(emailPend, sizeof emailPend); apagar(senhaPend, sizeof senhaPend);
      return;
    }
    estado = SES_EMAIL;
    soltar(fioEmail);
    if (!fioVivo) {
      apagar(emailPend, sizeof emailPend); apagar(senhaPend, sizeof senhaPend);
      snprintf(erroEmail, sizeof erroEmail, "%s", i18n("O servidor da conta Nuvio não respondeu. Tente de novo em alguns minutos."));
      estado = SES_DESLOGADO;
    }
    return;
  }

  if (estado == SES_AGUARDANDO) {
    if (!loginComecouMs) loginComecouMs = agoraMs;
    if (agoraMs - loginComecouMs > LOGIN_LIMITE_MS) {
      snprintf(erro, sizeof erro, "o codigo expirou");
      estado = SES_ERRO;
      codigo[0] = 0;
      return;
    }
    if (agoraMs >= proximoPoll) {
      proximoPoll = agoraMs + pollMs;
      soltar(fioPoll);
    }
  }
}

void sessao_cancelar(void) {
  if (estado == SES_PEDINDO || estado == SES_AGUARDANDO ||
      estado == SES_TROCANDO || estado == SES_ERRO) {
    codigo[0] = 0;
    erro[0] = 0;
    estado = sessao_logada() ? SES_LOGADO : SES_DESLOGADO;
  }
}

void sessao_sair(void) {
  limpar();
  dados_apagar(ARQ_SESSAO);
  codigo[0] = 0;
  erro[0] = 0;
  estado = SES_DESLOGADO;
  printf("[sessao] sessao encerrada e apagada do disco\n");
}
