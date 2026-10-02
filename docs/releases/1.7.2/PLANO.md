# Plano proposto para 1.7.2 — 02/10/2026

Somente local, em `release/1.7.2` (`/private/tmp/nv-172`), base 1.7.1 `2bc9659b`. Nenhum merge adicional aprovado por este documento; propostas de escopo, não promessas de correção. Testes focados + i18n; comportamento nas TVs testado pelo dono. Nenhum push/tag/publicação/comentário nesta rodada.

## Já integrado

`agente/abrirrapido` (inclui `ilhavolta` e `i221`) e `agente/i216`. Nova ilha, fontes parciais, espera configurável pelos addons, fonte da última sessão com fallback, toque/arrastar e login por e-mail. Retenção decidida em dois minutos, commit `1bf49696`; teste 119/120 s passou. Nove testes focados aprovados. TCL recebeu apenas APK estável 1.7.1.

Conferência da fonte guardada já roda em paralelo (`app.c:tocarFonteGuardada`); não contabilizar como trabalho futuro nesse caminho. Conferência da abertura normal permanece uma proposta separada.

## Plano de execução

1. **Correções de consistência antes das otimizações.** Proteger as leituras/escritas de `cat_definir_na_lista`, `_imdb` e outras operações no vetor publicado, sem mutex recursivo nem retenção de ponteiro antigo. Isolar mapa de assistidos por conta/perfil e descartar respostas atrasadas da identidade anterior: a chave atual é IMDb+tipo e não há reset da tabela no código consultado. Testar troca A→B, logout, resposta tardia e fallback de progresso.
2. **Autoatualização TPK e arte.** Cinco registros 1.7.1 não encontram `aj_smartphone.png`; os quatro pacotes completos 1.7.1 contêm esse arquivo. Autoatualização troca somente `.so`, portanto atualização parcial é hipótese concreta, ainda não causa comprovada de todos os registros. Confirmar manifesto/host instalado e corrigir contrato biblioteca/arte (fallback embutido para ícones essenciais ou atualização versionada de recursos). Evitar caminho quente tentando abrir arquivo ausente em cada quadro.
3. **Sincronização e indisponibilidade.** Dois registros Android trazem push de addons HTTP500; há 521/522/429 em operações Nuvio. Identificar rota/status/corpo sanitizado, separar servidor e cliente; testar preservação da fila, backoff e ausência de rajadas. Não perder sessão, biblioteca nem mudanças locais. Corrigir servidor apenas se causa demonstrada, sem deploy automático.
4. **Abertura de filme.** Instrumentar pedido→fontes→URL→pronto→primeiro quadro→ponto salvo. Android: preparar Media3 já na posição inicial quando houver posição válida; manter fallback se duração desconhecida e não executar seek duplicado. Ganho de 3–6 s citado no handoff é estimativa/medição anterior, não resultado desta implementação. Avaliar conferência paralela para primeira abertura com fallback e cancelamento; não abrir duas reproduções nem consumir debrid especulativamente.
5. **Pré-busca MKV.** Tempo unsigned pode estourar se `agora` antecede timestamp criado durante o mesmo quadro. Corrigir diagnóstico de tempo primeiro; separar alteração da espera de até 4 s, pois muda latência e disponibilidade da legenda no início. Critério: nenhum tempo falso e nenhuma regressão silenciosa na legenda embutida.
6. **Relatos visuais/reprodução.** Reproduzir proporção que falha (#195), cintilação de Continuar assistindo (#144), catálogos ainda ausentes (#195/#197) e título/descrição localizada (#209) com log atual e fonte/configuração. Não assumir que todos vêm da mesma causa. Confirmar #158 com usuário após 1.7.1.
7. **Escolha de UX.** Decidir `ajustesux` antes de combinar `ajustesvisual`; comparar controle remoto, toque, contraste, memória e legibilidade. Branch limpa no momento da inspeção, porém contém reorganização grande e 94 frases por idioma. Não mesclar as duas cegamente. Ícones de apoiador exigem decisão e teste do alias/launcher Android.
8. **Build para o dono.** Identificar versão/commit; gerar LG normal/highcache, WGT, quatro TPKs, dois .so e APK dual ABI, conferir arte/configuração/assinatura/checksums. Dono testa C9, TCL e Samsung. Casos curtos: retomada antes/depois de dois minutos, fonte vencida, addon lento, troca de perfil, login/toque, proporção e Xtream. Publicação só com autorização posterior.

## Evidências dos logs

Consulta somente leitura de 65 registros 1.7.1 disponíveis: 40 Android, 11 TPK, 14 webOS; 64 corpos diferentes. Isso não equivale a 64 sessões independentes. 17 registros contêm HTTP5xx; cinco TPKs têm ícone ausente. 36 contêm encerramento sem despedida (27 Android, nove LG), que não prova crash. Nenhum marcador explícito `FATAL EXCEPTION`, `Fatal signal`, `Segmentation fault`, `RuntimeError:` ou `Aborted` encontrado neste recorte. Não declarar estabilidade plena nem regressão a partir das contagens. Nenhum registro 1.7.2 foi usado como validação da nova build.

Relatórios automáticos #192 confundem fallback ASS, código zero e sessão sem despedida com erro/crash em algumas conclusões. Validar sessão, versão real do núcleo e versão do pacote/host antes de priorizar cada evento. Dados privados permanecem fora do repositório.

## Otimização por plataforma

- Android: posição inicial Media3, coerência do plano de vídeo/HDR ao sair e voltar, UI adequada à GPU da TCL, toque sem acionamento duplicado; memória limitada e duas ABIs.
- LG: pipeline único, pausa confirmada antes da retenção, corrida catálogo/histórico, comportamento de duas conexões do proxy Xtream; respeitar capacidade de TVs antigas.
- TPK: recursos atualizados junto do núcleo, proporção/single-window e retorno ao app; ABI4/5 sem TLS e com DT_HASH. Mudança no host .NET demanda evidência específica no aparelho.
- WGT: fontes parciais/cache limitado, compatibilidade Chrome69 e ausência de bloqueio da UI; não tentar P2P com sockets nativos indisponíveis.

## Branches soltas

Inventário por ancestralidade + `git cherry` contra HEAD: 18 branches com patches sem equivalência exata. Contagem não indica que todo patch é funcionalmente novo; cherry-pick adaptado pode aparecer como não integrado.

| Branch | Patches sem equivalência exata | Encaminhamento |
|---|---:|---|
| `agente/ajustesux` | 5 | Candidata 1.7.2 mediante escolha da UX |
| `agente/ajustesvisual` | 2 | Experimento A/B; escolher direção antes de integrar |
| `agente/guiaunicode` | 1 | Conserto de gênero Unicode já presente; não reverter busca Spotlight mais nova |
| `agente/i195` | 1 | Canário/porte antigo; preservar e confirmar obsolescência, não mesclar nem apagar agora |
| `agente/i195b` | 1 | Canário/porte antigo; preservar e confirmar obsolescência, não mesclar nem apagar agora |
| `agente/i195c` | 2 | Canário/porte antigo; preservar e confirmar obsolescência, não mesclar nem apagar agora |
| `agente/i195d` | 3 | Canário/porte antigo; preservar e confirmar obsolescência, não mesclar nem apagar agora |
| `agente/i203` | 1 | Canário/porte antigo; preservar e confirmar obsolescência, não mesclar nem apagar agora |
| `agente/icones` | 5 | Decisão do dono; risco de alteração do launcher Android |
| `agente/p2p` | 6 | Versão antiga; não mesclar |
| `agente/p2p2` | 13 | 1.8; requer validação de plataforma/cancelamento/disco |
| `agente/plugins` | 14 | Versão antiga; não mesclar |
| `agente/plugins2` | 21 | 1.8; requer validação de plataforma/cancelamento/disco |
| `agente/reacao` | 6 | Outra sessão; não tocar sem decisão. Socialui contém trabalho não commitado |
| `agente/socialsrv` | 7 | Outra sessão; não tocar sem decisão. Socialui contém trabalho não commitado |
| `agente/socialui` | 6 | Outra sessão; não tocar sem decisão. Socialui contém trabalho não commitado |
| `feat/tizen4-coop` | 5 | Canário/porte antigo; preservar e confirmar obsolescência, não mesclar nem apagar agora |
| `feat/vidaa` | 7 | Canário/porte antigo; preservar e confirmar obsolescência, não mesclar nem apagar agora |

Socialsrv/socialui avançaram desde o handoff: sete/seis patches sem equivalência exata nesta inspeção. Não assumir estabilidade por número de commits. Não foram mescladas nesta rodada.

## Issues abertas consultadas

Consulta GitHub atual: 20 abertas; sem PR aberta. #212/#213/#215 já fechadas e #158 respondida em 1.7.1 por outra sessão. Não refazer respostas.

| Issue | Encaminhamento |
|---|---|
| [#222](https://github.com/iqui27/nuvio-native-legacy/issues/222) | Pedido Discord RP: estudar viabilidade/dependência de aplicativo auxiliar; fora do bloqueio de bug TV |
| [#221](https://github.com/iqui27/nuvio-native-legacy/issues/221) | Integrada: addons chegam incrementalmente; confirmação do dono em Samsung pendente |
| [#217](https://github.com/iqui27/nuvio-native-legacy/issues/217) | Portabilidade Linux: sed, conversor de arte, seleção de pacotes e NV_WEBOS; sem PR aberta, coordenar antes de duplicar contribuição |
| [#216](https://github.com/iqui27/nuvio-native-legacy/issues/216) | Integrada: toque/arrastar/login; falta validação do dono em dispositivo apropriado |
| [#211](https://github.com/iqui27/nuvio-native-legacy/issues/211) | Startup UK6540: sem log/firmware; aguardando evidência, não declarar corrigida |
| [#209](https://github.com/iqui27/nuvio-native-legacy/issues/209) | Correção publicada; confirmar comportamento/localização no cenário atual |
| [#202](https://github.com/iqui27/nuvio-native-legacy/issues/202) | Contraste corrigido publicado; confirmar se nova UX mantém comportamento |
| [#201](https://github.com/iqui27/nuvio-native-legacy/issues/201) | Usuário confirmou primeira fileira; pedido de mais itens TopN é decisão de produto separada |
| [#197](https://github.com/iqui27/nuvio-native-legacy/issues/197) | Catálogo depende de addon/perfil/retorno vazio; pedir cenário/log atual se persistir |
| [#195](https://github.com/iqui27/nuvio-native-legacy/issues/195) | Trailer confirmado corrigido; novo relato inclui proporção e catálogos, além de pedidos de recursos |
| [#192](https://github.com/iqui27/nuvio-native-legacy/issues/192) | Agregador: revisar eventos brutos, não tomar rótulo automático como diagnóstico |
| [#190](https://github.com/iqui27/nuvio-native-legacy/issues/190) | Usuário confirmou resolução; tarefa administrativa pendente |
| [#188](https://github.com/iqui27/nuvio-native-legacy/issues/188) | Correção anterior; confirmar mesmo cenário no TPK atual |
| [#172](https://github.com/iqui27/nuvio-native-legacy/issues/172) | Pedido visual; avaliar UX sem penalizar TVs antigas |
| [#171](https://github.com/iqui27/nuvio-native-legacy/issues/171) | P2P nativo reservado 1.8 |
| [#158](https://github.com/iqui27/nuvio-native-legacy/issues/158) | Resposta 1.7.1 já enviada; aguardar playback contínuo do relator |
| [#155](https://github.com/iqui27/nuvio-native-legacy/issues/155) | Agradecimento, nenhum defeito para corrigir |
| [#144](https://github.com/iqui27/nuvio-native-legacy/issues/144) | UX/paridade e relato de cintilação no Continuar assistindo: reproduzir |
| [#135](https://github.com/iqui27/nuvio-native-legacy/issues/135) | VIDAA experimental, fora de1.7.2 |
| [#134](https://github.com/iqui27/nuvio-native-legacy/issues/134) | Plugins reservados1.8 |

## Critérios para fechar o escopo

Dados de outro perfil não aparecem; atualização TPK não depende de arte inexistente; falhas Nuvio conservam dados e fila; retomada e fallback funcionam; formato de vídeo e legendas não regridem. Medir melhoria de tempo por etapa na mesma fonte e aparelho, sem prometer instantâneo na abertura de mídia nova. Testes do código alterado + i18n e testes curtos do dono; não repetir suíte longa.
