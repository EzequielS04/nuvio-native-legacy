# Mapa vivo das issues

Base: `80fc8e6d` (integracao/2.0.3). Atualizado em 2026-10-08. 354 issues (abertas e fechadas) de iqui27/nuvio-native-legacy.

## Como atualizar

1. Chegou issue nova, ou um conserto entrou numa branch/tag: edite **uma** entrada em `docs/issues/mapa.json` (procure por `"numero": N`) e rode `python3 docs/issues/mapa.py`, que valida e reescreve este arquivo. Sem o script, edite a linha equivalente aqui.
2. Campos: `numero`, `titulo`, `plataforma` (LG, Samsung .tpk, Samsung .wgt, Android, all, `?`), `tipo` (bug, feature, question, meta), `status`, `release` (2.0.2 ou tag antiga, 2.0.3 = integracao/2.0.3, `2.0.4 (branch)`, 2.1/2.2/futuro, `-`), `conserto` (hashes curtos com a ref entre parênteses), `ultima_resposta` (`nos` = o último comentário é nosso, `data`, `ultimo_comentario_por`, `ultima_nossa`), `proximo_passo`, `notas`, `alvo` (só issues ABERTAS, obrigatório: `2.0.3`, `2.0.4`, `2.1`, `2.2`, `futuro`, `nao vamos fazer`, ou `ja-lancada` quando já saiu e só falta fechar). `python3 docs/issues/mapa.py --check` valida e confere se o MAPA.md está em dia, sem escrever; o gerador sai com erro se uma issue aberta não tiver alvo. O roadmap e as decisões ficam em `roadmap` e `decisoes` no json.
3. Vocabulário de `status`: `aberta`, `respondida`, `consertada-nao-lancada`, `lancada`, `por-desenho`, `fora-do-escopo`, `precisa-log`, `duplicada`. Extra: `fechada-sem-resposta` (issue fechada sem nenhum comentário).
4. Regra de honestidade: só vale `consertada-nao-lancada`/`lancada` com commit na ref. "Lançado" = commit contido numa tag `v*`. Sem commit, escreva "suspeita" ou "sem commit" em `conserto`/`notas`. "Lançada" em issue antiga sem commit com `#N` quer dizer: a nossa resposta cita uma versão que existe como tag (ver nota na linha).
5. Atenção: mensagens de commit com `(#203)` / `(#204)` falam da VERSÃO 2.0.3 / 2.0.4, não das issues #203/#204. Esses dois números foram ignorados na busca por commits.
6. Próximo passo "postar correção": há rascunhos em `/Volumes/ExternalSSD/tmp/203-respostas-correcao.md` (#302 #350 #286 #312 #368 #360); o dono decide. Correção conhecida: "Dolby Vision in MKV" é DESLIGADO por padrão (a resposta do #312 disse ligado).

## Roadmap

Alvo de cada issue aberta e os recursos por versão. Alvos são decisão de planejamento, não promessa pública; os pontos marcados SUSPEITA não têm confirmação.

### 2.0.3 - congelada (integracao/2.0.3, ainda não lançada)

Só entra o que já está em integracao/2.0.3. Lista completa em docs/releases/2.0.3/NOTAS.md; o mapa abaixo marca cada issue aberta com conserto lá. Inclui #361 (URL de cartaz longa).


### 2.0.4 - planejada; já tem branches agente/203-3xx e agente/204-*, nada integrado

Pequenos recursos de player e Biblioteca que já têm código fora da 2.0.3, mais triagem dos bugs sem conserto (pedir log).

- **Próximo episódio automático após 5 s** (#256, #331, #326, #329): Estende a contagem estilo Netflix do #331 (agente/203-331) e o pedido #256; é uma opção no mesmo card, então vai junto. Sem branch próprio para os 5 s: SUSPEITA de que baste um valor padrão.
- **Estatísticas do player e tamanho do buffer** (#338): Código pronto em agente/203-338; a resposta já prometeu "next build".
- **Biblioteca: indicadores de visto, esconder assistidos, vitrine de coleção, detalhes da fonte** (#352, #366, #362, #355): Pequenos, cada um em seu branch agente/204-*; não dependem do redesign.
- **Catálogos da conta, sync de add-ons, fonte recusada, P2P por formato** (#358, #365, #360, #364, #349): Branches agente/204-conta-catalogos, 204-sync-addons, 204-fonte-recusada prontos ou quase.
- **Samsung: DTS/TrueHD escondido (fase 1)** (#313): Plano e spike em agente/204-samsung-dts (docs/plans); fase 1 já planejada para a 2.0.4.
- **Android: fileiras ilimitadas na Home, armazenamento rotativo do P2P** (#334): agente/204-fileiras-android e agente/203-334-janela (janela de streaming do P2P) existem e ficaram fora da 2.0.3 por risco.
- **Triagem de bugs sem conserto e dois pedidos pequenos (#306, #337)** (#288, #315, #316, #344, #345, #346, #353, #357, #367, #372, #373, #378, #379, #306, #337): Todos aguardam log do autor; entram na 2.0.4 se o log aparecer a tempo, senão escorregam.

### 2.1 - aprovada pelo dono em 06/10/2026 (e 07/10 para Ajustes); sem data

Os seis recursos aprovados pelo dono, mais perfis com o pacote de UX dos Ajustes. Os itens marcados "suspeita" foram colocados aqui pelo agente anterior e NÃO estão aprovados.

- **Watch Together (Assistir Juntos)**: Aprovado pelo dono para a 2.1. Sala compartilhada entre aparelhos; o branch já carrega a marca de perfil infantil enviada para a sala. Origem: dono 06/10. Branches: `feat/watch-together`.
- **Diário + sono (Diário + sono)**: Aprovado pelo dono para a 2.1. Origem: dono 06/10. Branches: `feat/sp-diario`.
- **Trava (Lock)**: Aprovado pelo dono para a 2.1. Origem: dono 06/10. Branches: `feat/sp-trava`.
- **Caça / Conquistas (Hunt/Achievements)**: Aprovado pelo dono para a 2.1. Origem: dono 06/10. Branches: `feat/sp-conquistas`.
- **Redesign da Biblioteca (Biblioteca)**: Branch agente/202-biblioteca (procedência por título, aba Listas); o plano de dados está em docs/plans/biblioteca-ilimitada.md (branch agente/biblioteca-ilimitada-plano) e diz "não na 2.0.3". Mexe em memória e navegação; não cabe num ponto de versão. Origem: dono 06/10. Branches: `agente/202-biblioteca`, `agente/biblioteca-ilimitada-plano (docs/plans/biblioteca-ilimitada.md)`.
- **Coleções + fileiras**: Aprovado pelo dono para a 2.1. Os pequenos ajustes de vitrine (#362) continuam na 2.0.4 em agente/204-colecao-vitrine e não dependem disto. Origem: dono 06/10. Branches: `feat/colecoes-fileiras`, `agente/201-colecoes`.
- **Perfis + tour, "Começar", "Resolver um problema" e busca com i18n nos Ajustes**: Decisão de UX dos Ajustes de 07/10: esses quatro itens saem na 2.1 junto com perfis. Branch de cada um: não identificado (suspeita: parte do trabalho está em feat/ajustes-ux e agente/204-perfil, não confirmado). Origem: dono 07/10.
- **Limites maiores de Trakt/Biblioteca (sem teto)**: Parte do plano biblioteca-ilimitada, que o dono aprovou só como redesign; tirar os tetos do Trakt como item próprio não foi citado por ele. A 2.0.3 só pagina a watchlist do Trakt até 400 itens e mantém os tetos de hoje (96ccdab1). Tirar o teto exige o índice leve + janela de CatItem do plano biblioteca-ilimitada, que é parte do redesign. Origem: suspeita (sugestão do agente, não aprovado).
- **Remoção de barras pretas embutidas no quadro (blackbar/crop)** (#341): Recurso novo, sem issue nem branch (relacionados: #341 aspecto/zoom e #241 barras do trailer, esse já tratado). Precisa detectar a barra no quadro em cada pipeline (LG, .tpk, Android). SUSPEITA de 2.1; pode virar 2.0.4 se for só um recorte manual por título. Origem: suspeita (sugestão do agente, não aprovado).
- **Dolby Vision em MKV na Samsung**: A 2.0.3 só traz DV em MKV na LG (opcional, desligado) e a Samsung nunca escolhe DV. Na Samsung falta prova de que o decodificador aceita; precisa de spike por modelo antes de prometer. SUSPEITA de 2.1. Origem: suspeita (sugestão do agente, não aprovado).
- **Menus em árabe e auto sync de legenda por linha** (#250, #333, #374): SUSPEITA do agente, fora do roadmap do dono: o árabe nos menus foi prometido na resposta do #325 e o auto sync tem pesquisa em legenda-sync-lg-samsung-pesquisa.md, mas nenhum dos dois consta da decisão de 06/10. Pode sair da 2.1 para futuro se o dono preferir. Origem: suspeita (sugestão do agente, não aprovado).
- **Plano de refatoração geral**: docs/plans/refatoracao-geral.md (branch agente/refatoracao-plano, 0dc06334): 23 mudanças pequenas, cada uma com porteiro de teste. Fazer depois da 2.0.3 sair e fora da 2.0.4, para não misturar refatoração com os recursos acima. Itens 01-04 (apagar morto, deduplicar Biblioteca) vão melhor junto do redesign da Biblioteca. Origem: suspeita (sugestão do agente, não aprovado). Branches: `agente/refatoracao-plano`.

### 2.2 - aprovada pelo dono em 06/10/2026; sem data

Servidores locais, guia de TV e música.

- **Servidores locais (Jellyfin / Plex / Emby)**: Aprovado pelo dono para a 2.2; ele tem um servidor real para testar. Origem: dono 06/10. Branches: `f11-jellyfin`, `feat/emby-plex`.
- **Guia de TV (guia pro)**: Aprovado pelo dono para a 2.2. Existem várias linhas de trabalho: guia-tv, guia-pro, guia-pro-2, lembrete e a UI w18; qual é a mais recente fica a conferir. Origem: dono 06/10. Branches: `feat/guia-tv`, `feat/guia-pro`, `feat/guia-pro-2`, `feat/guia-lembrete`, `ui/w18-guia-tv`.
- **Música (álbum da trilha sonora)**: Aprovado pelo dono para a 2.2: álbum da trilha, só link do Spotify, prévia para testar na TV e no celular, crowdsourcing no Android permitido, Apple Insight via uts-api timed-metadata. Branch: nenhum encontrado. Origem: dono 06/10.

### futuro - sem data nem dono

Pedidos que fazem sentido mas não têm plano.

- **Porte VIDAA, layout RTL, anime-skip.com, busca por microfone, pontos do carrossel estilo Apple TV, serviços na barra lateral** (#135, #260, #343, #347, #348, #304, #342): Cada um é grande ou depende de terceiros; nenhum tem branch.

### nao vamos fazer - fora do escopo do produto

PC, iOS, e itens que não são trabalho de código.

- **App para PC (#354), .ipa para iOS (#307), relatório automático de logs (#324), aviso de desempenho em 4K (#292)** (#354, #307, #324, #292): Plataformas que o projeto não atende ou issues que não pedem mudança.

Plano de refatoração: `docs/plans/refatoracao-geral.md` (branch `agente/refatoracao-plano`, ainda não integrado).

### Issues abertas por alvo

| Alvo | Qtd | Issues |
|---|---|---|
| 2.0.3 | 36 | #246, #266, #269, #280, #283, #284, #286, #294, #302, #305, #308, #311, #312, #317, #318, #319, #320, #321, #322, #323, #328, #330, #332, #334, #335, #339, #340, #341, #350, #356, #359, #361, #363, #368, #369, #370 |
| 2.0.4 | 30 | #256, #288, #306, #313, #315, #316, #326, #329, #331, #337, #338, #344, #345, #346, #349, #352, #353, #355, #357, #358, #360, #362, #364, #365, #366, #367, #372, #373, #378, #379 |
| 2.1 | 3 | #250, #333, #374 |
| 2.2 | 0 |  |
| futuro | 7 | #135, #260, #304, #342, #343, #347, #348 |
| nao vamos fazer | 4 | #292, #307, #324, #354 |
| ja-lancada | 9 | #144, #252, #287, #290, #293, #296, #298, #300, #303 |

## Decisões para o dono

1. Próximo episódio automático após 5 s: entra na 2.0.4 junto da contagem do #331, ligado por padrão ou desligado? Recomendação: 2.0.4, desligado por padrão, valor configurável. O #256 ainda pede que o card volte depois de dispensado; isso fica fora.
2. #360 (botão Sincronizar addons) fica em 2.0.4 mesmo com a mescla do sync já na 2.0.3? Recomendação: Sim, 2.0.4; corrigir a resposta que perguntou "on 2.0.3?" dizendo que a mescla vem agora e o botão depois.
3. #334 sai como 2.0.3 só com o limite de P2P e a limpeza, e a janela de streaming (armazenamento rotativo, agente/203-334-janela) fica para a 2.0.4? Recomendação: Sim: limite e limpeza já estão em 2.0.3; a janela muda o motor de P2P e a 2.0.3 está congelada. Avisar na issue que o rotativo vem na 2.0.4.
4. #326 e #338 foram prometidos para "próxima atualização/next build" e o código só está em branches de 2.0.4. Responder já avisando 2.0.4? Recomendação: Sim, corrigir a promessa agora.
5. Bugs sem log (#288, #315, #316, #344-346, #353, #357, #367, #372, #373, #378, #379) ficam todos em 2.0.4? Recomendação: Sim como alvo de triagem, mas sem prometer: pedir log primeiro; os que não chegarem com log em 30 dias viram "precisa-log" e saem do alvo.
6. "nao vamos fazer": #354 (PC), #307 (.ipa iOS), #324 (relatório de logs), #292 (aviso 4K Android). Confirma? Recomendação: Confirmar #354 e #307 (fora de escopo, já classificados fora-do-escopo). #324 e #292 não são pedidos de recurso: fechar como informativos em vez de "não vamos fazer".
7. Pedidos de interface (#304 serviços na barra lateral, #347 microfone, #348 pontos do carrossel/botão Reproduzir, #342 botão Pular cena) ficam em "futuro" ou "nao vamos fazer"? Recomendação: Futuro para #304, #348 e #347. #342 (conteúdo explícito) depende de base de dados de terceiros que não existe: recomendo nao vamos fazer.
8. #135 (porte VIDAA) e #260 (RTL) em "futuro" ou "nao vamos fazer"? Recomendação: Futuro; há trabalho feito em feat/vidaa e uma resposta pública dizendo "depois" para o RTL.
9. Issues abertas só aguardando fechamento (#144, #252, #287, #290, #293, #296, #298, #300, #303): fechar? Recomendação: Fechar com comentário, depois que o dono aprovar o texto. Alvo registrado como ja-lancada (valor extra além dos cinco pedidos).
10. Blackbar/crop (remover barras pretas embutidas) é 2.1, 2.2 ou 2.0.4? Não consta do roadmap aprovado. Recomendação: Sugestão do agente, não aprovada: manter fora das versões aprovadas; se o dono quiser, 2.2 ou futuro, pois é recurso novo, sem branch e toca os três pipelines de vídeo.
11. Dolby Vision em MKV na Samsung: manter na 2.1, passar para 2.2 ou futuro? Não consta do roadmap aprovado. Recomendação: Sugestão do agente, não aprovada: futuro, só como spike de pesquisa; não prometer ao público até haver prova num modelo.
12. Limites maiores do Trakt/Biblioteca: entram dentro do redesign aprovado da 2.1 ou ficam fora? Recomendação: Sugestão do agente, não aprovada: dentro do redesign (o plano biblioteca-ilimitada é parte dele). O redesign em si já está aprovado; a 2.0.4 fica só com indicadores de visto (#352) e esconder assistidos (#366).
13. Plano de refatoração geral (docs/plans/refatoracao-geral.md): quando? Não consta do roadmap aprovado. Recomendação: Sugestão do agente, não aprovada: depois da 2.0.3, na 2.1 como trabalho próprio; na 2.0.4 no máximo os itens 01-04 (apagar morto). Não misturar com recursos.
14. Menus em árabe (#250, #333) e auto sync de legenda (#374) seguem na 2.1? Não constam do roadmap aprovado; o árabe foi prometido na resposta do #325. Recomendação: Sugestão do agente, não aprovada: manter na 2.1 só se sobrar espaço; senão futuro. Corrigir a promessa pública do #325 se for para futuro.

## Resumo

Por status:

| Status | Qtd |
|---|---|
| lancada | 234 |
| consertada-nao-lancada | 44 |
| aberta | 24 |
| respondida | 20 |
| por-desenho | 14 |
| precisa-log | 7 |
| fechada-sem-resposta | 6 |
| fora-do-escopo | 3 |
| duplicada | 2 |

Por release (grupo de planejamento):

| Grupo | Qtd |
|---|---|
| lançadas em tag v* (qualquer versão) | 234 |
| sem release | 67 |
| 2.0.3 (integracao/2.0.3) | 37 |
| 2.0.4 (branches) | 11 |
| futuro (2.1/2.2) | 5 |

Lançadas por versão: 1.0.7: 2, 1.0.10: 1, 1.0.13: 1, 1.0.15: 1, 1.0.16: 1, 1.0.21: 1, 1.0.23: 1, 1.0.29: 1, 1.0.30: 3, 1.0.31: 1, 1.0.32: 1, 1.0.34: 1, 1.0.35: 1, 1.0.36: 1, 1.0.38: 2, 1.0.41: 1, 1.0.43: 5, 1.0.44: 4, 1.0.45: 1, 1.0.51: 4, 1.0.53: 1, 1.0.54: 1, 1.0.55: 1, 1.0.56: 1, 1.1.0: 2, 1.1.2: 2, 1.2.1: 4, 1.3.0: 1, 1.3.2: 4, 1.3.4: 6, 1.3.4-comparacao1: 1, 1.3.5: 1, 1.3.7: 1, 1.3.10: 1, 1.3.11: 2, 1.3.12: 4, 1.4: 6, 1.4.1: 1, 1.4.2: 9, 1.4.3: 9, 1.4.4: 2, 1.4.5: 2, 1.4.6: 8, 1.4.7: 2, 1.5.0: 1, 1.5.1: 5, 1.5.2: 8, 1.5.3: 3, 1.5.4: 4, 1.6.0: 11, 1.6.1: 1, 1.6.2: 3, 1.6.3: 2, 1.6.4: 5, 1.6.5: 4, 1.7.0: 11, 1.7.1: 4, 1.7.2: 3, 1.7.4: 4, 2.0.0: 28, 2.0.1: 12, 2.0.2: 19.

Abertas no GitHub: 89. Fechadas: 265.
Abertas sem nenhum comentário nosso: 37.

## Sai na 2.0.3 (integracao/2.0.3, ainda não lançada)

37 issues.

| # | Título | Plat. | Tipo | Status | Release | Alvo | Conserto | Última resposta | Próximo passo |
|---|---|---|---|---|---|---|---|---|---|
| [#246](https://github.com/iqui27/nuvio-native-legacy/issues/246) | HEVC anime videos can’t be “seeked in player” | ? | bug | precisa-log | 2.0.3 | 2.0.3 | a7f23592, a8048e0d, 023497dd | nós 10-06 | pedir log (pedido em 06/10, sem retorno) |
| [#266](https://github.com/iqui27/nuvio-native-legacy/issues/266) | Bug: Doesn't open on ATv | Android | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | eabe3afd, b03aa2f0, 35421d1a | autor 10-08 | responder (confirmou QR com teste-318.x; nova queixa: legenda diferente do Nuvio oficial) |
| [#269](https://github.com/iqui27/nuvio-native-legacy/issues/269) | Embedded subtitles are not being detected or not being displ | Samsung (tpk/wgt?) | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 80569112, 6ec3907b, b35bcbcf | autor 10-07 | nada (aguardar release 2.0.3) |
| [#280](https://github.com/iqui27/nuvio-native-legacy/issues/280) | Home Row does not update properly (Bingecat Addon) and Some  | ? | bug | precisa-log | 2.0.3 | 2.0.3 | 3c51d5b7 | autor 10-06 | pedir log (autor mandou só captura "See logs", sem código) |
| [#284](https://github.com/iqui27/nuvio-native-legacy/issues/284) | Source result ("Best for this tv") missing resolution | ? | feature | consertada-nao-lancada | 2.0.3 | 2.0.3 | df492356, a63ec982 | sem comentários | responder (sem resposta nossa ainda) |
| [#286](https://github.com/iqui27/nuvio-native-legacy/issues/286) | Performance on older Samsung UA40N5300 | Samsung .tpk | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 325ed7a0, e03080dd | nós 10-08 | postar correção |
| [#294](https://github.com/iqui27/nuvio-native-legacy/issues/294) | 🐛 Home collection order resets after switching profiles | ? | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 66e6ae9e, 26d845c6, abd0770a | nós 10-07 | nada (aguardar release 2.0.3) |
| [#302](https://github.com/iqui27/nuvio-native-legacy/issues/302) | Live TV issue persists with 2.1 tpk65 | Samsung .tpk | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 6c4d3863, 8fea5018, 0021b573 | nós 10-08 | postar correção |
| [#305](https://github.com/iqui27/nuvio-native-legacy/issues/305) | Hide player ui when pressing up | ? | feature | consertada-nao-lancada | 2.0.3 | 2.0.3 | 98bee79f | nós 10-07 | nada (aguardar release 2.0.3) |
| [#308](https://github.com/iqui27/nuvio-native-legacy/issues/308) | [bug] subtitles block source | ? | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | e52fae3f, 6269f306, b65c3324 | nós 10-07 | nada (aguardar release 2.0.3) |
| [#311](https://github.com/iqui27/nuvio-native-legacy/issues/311) | [suggestion] option to disable "from nuvio search" | ? | feature | consertada-nao-lancada | 2.0.3 | 2.0.3 | 607b25bf, a86e55c2, 0d5ca11b | nós 10-08 | avisar Namer03 quando sair (prometido: "I'll ping") |
| [#312](https://github.com/iqui27/nuvio-native-legacy/issues/312) | few minor bugs, none affect use | LG | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | a86e55c2, 317cb179 | nós 10-08 | postar correção |
| [#317](https://github.com/iqui27/nuvio-native-legacy/issues/317) | App doesn't  open on my LG webOS Tv UK6550PSB | LG | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 9a17b810, 8cf12920, d423b051 | nós 10-07 | nada (aguardar release 2.0.3) |
| [#318](https://github.com/iqui27/nuvio-native-legacy/issues/318) | screen flickers and stops responding when pressing OK (andro | Android | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 142ae407, 773658c8, 730556e1 | autor 10-08 | responder (autor testou teste-318.3: ok, enviou log) |
| [#319](https://github.com/iqui27/nuvio-native-legacy/issues/319) | Home row still not updating properly | ? | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 66e6ae9e, 3c51d5b7, 0fcfa601 | nós 10-07 | nada (aguardar release 2.0.3) |
| [#320](https://github.com/iqui27/nuvio-native-legacy/issues/320) | Arabic language in Subtitle shows no glyph font | Samsung .tpk | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 22ecac46, 8ab3b78f | nós 10-07 | nada (aguardar release 2.0.3) |
| [#321](https://github.com/iqui27/nuvio-native-legacy/issues/321) | The "not started" label does not shift downwards. | Samsung .tpk | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | b347004c | nós 10-07 | nada (aguardar release 2.0.3) |
| [#322](https://github.com/iqui27/nuvio-native-legacy/issues/322) | Can't delete from Continue watching on the home screen | Samsung .tpk | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | b8d110de | nós 10-07 | nada (aguardar release 2.0.3) |
| [#323](https://github.com/iqui27/nuvio-native-legacy/issues/323) | App crashes on playback | Android | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 1b6c9a89, 696afa74 | autor 10-07 | nada (autor confirmou: sem o passo de múltiplas fontes não cai) |
| [#327](https://github.com/iqui27/nuvio-native-legacy/issues/327) | Hero doesn’t working / Catalog bug | Samsung .wgt | bug | consertada-nao-lancada | 2.0.3 | - | 8ab1b19b, d268b069 | autor 10-07 | nada (aguardar release 2.0.3) |
| [#328](https://github.com/iqui27/nuvio-native-legacy/issues/328) | 🐛 Bug Report: Episodes Are Being Duplicated | ? | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 9c5d0027, 7edab4b0 | sem comentários | responder (sem resposta nossa) |
| [#330](https://github.com/iqui27/nuvio-native-legacy/issues/330) | [Bug]extremely laggy after repeated playback | ? | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 2f047898 | sem comentários | responder (sem resposta nossa) |
| [#332](https://github.com/iqui27/nuvio-native-legacy/issues/332) | Nuvio didn't finish opening | Android | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | eabe3afd, b03aa2f0, 35421d1a | nós 10-07 | pedir log (após build com /dev/urandom + HttpURLConnection) |
| [#334](https://github.com/iqui27/nuvio-native-legacy/issues/334) | P2P filled the TV's free space" error when playing large/4K  | ? | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | c3e19d00, d3ca720b, 8e09dad2 | nós 10-07 | nada (aguardar release 2.0.3) |
| [#335](https://github.com/iqui27/nuvio-native-legacy/issues/335) | Extend Arabic subtitle size scale (200%-250%), ASS support,  | Samsung (tpk/wgt?) | feature | consertada-nao-lancada | 2.0.3 | 2.0.3 | 01016036, 531e344a, bbc31cb2 | nós 10-07 | nada (aguardar release 2.0.3) |
| [#339](https://github.com/iqui27/nuvio-native-legacy/issues/339) | Pantalla de ajustes | ? | feature | consertada-nao-lancada | 2.0.3 | 2.0.3 | 607b25bf, b1b53000, 30e13fa4 | nós 10-07 | nada (aguardar release 2.0.3) |
| [#340](https://github.com/iqui27/nuvio-native-legacy/issues/340) | Quicker Seek/skipping | ? | feature | consertada-nao-lancada | 2.0.3 | 2.0.3 | bc63d8ca | sem comentários | responder (sem resposta nossa) |
| [#341](https://github.com/iqui27/nuvio-native-legacy/issues/341) | Aspect Ratio / Crop feature to fill screen does not work on  | Samsung .tpk | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | beb5007a | sem comentários | responder (sem resposta nossa; pedir teste) |
| [#350](https://github.com/iqui27/nuvio-native-legacy/issues/350) | Major bug | Samsung .tpk | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 4b8ba24b | autor 10-08 | postar correção |
| [#356](https://github.com/iqui27/nuvio-native-legacy/issues/356) | Upcoming shows not appearing in continue watching | ? | bug | precisa-log | 2.0.3 | 2.0.3 | d47a78c8, 9af09e0c | nós 10-08 | pedir log (já pedido em 08/10) |
| [#359](https://github.com/iqui27/nuvio-native-legacy/issues/359) | 🐛 Bug Report: Floating Sidebar Overlaps/Interferes With Cont | ? | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 1860216b | nós 10-08 | nada (aguardar release 2.0.3) |
| [#361](https://github.com/iqui27/nuvio-native-legacy/issues/361) | Poster URL Max Character Length Too Short | all | feature | consertada-nao-lancada | 2.0.3 | 2.0.3 | 9ac3531d, 2220b019, 81f7128b | sem comentários | responder (sem resposta nossa); avisar quando a 2.0.3 sair |
| [#363](https://github.com/iqui27/nuvio-native-legacy/issues/363) | Unable to Send Recommendation to a Friend | Android | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 54cf2fab, 9bb93887, 08e58b14 | autor 10-08 | nada (aguardar release 2.0.3) |
| [#368](https://github.com/iqui27/nuvio-native-legacy/issues/368) | Tab enhancement | ? | feature | consertada-nao-lancada | 2.0.3 | 2.0.3 | dbec4f44, 1af6067e | nós 10-08 | postar correção |
| [#369](https://github.com/iqui27/nuvio-native-legacy/issues/369) | [Bug] Multiple issues/Missing Features on Android TV version | Android | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 712557ca, 5f04c36b | sem comentários | responder (sem resposta); relatório de 9 itens, só 2 com commit |
| [#370](https://github.com/iqui27/nuvio-native-legacy/issues/370) | Subtitles sync issues | Samsung .wgt | bug | consertada-nao-lancada | 2.0.3 | 2.0.3 | 2aee231b, 3fe3c8ca, 56c9832e | nós 10-08 | nada (aguardar release 2.0.3) |
| [#371](https://github.com/iqui27/nuvio-native-legacy/issues/371) | [bug] Ui issue viewing "profile & stats" | Samsung .tpk | bug | consertada-nao-lancada | 2.0.3 | - | b39f4149, 1cdd1cbb, 2fa6e51a | autor 10-08 | nada (aguardar release 2.0.3) |

Notas:

- **#246**: NOTAS 2.0.3: "LG: resume no longer drops the source on a failed seek (#246)". Não confirmado que cobre o relato HEVC/anime; plataforma do relato desconhecida. ALVO 2.0.3 = SUSPEITA: o conserto está nas NOTAS mas não está confirmado que cobre o relato; falta log do autor.
- **#266**: Vários relatos juntos (Shield/TCL/BRAVIA). Reporter charles474 confirmou QR com build de teste em 08/10. Parte do conserto já na 2.0.2 (a1d5e039).
- **#269**: MKV >2 GB rejeitado pelo leitor de legendas; conserto 80569112 só na 2.0.3 (2.0.1/2.0.2 tiveram partes). Resposta também prometeu "setting para o usuário escolher este comportamento": item vago, sem commit identificado.
- **#280**: SUSPEITA: único commit que cita #280 é 3c51d5b7 (remontagens da descoberta, 2.0.3); a correção é genérica de fileiras. ALVO 2.0.3 = SUSPEITA: o conserto está nas NOTAS mas não está confirmado que cobre o relato; falta log do autor.
- **#284**: a63ec982: linha "Melhor para esta TV" mostra resolução. Sem comentário nosso na issue.
- **#286**: Rascunho de correção em 203-respostas-correcao.md: nome certo é "Resolução da interface > 720p (leve)". Mali-400 (Utgard).
- **#294**: Parte na 2.0.2 (b4186acd); 66e6ae9e/26d845c6/abd0770a na 2.0.3.
- **#302**: Rascunho de correção: o conserto do live TV NÃO está na 2.0.2 (resposta anterior dizia que sim). 0021b573 (2.0.2) é parcial.
- **#311**: Resposta corrigiu: opções de busca não estão na 2.0.2, saem na 2.0.3.
- **#312**: Nossa resposta disse que "Dolby Vision in MKV" vem LIGADO por padrão: ERRADO, é DESLIGADO (docs 203-dvmkv-decisao.md; ajustes_ux_padrao.inc). Resumo traduzido e "All sources" saem na 2.0.3, não na 2.0.2.
- **#317**: LG webOS 4 com pouca RAM; partes na 2.0.2 (0fcfa601, 937f8e6b).
- **#318**: Tudo integrado em 2.0.3. O relator de queda nativa no Android < 12 (c8793c11 em agente/203-318-play) entrou como 142ae407, o mesmo commit com outro hash; tests/queda.sh passa em c3c1032e.
- **#320**: Nenhum commit cita #320; 22ecac46/8ab3b78f ("tpk: arabic plain subtitles find Noto Naskh when the installed res/ predates 2.0.2") batem com a promessa "o próximo update busca a fonte sozinho".
- **#321**: b347004c "rotulo de secao desce com o cartao aberto" (cita #203 = versão); NOTAS 2.0.3 o lista.
- **#322**: b8d110de: up next removível + opção para desligar (cita "#203" = versão).
- **#323**: Tudo integrado em 2.0.3. O SIGSEGV em verificarOuParar (Conferencia na pilha) está consertado por 696afa74, que veio no merge 1b6c9a89: a Conferencia vai para o heap com contagem e é solta pelo último fio. f349c2f7/4d22e4f7 são uma versão anterior do mesmo conserto (sem liberar) e não precisam entrar; SANITIZE=1 tests/fonteparalela.sh passa em c3c1032e.
- **#327**: Fechada no GitHub antes do release 2.0.3.
- **#332**: SUSPEITA: relato Sony BRAVIA preso em "rede _preparar"; commits citados são os do #266. A resposta final do dono fala de outro assunto (contorno do vidro dos pôsteres, citado como "corrigido para a próxima versão"). teste-shield-266.2 não resolveu segundo EzequielS04.
- **#334**: Limite de P2P e limpeza integrados em 2.0.3 (d3ca720b, c3e19d00, merge 891a72a9 de agente/203-334b; agente/203-334 tem os mesmos dois commits com outro hash). Só a janela de streaming ("armazenamento rotativo", prometido como planejado: d453fd1c e 8e09dad2 em agente/203-334-janela) NÃO está integrada; alvo 2.0.4 para essa parte.
- **#341**: SUSPEITA: nenhum commit cita #341; beb5007a "tpk 4/5: botão de aspecto/zoom do player passa a ter efeito" está na 2.0.3 e NOTAS cita "aspect/zoom button on Tizen 4/5".
- **#350**: Rascunho de correção: nome do ajuste é "OK no card"; "Ver detalhes" no menu de segurar sai na 2.0.3. Autor relatou depois filme travando durante TV ao vivo: responder.
- **#356**: d47a78c8 (Cinemeta antes da ficha Nuvio, 2.0.3) cobre uma suspeita, sem confirmação. Config "quanto à frente" planejada 2.0.4 (sem commit).
- **#361**: NOTAS 2.0.3: posters com URL longa (>~500 caracteres) chegam inteiros ao fundo, detalhe, Salvos e conta (merge 9ac3531d, agente/203-361). Pedido original pedia limite maior; URL de fundo/logo longa demais é descartada com linha de log.
- **#368**: Rascunho de correção: ocultar add-ons no guia sai na 2.0.3, não na 2.0.2; busca espera 300 ms.
- **#369**: Parcial: tailandês (712557ca) e ocultar não lançados (5f04c36b). Os outros itens do relatório de 9: sem commit identificado.
- **#371**: Fechada no GitHub antes do release 2.0.3.

## Planejado na 2.0.4 (com branch)

11 issues.

| # | Título | Plat. | Tipo | Status | Release | Alvo | Conserto | Última resposta | Próximo passo |
|---|---|---|---|---|---|---|---|---|---|
| [#313](https://github.com/iqui27/nuvio-native-legacy/issues/313) | [suggestion] downmix DTS to AC3 5.1 | Samsung | feature | aberta | 2.0.4 (agente/204-samsung-dts, só plano) | 2.0.4 | bf3e71e4, f45ddc14 | autor 10-07 | nada (aguardar fase 1 da 2.0.4); autor se ofereceu para testar |
| [#326](https://github.com/iqui27/nuvio-native-legacy/issues/326) | Add continue watching options | ? | feature | consertada-nao-lancada | 2.0.4 (agente/203-326, agente/203-331) | 2.0.4 | ac424deb, 56868756 | nós 10-07 | corrigir a promessa: dissemos "próxima atualização"; código só em branch 2.0.4 |
| [#329](https://github.com/iqui27/nuvio-native-legacy/issues/329) | Add skip intro, recaps and credits. | ? | feature | consertada-nao-lancada | 2.0.4 (agente/203-331) | 2.0.4 | a3cd9757, 7a69fda5, a0a392fb | sem comentários | responder (sem resposta nossa) |
| [#331](https://github.com/iqui27/nuvio-native-legacy/issues/331) | 💡 Feature Request: Netflix-Style Next Episode Countdown | ? | feature | consertada-nao-lancada | 2.0.4 (agente/203-331) | 2.0.4 | a3cd9757, 9e065265, 4c17e680 | sem comentários | responder (sem resposta nossa) |
| [#338](https://github.com/iqui27/nuvio-native-legacy/issues/338) | Add Stats for Nerds and Custom Buffer Size | all | feature | consertada-nao-lancada | 2.0.4 (agente/203-338) | 2.0.4 | c446b6a1, 8c856535 | nós 10-07 | corrigir a promessa: dissemos "next build"; código só em branch 2.0.4 |
| [#352](https://github.com/iqui27/nuvio-native-legacy/issues/352) | [Feature Request] Library Watched Indicators, Watched/Unwatc | ? | feature | consertada-nao-lancada | 2.0.4 (agente/204-biblioteca-visto) | 2.0.4 | 402e2438 | nós 10-08 | nada (aguardar 2.0.4) |
| [#355](https://github.com/iqui27/nuvio-native-legacy/issues/355) | Option to view full/untruncated text and metadata for addon  | ? | feature | consertada-nao-lancada | 2.0.4 (agente/204-fonte-detalhes) | 2.0.4 | b176eff5 | autor 10-08 | nada (aguardar 2.0.4) |
| [#358](https://github.com/iqui27/nuvio-native-legacy/issues/358) | 🐛 Bug Report: Some Add-on Catalogs Are Incorrectly Moved to  | Samsung .tpk | bug | consertada-nao-lancada | 2.0.4 (agente/204-conta-catalogos) | 2.0.4 | da8fdee7, 6212d5b8, 29c640b7 | autor 10-08 | ler log MD8G2R e responder |
| [#360](https://github.com/iqui27/nuvio-native-legacy/issues/360) | Sync addons | ? | feature | consertada-nao-lancada | 2.0.4 (agente/204-sync-addons) | 2.0.4 | 46507ea5, 6b305295, e2f1eede | autor 10-08 | postar correção |
| [#362](https://github.com/iqui27/nuvio-native-legacy/issues/362) | Collection View Options | ? | feature | consertada-nao-lancada | 2.0.4 (agente/204-colecao-vitrine) | 2.0.4 | 22320582, f49af7d4 | nós 10-08 | nada (aguardar 2.0.4) |
| [#364](https://github.com/iqui27/nuvio-native-legacy/issues/364) | Sources unbable to open | ? | bug | consertada-nao-lancada | 2.0.4 (agente/204-fonte-recusada) | 2.0.4 | bf666aa7, 90abb8aa | sem comentários | responder (sem resposta nossa) |

Notas:

- **#313**: bf3e71e4 (2.0.2) converte DTS->AC3 só no webOS; f45ddc14 (2.0.2) apenas detecta e avisa na tpk. Conversão na Samsung = fase 2 do plano docs/plans/samsung-dts.md (sem código).
- **#326**: PROMESSA SEM CÓDIGO NA 2.0.3: resposta "Both in the next update", mas ac424deb/56868756 só estão em agente/203-326 e agente/203-331 (2.0.4).
- **#329**: Pular intro/recap automático em agente/203-331. 7a69fda5 é de PR externo (pr-351).
- **#338**: PROMESSA SEM CÓDIGO NA 2.0.3: estatísticas ao vivo (e52360ed, agente/203-central/203-veloc) e buffer Android (c446b6a1, agente/203-338) não estão em integracao/2.0.3.
- **#352**: Selo de assistido e ordenar na Biblioteca; menu em árabe separado (futuro).
- **#358**: 29c640b7 em agente/204-conta-catalogos. Commits da8fdee7 e 6212d5b8 citam #358 mas não estão em nenhuma branch local.
- **#360**: Mescla do sync em 2.0.3 (46507ea5, 6b305295); botão "Sincronizar addons" só 2.0.4 (e2f1eede). Namer03 perguntou "on 2.0.3?"; rascunho de correção. Alvo 2.0.4 porque a issue pede o botão "Sincronizar addons" (e2f1eede, fora da 2.0.3); a mescla do sync já está em 2.0.3.

## Futuro (2.1, 2.2, depois)

3 issues.

| # | Título | Plat. | Tipo | Status | Release | Alvo | Conserto | Última resposta | Próximo passo |
|---|---|---|---|---|---|---|---|---|---|
| [#250](https://github.com/iqui27/nuvio-native-legacy/issues/250) | Add Arabic language support | all | feature | respondida | 2.1 | 2.1 | - | nós 10-06 | nada |
| [#260](https://github.com/iqui27/nuvio-native-legacy/issues/260) | Add an optional layout switch to change UI direction from Ri | all | feature | respondida | futuro | futuro | - | autor 10-06 | nada |
| [#343](https://github.com/iqui27/nuvio-native-legacy/issues/343) | [suggestion] anime-skip.com | ? | feature | respondida | futuro (anime-skip.com) | futuro | - | nós 10-08 | nada |

Notas:

- **#250**: Metadados em árabe na 2.0.1; menus em árabe planejados para 2.1 (segundo a resposta em #325).
- **#260**: Layout RTL "planejado para depois"; sem branch.
- **#343**: AniSkip/TheIntroDB já na 2.0.1 (3a02817d); anime-skip.com "na atualização seguinte".

## Aberta sem plano

32 issues.

| # | Título | Plat. | Tipo | Status | Release | Alvo | Conserto | Última resposta | Próximo passo |
|---|---|---|---|---|---|---|---|---|---|
| [#135](https://github.com/iqui27/nuvio-native-legacy/issues/135) | Looking for testers: experimental Hisense VIDAA port (Hisens | VIDAA | feature | respondida | - | futuro | faacf493, 08d21c20, a81639d2 | autor 10-06 | responder (feedback do gabo748 em 06/10 sem retorno) |
| [#144](https://github.com/iqui27/nuvio-native-legacy/issues/144) | Align UI with nuvioTV (android) | Android | feature | respondida | - | ja-lancada | e1a31298, b7d10412, 581b2f82 | autor 10-08 | nada (autor disse que não vê mais os problemas; pode fechar) |
| [#252](https://github.com/iqui27/nuvio-native-legacy/issues/252) | Add default audio language setting for anime | ? | feature | por-desenho | - | ja-lancada | - | autor 10-08 | nada (autor confirmou que funciona; pode fechar) |
| [#256](https://github.com/iqui27/nuvio-native-legacy/issues/256) | Add Autoplay for Next Episode & Allow Customizable "Up Next" | ? | feature | aberta | - | 2.0.4 | - | autor 10-08 | responder (novo pedido: card no canto e autoplay mesmo após dispensar) |
| [#288](https://github.com/iqui27/nuvio-native-legacy/issues/288) | Profile selection background is grainy dark video | LG? (TCL C6K no formulário) | bug | aberta | - | 2.0.4 | - | sem comentários | responder / pedir log (sem resposta) |
| [#292](https://github.com/iqui27/nuvio-native-legacy/issues/292) | GoogleTV performance UPDATE | Android | question | por-desenho | - | nao vamos fazer | - | sem comentários | nada |
| [#304](https://github.com/iqui27/nuvio-native-legacy/issues/304) | Apple tv dynamic homeacreen | ? | feature | aberta | - | futuro | - | sem comentários | responder (sem resposta) |
| [#306](https://github.com/iqui27/nuvio-native-legacy/issues/306) | Refresh Live TV | ? | feature | aberta | - | 2.0.4 | - | sem comentários | responder (sem resposta) |
| [#307](https://github.com/iqui27/nuvio-native-legacy/issues/307) | Request | iOS | feature | fora-do-escopo | - | nao vamos fazer | - | autor 10-07 | responder (sem resposta; pedido de .ipa) |
| [#315](https://github.com/iqui27/nuvio-native-legacy/issues/315) | .avi media files fail to play ("Could not open the source") | Samsung .tpk | bug | aberta | - | 2.0.4 | - | sem comentários | responder / pedir log (sem resposta) |
| [#316](https://github.com/iqui27/nuvio-native-legacy/issues/316) | Live tv schedule | Samsung .tpk | bug | aberta | - | 2.0.4 | - | sem comentários | responder / pedir log (sem resposta) |
| [#324](https://github.com/iqui27/nuvio-native-legacy/issues/324) | Relatório de logs | all | meta | por-desenho | - | nao vamos fazer | 83c2cedd | nós 10-08 | nada |
| [#333](https://github.com/iqui27/nuvio-native-legacy/issues/333) | Arabic language | all | feature | duplicada | 2.1 | 2.1 | - | autor 10-07 | responder (apontar #250/#260; sem resposta nossa) |
| [#337](https://github.com/iqui27/nuvio-native-legacy/issues/337) | Be able to hide the "skip" intro button | ? | feature | aberta | - | 2.0.4 | - | sem comentários | responder (sem resposta) |
| [#342](https://github.com/iqui27/nuvio-native-legacy/issues/342) | Add an optional "Skip Scene" floating button for explicit co | all | feature | aberta | - | futuro | - | sem comentários | responder (sem resposta) |
| [#344](https://github.com/iqui27/nuvio-native-legacy/issues/344) | Live TV search crash | Android | bug | aberta | - | 2.0.4 | - | sem comentários | ler log AX1R49 e responder |
| [#345](https://github.com/iqui27/nuvio-native-legacy/issues/345) | Audio non-existent on startup | Android | bug | aberta | - | 2.0.4 | - | sem comentários | ler log DEPCJ0 e responder |
| [#346](https://github.com/iqui27/nuvio-native-legacy/issues/346) | Continue Watching unselectable | Android | bug | aberta | - | 2.0.4 | - | sem comentários | ler log e responder |
| [#347](https://github.com/iqui27/nuvio-native-legacy/issues/347) | [suggestion] search by microphone | ? | feature | aberta | - | futuro | - | sem comentários | responder (sem resposta) |
| [#348](https://github.com/iqui27/nuvio-native-legacy/issues/348) | Dots y imdb | ? | feature | aberta | - | futuro | - | sem comentários | responder (sem resposta) |
| [#349](https://github.com/iqui27/nuvio-native-legacy/issues/349) | Source Preference Option for MP4 and P2P | ? | feature | aberta | - | 2.0.4 | - | sem comentários | responder (sem resposta); relacionado a #364 |
| [#353](https://github.com/iqui27/nuvio-native-legacy/issues/353) | Hero page trailer only plays once and has no sound | LG | bug | aberta | - | 2.0.4 | - | nós 10-08 | responder / pedir log (sem resposta humana) |
| [#354](https://github.com/iqui27/nuvio-native-legacy/issues/354) | Bring the Full TV Experience to PC - Same App, No Compromise | PC | feature | fora-do-escopo | - | nao vamos fazer | - | sem comentários | responder (sem resposta) |
| [#357](https://github.com/iqui27/nuvio-native-legacy/issues/357) | Nuvio v2.0.1/2.0.2 – Crashes on Zidoo Z9X 8K and Ugoos AM9 P | Android | bug | precisa-log | - | 2.0.4 | - | autor 10-08 | responder: como tirar log sem abrir o app (adb logcat) |
| [#365](https://github.com/iqui27/nuvio-native-legacy/issues/365) | 🐛 Bug Report: Ghost Catalogs Still Remain in “Not on Home Sc | Samsung .tpk | bug | aberta | - | 2.0.4 | - | sem comentários | responder / pedir log (sem resposta) |
| [#366](https://github.com/iqui27/nuvio-native-legacy/issues/366) | [Feature request] Optional setting to hide watched movies fr | ? | feature | aberta | - | 2.0.4 | - | sem comentários | responder (sem resposta) |
| [#367](https://github.com/iqui27/nuvio-native-legacy/issues/367) | [Bug][webOS] Audio control sometimes opens Subtitles on firs | LG | bug | aberta | - | 2.0.4 | - | sem comentários | responder / pedir log (sem resposta) |
| [#372](https://github.com/iqui27/nuvio-native-legacy/issues/372) | [Bug] Incorrect Season/Episode Metadata Mapping for TV Anime | Samsung .tpk | bug | aberta | - | 2.0.4 | - | sem comentários | responder / pedir log (sem resposta) |
| [#373](https://github.com/iqui27/nuvio-native-legacy/issues/373) | [bug] missing info after continue watching | Samsung .tpk | bug | aberta | - | 2.0.4 | - | autor 10-08 | responder (autor deu mais detalhes) |
| [#374](https://github.com/iqui27/nuvio-native-legacy/issues/374) | [port] sync subtitles by line "auto sync" | ? | feature | aberta | - | 2.1 | - | sem comentários | responder (sem resposta) |
| [#378](https://github.com/iqui27/nuvio-native-legacy/issues/378) | “From Account” subtitle setting defaults to “NONE” on playba | Samsung .tpk | bug | aberta | - | 2.0.4 | - | autor 10-08 | responder / pedir log (ghbarker confirmou com +1) |
| [#379](https://github.com/iqui27/nuvio-native-legacy/issues/379) | Movie or TV show at the end never return to homescreen. | Samsung .tpk | bug | aberta | - | 2.0.4 | - | sem comentários | responder / pedir log (sem resposta) |

Notas:

- **#135**: Porte experimental VIDAA; issue de chamada de testadores. Commits faacf493 (2.0.2) e feat/vidaa.
- **#144**: Alvo ja-lancada: já saiu numa versão publicada; só falta fechar.
- **#252**: Opção já existe: Ajustes > Idiomas e legendas > Idioma do áudio. Alvo ja-lancada: a opção já existe.
- **#256**: Parte do pedido tem relação com #331 (contagem estilo Netflix, em agente/203-331 = 2.0.4). Ajuste de tempo do card "ainda não existe" (nossa resposta).
- **#288**: Formulário diz LG webOS mas o modelo é TCL C6K: plataforma incerta.
- **#292**: Aviso nosso (virou issue a partir da discussão #291): resolução da interface em 4K deixa o Android lento. Alvo nao vamos fazer: é aviso/pergunta, não há mudança de código planejada (ver decisões).
- **#307**: SUSPEITA de fora-do-escopo: pedido de .ipa, app é C/SDL para TVs. O dono decide.
- **#324**: Relatório automático de triagem de logs (comentários nossos); não é bug. Alvo nao vamos fazer: não é bug nem pedido.
- **#333**: Duplicata de #250 (Arabic). Comentário no commit c824675c (v1.0.1) é falso positivo.
- **#344**: Log code no corpo; sem resposta nossa.
- **#345**: Log code no corpo; sem resposta nossa.
- **#346**: Sem resposta nossa. Possível relação com #373 (Samsung).
- **#353**: Os 2 comentários nossos na issue são relatórios de triagem automática, não resposta ao autor.
- **#354**: SUSPEITA: versão desktop está fora do escopo (app é para TVs).
- **#357**: Zidoo Z9X 8K (Android 11) fecha ao abrir; Ugoos AM9 Pro. Autor comentou que não achou o aparelho no relatório #324 e não consegue enviar log.
- **#365**: Autor diz que o problema principal da Home foi resolvido; fantasmas permanecem em "Fora da Home". Relacionado a #358.

## Já lançado

234 issues.

### Lançadas e ainda abertas no GitHub (8)

| # | Título | Plat. | Tipo | Status | Release | Alvo | Conserto | Última resposta | Próximo passo |
|---|---|---|---|---|---|---|---|---|---|
| [#283](https://github.com/iqui27/nuvio-native-legacy/issues/283) | Minha tv lg nao consegue abrir nenhum canal dos meus addons | LG | bug | lancada | 2.0.2 | 2.0.3 | 1424e409, 6de601a5, b68bad96 | autor 10-07 | ler log MWASFG e responder |
| [#287](https://github.com/iqui27/nuvio-native-legacy/issues/287) | Auto select forced embedded sub | ? | feature | lancada | 2.0.2 | ja-lancada | 0702a143 | autor 10-07 | nada (autor confirmou; pode fechar) |
| [#290](https://github.com/iqui27/nuvio-native-legacy/issues/290) | Few minor issues on 2.1 | ? | bug | lancada | 2.0.2 | ja-lancada | 75cee76f, 4843e5b0, b5deeb25 | autor 10-06 | nada (pedir confirmação se quiser) |
| [#293](https://github.com/iqui27/nuvio-native-legacy/issues/293) | Audio codec details on player | ? | feature | lancada | 2.0.2 | ja-lancada | 8c856535, db1f5891 | nós 10-07 | nada |
| [#296](https://github.com/iqui27/nuvio-native-legacy/issues/296) | Si podrías agregar estas visitas seria grandioso | Samsung .wgt | feature | lancada | 2.0.2 | ja-lancada | sem commit | autor 10-07 | nada (pode fechar) |
| [#298](https://github.com/iqui27/nuvio-native-legacy/issues/298) | The Arabic subtitles | Samsung (Tizen 6) | feature | lancada | 2.0.2 | ja-lancada | sem commit | autor 10-07 | responder (autor informou Tizen 6; dizer em qual versão testar) |
| [#300](https://github.com/iqui27/nuvio-native-legacy/issues/300) | Default Aspect Ratio Option | ? | feature | lancada | 2.0.2 | ja-lancada | sem commit | nós 10-07 | nada (pode fechar) |
| [#303](https://github.com/iqui27/nuvio-native-legacy/issues/303) | how to remove continue watching from opening screen | ? | feature | lancada | 2.0.2 | ja-lancada | 1300a834 | nós 10-07 | nada (pode fechar) |

Notas:

- **#283**: Canais de add-on passam pelo proxy com headers na 2.0.2 (b68bad96); 1424e409/6de601a5 na 2.0.3. Autor mandou log MWASFG depois da resposta: não confirmado que resolveu. Alvo 2.0.3: NOTAS 2.0.3 cita #283 (guia de TV por add-on); aguarda confirmação do autor.
- **#287**: Alvo ja-lancada: já saiu numa versão publicada; só falta fechar.
- **#290**: 7 commits #290 na 2.0.2 (gradiente do hero sobre trailer em janela, aspecto, etc.). Sem confirmação do autor; base08 confirmou o ponto 1 antes do release. Título original "Few minor issues on 2.1". Alvo ja-lancada: já saiu numa versão publicada; só falta fechar.
- **#293**: Codec/canais do áudio no player na 2.0.2 (db1f5891). Estatísticas completas em 8c856535 (agente/203-338 = 2.0.4). Alvo ja-lancada: já saiu numa versão publicada; só falta fechar.
- **#296**: Coberto pela "Elección de la fuente" (#310) na 2.0.2; nenhum commit cita #296. Alvo ja-lancada: já saiu numa versão publicada; só falta fechar.
- **#298**: #273 (legendas árabes, 546c7418/57e24174) está na 2.0.2; #335 refina na 2.0.3. Nossa resposta de 07/10 disse "próxima release"; já estava na 2.0.2. Alvo ja-lancada: já saiu numa versão publicada; só falta fechar.
- **#300**: Resposta cita 2.0.2; nenhum commit cita #300. Alvo ja-lancada: já saiu numa versão publicada; só falta fechar.
- **#303**: Alvo ja-lancada: já saiu numa versão publicada; só falta fechar.

### Lançadas e fechadas (226)

| # | Título | Plat. | Tipo | Status | Release | Alvo | Conserto | Última resposta | Próximo passo |
|---|---|---|---|---|---|---|---|---|---|
| [#1](https://github.com/iqui27/nuvio-native-legacy/issues/1) | poster dont load on tiles | Samsung (tpk/wgt?) | ? | lancada | 1.1.2 | - | 47473f36, 8a1d6789, a9b251f4 | nós 09-08 | nada (fechada) |
| [#2](https://github.com/iqui27/nuvio-native-legacy/issues/2) | Main Nuvio icon not showing in apps list | LG | bug | lancada | 2.0.0 | - | 6e4bdfb3, 80d1350f, 94e9a6eb | nós 09-07 | nada (fechada) |
| [#3](https://github.com/iqui27/nuvio-native-legacy/issues/3) | Language still showing spanish | ? | ? | lancada | 1.1.2 | - | 8a1d6789, 6eef9cda, 7716f148 | nós 09-07 | nada (fechada) |
| [#4](https://github.com/iqui27/nuvio-native-legacy/issues/4) | Nuvio Keyboard Help | ? | ? | lancada | 1.1.0 | - | a9b251f4, 828731ba, 04c37f87 | autor 09-07 | nada (fechada) |
| [#5](https://github.com/iqui27/nuvio-native-legacy/issues/5) | Nuvio Sync Issue | ? | bug | lancada | 2.0.0 | - | 1f00125d, a9b251f4, 30b2c079 | nós 09-07 | nada (fechada) |
| [#6](https://github.com/iqui27/nuvio-native-legacy/issues/6) | Video Black Screen Issue | ? | bug | lancada | 1.1.0 | - | 7b9e641a, 96ccdab1, d30344ab | autor 09-22 | nada (fechada) |
| [#7](https://github.com/iqui27/nuvio-native-legacy/issues/7) | UI Resolution & Torrent Menu Lag | ? | bug | lancada | 2.0.0 | - | d4eae363, 04c37f87 | autor 09-07 | nada (fechada) |
| [#8](https://github.com/iqui27/nuvio-native-legacy/issues/8) | Profile Switching & Metadata Language Bug | ? | bug | lancada | 1.0.7 | - | 04c37f87 | autor 09-07 | nada (fechada) |
| [#9](https://github.com/iqui27/nuvio-native-legacy/issues/9) | Missing Subtitle Languages Bug | ? | bug | lancada | 1.0.7 | - | 04c37f87 | autor 09-07 | nada (fechada) |
| [#10](https://github.com/iqui27/nuvio-native-legacy/issues/10) | Bug: Collection is completely empty on Tizen | Samsung (tpk/wgt?) | bug | lancada | 1.0.21 | - | b202e3e3, 7791a043, 04c37f87 | autor 09-07 | nada (fechada) |
| [#11](https://github.com/iqui27/nuvio-native-legacy/issues/11) | Feedback for v1.0.7: Profile Management Bug, Syncing & Setti | ? | bug | lancada | 1.0.31 | - | bbfc4bd1, 90743102 | nós 09-07 | nada (fechada) |
| [#12](https://github.com/iqui27/nuvio-native-legacy/issues/12) | Bug: Some UI text is not translated to English on Tizen | Samsung (tpk/wgt?) | bug | lancada | 1.0.43 | - | 1142ee84, 4daaf18a, 0bc78c61 | autor 09-08 | nada (fechada) |
| [#13](https://github.com/iqui27/nuvio-native-legacy/issues/13) | Collections Installed from Nuvio Account Do Not Appear on Ti | Samsung (tpk/wgt?) | bug | lancada | 1.0.10 | - | 7716f148 | nós 09-09 | nada (fechada) |
| [#14](https://github.com/iqui27/nuvio-native-legacy/issues/14) | Bug: Next Episode Does Not Play After Confirming “Play Next” | ? | bug | lancada | 1.0.16 | - | e6cf02a9, 7716f148 | autor 09-08 | nada (fechada) |
| [#16](https://github.com/iqui27/nuvio-native-legacy/issues/16) | Bug: Detail Page Flickers and Randomly Opens a Different Tit | ? | bug | lancada | 1.0.13 | - | 7791a043, b28db878 | autor 09-08 | nada (fechada) |
| [#17](https://github.com/iqui27/nuvio-native-legacy/issues/17) | Bug: Posters Do Not Load When Using Better Posters URL | ? | bug | lancada | 1.0.15 | - | 4723f22f | autor 09-21 | nada (fechada) |
| [#18](https://github.com/iqui27/nuvio-native-legacy/issues/18) | Bug: Xperience Collections Are Split Into Multiple Home Rows | ? | bug | lancada | 1.4.2 | - | 6a3f3a05, 820e5fdb, 2d4e0568 | nós 09-14 | nada (fechada) |
| [#19](https://github.com/iqui27/nuvio-native-legacy/issues/19) | Bug: Random Profile Data Appears Briefly Before My Trakt Pro | ? | bug | lancada | 1.0.23 | - | 5413db8a, 45ae240c, 6aba13b6 | autor 09-09 | nada (fechada) |
| [#21](https://github.com/iqui27/nuvio-native-legacy/issues/21) | Backdrop/Hero image takes too long to load after moving focu | ? | ? | lancada | 1.5.2 | - | cda85061, f59496e6, 6aeb62a9 | autor 09-09 | nada (fechada) |
| [#22](https://github.com/iqui27/nuvio-native-legacy/issues/22) | Unable to remove items from Continue Watching | ? | bug | lancada | 1.0.30 | - | 19b68a33, 65962f31, 9f57ca70 | autor 09-09 | nada (fechada) |
| [#23](https://github.com/iqui27/nuvio-native-legacy/issues/23) | Seeing Portuguese strings in settings and Home Screen | LG | ? | lancada | 1.0.36 | - | 52f43f60, 7ef20234 | nós 09-14 | nada (fechada) |
| [#24](https://github.com/iqui27/nuvio-native-legacy/issues/24) | 🐛 BingeCat Catalogs Not Showing in the Fork | ? | bug | lancada | 1.0.29 | - | 8f1f7a97, ced5436f | autor 09-09 | nada (fechada) |
| [#25](https://github.com/iqui27/nuvio-native-legacy/issues/25) | Bug: Continue Watching Does Not Update Until App Restart | ? | bug | lancada | 1.0.30 | - | sem commit | autor 09-09 | nada (fechada) |
| [#26](https://github.com/iqui27/nuvio-native-legacy/issues/26) | Support question WebOS 26 (LG B4 2024) | LG | question | lancada | 1.0.30 | - | de71b7f9 | nós 09-09 | nada (fechada) |
| [#27](https://github.com/iqui27/nuvio-native-legacy/issues/27) | Log debug | ? | bug | lancada | 1.6.5 | - | 46b1d1b7 | nós 09-15 | nada (fechada) |
| [#28](https://github.com/iqui27/nuvio-native-legacy/issues/28) | Viewport resolution | ? | ? | lancada | 1.0.32 | - | 2de0c2ec, 82b16916 | autor 09-10 | nada (fechada) |
| [#29](https://github.com/iqui27/nuvio-native-legacy/issues/29) | Collection Focus GIFs Do Not Play on Tizen | Samsung (tpk/wgt?) | bug | lancada | 1.0.44 | - | 24f8e451, f3d40b21 | nós 09-14 | nada (fechada) |
| [#30](https://github.com/iqui27/nuvio-native-legacy/issues/30) | Connecting Trakt Causes Collections to Disappear From Home | ? | ? | lancada | 1.0.34 | - | 6559dff3, d34d9d1d, f59dc32c | nós 09-10 | nada (fechada) |
| [#31](https://github.com/iqui27/nuvio-native-legacy/issues/31) | Content Advisory Badges Not Showing Consistently | ? | bug | lancada | 1.0.43 | - | bacddc9c, 6f975f57, 1173250a | autor 09-14 | nada (fechada) |
| [#32](https://github.com/iqui27/nuvio-native-legacy/issues/32) | Bug: “More Like This” menu appears when a movie starts inste | ? | bug | lancada | 1.0.35 | - | sem commit | autor 09-11 | nada (fechada) |
| [#33](https://github.com/iqui27/nuvio-native-legacy/issues/33) | Bug Report: Severe UI stutters / freezes during navigation o | Samsung (tpk/wgt?) | bug | lancada | 1.0.41 | - | 6f975f57, 1173250a, 1444a771 | autor 09-11 | nada (fechada) |
| [#34](https://github.com/iqui27/nuvio-native-legacy/issues/34) | Play next  triggers too soon | ? | ? | lancada | 1.4.6 | - | 8d36a1f2, 0e43c9ad | nós 09-14 | nada (fechada) |
| [#35](https://github.com/iqui27/nuvio-native-legacy/issues/35) | Episodes listed when clicking a show with multiple seasons. | ? | ? | lancada | 1.0.38 | - | 2d995944 | nós 09-14 | nada (fechada) |
| [#36](https://github.com/iqui27/nuvio-native-legacy/issues/36) | Remove from continue watching not highlighting | ? | bug | lancada | 1.0.38 | - | 87d986be | nós 09-14 | nada (fechada) |
| [#37](https://github.com/iqui27/nuvio-native-legacy/issues/37) | Support for New Addon Types | ? | feature | lancada | 1.6.0 | - | 2e9bbce8, 1b84d4bd, bee9db0a | nós 09-15 | nada (fechada) |
| [#38](https://github.com/iqui27/nuvio-native-legacy/issues/38) | Continue Watching row does not update or takes a long time t | ? | bug | lancada | 1.0.44 | - | 24f8e451 | nós 09-14 | nada (fechada) |
| [#39](https://github.com/iqui27/nuvio-native-legacy/issues/39) | Hero/backdrop is delayed | ? | ? | lancada | 1.0.44 | - | 24f8e451 | nós 09-14 | nada (fechada) |
| [#40](https://github.com/iqui27/nuvio-native-legacy/issues/40) | Play/Pause button not working | Samsung (tpk/wgt?) | bug | lancada | 1.0.43 | - | a3c48b28, d04298af | nós 09-15 | nada (fechada) |
| [#41](https://github.com/iqui27/nuvio-native-legacy/issues/41) | Portuguese text in subtitle menu | ? | ? | lancada | 1.0.43 | - | 1142ee84 | nós 09-15 | nada (fechada) |
| [#42](https://github.com/iqui27/nuvio-native-legacy/issues/42) | Issues with version 1.0.42 | ? | bug | lancada | 1.0.51 | - | 06358c1a, 820e5fdb, 2d4e0568 | nós 09-15 | nada (fechada) |
| [#43](https://github.com/iqui27/nuvio-native-legacy/issues/43) | Continue watching row. Always sets you to the first episode  | ? | ? | lancada | 1.0.43 | - | 31ad46da | nós 09-15 | nada (fechada) |
| [#44](https://github.com/iqui27/nuvio-native-legacy/issues/44) | Collections Installed from Nuvio Website Do Not Appear in Ap | Samsung (tpk/wgt?) | bug | lancada | 1.0.44 | - | 24f8e451 | autor 09-14 | nada (fechada) |
| [#45](https://github.com/iqui27/nuvio-native-legacy/issues/45) | Support for gifs as profile picture | ? | feature | lancada | 1.6.5 | - | 46b1d1b7, 24f8e451 | nós 09-14 | nada (fechada) |
| [#46](https://github.com/iqui27/nuvio-native-legacy/issues/46) | Start from the beginning | ? | ? | lancada | 1.0.45 | - | 3267613a | nós 09-15 | nada (fechada) |
| [#48](https://github.com/iqui27/nuvio-native-legacy/issues/48) | Startup log screen on startup | ? | ? | lancada | 1.7.4 | - | e0596c71 | autor 09-15 | nada (fechada) |
| [#49](https://github.com/iqui27/nuvio-native-legacy/issues/49) | GIF plays briefly then flickers and reverts to a static imag | ? | bug | lancada | 1.0.54 | - | 21f68756, 6efca7e5 | autor 09-16 | nada (fechada) |
| [#50](https://github.com/iqui27/nuvio-native-legacy/issues/50) | Player loading screen briefly shows the wrong title when swi | ? | bug | lancada | 1.0.51 | - | sem commit | autor 09-15 | nada (fechada) |
| [#51](https://github.com/iqui27/nuvio-native-legacy/issues/51) | Active catalogs/collections appear in the middle or bottom o | ? | ? | lancada | 1.0.51 | - | sem commit | autor 09-15 | nada (fechada) |
| [#52](https://github.com/iqui27/nuvio-native-legacy/issues/52) | Portugues text in integration tab and studio tab | ? | ? | lancada | 1.0.51 | - | sem commit | nós 09-15 | nada (fechada) |
| [#54](https://github.com/iqui27/nuvio-native-legacy/issues/54) | Feature request | ? | feature | lancada | 1.0.53 | - | 78793586 | autor 09-16 | nada (fechada) |
| [#55](https://github.com/iqui27/nuvio-native-legacy/issues/55) | Bug | ? | bug | lancada | 1.3.2 | - | 8732c235, 758aa5e3 | nós 09-19 | nada (fechada) |
| [#56](https://github.com/iqui27/nuvio-native-legacy/issues/56) | Feature Request – Remember the selected source between episo | ? | feature | lancada | 1.0.56 | - | 98c5bbfb, 1e6a770e | autor 09-16 | nada (fechada) |
| [#57](https://github.com/iqui27/nuvio-native-legacy/issues/57) | Resume opens the source panel instead of the previously used | ? | ? | lancada | 1.0.55 | - | 1e6a770e | nós 09-18 | nada (fechada) |
| [#60](https://github.com/iqui27/nuvio-native-legacy/issues/60) | Trailers of movie under a different one | ? | ? | lancada | 1.2.1 | - | 699e244e | nós 09-19 | nada (fechada) |
| [#61](https://github.com/iqui27/nuvio-native-legacy/issues/61) | bug | ? | bug | lancada | 1.2.1 | - | 699e244e | nós 09-19 | nada (fechada) |
| [#62](https://github.com/iqui27/nuvio-native-legacy/issues/62) | bug | ? | bug | lancada | 1.2.1 | - | 699e244e | nós 09-19 | nada (fechada) |
| [#65](https://github.com/iqui27/nuvio-native-legacy/issues/65) | Application crashes after reaching end of row | ? | bug | lancada | 1.2.1 | - | 699e244e, ecaa6af5 | autor 09-19 | nada (fechada) |
| [#66](https://github.com/iqui27/nuvio-native-legacy/issues/66) | 'Continue Watching' missing shows from Trakt | ? | bug | lancada | 1.3.0 | - | 8e64134a | nós 09-21 | nada (fechada) |
| [#67](https://github.com/iqui27/nuvio-native-legacy/issues/67) | Posters not loading | ? | bug | lancada | 1.3.7 | - | 68f87611, 967fa084, 8732c235 | autor 09-21 | nada (fechada) |
| [#68](https://github.com/iqui27/nuvio-native-legacy/issues/68) | Crashes | ? | bug | lancada | 1.3.2 | - | a8970d44 | autor 09-20 | nada (fechada) |
| [#69](https://github.com/iqui27/nuvio-native-legacy/issues/69) | Hero/backdrop | ? | bug | lancada | 1.3.4 | - | 60bcfc02, a8970d44 | nós 09-19 | nada (fechada) |
| [#70](https://github.com/iqui27/nuvio-native-legacy/issues/70) | Mark as watched issues | ? | bug | lancada | 1.3.2 | - | 701f3db1 | nós 09-19 | nada (fechada) |
| [#71](https://github.com/iqui27/nuvio-native-legacy/issues/71) | Unable to change value on memory used by images | ? | bug | lancada | 1.3.2 | - | 097ef52f | nós 09-19 | nada (fechada) |
| [#72](https://github.com/iqui27/nuvio-native-legacy/issues/72) | Navigating now strutting | Samsung (tpk/wgt?) | bug | lancada | 1.3.4 | - | 47c7b45b, bdca515f, 9a8b220e | nós 09-20 | nada (fechada) |
| [#73](https://github.com/iqui27/nuvio-native-legacy/issues/73) | 'Up next' appearing before the credits again | ? | bug | lancada | 1.4 | - | 876742e2 | nós 09-22 | nada (fechada) |
| [#74](https://github.com/iqui27/nuvio-native-legacy/issues/74) | Very low resolution ticks | ? | ? | lancada | 1.3.4 | - | f4e96196 | nós 09-20 | nada (fechada) |
| [#76](https://github.com/iqui27/nuvio-native-legacy/issues/76) | Build not reading the catalogue correctly | ? | bug | lancada | 1.3.4 | - | 91450a5f | nós 09-20 | nada (fechada) |
| [#77](https://github.com/iqui27/nuvio-native-legacy/issues/77) | Tyzen os v 1.3.3 - TV got struck | ? | bug | lancada | 1.3.5 | - | 3b2e154a, 2764aa05 | autor 09-21 | nada (fechada) |
| [#78](https://github.com/iqui27/nuvio-native-legacy/issues/78) | Unable to scroll through trakt ratings for TV Show | ? | bug | lancada | 1.3.4 | - | 9e388586 | autor 09-20 | nada (fechada) |
| [#79](https://github.com/iqui27/nuvio-native-legacy/issues/79) | Issue with TV Show seasons | ? | bug | lancada | 1.3.4 | - | 9e388586 | nós 09-20 | nada (fechada) |
| [#80](https://github.com/iqui27/nuvio-native-legacy/issues/80) | Samsung: Resume opens source list, slow first stream load, a | Samsung (tpk/wgt?) | bug | lancada | 1.3.4-comparacao1 | - | b43ffef0 | autor 09-20 | nada (fechada) |
| [#82](https://github.com/iqui27/nuvio-native-legacy/issues/82) | Trailer | Samsung (tpk/wgt?) | ? | lancada | 1.3.11 | - | 58d491a7, b3b635d4 | nós 09-21 | nada (fechada) |
| [#83](https://github.com/iqui27/nuvio-native-legacy/issues/83) | Certain addons not showing | ? | bug | lancada | 1.4 | - | 876742e2 | autor 09-22 | nada (fechada) |
| [#84](https://github.com/iqui27/nuvio-native-legacy/issues/84) | GIF not smooth | ? | bug | lancada | 1.4.7 | - | 15e8e1a5, 2883053e | autor 09-25 | nada (fechada) |
| [#85](https://github.com/iqui27/nuvio-native-legacy/issues/85) | Minor visual bugs + settings not saved | Samsung (tpk/wgt?) | bug | lancada | 1.3.10 | - | 6f2eb5f7 | autor 09-21 | nada (fechada) |
| [#86](https://github.com/iqui27/nuvio-native-legacy/issues/86) | Trailer don't play | ? | bug | lancada | 1.3.11 | - | 58d491a7 | autor 09-21 | nada (fechada) |
| [#87](https://github.com/iqui27/nuvio-native-legacy/issues/87) | IMDB rating not shown on continue watching and on episodes | ? | bug | lancada | 1.3.12 | - | 6542d85e | nós 09-21 | nada (fechada) |
| [#88](https://github.com/iqui27/nuvio-native-legacy/issues/88) | App working fine now | ? | ? | lancada | 1.4.1 | - | d683d997 | nós 09-23 | nada (fechada) |
| [#89](https://github.com/iqui27/nuvio-native-legacy/issues/89) | Posters from xperience are cropped (not sized correctly  | ? | bug | lancada | 1.3.12 | - | 9cc8d044 | nós 09-21 | nada (fechada) |
| [#92](https://github.com/iqui27/nuvio-native-legacy/issues/92) | Anime Embedded subtitles broken | LG | bug | lancada | 1.5.0 | - | b61d74a6, 57644d6a, b7515cb8 | autor 09-25 | nada (fechada) |
| [#93](https://github.com/iqui27/nuvio-native-legacy/issues/93) | Autoplay doesnt work on continue watching | ? | bug | lancada | 1.3.12 | - | 8882ce89 | nós 09-21 | nada (fechada) |
| [#94](https://github.com/iqui27/nuvio-native-legacy/issues/94) | Cast list only shows three cast members | ? | ? | lancada | 1.5.1 | - | e3ae1533, ebdf2cd3 | nós 09-22 | nada (fechada) |
| [#95](https://github.com/iqui27/nuvio-native-legacy/issues/95) | Home catalog rows remember previous tile focus after restart | ? | bug | lancada | 1.4.6 | - | 89eab64c, c2f5cef3, 876742e2 | nós 09-24 | nada (fechada) |
| [#97](https://github.com/iqui27/nuvio-native-legacy/issues/97) | All english is in portugues | ? | ? | lancada | 1.3.12 | - | 2b94b895 | autor 09-21 | nada (fechada) |
| [#99](https://github.com/iqui27/nuvio-native-legacy/issues/99) | Possibility of having LG magic mouse cursor support? | LG | question | lancada | 1.4.2 | - | ba8b96c6, 81842912, 37a1a868 | autor 09-23 | nada (fechada) |
| [#100](https://github.com/iqui27/nuvio-native-legacy/issues/100) | Episodes not being marked as watched when completed | LG | bug | lancada | 1.4 | - | 876742e2 | autor 09-23 | nada (fechada) |
| [#101](https://github.com/iqui27/nuvio-native-legacy/issues/101) | Playing an episode sometimes loads sources of previous watch | LG | bug | lancada | 1.4 | - | 876742e2 | autor 09-23 | nada (fechada) |
| [#102](https://github.com/iqui27/nuvio-native-legacy/issues/102) | Episode carousel auto scrolls to first episode of the season | LG | bug | lancada | 1.4 | - | 876742e2 | autor 09-23 | nada (fechada) |
| [#103](https://github.com/iqui27/nuvio-native-legacy/issues/103) | 🐛 Catalog row: Last focused tile gets clipped at the right e | ? | ? | lancada | 1.4 | - | 876742e2 | autor 09-22 | nada (fechada) |
| [#104](https://github.com/iqui27/nuvio-native-legacy/issues/104) | Theme colors | ? | feature | lancada | 1.4.2 | - | e7453e91, 6a61e67e | autor 09-23 | nada (fechada) |
| [#105](https://github.com/iqui27/nuvio-native-legacy/issues/105) | Bug: Auto-selected source can get stuck loading indefinitely | ? | bug | lancada | 1.4.2 | - | 6a61e67e | autor 09-23 | nada (fechada) |
| [#106](https://github.com/iqui27/nuvio-native-legacy/issues/106) | Catalog order doesn't persist between app updates | ? | bug | lancada | 1.4.2 | - | 6a3f3a05, 6a61e67e | nós 09-23 | nada (fechada) |
| [#108](https://github.com/iqui27/nuvio-native-legacy/issues/108) | Request. Pressing 'Season' brings up option to mark all as w | ? | feature | lancada | 1.4.2 | - | sem commit | nós 09-23 | nada (fechada) |
| [#109](https://github.com/iqui27/nuvio-native-legacy/issues/109) | Media player focus | ? | ? | lancada | 1.4.3 | - | 9cc535e8, d683d997 | autor 09-23 | nada (fechada) |
| [#110](https://github.com/iqui27/nuvio-native-legacy/issues/110) | SIMKL no Option for continue list | ? | feature | lancada | 1.4.2 | - | a00f2e23, ae51d9b0 | nós 09-23 | nada (fechada) |
| [#111](https://github.com/iqui27/nuvio-native-legacy/issues/111) | Movie starts with black screen | LG | bug | lancada | 1.4.2 | - | 6a3f3a05 | autor 09-24 | nada (fechada) |
| [#112](https://github.com/iqui27/nuvio-native-legacy/issues/112) | iptv channel don't work in samsung | Samsung (tpk/wgt?) | bug | lancada | 1.4.2 | - | 1c9f7c59 | nós 09-23 | nada (fechada) |
| [#113](https://github.com/iqui27/nuvio-native-legacy/issues/113) | Diagnostic | Samsung (tpk/wgt?) | ? | lancada | 1.4.3 | - | 10e4c3c0, 9cacb9c5 | autor 09-23 | nada (fechada) |
| [#114](https://github.com/iqui27/nuvio-native-legacy/issues/114) | Artwork / Backdrop Loading Regression in v1.4.2 | Samsung (tpk/wgt?) | ? | lancada | 1.4.3 | - | eedf7c81 | autor 09-23 | nada (fechada) |
| [#115](https://github.com/iqui27/nuvio-native-legacy/issues/115) | More like this appears too early  | ? | bug | lancada | 1.4.6 | - | 89eab64c, 8d36a1f2 | nós 09-24 | nada (fechada) |
| [#116](https://github.com/iqui27/nuvio-native-legacy/issues/116) | icons appear to be glitched or incorrect | ? | ? | lancada | 1.4.3 | - | 0fde2217 | nós 09-23 | nada (fechada) |
| [#117](https://github.com/iqui27/nuvio-native-legacy/issues/117) | App appears to be slower in newer version  | ? | bug | lancada | 1.4.3 | - | sem commit | nós 09-23 | nada (fechada) |
| [#118](https://github.com/iqui27/nuvio-native-legacy/issues/118) | incorrect hero artwork in continue watching | ? | bug | lancada | 1.4.3 | - | 10e4c3c0, a994a0fc | nós 09-23 | nada (fechada) |
| [#119](https://github.com/iqui27/nuvio-native-legacy/issues/119) | Update screen: scroll problem and no update button | Samsung (tpk/wgt?) | bug | lancada | 1.4.6 | - | sem commit | autor 09-25 | nada (fechada) |
| [#120](https://github.com/iqui27/nuvio-native-legacy/issues/120) | Send log screen popup on every start | Samsung (tpk/wgt?) | bug | lancada | 1.4.3 | - | d1427b55, a8eb6296 | autor 09-24 | nada (fechada) |
| [#121](https://github.com/iqui27/nuvio-native-legacy/issues/121) | Rewind, forward focus problem | Samsung (tpk/wgt?) | bug | lancada | 1.4.5 | - | 6cce1253, d1427b55, 9cc535e8 | autor 09-24 | nada (fechada) |
| [#122](https://github.com/iqui27/nuvio-native-legacy/issues/122) | Embedded subtitles isn't displayed | Samsung (tpk/wgt?) | bug | lancada | 1.4.3 | - | d1427b55, bae63187, f4f05699 | autor 09-24 | nada (fechada) |
| [#123](https://github.com/iqui27/nuvio-native-legacy/issues/123) | TV Shows / Series — Trailer Support | ? | feature | lancada | 1.4.3 | - | bd405a92, 9293bf5b | nós 09-24 | nada (fechada) |
| [#124](https://github.com/iqui27/nuvio-native-legacy/issues/124) | Trailer Playback in Hero / Backdrop | Samsung (tpk/wgt?) | feature | lancada | 1.5.2 | - | 72e3378f | nós 09-27 | nada (fechada) |
| [#125](https://github.com/iqui27/nuvio-native-legacy/issues/125) | Metadata sync delay | ? | ? | lancada | 1.4.4 | - | sem commit | nós 09-24 | nada (fechada) |
| [#126](https://github.com/iqui27/nuvio-native-legacy/issues/126) | Missing catalogs | ? | bug | lancada | 1.4.6 | - | b61d74a6, 32a83118, 8874e60f | autor 09-24 | nada (fechada) |
| [#127](https://github.com/iqui27/nuvio-native-legacy/issues/127) | No separate upcoming row | ? | bug | lancada | 1.4.7 | - | 563ccc81 | nós 09-27 | nada (fechada) |
| [#128](https://github.com/iqui27/nuvio-native-legacy/issues/128) | (QOL) Hide ui except for progress bar while seeking | ? | ? | lancada | 1.4.5 | - | 6cce1253 | nós 09-24 | nada (fechada) |
| [#129](https://github.com/iqui27/nuvio-native-legacy/issues/129) | Subtitle/audio language doesnt save | ? | bug | lancada | 1.4.4 | - | 94c59a3e, 725a0cbc, 0e6f0da6 | nós 09-24 | nada (fechada) |
| [#130](https://github.com/iqui27/nuvio-native-legacy/issues/130) | Source selection loading multiple files before playing | ? | ? | lancada | 2.0.2 | - | 46a340a3, cd440320, 89c77368 | nós 09-24 | nada (fechada) |
| [#131](https://github.com/iqui27/nuvio-native-legacy/issues/131) | Number of Titles limited to 205 | ? | bug | lancada | 1.4.6 | - | sem commit | nós 09-24 | nada (fechada) |
| [#132](https://github.com/iqui27/nuvio-native-legacy/issues/132) | Only 1 source is listed | Samsung (tpk/wgt?) | bug | lancada | 1.4.6 | - | 52316a09, cd440320 | autor 09-27 | nada (fechada) |
| [#133](https://github.com/iqui27/nuvio-native-legacy/issues/133) | Poster unwatched blur effect doesn't work | Samsung (tpk/wgt?) | bug | lancada | 1.6.0 | - | 340e5791, 3a6960ca, 91ceeeea | autor 09-25 | nada (fechada) |
| [#134](https://github.com/iqui27/nuvio-native-legacy/issues/134) | Please support plugin | ? | feature | lancada | 2.0.0 | - | sem commit | nós 10-06 | nada (fechada) |
| [#136](https://github.com/iqui27/nuvio-native-legacy/issues/136) | Trailer playback issues by source on Samsung AU7000 | Samsung (tpk/wgt?) | bug | lancada | 1.4.6 | - | a81639d2, ea872c3b | autor 09-25 | nada (fechada) |
| [#137](https://github.com/iqui27/nuvio-native-legacy/issues/137) | Samsung: help test a native (.tpk) Nuvio — 2 minutes, TVs fr | Samsung (tpk/wgt?) | ? | lancada | 1.5.4 | - | 6dac818e, 4278426c, dde1abd2 | nós 09-30 | nada (fechada) |
| [#138](https://github.com/iqui27/nuvio-native-legacy/issues/138) | Samsung 2018/2019 (Tizen 4/5): experimental build available, | ? | ? | lancada | 1.6.0 | - | sem commit | nós 09-30 | nada (fechada) |
| [#141](https://github.com/iqui27/nuvio-native-legacy/issues/141) | Some catalog GIFs not playing | ? | bug | lancada | 1.5.3 | - | b8b7a114, d7c0adbb, 68521ca4 | autor 09-28 | nada (fechada) |
| [#142](https://github.com/iqui27/nuvio-native-legacy/issues/142) | Possibility of manually selecting hero artwork | ? | feature | lancada | 1.6.4 | - | 9a352549, 86c5a1c9 | autor 09-25 | nada (fechada) |
| [#145](https://github.com/iqui27/nuvio-native-legacy/issues/145) | The image only uses a small part of the window on the upper  | LG | bug | lancada | 1.6.0 | - | c79995b6 | nós 09-30 | nada (fechada) |
| [#146](https://github.com/iqui27/nuvio-native-legacy/issues/146) | Add more languages | Android | feature | lancada | 1.6.0 | - | sem commit | nós 09-30 | nada (fechada) |
| [#147](https://github.com/iqui27/nuvio-native-legacy/issues/147) | Remote Button Input Delay — Volume and Home Buttons | Samsung (tpk/wgt?) | bug | lancada | 1.5.2 | - | 378127d9, dc18b5e1 | nós 09-30 | nada (fechada) |
| [#149](https://github.com/iqui27/nuvio-native-legacy/issues/149) | Send logs automatically doesn't stay activated | ? | bug | lancada | 1.5.1 | - | 400dd41d, ea0a1373 | nós 09-27 | nada (fechada) |
| [#150](https://github.com/iqui27/nuvio-native-legacy/issues/150) | Episode Descriptions Remain in English Despite Portuguese La | ? | ? | lancada | 1.5.1 | - | 035c538a | nós 09-27 | nada (fechada) |
| [#151](https://github.com/iqui27/nuvio-native-legacy/issues/151) | The next episode doesn't play automatically / disappears fro | Samsung (tpk/wgt?) | bug | lancada | 1.5.3 | - | b274752b, 61fbfa8c | autor 09-28 | nada (fechada) |
| [#153](https://github.com/iqui27/nuvio-native-legacy/issues/153) | Cast artwork does not match names of actors  | ? | bug | lancada | 1.5.1 | - | e3ae1533 | nós 09-27 | nada (fechada) |
| [#156](https://github.com/iqui27/nuvio-native-legacy/issues/156) | Long single line subtitles overflow as ellipsis instead of w | LG | ? | lancada | 1.5.1 | - | 96f5abe4 | nós 09-27 | nada (fechada) |
| [#158](https://github.com/iqui27/nuvio-native-legacy/issues/158) | Live TV (Xtream): channels don't play and guide is empty (LG | LG | bug | lancada | 2.0.2 | - | b68bad96, aad1c947, 31a3da42 | nós 10-06 | nada (fechada) |
| [#159](https://github.com/iqui27/nuvio-native-legacy/issues/159) | Source panel lags while textures are loading | ? | bug | lancada | 1.5.2 | - | 821b39f0 | autor 09-27 | nada (fechada) |
| [#160](https://github.com/iqui27/nuvio-native-legacy/issues/160) | Hero catalogs | ? | ? | lancada | 1.5.2 | - | ebf1a825 | nós 09-27 | nada (fechada) |
| [#162](https://github.com/iqui27/nuvio-native-legacy/issues/162) | Allow removal of certain options from sidebar | ? | feature | lancada | 1.5.2 | - | d4037a42 | nós 09-27 | nada (fechada) |
| [#163](https://github.com/iqui27/nuvio-native-legacy/issues/163) | Number of catalog options shown | ? | feature | lancada | 1.5.2 | - | 6b05521d, 426b0296 | nós 09-27 | nada (fechada) |
| [#164](https://github.com/iqui27/nuvio-native-legacy/issues/164) | Hero "Loading artwork…" placeholder makes focus transitions  | ? | ? | lancada | 1.5.2 | - | cda85061 | nós 09-27 | nada (fechada) |
| [#165](https://github.com/iqui27/nuvio-native-legacy/issues/165) | native app works prefectly | Samsung (tpk/wgt?) | ? | lancada | 1.7.0 | - | 94690ba3, c4e4d922 | autor 09-29 | nada (fechada) |
| [#169](https://github.com/iqui27/nuvio-native-legacy/issues/169) | Can P2P content play on LG? No option to enable it | LG | question | lancada | 1.6.0 | - | sem commit | autor 10-05 | nada (fechada) |
| [#170](https://github.com/iqui27/nuvio-native-legacy/issues/170) | The Tyzen os Native app (tpk - experimental 1.5.2) not openi | Samsung (tpk/wgt?) | bug | lancada | 1.5.4 | - | f7fee47c, 95557d50, 2456b563 | autor 09-30 | nada (fechada) |
| [#171](https://github.com/iqui27/nuvio-native-legacy/issues/171) | P2P addons (Torrentio) missing from sources but shown in the | Samsung (tpk/wgt?) | question | lancada | 2.0.0 | - | 775f9db9 | nós 09-30 | nada (fechada) |
| [#172](https://github.com/iqui27/nuvio-native-legacy/issues/172) | UI: port ideas from the Corby7 fork | ? | feature | lancada | 1.6.2 | - | 2b99d09b, a65080ae, 414cd61e | nós 09-29 | nada (fechada) |
| [#173](https://github.com/iqui27/nuvio-native-legacy/issues/173) | "Update now" not appearing on version 1.5.2 | ? | bug | lancada | 1.5.3 | - | sem commit | autor 09-30 | nada (fechada) |
| [#174](https://github.com/iqui27/nuvio-native-legacy/issues/174) | Episodes missing of some Tv series | Samsung (tpk/wgt?) | bug | lancada | 1.6.0 | - | 582bc2e6 | autor 09-30 | nada (fechada) |
| [#175](https://github.com/iqui27/nuvio-native-legacy/issues/175) | Allow users to choose different Search sources | ? | feature | lancada | 1.6.0 | - | 582bc2e6 | nós 09-30 | nada (fechada) |
| [#176](https://github.com/iqui27/nuvio-native-legacy/issues/176) | LG G5 (webOS 10) feedback: languages, half-size video with 4 | LG | bug | lancada | 1.7.0 | - | 0da72c2a, 6def852c, 3e90ae7f | autor 09-30 | nada (fechada) |
| [#177](https://github.com/iqui27/nuvio-native-legacy/issues/177) | Next episode thumbnail isn't blurred | ? | bug | lancada | 1.6.0 | - | 3f834522, 340e5791, 3a6960ca | autor 10-03 | nada (fechada) |
| [#178](https://github.com/iqui27/nuvio-native-legacy/issues/178) | Trailer zoom options + fullscreen audio issues | Samsung (tpk/wgt?) | bug | lancada | 1.6.3 | - | 5b0e4957, cc3a9d19, 9efe205f | autor 09-30 | nada (fechada) |
| [#179](https://github.com/iqui27/nuvio-native-legacy/issues/179) | Trakt playback scrobbling not working (Now Watching, progres | LG | bug | lancada | 1.6.0 | - | 4d6f3461, 3f834522, 14e69356 | nós 09-30 | nada (fechada) |
| [#180](https://github.com/iqui27/nuvio-native-legacy/issues/180) | Tizen 4/5 native: app loads but exits during sign-in | Samsung (tpk/wgt?) | bug | lancada | 1.6.0 | - | be115c3f, c121f7d1, 6f581853 | nós 09-30 | nada (fechada) |
| [#181](https://github.com/iqui27/nuvio-native-legacy/issues/181) | Native self-update via memfd (download new libnuvio.so, no r | Samsung (tpk/wgt?) | feature | lancada | 1.5.4 | - | ae0ba290 | nós 09-30 | nada (fechada) |
| [#182](https://github.com/iqui27/nuvio-native-legacy/issues/182) | Addons not loading sometimes in LG web os 26 | LG | bug | lancada | 1.5.4 | - | 3f834522, abc5408e | autor 09-30 | nada (fechada) |
| [#184](https://github.com/iqui27/nuvio-native-legacy/issues/184) | Auto-update on native samsung tizen not working | Samsung .tpk | bug | lancada | 1.6.4 | - | 0da4861a | nós 10-01 | nada (fechada) |
| [#185](https://github.com/iqui27/nuvio-native-legacy/issues/185) | colored line bug | Samsung .tpk | bug | lancada | 1.6.3 | - | 359937e6, e9406ad3 | autor 09-30 | nada (fechada) |
| [#186](https://github.com/iqui27/nuvio-native-legacy/issues/186) | Stream Badges are B&W | LG | bug | lancada | 1.6.1 | - | 53d01871 | sem comentários | nada (fechada) |
| [#187](https://github.com/iqui27/nuvio-native-legacy/issues/187) | The Guide feature does not open a title | LG | bug | lancada | 1.6.5 | - | 4c3bcdce, 3951e747, 5aebadf3 | nós 10-01 | nada (fechada) |
| [#188](https://github.com/iqui27/nuvio-native-legacy/issues/188) | Video playback bug | Samsung .tpk | bug | lancada | 2.0.2 | - | a6d357c8, b5deeb25, bfeffd13 | nós 09-30 | nada (fechada) |
| [#190](https://github.com/iqui27/nuvio-native-legacy/issues/190) | Incorrect Title and Epsiode List | LG | bug | lancada | 1.6.2 | - | 000015d7 | autor 10-02 | nada (fechada) |
| [#191](https://github.com/iqui27/nuvio-native-legacy/issues/191) | 1.6.1 and 1.6.0 (.wgt) is extremely lagging | Samsung .wgt | bug | lancada | 1.6.2 | - | 2b99d09b | nós 10-01 | nada (fechada) |
| [#193](https://github.com/iqui27/nuvio-native-legacy/issues/193) | Black screen when exit from app | Samsung .tpk | bug | lancada | 1.7.0 | - | sem commit | autor 10-02 | nada (fechada) |
| [#194](https://github.com/iqui27/nuvio-native-legacy/issues/194) | TMDB Collections Not Working | LG | bug | lancada | 1.6.4 | - | f25d2e7d, 34db0d1a | autor 09-30 | nada (fechada) |
| [#195](https://github.com/iqui27/nuvio-native-legacy/issues/195) | Tpk 1.6.1 - trailer autoplay bug | Samsung (tpk/wgt?) | bug | lancada | 2.0.2 | - | a6d357c8, b5deeb25, 2aa724f4 | autor 10-02 | nada (fechada) |
| [#196](https://github.com/iqui27/nuvio-native-legacy/issues/196) | Not Available toast message upon pressing Play/Pause button | Samsung .tpk | bug | lancada | 1.6.4 | - | 46a80368 | autor 10-01 | nada (fechada) |
| [#197](https://github.com/iqui27/nuvio-native-legacy/issues/197) | Catalogue Not showing | Samsung .tpk | bug | lancada | 1.6.4 | - | f5813122, b98ae904 | nós 10-01 | nada (fechada) |
| [#198](https://github.com/iqui27/nuvio-native-legacy/issues/198) | Stream Badge | LG | feature | lancada | 1.6.5 | - | f26efc21, 46b1d1b7 | autor 10-01 | nada (fechada) |
| [#199](https://github.com/iqui27/nuvio-native-legacy/issues/199) | Continue watching is not showing unaired up next | LG | bug | lancada | 1.7.0 | - | dc9ca3bf, 6e38bb5b, 149f1235 | autor 10-08 | nada (fechada) |
| [#200](https://github.com/iqui27/nuvio-native-legacy/issues/200) | Custom posters not working correctly | LG | bug | lancada | 1.7.0 | - | 98c61346, f0735c7b | autor 10-02 | nada (fechada) |
| [#201](https://github.com/iqui27/nuvio-native-legacy/issues/201) |  Top 10 view inconsistent with automatic view | ? | bug | lancada | 2.0.1 | - | cda0b4fd, 522aba65, 47a3d55d | autor 10-02 | nada (fechada) |
| [#202](https://github.com/iqui27/nuvio-native-legacy/issues/202) | Matching accent colour in settings obscures text | LG | bug | lancada | 2.0.2 | - | 9768862e, 28ca12d9, 325f4e2b | nós 10-02 | nada (fechada) |
| [#203](https://github.com/iqui27/nuvio-native-legacy/issues/203) | Menu bar remain visible during play | Samsung .tpk | bug | lancada | 1.7.0 | - | sem commit | autor 10-02 | nada (fechada) |
| [#204](https://github.com/iqui27/nuvio-native-legacy/issues/204) | Automatic Trailer and LG Smart Magic Not Working | LG | bug | lancada | 1.7.0 | - | sem commit | nós 10-02 | nada (fechada) |
| [#205](https://github.com/iqui27/nuvio-native-legacy/issues/205) | Disappearing "Continue Watching" items and appearing "Resume | Samsung .tpk | bug | lancada | 1.7.0 | - | dc9ca3bf | autor 10-02 | nada (fechada) |
| [#206](https://github.com/iqui27/nuvio-native-legacy/issues/206) | Embedded subtitle and audio language list display during pla | Samsung .tpk | bug | lancada | 1.7.0 | - | 94690ba3 | autor 10-02 | nada (fechada) |
| [#208](https://github.com/iqui27/nuvio-native-legacy/issues/208) | Couldn't resume a movie from the tile | Samsung .tpk | bug | lancada | 1.7.0 | - | 39e4c794, e221d9e7 | autor 10-02 | nada (fechada) |
| [#209](https://github.com/iqui27/nuvio-native-legacy/issues/209) | Series Titles and Descriptions Displayed in English Despite  | LG | bug | lancada | 1.7.1 | - | 5658d0b0, 0da72c2a, f50b1915 | nós 10-02 | nada (fechada) |
| [#210](https://github.com/iqui27/nuvio-native-legacy/issues/210) | Username stucks on collapsed 'Modern sidebar' | Samsung .tpk | bug | lancada | 1.7.0 | - | 3e203fcf | autor 10-02 | nada (fechada) |
| [#211](https://github.com/iqui27/nuvio-native-legacy/issues/211) | LG UK6540PSB Crashes on startup. | LG | bug | lancada | 2.0.2 | - | 937f8e6b | nós 10-02 | nada (fechada) |
| [#212](https://github.com/iqui27/nuvio-native-legacy/issues/212) | Not being able to tell if I watched something or not. | LG | bug | lancada | 1.7.1 | - | 7a5896fa, 964bb741, 24a14ed0 | autor 10-03 | nada (fechada) |
| [#213](https://github.com/iqui27/nuvio-native-legacy/issues/213) | Continue watching shows incorrect titles | LG | bug | lancada | 1.7.1 | - | 7a5896fa, 8557e459, 880a4f76 | autor 10-03 | nada (fechada) |
| [#214](https://github.com/iqui27/nuvio-native-legacy/issues/214) | no carga el QR para iniciar sesión | LG | bug | lancada | 1.7.1 | - | sem commit | nós 10-02 | nada (fechada) |
| [#215](https://github.com/iqui27/nuvio-native-legacy/issues/215) | 1.7 introduced bugs on LG | LG | bug | lancada | 2.0.0 | - | b324c7d0, 0c3d2e4e, b6395e4c | nós 10-02 | nada (fechada) |
| [#216](https://github.com/iqui27/nuvio-native-legacy/issues/216) | Addition of touch controls | all | feature | lancada | 1.7.2 | - | 941ac3f0, 80bb3dd2, 7a9cf25b | autor 10-03 | nada (fechada) |
| [#221](https://github.com/iqui27/nuvio-native-legacy/issues/221) | Sources take too long to fetch links | Samsung .tpk | bug | lancada | 2.0.0 | - | c7278fc7, 98607591, 8ad11beb | autor 10-03 | nada (fechada) |
| [#222](https://github.com/iqui27/nuvio-native-legacy/issues/222) | Discord Rich Presence Integration | all | feature | lancada | 1.7.4 | - | 1ea1510b | autor 10-03 | nada (fechada) |
| [#223](https://github.com/iqui27/nuvio-native-legacy/issues/223) | [Android TV] Blank / black screen on launch after "Preparing | ? | bug | lancada | 2.0.0 | - | 9ca29519, 665646f6, 105e759b | nós 10-06 | nada (fechada) |
| [#224](https://github.com/iqui27/nuvio-native-legacy/issues/224) | WebOS crashing | LG | bug | lancada | 1.7.2 | - | 67401c73 | nós 10-03 | nada (fechada) |
| [#225](https://github.com/iqui27/nuvio-native-legacy/issues/225) | App does not launch after updating | LG | bug | lancada | 1.7.2 | - | 67401c73 | nós 10-03 | nada (fechada) |
| [#226](https://github.com/iqui27/nuvio-native-legacy/issues/226) | alt/old icon | ? | feature | lancada | 2.0.1 | - | sem commit | autor 10-07 | nada (fechada) |
| [#227](https://github.com/iqui27/nuvio-native-legacy/issues/227) | Adding localized movie/series title somewhere | LG | feature | lancada | 1.7.4 | - | sem commit | nós 10-03 | nada (fechada) |
| [#228](https://github.com/iqui27/nuvio-native-legacy/issues/228) | Auto trailer doesn't work | Samsung .tpk | bug | lancada | 2.0.1 | - | 3076c3a5, 1e03fc30 | autor 10-06 | nada (fechada) |
| [#229](https://github.com/iqui27/nuvio-native-legacy/issues/229) | Different IMDB score | Samsung .tpk | bug | lancada | 1.7.4 | - | sem commit | autor 10-03 | nada (fechada) |
| [#231](https://github.com/iqui27/nuvio-native-legacy/issues/231) | Option to disable Cinemata | Samsung (tpk/wgt?) | feature | lancada | 2.0.0 | - | d6495798 | nós 10-06 | nada (fechada) |
| [#232](https://github.com/iqui27/nuvio-native-legacy/issues/232) | Next episode thumbnail isn’t blurred | ? | ? | lancada | 2.0.0 | - | d6495798, 2495e39e | nós 10-06 | nada (fechada) |
| [#233](https://github.com/iqui27/nuvio-native-legacy/issues/233) | Nuvio account isn't synced by app | Samsung .tpk | bug | lancada | 2.0.0 | - | 3b625f58 | nós 10-06 | nada (fechada) |
| [#234](https://github.com/iqui27/nuvio-native-legacy/issues/234) | Customization options for app icon/splash screen, trailer UI | LG | feature | lancada | 2.0.0 | - | b8a900f5, 652b759d | nós 10-06 | nada (fechada) |
| [#235](https://github.com/iqui27/nuvio-native-legacy/issues/235) | Seeking | ? | ? | lancada | 2.0.0 | - | b8a900f5, 652b759d | nós 10-06 | nada (fechada) |
| [#237](https://github.com/iqui27/nuvio-native-legacy/issues/237) | Missing forward slash ('/') in TV channel source input (Stal | ? | bug | lancada | 2.0.0 | - | 8f8be06a, e5a848c3 | nós 10-06 | nada (fechada) |
| [#238](https://github.com/iqui27/nuvio-native-legacy/issues/238) | “Wait for add-ons” in playback is blocked by “depth effect” | ? | ? | lancada | 2.0.0 | - | d6495798, ebaef8ad, 73f93686 | nós 10-06 | nada (fechada) |
| [#239](https://github.com/iqui27/nuvio-native-legacy/issues/239) | Arabic Subtitle Letters Appear Disconnected | LG | bug | lancada | 2.0.0 | - | d00870f0, 6bb186ac | nós 10-06 | nada (fechada) |
| [#241](https://github.com/iqui27/nuvio-native-legacy/issues/241) | Trailer black bar are back | Samsung .tpk | bug | lancada | 2.0.2 | - | b5deeb25, 50df3f42, 9a0cce0f | autor 10-06 | nada (fechada) |
| [#243](https://github.com/iqui27/nuvio-native-legacy/issues/243) | IMDb rating missing on Continue Watching and a placeholder " | Samsung .tpk | bug | lancada | 2.0.0 | - | ccfe01a8, 8e7f4098 | nós 10-06 | nada (fechada) |
| [#244](https://github.com/iqui27/nuvio-native-legacy/issues/244) | Continue Watching: the same episode repeats 5 times and mark | Samsung .tpk | bug | lancada | 2.0.0 | - | d6495798, 8e7f4098 | nós 10-06 | nada (fechada) |
| [#245](https://github.com/iqui27/nuvio-native-legacy/issues/245) | Arabic subtitles not shaped/RTL — letters appear disconnecte | LG | bug | lancada | 2.0.0 | - | d00870f0, 6bb186ac | nós 10-06 | nada (fechada) |
| [#247](https://github.com/iqui27/nuvio-native-legacy/issues/247) | Arabic subtitle | LG | bug | lancada | 2.0.0 | - | a1cf2e46, d00870f0, 6bb186ac | nós 10-06 | nada (fechada) |
| [#249](https://github.com/iqui27/nuvio-native-legacy/issues/249) | [Feature]: Keep the current video full-size when the “Up Nex | ? | feature | lancada | 2.0.0 | - | 6c971d52 | autor 10-06 | nada (fechada) |
| [#253](https://github.com/iqui27/nuvio-native-legacy/issues/253) | Arabic subtitles/UI rendering as squares on Samsung Smart TV | Samsung .tpk | bug | lancada | 2.0.0 | - | a1cf2e46 | nós 10-06 | nada (fechada) |
| [#255](https://github.com/iqui27/nuvio-native-legacy/issues/255) | Home screen only displays 5 collections | Samsung .tpk | bug | lancada | 2.0.1 | - | 40bc9180 | nós 10-06 | nada (fechada) |
| [#258](https://github.com/iqui27/nuvio-native-legacy/issues/258) | Arabic subtitles not working | Samsung .tpk | bug | lancada | 2.0.0 | - | a1cf2e46 | nós 10-06 | nada (fechada) |
| [#261](https://github.com/iqui27/nuvio-native-legacy/issues/261) | Arabic Subtitles doesnt workining. | LG | bug | lancada | 2.0.0 | - | a1cf2e46 | nós 10-06 | nada (fechada) |
| [#262](https://github.com/iqui27/nuvio-native-legacy/issues/262) | Subtitle delay slider | ? | feature | lancada | 2.0.0 | - | sem commit | nós 10-06 | nada (fechada) |
| [#263](https://github.com/iqui27/nuvio-native-legacy/issues/263) | Add an option to toggle between 12-hour and 24-hour clock fo | Samsung (tpk/wgt?) | feature | lancada | 2.0.0 | - | sem commit | nós 10-06 | nada (fechada) |
| [#265](https://github.com/iqui27/nuvio-native-legacy/issues/265) | LG Smart TV – Freezing, Stuttering | LG | bug | lancada | 2.0.1 | - | sem commit | autor 10-07 | nada (fechada) |
| [#271](https://github.com/iqui27/nuvio-native-legacy/issues/271) | Intermittent App Crash When Navigating Settings | LG | bug | lancada | 2.0.0 | - | sem commit | autor 10-07 | nada (fechada) |
| [#272](https://github.com/iqui27/nuvio-native-legacy/issues/272) | Alternate App Icon Selection Does Not Change App Icon | LG | bug | lancada | 2.0.1 | - | sem commit | autor 10-06 | nada (fechada) |
| [#274](https://github.com/iqui27/nuvio-native-legacy/issues/274) | Arabic Language, TMDb Metadata, and Logos/Backdrops Issues | Samsung .tpk | bug | lancada | 2.0.1 | - | sem commit | autor 10-07 | nada (fechada) |
| [#275](https://github.com/iqui27/nuvio-native-legacy/issues/275) | Data Saving Option (Max/Min File Size Limit for Playback) | Samsung (tpk/wgt?) | feature | lancada | 2.0.1 | - | sem commit | nós 10-06 | nada (fechada) |
| [#276](https://github.com/iqui27/nuvio-native-legacy/issues/276) | Integrating QuickJS Engine into TPK Build to Enable External | Samsung (tpk/wgt?) | feature | lancada | 2.0.0 | - | sem commit | nós 10-06 | nada (fechada) |
| [#277](https://github.com/iqui27/nuvio-native-legacy/issues/277) | Addon not showing | LG | bug | lancada | 2.0.1 | - | sem commit | nós 10-06 | nada (fechada) |
| [#278](https://github.com/iqui27/nuvio-native-legacy/issues/278) | Arabic subtitle symbols not shown | LG | bug | lancada | 2.0.1 | - | sem commit | autor 10-07 | nada (fechada) |
| [#281](https://github.com/iqui27/nuvio-native-legacy/issues/281) | Auto-trailer doesn’t have sound on Tizen 9 | Samsung .tpk | bug | lancada | 2.0.1 | - | 3076c3a5 | nós 10-06 | nada (fechada) |
| [#282](https://github.com/iqui27/nuvio-native-legacy/issues/282) | Collection images/AIO regex patterns | Samsung .tpk | bug | lancada | 2.0.1 | - | sem commit | autor 10-06 | nada (fechada) |
| [#289](https://github.com/iqui27/nuvio-native-legacy/issues/289) | Eliminate “OK” button on profile pin entry | all | feature | lancada | 2.0.2 | - | 3eb5aef5 | nós 10-07 | nada (fechada) |
| [#295](https://github.com/iqui27/nuvio-native-legacy/issues/295) | Profile picker background — “Profile art” option not working | Samsung .tpk | bug | lancada | 2.0.2 | - | 881f7cae | nós 10-07 | nada (fechada) |
| [#297](https://github.com/iqui27/nuvio-native-legacy/issues/297) |  Issue: P2P stream stops due to full TV storage. | Samsung .tpk | bug | lancada | 2.0.2 | - | 325f4e2b, 1b6a5439 | nós 10-07 | nada (fechada) |
| [#310](https://github.com/iqui27/nuvio-native-legacy/issues/310) | [port] regex/options for autoplay | ? | feature | lancada | 2.0.2 | - | sem commit | autor 10-07 | nada (fechada) |

Notas:

- **#1**: commits em várias versões (1.0.7 a 1.1.2).
- **#2**: commits em várias versões (1.0.7 a 2.0.0).
- **#3**: commits em várias versões (1.0.7 a 1.1.2).
- **#4**: commits em várias versões (1.0.7 a 1.1.0).
- **#5**: commits em várias versões (1.0.7 a 2.0.0).
- **#6**: commits em várias versões (1.0.7 a 1.1.0).
- **#7**: commits em várias versões (1.0.7 a 2.0.0).
- **#10**: commits em várias versões (1.0.7 a 1.0.21).
- **#11**: commits em várias versões (1.0.8 a 1.0.31).
- **#12**: commits em várias versões (1.0.10 a 1.0.43).
- **#14**: commits em várias versões (1.0.10 a 1.0.16).
- **#16**: commits em várias versões (1.0.11 a 1.0.13).
- **#18**: commits em várias versões (1.0.11 a 1.4.2).
- **#19**: commits em várias versões (1.0.11 a 1.0.23).
- **#21**: commits em várias versões (1.0.24 a 1.5.2).
- **#22**: commits em várias versões (1.0.22 a 1.0.30).
- **#23**: commits em várias versões (1.0.32 a 1.0.36).
- **#25**: Sem commit #25; versão 1.0.30 citada na nossa resposta.
- **#29**: commits em várias versões (1.0.34 a 1.0.44).
- **#30**: commits em várias versões (1.0.31 a 1.0.34).
- **#31**: commits em várias versões (1.0.31 a 1.0.43).
- **#32**: Sem commit #32; versão 1.0.35 citada na nossa resposta.
- **#33**: commits em várias versões (1.0.39 a 1.0.41).
- **#34**: commits em várias versões (1.0.37 a 1.4.6).
- **#37**: commits em várias versões (1.0.42 a 1.6.0).
- **#42**: commits em várias versões (1.0.43 a 1.0.51).
- **#45**: commits em várias versões (1.0.44 a 1.6.5).
- **#49**: commits em várias versões (1.0.52 a 1.0.54).
- **#50**: Sem commit #50; versão 1.0.51 citada na nossa resposta.
- **#51**: Sem commit #51; versão 1.0.51 citada na nossa resposta.
- **#52**: Sem commit #52; versão 1.0.51 citada na nossa resposta.
- **#55**: commits em várias versões (1.0.55 a 1.3.2).
- **#56**: commits em várias versões (1.0.55 a 1.0.56).
- **#65**: commits em várias versões (1.2.0 a 1.2.1).
- **#67**: commits em várias versões (1.3.2 a 1.3.7).
- **#69**: commits em várias versões (1.3.2 a 1.3.4).
- **#82**: commits em várias versões (1.3.9 a 1.3.11).
- **#84**: commits em várias versões (1.3.8 a 1.4.7).
- **#92**: commits em várias versões (1.4.2 a 1.5.0).
- **#94**: commits em várias versões (1.3.12 a 1.5.1).
- **#95**: commits em várias versões (1.4 a 1.4.6).
- **#108**: Sem commit #108; versão 1.4.2 citada na nossa resposta.
- **#109**: commits em várias versões (1.4.1 a 1.4.3).
- **#117**: Sem commit #117; versão 1.4.3 citada na nossa resposta.
- **#119**: Sem commit #119; versão 1.4.6 citada na nossa resposta.
- **#121**: commits em várias versões (1.4.3 a 1.4.5).
- **#125**: Sem commit #125; versão 1.4.4 citada na nossa resposta.
- **#130**: commits em várias versões (1.4.4 a 2.0.2).
- **#131**: Sem commit #131; versão 1.4.6 citada na nossa resposta.
- **#133**: commits em várias versões (1.4.6 a 1.6.0).
- **#134**: Sem commit #134; versão 2.0.0 citada na nossa resposta.
- **#137**: commits em várias versões (1.4.6 a 1.5.4).
- **#138**: Substituída: .tpk Tizen 4/5 em toda release desde 1.6.0 (resposta); acompanhamento em #180.
- **#141**: commits em várias versões (1.5.0 a 1.5.3).
- **#142**: commits em várias versões (1.5.0 a 1.6.4).
- **#146**: Sem commit #146; versão 1.6.0 citada na nossa resposta.
- **#151**: commits em várias versões (1.5.1 a 1.5.3).
- **#158**: commits em várias versões (1.5.2 a 2.0.2).
- **#165**: commits em várias versões (1.5.4 a 1.7.0).
- **#169**: Sem commit #169; versão 1.6.0 citada na nossa resposta.
- **#172**: commits em várias versões (1.6.0 a 1.6.2).
- **#173**: Sem commit #173; versão 1.5.3 citada na nossa resposta.
- **#176**: commits em várias versões (1.6.0 a 1.7.0).
- **#177**: commits em várias versões (1.5.4 a 1.6.0).
- **#178**: commits em várias versões (1.5.4 a 1.6.3).
- **#179**: commits em várias versões (1.5.4 a 1.6.0).
- **#180**: commits em várias versões (1.5.4 a 1.6.0).
- **#187**: commits em várias versões (1.6.1 a 1.6.5).
- **#188**: commits em várias versões (1.6.3 a 2.0.2).
- **#193**: Sem commit #193; versão 1.7.0 citada na nossa resposta.
- **#194**: commits em várias versões (1.6.2 a 1.6.4).
- **#195**: commits em várias versões (1.6.3 a 2.0.2).
- **#199**: commits em várias versões (1.6.5 a 1.7.0).
- **#200**: commits em várias versões (1.6.5 a 1.7.0).
- **#201**: commits em várias versões (1.7.0 a 2.0.1).
- **#202**: commits em várias versões (1.7.0 a 2.0.2).
- **#203**: Sem commit #203; versão 1.7.0 citada na nossa resposta.
- **#204**: Sem commit #204; versão 1.7.0 citada na nossa resposta.
- **#209**: commits em várias versões (1.7.0 a 1.7.1).
- **#214**: Sem commit #214; versão 1.7.1 citada na nossa resposta.
- **#215**: commits em várias versões (1.7.1 a 2.0.0).
- **#221**: commits em várias versões (1.7.2 a 2.0.0).
- **#223**: commits em várias versões (1.7.2 a 2.0.0).
- **#226**: Sem commit #226; versão 2.0.1 citada na nossa resposta.
- **#227**: Sem commit #227; versão 1.7.4 citada na nossa resposta.
- **#228**: commits em várias versões (2.0.0 a 2.0.1).
- **#229**: Sem commit #229; versão 1.7.4 citada na nossa resposta.
- **#232**: commits em várias versões (1.7.4 a 2.0.0).
- **#241**: commits em várias versões (2.0.0 a 2.0.2).
- **#262**: Sem commit #262; versão 2.0.0 citada na nossa resposta.
- **#263**: Sem commit #263; versão 2.0.0 citada na nossa resposta.
- **#265**: Sem commit #265; versão 2.0.1 citada na nossa resposta.
- **#271**: Sem commit #271; versão 2.0.0 citada na nossa resposta.
- **#272**: Sem commit #272; versão 2.0.1 citada na nossa resposta.
- **#274**: Sem commit #274; versão 2.0.1 citada na nossa resposta.
- **#275**: Sem commit #275; versão 2.0.1 citada na nossa resposta.
- **#276**: Sem commit #276; versão 2.0.0 citada na nossa resposta.
- **#277**: Sem commit #277; versão 2.0.1 citada na nossa resposta.
- **#278**: Sem commit #278; versão 2.0.1 citada na nossa resposta.
- **#282**: Sem commit #282; versão 2.0.1 citada na nossa resposta.
- **#310**: Sem commit #310; versão 2.0.2 citada na nossa resposta.

## Fechado sem conserto (por-desenho, fora-do-escopo, duplicada, respondida, sem resposta)

37 issues.

| # | Título | Plat. | Tipo | Status | Release | Alvo | Conserto | Última resposta | Próximo passo |
|---|---|---|---|---|---|---|---|---|---|
| [#15](https://github.com/iqui27/nuvio-native-legacy/issues/15) | Bug: Trailer Does Not Play When Selected on Detail Page | ? | bug | respondida | - | - | - | autor 09-16 | nada (fechada) |
| [#20](https://github.com/iqui27/nuvio-native-legacy/issues/20) | Library does not populate from account | LG | bug | respondida | - | - | - | nós 09-08 | nada (fechada) |
| [#47](https://github.com/iqui27/nuvio-native-legacy/issues/47) | Feature request | ? | feature | respondida | - | - | - | nós 09-15 | nada (fechada) |
| [#53](https://github.com/iqui27/nuvio-native-legacy/issues/53) | Sources not showing up | ? | bug | respondida | - | - | - | autor 09-16 | nada (fechada) |
| [#58](https://github.com/iqui27/nuvio-native-legacy/issues/58) | Trakt Library titles are not appearing | Samsung (tpk/wgt?) | bug | por-desenho | - | - | - | nós 09-19 | nada (fechada) |
| [#59](https://github.com/iqui27/nuvio-native-legacy/issues/59) | "Meu Futebol" add-on not working | Samsung (tpk/wgt?) | bug | por-desenho | 1.2.1 (LG); Samsung sem solução | - | - | nós 09-19 | nada (fechada) |
| [#63](https://github.com/iqui27/nuvio-native-legacy/issues/63) | Support for Tizen 5 | Samsung (tpk/wgt?) | feature | por-desenho | - | - | - | nós 09-19 | nada (fechada) |
| [#64](https://github.com/iqui27/nuvio-native-legacy/issues/64) | no highcache version for 1.1.2? | ? | question | respondida | - | - | - | nós 09-19 | nada (fechada) |
| [#75](https://github.com/iqui27/nuvio-native-legacy/issues/75) | Request: settings menu navigation | ? | feature | fechada-sem-resposta | - | - | - | sem comentários | nada (fechada) |
| [#81](https://github.com/iqui27/nuvio-native-legacy/issues/81) | Seek not functioning properly  | ? | bug | respondida | 1.3.7 | - | - | nós 09-20 | nada (fechada) |
| [#90](https://github.com/iqui27/nuvio-native-legacy/issues/90) | feature request | ? | feature | respondida | - | - | - | nós 09-23 | nada (fechada) |
| [#91](https://github.com/iqui27/nuvio-native-legacy/issues/91) | MP4 container toggle | ? | feature | fechada-sem-resposta | - | - | - | sem comentários | nada (fechada) |
| [#96](https://github.com/iqui27/nuvio-native-legacy/issues/96) | Request | Samsung (tpk/wgt?) | feature | respondida | - | - | - | autor 09-27 | nada (fechada) |
| [#98](https://github.com/iqui27/nuvio-native-legacy/issues/98) | Dunno how github does its uploads but the there's no release | ? | ? | fechada-sem-resposta | - | - | - | sem comentários | nada (fechada) |
| [#107](https://github.com/iqui27/nuvio-native-legacy/issues/107) | Not searching files from addon | ? | bug | precisa-log | - | - | - | autor 09-23 | nada (fechada) |
| [#140](https://github.com/iqui27/nuvio-native-legacy/issues/140) | trailer no sound | ? | ? | fechada-sem-resposta | - | - | - | sem comentários | nada (fechada) |
| [#148](https://github.com/iqui27/nuvio-native-legacy/issues/148) | Sudden crash by moving through the app | ? | bug | precisa-log | - | - | - | autor 09-27 | nada (fechada) |
| [#154](https://github.com/iqui27/nuvio-native-legacy/issues/154) | Relatório de Logs | ? | meta | por-desenho | - | - | - | nós 09-27 | nada (relatório automático de logs) |
| [#155](https://github.com/iqui27/nuvio-native-legacy/issues/155) | Thanks for building this! | LG | meta | respondida | - | - | - | autor 10-06 | nada (fechada) |
| [#161](https://github.com/iqui27/nuvio-native-legacy/issues/161) | Relatório de logs | ? | meta | por-desenho | - | - | - | nós 09-30 | nada (relatório automático de logs) |
| [#167](https://github.com/iqui27/nuvio-native-legacy/issues/167) | Tizen 4.0 | Samsung (tpk/wgt?) | ? | fechada-sem-resposta | - | - | - | sem comentários | nada (fechada) |
| [#168](https://github.com/iqui27/nuvio-native-legacy/issues/168) | Trailer plays without sound | ? | bug | fechada-sem-resposta | - | - | - | sem comentários | nada (fechada) |
| [#183](https://github.com/iqui27/nuvio-native-legacy/issues/183) | trailer resolution | ? | ? | respondida | - | - | - | nós 09-30 | nada (fechada) |
| [#189](https://github.com/iqui27/nuvio-native-legacy/issues/189) | Actors UI | ? | feature | respondida | - | - | - | autor 09-30 | nada (fechada) |
| [#192](https://github.com/iqui27/nuvio-native-legacy/issues/192) | Relatório de logs | ? | meta | por-desenho | - | - | 77227ac1, b61d74a6 | nós 10-07 | nada (relatório automático de logs) |
| [#217](https://github.com/iqui27/nuvio-native-legacy/issues/217) | Building on a Linux host: three small blockers (and one targ | ? | bug | respondida | - | - | - | nós 10-02 | nada (fechada) |
| [#240](https://github.com/iqui27/nuvio-native-legacy/issues/240) | tardan demasiado en cargar una serie o pelicula | LG | bug | precisa-log | - | - | - | autor 10-06 | nada (fechada) |
| [#242](https://github.com/iqui27/nuvio-native-legacy/issues/242) | API QUESTION | ? | question | respondida | - | - | - | nós 10-06 | nada (fechada) |
| [#248](https://github.com/iqui27/nuvio-native-legacy/issues/248) | Catalog sequencing mismatched | Samsung .tpk | bug | por-desenho | - | - | - | autor 10-04 | nada (fechada) |
| [#254](https://github.com/iqui27/nuvio-native-legacy/issues/254) | Home row does not update properly after installing/removing  | Samsung .tpk | bug | respondida | 2.0.0 | - | - | nós 10-06 | nada (fechada) |
| [#257](https://github.com/iqui27/nuvio-native-legacy/issues/257) | FYI: included in Tizen Community Packages | Samsung (tpk/wgt?) | meta | respondida | - | - | - | nós 10-06 | nada (fechada) |
| [#267](https://github.com/iqui27/nuvio-native-legacy/issues/267) | No blue button on newer Samsung remote | Samsung (tpk/wgt?) | feature | por-desenho | - | - | - | autor 10-06 | nada (fechada) |
| [#270](https://github.com/iqui27/nuvio-native-legacy/issues/270) | 📊 Relatório de logs | ? | meta | por-desenho | - | - | d560db8c, 666428cc | nós 10-07 | nada (relatório automático de logs) |
| [#279](https://github.com/iqui27/nuvio-native-legacy/issues/279) | Match framerate | Samsung (tpk/wgt?) | feature | fora-do-escopo | - | - | - | autor 10-06 | nada (fechada) |
| [#309](https://github.com/iqui27/nuvio-native-legacy/issues/309) | [suggestion] sources formatter | ? | feature | por-desenho | - | - | - | autor 10-07 | nada (fechada) |
| [#325](https://github.com/iqui27/nuvio-native-legacy/issues/325) | Add Arabic Language Support | all | feature | duplicada | 2.1 | - | - | autor 10-07 | nada (fechada) |
| [#336](https://github.com/iqui27/nuvio-native-legacy/issues/336) | App wont launch | Samsung .tpk | meta | por-desenho | - | - | - | autor 10-08 | nada (relatório automático de logs) |

Notas:

- **#15**: Resposta: plano de passar o trailer ao app de YouTube da TV; rawldon confirmou depois que toca (com atraso). Sem commit #15.
- **#20**: "Fixed on master; next release" (08/09); nenhum commit cita #20.
- **#58**: Salvos = união da lista Nuvio e Trakt watchlist, de propósito.
- **#59**: LG manda Referer desde 1.2.1; AVPlay da Samsung não permite header.
- **#63**: Tizen 5.0 sem WebAssembly threads; depois o .tpk 4/5 (1.6.0) resolveu por outro caminho.
- **#81**: v1.3.7 apenas registra o tempo de seek na Samsung (diagnóstico).
- **#90**: "Not in 1.4.2 yet — still on the list".
- **#189**: "Vou tentar ajustes em breve", sem commit #189.
- **#217**: Contribuidor oferece PRs de build em Linux; resposta aceita.
- **#248**: Resposta diz que é comportamento da plataforma Tizen.
- **#254**: Resposta: busca atualiza ao mudar add-ons na 2.0; a parte da Home não confirmada.
- **#267**: Resposta: CH+/CH- já atribuídos.
- **#279**: Tizen não tem API pública para trocar a taxa de atualização.
- **#309**: Já existe: Fontes e add-ons > Texto da fonte.
- **#325**: Duplicata de #250/#260.

## Fora do GitHub (Reddit)

### Relatório Shield (22 itens, Reddit)

- Plataforma: Android (Shield)
- Status: desconhecido
- Release: 2.0.3 (parcial)
- Conserto: d30344ab/96ccdab1 (#6 Shield: tetos do Android e paginação da watchlist do Trakt, 2.0.3; 7b9e641a, o revert do 96ccdab1, não está em nenhuma branch); 5413db8a (Shield #19: guia "Mostrando N de M canais", Android guarda 3000 canais/128 categorias, 2.0.3); 6b3aaae1 (Shield 318.3: card do Continuar com o still do episódio; filme assistido diz Reproduzir, 2.0.3); eabe3afd/b03aa2f0/35421d1a/73c4ab4c (#266/#332: curl fora do fio principal, /dev/urandom, login por HttpURLConnection, 2.0.3)
- Próximo passo: achar o texto original do relatório e mapear os 22 itens um a um
- Notas: O texto dos 22 itens NÃO está no repositório nem nas issues. Só três grupos de commits citam "Shield". Mapeamento item a item: desconhecido.

### Relatório Android TV (9 itens) = #369

- Plataforma: Android
- Status: consertada-nao-lancada (parcial)
- Release: 2.0.3
- Conserto: 712557ca (tailandês); 5f04c36b (ocultar não lançados)
- Próximo passo: responder no #369 e listar os 7 itens sem commit
- Notas: Só 2 dos 9 itens têm commit identificado. #369 não tem resposta nossa.

### Samsung: faixa de áudio escondida pela TV (DTS/TrueHD)

- Plataforma: Samsung .tpk (e .wgt na fase 1)
- Status: consertada-nao-lancada (casar faixa) + planejado (aviso)
- Release: 2.0.3 (casar faixa); 2.0.4 (aviso, fase 1)
- Conserto: 4112f3f7 + efd1014b (agente/203-casarfaixa, em integracao/2.0.3): TV com menos faixas que o MKV casa por idioma, codec e canais; 0ef8ddc9 (agente/204-samsung-dts): só o plano docs/plans/samsung-dts.md, sem código; f45ddc14 (2.0.2, #313): aviso só quando TODAS as faixas são recusadas
- Próximo passo: implementar fase 1 do plano (aviso + faixa cinza na folha de áudio, 1,5-2 dias); fase 2 (ESPlayer) depende de teste de assinatura Partner em TV
- Notas: Relato do Reddit: 2 faixas no box, 1 na TV. Relacionado a #313. ESPlayer exige privilégio Partner (no 4/5 isso já quebrou a instalação, 118014).

### Samsung: pacote de 80 GB

- Plataforma: Samsung
- Status: desconhecido
- Release: -
- Conserto: nenhum encontrado
- Próximo passo: perguntar ao dono o que é
- Notas: Nada encontrado em commits, docs, notas de release ou issues (buscas por "80 GB"/"80GB"). Não preencher sem a fonte.
