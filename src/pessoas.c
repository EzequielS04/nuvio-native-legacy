// Ver pessoas.h. Uma moldura (a mesma de recenviar.c), sete paginas.
//
// O MODELO: a pagina diz QUE linhas existem (montar()), o desenho pinta todas
// do mesmo jeito e o OK despacha pelo codigo da linha. Nada aqui guarda dado de
// terceiro alem do que recomenda.c entrega por copia — e nada aqui chega perto
// da rede.
#include "pessoas.h"
#include "recomenda.h"
#include "teclado.h"
#include "gfx.h"
#include "text.h"
#include "anim.h"
#include "layout.h"
#include "ajustes.h"
#include "idioma.h"
#include "tex_cache.h"
#include "botoes.h"
#define NV_ESCALA_TELA_ATIVA   // mede pela tela do fator ativo (escala.h)
#include "escala.h"
#include <stdio.h>
#include <string.h>

#define PE_W        860.0f
#define PE_PAD       48.0f
#define PE_LINHA    BOTAO_H_PRIMARIO
#define PE_GAP      BOTAO_GAP
#define PE_RODAPE    70.0f
#define PE_JANELA     5
// O MENU MOSTRA AS SEIS de uma vez: com "Comunidade Nuvio Native" ele passou a
// ter seis linhas, e a sexta ("Pessoas bloqueadas") so apareceria rolando — num
// menu de seis itens isso le como se ela nao existisse. Cabe: 892 px de altura.
#define PE_JANELA_MENU 6
static int pagina;
static int janela(void) { return pagina == 0 ? PE_JANELA_MENU : PE_JANELA; }
#define PE_INTERNO  (PE_W - PE_PAD * 2.0f)
#define PE_MAXL      48   // a comunidade: REC_COMUNIDADE_MAX pessoas + "Ver mais"
#define PE_AV_CARTAO 112.0f
#define PE_SW_W       72.0f
#define PE_SW_H       36.0f

enum { PG_MENU = 0, PG_LISTA, PG_CARTAO, PG_PEDIDOS, PG_BLOQ, PG_PERFIL, PG_GEN };

// O que cada linha faz ao OK.
enum { A_NADA = 0,
       A_BUSCAR, A_GOSTO, A_PEDIDOS, A_PERFIL, A_BLOQUEADOS,
       A_COMUNIDADE, A_MAIS,           // a lista da comunidade e "Ver mais"
       A_PESSOA,                       // abre o cartao de quem esta na linha
       A_PEDIR, A_CANCELAR, A_ACEITAR, A_RECUSAR, A_BLOQUEAR, A_DESBLOQ,
       A_P_PESQ, A_P_APELIDO, A_P_BIO, A_P_GEN, A_P_FOTO, A_P_REC, A_P_ATIV,
       A_P_APAGAR,
       A_GEN };                        // alterna o genero de indice `n`

typedef struct {
  int  acao, n;
  char rot[96];
  char sub[120];
  int  sw;                 // -1 sem interruptor; 0/1
  int  temPessoa;
  RecPessoa p;             // avatar/nome da linha, quando e uma pessoa
  char pub[16];            // alvo de A_DESBLOQ
} Linha;

static int   aberto, foco, topo;
static float anim;
static Linha linhas[PE_MAXL];
static int   nL;
static float focoAnim[PE_MAXL];
static char  aviso[200];
static Uint32 avisoAte;
static int   opAtual;             // A_* em andamento na rede, ou 0
static int   confirmando = -1;    // indice de linha esperando o segundo OK
static RecPerfil rasc;            // rascunho de "Meu perfil"
static int   rascVivo;
static char  cartaoPub[16];
static int   voltaPara = -1;      // pagina de onde o cartao veio
static int   tecladoPara;         // 0 nada, A_BUSCAR, A_P_APELIDO, A_P_BIO
static int   focoVolta = -1;      // linha da lista de onde o cartao foi aberto

static const char *ALFA_TEXTO = "abcdefghijklmnopqrstuvwxyz0123456789-";

static int teclaOk(SDL_Keycode k) {
  return k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE;
}

int pessoas_aberto(void) { return aberto; }

static void dizer(const char *s, Uint32 agora, unsigned ms) {
  snprintf(aviso, sizeof aviso, "%s", s);
  avisoAte = ms ? agora + ms : 0;
}

static void carregarRascunho(void) {
  RecPerfil p;
  recomenda_perfil(&p);
  // Publicado: a verdade e a do aparelho. Nao publicado: o rascunho de memoria
  // (o que ela digitou e ainda nao ligou) so perde o que o aparelho ja zerou.
  if (p.publicado || !rascVivo) rasc = p;
  else rasc.ativ = p.ativ;
  rascVivo = 1;
}

static void irPara(int pg) {
  pagina = pg;
  foco = 0; topo = 0;
  confirmando = -1;
  memset(focoAnim, 0, sizeof focoAnim);
}

static void abrirComum(void) {
  aberto = 1;
  aviso[0] = 0; avisoAte = 0;
  opAtual = 0; confirmando = -1; voltaPara = -1; tecladoPara = 0;
  recomenda_soc_limpar();
  recomenda_pedir_agora();
  carregarRascunho();
}

int pessoas_abrir(void) {
  if (!recomenda_ativo()) return 0;
  abrirComum();
  irPara(PG_MENU);
  return 1;
}

int pessoas_abrir_perfil(void) {
  if (!recomenda_ativo()) return 0;
  abrirComum();
  irPara(PG_PERFIL);
  return 1;
}

// --- rascunho e publicacao -----------------------------------------------------

// Publicado, cada mudanca vai ao servidor na hora; nao publicado, so muda o
// rascunho. `ativ = -1` diz a recomenda_perfil_publicar "nao mexa no nivel".
static void aplicarRascunho(void) {
  if (!rasc.publicado) return;
  { RecPerfil p = rasc;
    p.ativ = -1;
    if (!recomenda_perfil_publicar(&p)) rasc.publicado = 0; }
}

int pessoas_definir_pesquisavel(int ligado) {
  if (!recomenda_ativo()) return 0;
  carregarRascunho();
  if (!ligado) {
    recomenda_perfil_despublicar();
    rasc.publicado = 0;
    return 1;
  }
  if (strlen(rasc.apelido) < 2) {
    // SEM APELIDO NAO LIGA: e o unico nome que estranhos veem, e a pessoa tem de
    // escolhe-lo. O caminho e abrir "Meu perfil" e pedir.
    pessoas_abrir_perfil();
    tecladoPara = A_P_APELIDO;
    teclado_abrir_com("Apelido",
                      "Como as pessoas vão te achar. Use - entre as palavras.",
                      REC_APELIDO_MAX, ALFA_TEXTO, NULL);
    return 0;
  }
  rasc.publicado = 1;
  { RecPerfil p = rasc; p.ativ = -1;
    if (!recomenda_perfil_publicar(&p)) { rasc.publicado = 0; return 0; } }
  return 1;
}

// --- linhas de cada pagina --------------------------------------------------------

static Linha *nova(int acao, const char *rot) {
  Linha *l;
  if (nL >= PE_MAXL) return NULL;
  l = &linhas[nL++];
  memset(l, 0, sizeof *l);
  l->acao = acao; l->sw = -1;
  snprintf(l->rot, sizeof l->rot, "%s", rot);
  return l;
}

static void rotuloRelacao(char *dst, size_t tam, const RecPessoa *p) {
  if (!strcmp(p->relacao, "amigo")) snprintf(dst, tam, "%s", i18n("Já é seu amigo"));
  else if (!strcmp(p->relacao, "enviado")) snprintf(dst, tam, "%s", i18n("Pedido enviado"));
  else if (!strcmp(p->relacao, "recebido")) snprintf(dst, tam, "%s", i18n("Quer ser seu amigo"));
  else if (p->emComum > 0) snprintf(dst, tam, i18n("%d títulos em comum"), p->emComum);
  else if (p->vendo[0]) snprintf(dst, tam, i18n("Assistiu recentemente: %s"), p->vendo);
  else snprintf(dst, tam, "%s", p->bio);
}

static void pessoaLinha(int acao, const RecPessoa *p, int comRelacao) {
  Linha *l = nova(acao, p->apelido);
  if (!l) return;
  l->temPessoa = 1; l->p = *p;
  if (comRelacao) rotuloRelacao(l->sub, sizeof l->sub, p);
  else snprintf(l->sub, sizeof l->sub, "%s", p->bio);
}

static const char *rotuloAtiv(int n) {
  return n == 2 ? "O que assisti e o que estou assistindo"
       : n == 1 ? "O que assisti" : "Desligada";
}

static void montar(void) {
  int i, np = recomenda_n_pedidos();
  char b[120];
  nL = 0;
  if (pagina == PG_MENU) {
    nova(A_BUSCAR, "Buscar por apelido ou código");
    nova(A_COMUNIDADE, "Comunidade Nuvio Native");
    nova(A_GOSTO, "Pessoas com gosto parecido");
    if (np > 0) { snprintf(b, sizeof b, i18n("Pedidos de amizade (%d)"), np); nova(A_PEDIDOS, b); }
    else nova(A_PEDIDOS, "Pedidos de amizade");
    nova(A_PERFIL, "Meu perfil");
    nova(A_BLOQUEADOS, "Pessoas bloqueadas");
  } else if (pagina == PG_LISTA) {
    int n = recomenda_n_achados();
    for (i = 0; i < n; i++) { RecPessoa p; if (recomenda_achado(i, &p)) pessoaLinha(A_PESSOA, &p, 1); }
    if (recomenda_achados_origem() == 3 && recomenda_comunidade_mais())
      nova(A_MAIS, opAtual == A_MAIS ? "Aguarde..." : "Ver mais pessoas");
  } else if (pagina == PG_PEDIDOS) {
    for (i = 0; i < np; i++) { RecPessoa p; if (recomenda_pedido(i, &p)) pessoaLinha(A_PESSOA, &p, 1); }
  } else if (pagina == PG_BLOQ) {
    int n = recomenda_n_bloqueados();
    for (i = 0; i < n; i++) {
      char pub[16], nome[64];
      if (!recomenda_bloqueado(i, pub, sizeof pub, nome, sizeof nome)) continue;
      { Linha *l = nova(A_DESBLOQ, nome[0] ? nome : pub);
        if (!l) break;
        snprintf(l->pub, sizeof l->pub, "%s", pub);
        snprintf(l->sub, sizeof l->sub, "%s", confirmando == nL - 1 ? i18n("OK de novo para desbloquear")
                                                                   : i18n("OK desbloqueia")); }
    }
  } else if (pagina == PG_CARTAO) {
    RecPessoa c;
    if (recomenda_cartao(&c)) {
      if (!strcmp(c.relacao, "recebido")) { nova(A_ACEITAR, "Aceitar pedido"); nova(A_RECUSAR, "Recusar pedido"); }
      else if (!strcmp(c.relacao, "enviado")) nova(A_CANCELAR, "Cancelar pedido");
      else if (strcmp(c.relacao, "amigo")) nova(A_PEDIR, "Pedir amizade");
      { Linha *l = nova(A_BLOQUEAR, confirmando >= 0 ? "Bloquear? OK confirma" : "Bloquear");
        (void)l; }
    }
  } else if (pagina == PG_PERFIL) {
    Linha *l;
    l = nova(A_P_PESQ, "Perfil pesquisável"); if (l) l->sw = rasc.publicado ? 1 : 0;
    l = nova(A_P_APELIDO, "Apelido");
    if (l) snprintf(l->sub, sizeof l->sub, "%s", rasc.apelido[0] ? rasc.apelido : i18n("Escolha como te acharão"));
    l = nova(A_P_BIO, "Bio curta");
    if (l) snprintf(l->sub, sizeof l->sub, "%s", rasc.bio[0] ? rasc.bio : i18n("Opcional"));
    l = nova(A_P_GEN, "Gêneros favoritos");
    if (l) {
      int k, n = 0;
      for (k = 0; k < REC_GENEROS_N; k++) if (rasc.generos & (1u << k)) n++;
      if (n) snprintf(l->sub, sizeof l->sub, i18n("%d escolhidos"), n);
      else snprintf(l->sub, sizeof l->sub, "%s", i18n("Opcional"));
    }
    l = nova(A_P_FOTO, "Mostrar minha foto"); if (l) l->sw = rasc.foto ? 1 : 0;
    l = nova(A_P_REC, "Mostrar o que assisti recentemente"); if (l) l->sw = rasc.recentes ? 1 : 0;
    l = nova(A_P_ATIV, "Atividade para amigos");
    if (l) snprintf(l->sub, sizeof l->sub, "%s", i18n(rotuloAtiv(rasc.ativ)));
    l = nova(A_P_APAGAR, confirmando >= 0 ? "Apagar tudo? OK confirma" : "Apagar meus dados sociais");
    (void)l;
  } else if (pagina == PG_GEN) {
    for (i = 0; i < REC_GENEROS_N; i++) {
      Linha *l = nova(A_GEN, rec_genero_rotulo(i));
      if (l) { l->n = i; l->sw = (rasc.generos & (1u << i)) ? 1 : 0; }
    }
  }
  if (foco >= nL) foco = nL > 0 ? nL - 1 : 0;
  if (foco < topo) topo = foco;
  if (foco >= topo + janela()) topo = foco - janela() + 1;
  if (topo < 0) topo = 0;
}

// --- acao ---------------------------------------------------------------------------

static void abrirTeclado(int alvo) {
  tecladoPara = alvo;
  if (alvo == A_BUSCAR)
    teclado_abrir_com("Buscar pessoas",
                      "Apelido (3 letras ou mais) ou o código de 6 letras.",
                      24, ALFA_TEXTO, NULL);
  else if (alvo == A_P_APELIDO)
    teclado_abrir_com("Apelido", "Como as pessoas vão te achar. Use - entre as palavras.",
                      REC_APELIDO_MAX, ALFA_TEXTO, rasc.apelido[0] ? rasc.apelido : NULL);
  else
    teclado_abrir_com("Bio curta", "Uma frase sobre você. Sem e-mail nem link.",
                      REC_BIO_MAX, ALFA_TEXTO, rasc.bio[0] ? rasc.bio : NULL);
}

static void aplicar(void) {
  Linha *l;
  Uint32 agora = SDL_GetTicks();
  RecPessoa c;
  if (foco < 0 || foco >= nL) return;
  l = &linhas[foco];
  switch (l->acao) {
    case A_BUSCAR: abrirTeclado(A_BUSCAR); return;
    case A_COMUNIDADE:
      // RECIPROCO, como o gosto parecido (e o servidor confere de novo): quem
      // nao se mostra nao ganha uma janela para ver todo mundo.
      if (!recomenda_pesquisavel()) {
        dizer(i18n("Ligue o Perfil pesquisável em Meu perfil para ver a comunidade."), agora, 4500);
        return;
      }
      if (recomenda_comunidade(0)) { opAtual = A_COMUNIDADE; dizer(i18n("Procurando..."), agora, 0); }
      return;
    case A_MAIS:
      if (opAtual) return;
      if (recomenda_comunidade(recomenda_comunidade_pagina() + 1)) opAtual = A_MAIS;
      return;
    case A_GOSTO:
      if (!recomenda_pesquisavel()) {
        dizer(i18n("Ligue o Perfil pesquisável em Meu perfil para ver pessoas com gosto parecido."), agora, 4500);
        return;
      }
      if (recomenda_sugeridos_gosto()) { opAtual = A_GOSTO; dizer(i18n("Procurando..."), agora, 0); }
      return;
    case A_PEDIDOS:
      irPara(PG_PEDIDOS);
      recomenda_listar_pedidos(); opAtual = A_PEDIDOS;
      return;
    case A_PERFIL: carregarRascunho(); irPara(PG_PERFIL); return;
    case A_BLOQUEADOS:
      irPara(PG_BLOQ);
      recomenda_listar_bloqueados(); opAtual = A_BLOQUEADOS;
      return;
    case A_PESSOA:
      if (!l->temPessoa) return;
      voltaPara = pagina;
      focoVolta = foco;
      snprintf(cartaoPub, sizeof cartaoPub, "%s", l->p.pub);
      if (recomenda_ver_perfil(cartaoPub)) { opAtual = A_PESSOA; dizer(i18n("Abrindo..."), agora, 0); }
      return;
    case A_PEDIR:
      if (recomenda_pedir_amizade(cartaoPub)) { opAtual = A_PEDIR; dizer(i18n("Enviando pedido..."), agora, 0); }
      return;
    case A_CANCELAR:
      if (recomenda_cancelar_pedido(cartaoPub)) opAtual = A_CANCELAR;
      return;
    case A_ACEITAR:
      if (recomenda_aceitar(cartaoPub)) opAtual = A_ACEITAR;
      return;
    case A_RECUSAR:
      if (recomenda_recusar(cartaoPub)) opAtual = A_RECUSAR;
      return;
    case A_BLOQUEAR:
      // DOIS OK: bloquear derruba a amizade nos dois lados, e um OK distraido
      // nao pode fazer isso.
      if (confirmando != foco) { confirmando = foco; return; }
      confirmando = -1;
      if (recomenda_cartao(&c) && recomenda_bloquear(c.pub)) opAtual = A_BLOQUEAR;
      return;
    case A_DESBLOQ:
      if (confirmando != foco) { confirmando = foco; return; }
      confirmando = -1;
      if (recomenda_desbloquear(l->pub)) opAtual = A_DESBLOQ;
      return;
    case A_P_PESQ:
      if (rasc.publicado) { pessoas_definir_pesquisavel(0); rasc.publicado = 0; }
      else pessoas_definir_pesquisavel(1);
      return;
    case A_P_APELIDO: abrirTeclado(A_P_APELIDO); return;
    case A_P_BIO: abrirTeclado(A_P_BIO); return;
    case A_P_GEN: irPara(PG_GEN); return;
    case A_P_FOTO: rasc.foto = !rasc.foto; aplicarRascunho(); return;
    case A_P_REC: rasc.recentes = !rasc.recentes; aplicarRascunho(); return;
    case A_P_ATIV:
      rasc.ativ = (rasc.ativ + 1) % 3;
      recomenda_atividade_nivel(rasc.ativ);
      return;
    case A_P_APAGAR:
      if (confirmando != foco) { confirmando = foco; return; }
      confirmando = -1;
      recomenda_apagar_dados_sociais();
      memset(&rasc, 0, sizeof rasc);
      dizer(i18n("Seus dados sociais foram apagados."), agora, 3500);
      return;
    case A_GEN:
      if (!(rasc.generos & (1u << l->n))) {
        int k, n = 0;
        for (k = 0; k < REC_GENEROS_N; k++) if (rasc.generos & (1u << k)) n++;
        // O servidor guarda no maximo 5: aceitar um sexto aqui deixaria a tela
        // dizendo uma coisa e o perfil outra.
        if (n >= 5) { dizer(i18n("Máximo de 5 gêneros."), agora, 3000); return; }
      }
      rasc.generos ^= 1u << l->n;
      aplicarRascunho();
      return;
    default: return;
  }
}

void pessoas_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto) return;
  if (teclado_aberto()) { teclado_evento(e); return; }
  if (e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (e->key.repeat && teclaOk(k)) return;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      k == SDLK_DELETE || k == SDLK_LEFT ||
      e->key.keysym.scancode == NV_SCANCODE_BACK) {
    if (confirmando >= 0) { confirmando = -1; return; }
    if (pagina == PG_GEN) { irPara(PG_PERFIL); return; }
    if (pagina == PG_CARTAO) {
      int volta = voltaPara;
      irPara(volta >= 0 ? volta : PG_MENU);
      // Volta para a MESMA pessoa da lista: numa comunidade de 40 nomes,
      // recomecar do primeiro a cada cartao fechado seria andar tudo de novo.
      if (volta >= 0 && focoVolta >= 0) { foco = focoVolta; montar(); }
      focoVolta = -1;
      return;
    }
    if (pagina != PG_MENU) { irPara(PG_MENU); return; }
    aberto = 0;
    return;
  }
  if (k == SDLK_UP)   { foco--; confirmando = -1; return; }
  if (k == SDLK_DOWN) { foco++; confirmando = -1; return; }
  if (teclaOk(k)) { aplicar(); return; }
}

// --- atualizar ---------------------------------------------------------------------------

static const char *textoEstado(int est) {
  switch (est) {
    case REC_SOC_LIMITE:      return "Muitas tentativas. Tente de novo mais tarde.";
    case REC_SOC_NAO_ACHOU:   return "Perfil indisponível.";
    case REC_SOC_SEM_APELIDO: return "Escolha um apelido em Meu perfil para pedir amizade.";
    case REC_SOC_CURTA:       return "Digite pelo menos 3 letras.";
    default:                  return "Não foi possível agora. Tente novamente.";
  }
}

// O fim de cada operacao de rede: para onde a tela vai e o que ela diz.
static void concluiu(int est, Uint32 agora) {
  int op = opAtual;
  opAtual = 0;
  if (est != REC_SOC_OK) {
    dizer(i18n(textoEstado(est)), agora, 3800);
    if (op == A_PEDIDOS || op == A_BLOQUEADOS) { /* fica na pagina, vazia */ }
    return;
  }
  switch (op) {
    case A_BUSCAR:
      irPara(PG_LISTA);
      dizer(recomenda_n_achados() ? "" : i18n("Ninguém encontrado com esse apelido ou código."), agora, 4000);
      break;
    case A_COMUNIDADE:
      irPara(PG_LISTA);
      dizer(recomenda_comunidade_fechada()
              ? i18n("Ligue o Perfil pesquisável em Meu perfil para ver a comunidade.")
              : recomenda_n_achados() ? "" : i18n("Ninguém na comunidade por enquanto."), agora, 4000);
      break;
    case A_MAIS:
      // Fica onde esta: o foco ja estava em "Ver mais", que agora e a primeira
      // pessoa nova (ou a ultima, se nao veio ninguem).
      aviso[0] = 0;
      break;
    case A_GOSTO:
      irPara(PG_LISTA);
      dizer(recomenda_n_achados() ? "" : i18n("Ninguém com gosto parecido por enquanto."), agora, 4000);
      break;
    case A_PESSOA:
      irPara(PG_CARTAO); aviso[0] = 0;
      break;
    case A_PEDIR:
      dizer(i18n("Pedido enviado. Quando a pessoa aceitar, vocês viram amigos."), agora, 4200);
      recomenda_ver_perfil(cartaoPub); opAtual = A_PESSOA + 1000;   // recarrega o cartao
      break;
    case A_PESSOA + 1000:
      irPara(PG_CARTAO); confirmando = -1;
      break;
    case A_CANCELAR:
      dizer(i18n("Pedido cancelado."), agora, 3000);
      recomenda_ver_perfil(cartaoPub); opAtual = A_PESSOA + 1000;
      break;
    case A_ACEITAR:
      dizer(i18n("Agora vocês são amigos."), agora, 3500);
      irPara(PG_MENU);
      break;
    case A_RECUSAR:
      dizer(i18n("Pedido recusado. A pessoa não é avisada."), agora, 3800);
      irPara(PG_MENU);
      break;
    case A_BLOQUEAR:
      dizer(i18n("Pessoa bloqueada."), agora, 3500);
      irPara(PG_MENU);
      break;
    case A_DESBLOQ:
      dizer(i18n("Pessoa desbloqueada."), agora, 3000);
      recomenda_listar_bloqueados(); opAtual = A_BLOQUEADOS;
      break;
    default: break;
  }
}

void pessoas_atualizar(float dt, Uint32 agora) {
  int i, est;
  if (!aberto && anim < 0.002f) { anim = 0.0f; return; }
  anim = ajustes_animacoes_reduzidas()
           ? (aberto ? 1.0f : 0.0f)
           : anim_mola(anim, aberto ? 1.0f : 0.0f, dt, NV_MOLA_TELA);
  teclado_atualizar(dt, agora);
  for (i = 0; i < PE_MAXL; i++)
    focoAnim[i] = ajustes_animacoes_reduzidas()
      ? (aberto && foco == i ? 1.0f : 0.0f)
      : anim_mola(focoAnim[i], aberto && foco == i ? 1.0f : 0.0f, dt, NV_MOLA_FOCO);
  if (!aberto) return;

  // O TECLADO: o resultado e CONSUMIDO NA LEITURA, entao isto roda uma vez por
  // confirmacao. So o alvo que abriu o teclado pode ler.
  if (tecladoPara && !teclado_aberto()) {
    int r = teclado_resultado();
    int alvo = tecladoPara;
    tecladoPara = 0;
    if (r == TECLADO_PRONTO) {
      const char *t = teclado_texto();
      if (alvo == A_BUSCAR) {
        if (recomenda_buscar(t)) { opAtual = A_BUSCAR; dizer(i18n("Procurando..."), agora, 0); }
        else dizer(i18n(textoEstado(recomenda_soc_estado())), agora, 3500);
      } else if (alvo == A_P_APELIDO) {
        snprintf(rasc.apelido, sizeof rasc.apelido, "%s", t);
        { char *w = rasc.apelido; for (; *w; w++) if (*w == '-') *w = ' '; }
        if (strlen(rasc.apelido) < 2) {
          rasc.apelido[0] = 0;
          dizer(i18n("O apelido precisa de 2 letras ou mais."), agora, 3500);
        } else {
          // Escolheu o apelido para ligar o perfil? Liga: era isso que o
          // interruptor de Ajustes tinha pedido.
          if (!rasc.publicado && !recomenda_pesquisavel()) pessoas_definir_pesquisavel(1);
          else aplicarRascunho();
        }
      } else if (alvo == A_P_BIO) {
        snprintf(rasc.bio, sizeof rasc.bio, "%s", t);
        { char *w = rasc.bio; for (; *w; w++) if (*w == '-') *w = ' '; }
        aplicarRascunho();
      }
    }
  }

  est = recomenda_soc_estado();
  if (est == REC_SOC_INDO) {
    if (!aviso[0]) snprintf(aviso, sizeof aviso, "%s", i18n("Aguarde..."));
  } else if (est != REC_SOC_NADA) {
    if (opAtual) concluiu(est, agora);
    else if (est == REC_SOC_CURTA) dizer(i18n(textoEstado(est)), agora, 3500);
    recomenda_soc_limpar();
  }
  if (avisoAte && (Sint32)(agora - avisoAte) >= 0) { aviso[0] = 0; avisoAte = 0; }
  montar();
}

// --- desenho -------------------------------------------------------------------------------

// O INTERRUPTOR USA A TINTA DO TEXTO DA LINHA, e nao o realce. O realce padrao
// e quase branco, e a linha em foco e uma pilula branca: trilha e bola sumiam
// uma na outra (visto na captura). Com a tinta do texto o contraste e o mesmo
// do rotulo ao lado, em repouso e em foco, ligado ou desligado.
static void desenhaInterruptor(float x, float y, int lig, int tinta, float a) {
  GfxRect t = { x, y, PE_SW_W, PE_SW_H };
  float bola = PE_SW_H - 8.0f, bx = lig ? x + PE_SW_W - bola - 4.0f : x + 4.0f;
  float tr = (float)tinta / 255.0f;
  int escura = tinta < 128;
  gfx_cor(t, 0.5f, tr, tr, tr, (lig ? 0.92f : 0.30f) * a);
  gfx_rect((GfxRect){ bx, y + 4.0f, bola, bola }, 0, GFX_DISCO, 0, 0, 0, 0,
           escura ? 0.97f : 0.10f, escura ? 0.97f : 0.10f, escura ? 0.98f : 0.12f, a);
}

static void desenhaLinha(float x, float y, const Linha *l, float f, float a) {
  GfxRect r = { x, y, PE_INTERNO, PE_LINHA };
  float tx = r.x + BOTAO_PAD_X, fimTexto = r.x + r.w - BOTAO_PAD_X;
  int cor = botao_superficie(r, f, a);
  const char *rot = i18n(l->rot);
  if (l->temPessoa) {
    float d = PE_LINHA - 16.0f;
    GfxRect av = { r.x + 8.0f, y + 8.0f, d, d };
    rec_avatar(av, l->p.avatar, l->p.apelido, l->p.pub, a);
    tx = av.x + d + 18.0f;
  }
  if (l->sw >= 0) fimTexto -= PE_SW_W + 20.0f;
  if (l->sub[0]) {
    TxtLinha t1 = txt_linha_corta(TXT_DET_BOTAO, l->temPessoa ? l->rot : rot, cor, cor, cor, 255, fimTexto - tx);
    TxtLinha t2 = txt_linha_corta(TXT_CAPTION2, l->sub, cor, cor, cor, 255, fimTexto - tx);
    float tot = t1.h + t2.h - 4.0f, oy = y + (PE_LINHA - tot) * 0.5f;
    txt_desenhar_alpha(t1, tx, oy, a);
    txt_desenhar_alpha(t2, tx, oy + t1.h - 4.0f, a * 0.78f);
  } else {
    TxtLinha t = txt_linha_corta(TXT_DET_BOTAO, l->temPessoa ? l->rot : rot, cor, cor, cor, 255, fimTexto - tx);
    txt_desenhar_alpha(t, tx, y + (PE_LINHA - t.h) * 0.5f, a);
  }
  if (l->sw >= 0)
    desenhaInterruptor(r.x + r.w - BOTAO_PAD_X + 6.0f - PE_SW_W, y + (PE_LINHA - PE_SW_H) * 0.5f,
                       l->sw, cor, a);
}

// Generos escolhidos, numa linha ("Terror · Drama").
static void textoGeneros(char *dst, size_t tam, unsigned m) {
  int i;
  size_t k = 0;
  dst[0] = 0;
  for (i = 0; i < REC_GENEROS_N; i++)
    if (m & (1u << i)) {
      const char *g = i18n(rec_genero_rotulo(i));
      k += (size_t)snprintf(dst + k, tam - k, "%s%s", k ? "  \xc2\xb7  " : "", g);
      if (k + 24 >= tam) break;
    }
}

static void pessoas_desenharCorpo_(Uint32 agora);
// Cartao de tela quase cheia: ampliado so se ainda couber (escala.h).
void pessoas_desenhar(Uint32 agora) {
  ESCALA_SE_COUBER_INI(PE_W, 700.0f);
  pessoas_desenharCorpo_(agora);
  ESCALA_SE_COUBER_FIM();
}
static void pessoas_desenharCorpo_(Uint32 agora) {
  float a = anim_suave(anim), alt, x, y, cab, hx, hy, hw;
  int i, vis;
  const char *chapeu, *titulo, *sub;
  RecPessoa c;
  int temC = 0;
  (void)agora;
  if (anim < 0.01f) return;

  if (pagina == PG_CARTAO) temC = recomenda_cartao(&c);

  // ALTURA DO CABECALHO, por pagina. Mesma regra de recenviar.c: o que esta
  // dentro dele soma; nao se estima. O cartao carrega avatar + nome + bio +
  // generos + recentes; as outras so o titulo e uma frase.
  switch (pagina) {
    case PG_MENU:   cab = 214.0f; break;
    case PG_PERFIL: cab = 236.0f; break;
    case PG_CARTAO: cab = 178.0f + PE_AV_CARTAO * 0.5f + (temC && c.bio[0] ? 46.0f : 0.0f)
                          + (temC && c.generos ? 36.0f : 0.0f)
                          + (temC && recomenda_cartao_n_recentes() ? 40.0f : 0.0f); break;
    case PG_GEN:    cab = 150.0f; break;
    default:        cab = 190.0f; break;   // subtitulo de ate 2 linhas (pt e mais longo que en)
  }
  vis = nL < janela() ? nL : janela();
  if (vis < 1) vis = 1;
  alt = PE_PAD * 2.0f + cab + (float)vis * (PE_LINHA + PE_GAP) - PE_GAP + PE_RODAPE;
  x = (NV_TELA_W - PE_W) * 0.5f;
  y = (NV_TELA_H - alt) * 0.5f + (1.0f - a) * 40.0f;

  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0, 0, 0, 0.72f * anim);
  { GfxRect p = { x, y, PE_W, alt };
    float ar, ag, ab, menor = alt < PE_W ? alt : PE_W, raio = 28.0f / menor;
    ajustes_acento(&ar, &ag, &ab);
    gfx_cor(p, raio, 0.055f, 0.058f, 0.068f, 0.94f * a);
    gfx_luz_canto(p, raio, PE_W * 0.1f, -PE_W * 0.1f, PE_W * 0.65f, ar, ag, ab, 0.22f * a); }

  chapeu = "AMIGOS";
  switch (pagina) {
    case PG_MENU:    titulo = "Encontrar pessoas";
                     sub = "Procure por apelido, ou pelo código de 6 letras que seu amigo te passou. Só aparece quem ligou o Perfil pesquisável."; break;
    case PG_LISTA:   titulo = recomenda_achados_origem() == 3 ? "Comunidade Nuvio Native"
                           : recomenda_achados_origem() == 2 ? "Gosto parecido" : "Resultados da busca";
                     sub = recomenda_achados_origem() == 3
                           ? "Todo mundo que ligou o Perfil pesquisável, com atividade mais recente primeiro. Abra um perfil para pedir amizade."
                           : recomenda_achados_origem() == 2
                           ? "Pessoas que também assistiram a vários dos seus títulos. Só entra quem publicou os vistos recentemente."
                           : "Abra um perfil para ver o que a pessoa escolheu mostrar."; break;
    case PG_PEDIDOS: titulo = "Pedidos de amizade";
                     sub = recomenda_n_pedidos() ? "Só vira amizade se você aceitar. Recusar não avisa a pessoa."
                                                 : "Nenhum pedido por enquanto."; break;
    case PG_BLOQ:    titulo = "Pessoas bloqueadas";
                     sub = recomenda_n_bloqueados() ? "Elas não te acham, não veem seu perfil e não te pedem amizade."
                                                    : "Você não bloqueou ninguém."; break;
    case PG_PERFIL:  titulo = "Meu perfil";
                     sub = "Estranhos veem só o apelido e o que você marcar aqui. Nunca aparecem e-mail, conta, addons nem o que você assiste sem você ligar. Desligar apaga tudo na hora."; break;
    case PG_GEN:     titulo = "Gêneros favoritos"; sub = "Até 5 aparecem no seu perfil."; break;
    default:         titulo = temC ? c.apelido : ""; sub = ""; break;
  }

  hx = x + PE_PAD; hw = PE_INTERNO; hy = y + PE_PAD;
  { TxtLinha t = txt_linha(TXT_CAPTION2, chapeu, 174, 178, 188, 255);
    txt_desenhar_alpha(t, hx, hy, a * 0.95f); hy += 28.0f; }

  if (pagina == PG_CARTAO && temC) {
    GfxRect av;
    float tx = hx + PE_AV_CARTAO + 26.0f;
    char rel[96], gens[200];
    hy += 10.0f;
    av = (GfxRect){ hx, hy, PE_AV_CARTAO, PE_AV_CARTAO };
    rec_avatar(av, c.avatar, c.apelido, c.pub, a);
    rel[0] = 0;
    if (!strcmp(c.relacao, "amigo")) snprintf(rel, sizeof rel, "%s", i18n("Já é seu amigo"));
    else if (!strcmp(c.relacao, "enviado")) snprintf(rel, sizeof rel, "%s", i18n("Pedido enviado"));
    else if (!strcmp(c.relacao, "recebido")) snprintf(rel, sizeof rel, "%s", i18n("Quer ser seu amigo"));
    { TxtLinha n = txt_linha_corta(TXT_HEADLINE, c.apelido, 245, 248, 255, 255, hw - PE_AV_CARTAO - 26.0f);
      // Sem linha de relacao o nome fica centrado na altura da foto.
      float ny = rel[0] ? hy + 8.0f : hy + (PE_AV_CARTAO - n.h) * 0.5f;
      txt_desenhar_alpha(n, tx, ny, a);
      if (rel[0]) {
        TxtLinha t = txt_linha_corta(TXT_DET_META2, rel, 170, 174, 185, 255, hw - PE_AV_CARTAO - 26.0f);
        txt_desenhar_alpha(t, tx, hy + 62.0f, a * 0.9f);
      } }
    hy += PE_AV_CARTAO + 18.0f;
    if (c.bio[0])
      hy += txt_bloco(TXT_DET_META2, c.bio, 214, 218, 228, hx, hy, hw, 30.0f, a * 0.95f, 2) + 12.0f;
    textoGeneros(gens, sizeof gens, c.generos);
    if (gens[0]) {
      TxtLinha t = txt_linha_corta(TXT_CAPTION, gens, 190, 194, 205, 255, hw);
      txt_desenhar_alpha(t, hx, hy, a * 0.92f); hy += 36.0f;
    }
    if (recomenda_cartao_n_recentes()) {
      char rec[300], tit[80];
      size_t k = (size_t)snprintf(rec, sizeof rec, "%s: ", i18n("Assistiu recentemente"));
      int q;
      for (q = 0; q < recomenda_cartao_n_recentes() && k + 90 < sizeof rec; q++)
        if (recomenda_cartao_recente(q, tit, sizeof tit))
          k += (size_t)snprintf(rec + k, sizeof rec - k, "%s%s", q ? "  \xc2\xb7  " : "", tit);
      { TxtLinha t = txt_linha_corta(TXT_CAPTION2, rec, 160, 164, 175, 255, hw);
        txt_desenhar_alpha(t, hx, hy, a * 0.9f); }
    }
  } else {
    hy += txt_bloco(TXT_HEADLINE, titulo, 245, 248, 255, hx, hy, hw, 46.0f, a, 1) + 6.0f;
    if (sub[0]) txt_bloco(TXT_DET_META2, sub, 170, 174, 185, hx, hy, hw, 30.0f, a * 0.92f, 3);
  }

  for (i = topo; i < nL && i - topo < janela(); i++) {
    float by = y + PE_PAD + cab + (float)(i - topo) * (PE_LINHA + PE_GAP);
    desenhaLinha(x + PE_PAD, by, &linhas[i], focoAnim[i], a);
  }

  if (aviso[0]) {
    TxtLinha t = txt_linha_corta(TXT_CAPTION, aviso, 214, 218, 228, 255, PE_INTERNO);
    txt_desenhar_alpha(t, x + PE_PAD, y + alt - PE_PAD - PE_RODAPE + 4.0f, a * 0.95f);
  }
  { TxtLinha t = txt_linha(TXT_CAPTION2,
        pagina == PG_MENU ? "↑ ↓ Navegar   OK Selecionar   Voltar Fechar"
                          : "↑ ↓ Navegar   OK Selecionar   Voltar Anterior",
        155, 159, 169, 255);
    txt_desenhar_alpha(t, x + PE_PAD, y + alt - PE_PAD - t.h, a * 0.86f); }

  teclado_desenhar(agora);
}
