# Integração local para 1.7.2 — 02/10/2026

Base: `origin/master` em `2bc9659b` (1.7.1). Worktree: `/private/tmp/nv-172`, branch `release/1.7.2`.

`agente/abrirrapido` integrada em `971788b2`; já contém `agente/ilhavolta` e `agente/i221`. `agente/i216` integrada em `4fa3647e`. Sem conflitos e sem push, tag, release ou comentário em issue.

## Verificação focada

Passaram: `i18n.sh`, `fonteauto.sh`, `fontecache_vod.sh`, `fontevolta.sh`, `ilha_voo.sh`, `ponteiro_toque.sh`, `sessao_email.sh`, `fontes_parcial.sh`, `player_retido.sh`. Logs locais em `/private/tmp/nv172-<teste>.log`. São testes de host, não confirmação nas TVs.

A TCL recebeu o APK estável 1.7.1 de `~/.cache/nuvio-rel-171`, com SHA256 conferido, instalação `-r` bem-sucedida e abertura confirmada por processo ativo. Android confirmou `versionCode=10701`, `versionName=1.7.1`. Não recebeu ainda as mudanças 1.7.2. O recorte de logcat da abertura não contém marcadores de crash; não comprova reprodução ou interação.

## Pendências de decisão e teste

- Dono decidiu retenção de dois minutos. Durante retenção o pipeline único impede trailer da Home; após o prazo, a fonte guardada continua disponível para retomada.
- A versão dos manifestos permanece 1.7.1 até fechar escopo e gerar build identificada de 1.7.2.
- O dono testa o comportamento nas TVs; não rodar suíte longa nem controlar aparelhos sem combinar e obter a trava exclusiva.
- Pré-busca MKV ainda tem cálculo unsigned que pode produzir tempo enorme no log. Alterar a espera pode aumentar abertura em até quatro segundos; decisão separada.
- Início no ponto salvo e conferência paralela da fonte continuam propostas, não implementações confirmadas desta integração.
- `cat_definir_na_lista` requer revisão da leitura de `n`/`itens` sem trava.
- Plugins/P2P ficam para 1.8. Ajustes UX/visual, ícones e social ficam fora desta integração.
- Respostas nas issues exigem autorização do dono.
