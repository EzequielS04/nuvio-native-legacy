# Handoff — integração Nuvio 2.0 (ex-1.8), 05/10/2026 ~14h

Quem lê: o agente que assume a coordenação da integração. Leia inteiro antes de juntar qualquer coisa.
Para desempenho há um documento à parte: [HANDOFF-PERFORMANCE.md](HANDOFF-PERFORMANCE.md).

## Onde

| Item | Valor |
|---|---|
| Checkout de integração | `/Users/hrocha/.codex/worktrees/integration-180-glass/nuvio-native-legacy` |
| Branch | `codex/integration-180-glass` — **sem push** (release pública = v1.7.4) |
| HEAD | `443b1f5d` (+ este documento) |
| Repo do dono | `/Users/hrocha/Projetos/Pessoal/LG WEB/nuvio-native-legacy` (branch `fix/tpk-selo-hdr`) — **não tocar** |
| Mockups 2.0 | `/Volumes/ExternalSSD/nv-analise/mockups-20/` |
| Auditoria issues/logs | `/Volumes/ExternalSSD/nv-auditoria/` (`ISSUES-MATRIZ.md`, `LOGS-ANALISE.md`) |

## Como trabalhar (combinado com o dono)

- O coordenador **não programa no checkout de integração**: subagentes trabalham em worktree própria no SSD
  (`git -C <integração> worktree add -b <branch> /Volumes/ExternalSSD/nv-<nome> HEAD`), o coordenador faz merge `--no-ff`.
- Sonnet por padrão; Opus só para causa difícil, estado/concorrência ou redesenho grande.
- `export TMPDIR=/Volumes/ExternalSSD/tmp` (disco interno quase cheio).
- Depois de cada merge: testes focados do que mudou ("menos teste, mais rápido") + compilação inteira no Mac limpa:
  `cc src/*.c -o /Volumes/ExternalSSD/<x>-bin -O1 -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -Wno-deprecated-declarations`
- Merges de Ajustes conflitam no fim do enum `AJ_*` (ajustes.c, ajustes_ux_padrao.inc, ajustes_ux_dados.inc): concatenar os blocos na ordem do merge e atualizar a assert da última opção em `tests/ajustes_ux_dados.c`. Opção nova precisa de ícone em `ajustes_ux_visual.inc` e i18n em todos `src/idioma_*.h`.
- **Instalar nas TVs só quando o dono pedir.** Não mandar teclas enquanto ele usa. Julgar visual por captura real da TV (Montserrat), não pelo fixture do Mac (Inter).
- Backend (D1 `nuvio-recomendacoes`, worker) **só com ok explícito do dono**, passo a passo.
- Nunca postar respostas de issue sem aprovação. Nunca imprimir/guardar token ou credencial.
- Commits em inglês terminando com `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## Build e deploy (quando o dono pedir)

1. `git -C <integração> worktree add --detach /Volumes/ExternalSSD/nuvio-180-apk-<sha> HEAD`
2. Trocar `1.7.4 → 1.8.0` em `deploy/app/appinfo.json` e `tools/tizen-config.xml`, commit.
3. `export NUVIO_PROPERTIES="/Users/hrocha/Projetos/Pessoal/LG WEB/NuvioWeb-0.3.38-beta/local.properties"`; `bash tools/env.sh --require-core`.
4. Android: `bash tools/release-android.sh` → `build/release-1.8.0/Nuvio-1.8.0-android.apk`; conferir `/v1/registro` no libmain.so;
   `adb -s 192.168.1.128:5555 install -r <apk>` (fora do sandbox); puxar o APK instalado e comparar sha256.
5. LG C9: `NUVIO_SSH_OPTS="-o ProxyCommand=none -o ConnectTimeout=10" bash tools/arm.sh --alto-cache` (sempre alto-cache; confere md5 e relança).
   Leitura: `sshpass -p alpine ssh -o StrictHostKeyChecking=no -o ProxyCommand=none root@192.168.1.32`, log em `/tmp/nuvio.log`.

**Nas TVs agora:** Android = `78ff220a`, LG = `ba1132a9`. Tudo depois disso (splash, handoff, etc.) ainda não foi instalado.

## Feito hoje (05/10)

- `29aad584` handoff de desempenho.
- `443b1f5d` merge `feat/n1b-splash`: splash do sistema neutra #0E0F12 sem logo (LG splash.png + bgColor, Android drawable + ícone transparente no v31, WGT sem icon.png; TPK não tem splash). Sem prova em TV; gradle não rodou (só aapt2 compile), tizen.sh não rodou (sem emcc).
- **Migração 009 (enquete) aplicada no D1 remoto** com ok do dono (tabelas `enquete`, `enquete_opcao`, `enquete_voto`, `enquete_optout` + índice, conferidas).
  `wrangler d1 execute --file` falha com `Authentication error [code: 10000]` (endpoint /import); usar `--command "<sql sem comentários>"`.
  **Falta (pedir ok):** `npx wrangler@4 deploy --config servidor/recomendacoes/wrangler.toml` e semear a primeira enquete (exemplo em `N3-ENQUETE.md`).
- Mockup What's New 2.0 publicado: https://claude.ai/artifact/4BTSfCUDQ1CJ7MrxWm49S7 (fonte `mockups-20/05-whatsnew-20.html`).
  Decisão do dono: o guia 2.0 abre uma vez **também em instalação nova** (apresenta o Guia de uso do menu). Aguardando aprovação do mockup para implementar.

## Agentes em voo (o novo coordenador NÃO recebe as notificações deles — conferir as worktrees)

| Frente | Worktree / branch | Modelo | Estado em 14h | Ao terminar |
|---|---|---|---|---|
| Frost / Arte borrada na LG (fundo escuro, arte sumida) reaproveitando o caminho da luz ambiente da Home Dinâmica + dump único `/tmp/nuvio-fundo-<tipo>.bmp` | `/Volumes/ExternalSSD/nv-frostamb`, `fix/frost-ambiente-lg` | Opus | editando `fundo.c/h`, `gfx.c/h`, `tests/fluidez_perf.c`, sem commit | Merge, compilação, `tests/fundo_assado.sh`; **perguntar antes** de mandar para a LG |
| Imagens distintas por submenu/seção de Ajustes + crossfade fluido | `/Volumes/ExternalSSD/nv-submenu`, `feat/ajustes-imagens-submenus` | Opus | sem mudanças visíveis ainda | Merge; conferir `ajustes_ux_visual.inc` e assert de `tests/ajustes_ux_dados.c` |
| Mockup da tela Explorar (única não redesenhada), 2–3 variações | sem worktree de código; saída `mockups-20/06-explorar.html` (já existe, 2,3 MB) | Sonnet | arquivo escrito, relatório não chegou | Ler o arquivo inteiro, publicar como artifact ("Explorar 2.0"), mandar link + resumo das variações ao dono |
| What's New 2.0 conforme mockup aprovado (abre 1x para todos na 1ª abertura da 2.0, inclusive instalação nova) | `/Volumes/ExternalSSD/nv-wn20`, `feat/whatsnew-20` | Opus | começando | Merge, fixture, i18n; conferir assert de Ajustes |
| Social: ligar Simkl e Letterboxd à pessoa única (tela na aba Amigos; servidor já aceita, ver `F08-SOCIAL-IDENTIDADE.md`) | `/Volumes/ExternalSSD/nv-simklbox`, `feat/social-simkl-letterboxd` | Sonnet | começando | Merge. Simkl precisa do secret `SIMKL_CLIENT_ID` no worker (pedir ok; nunca imprimir o valor) |

Se um agente sumiu sem commit (limite de tokens), retomar a partir da worktree parcial com um agente novo apontando o que já está feito (`git -C <wt> diff`).

## Esperando o dono

- Escolher a variação do Explorar: https://claude.ai/artifact/TMzk2mta36E4PhFPbbH6GR (recomendação: B + grade C como entrada + botão no Detalhe).
- Deploy do worker da enquete + semear enquete.
- Secret `SIMKL_CLIENT_ID` no worker.
- Página de perfil/estatísticas (refazer; mockup v2 foi achado "mal feito", adiado).
- Página de filme renovada/animada para a 2.0 (só mencionada, sem mockup).
- Logo "Clássico renovado" (desenhar).
- Postar respostas das issues (rascunhos em `ISSUES-MATRIZ.md`).
- Tamanho do D1 (4,2 GB).

## Pendências técnicas conhecidas

- Testes pré-existentes quebrados até `88d00591`: `detail_layout` e `home_hero_layout` (já consertados nesse commit; se voltarem, ver stub `desc_pedir_titulo_semente` e 11º argumento `friendsH`).
- Arquivo não rastreado no checkout: `docs/releases/1.8.0/HANDOFF-NEXT-AGENT.md` (de outra sessão; não apagar, não commitar sem saber de quem é).
- Fila de desempenho: ver `HANDOFF-PERFORMANCE.md` (carrossel do detalhe na C9, long tasks Samsung, onTrimMemory Android, arte pequena decodificada grande, etc.).
