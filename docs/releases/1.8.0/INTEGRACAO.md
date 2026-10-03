# Integração da 1.8.0 — Glass UI

Registro de revisão de 03/10/2026. A integração local reúne o Glass UI e as correções publicadas na 1.7.4. Este documento não confirma uma release pronta nem uma instalação na TV.

## Origem e escopo

| Linha | Commit usado na integração |
|---|---|
| `feat/glass-ilha` | `16b475b16ac4869207d1ea52edbcd0035f651f7b` |
| `origin/master` | `1345620bd2cee96a830b659f67d17765c0c84c43` |

Worktree de integração: `/Users/hrocha/.codex/worktrees/integration-180-glass/nuvio-native-legacy`. O checkout original, com trabalho não commitado do dono, foi preservado. A 1.7.4 já está pública e não deve ser republicada.

O [handoff](../../HANDOFF-1.8.0.md) orienta a união das duas linhas. As regras visuais consultadas estão em `/private/tmp/nv-codex-ctx/docs/glass-ui-contexto.md`; suas observações de builds antigas não são o estado atual da integração. As decisões mais recentes do dono prevalecem sobre as medidas antigas desse documento.

## Comportamentos preservados e ajustes

- **Apple TV:** preservar o véu de leitura, texto ampliado ao descer, bloqueio da troca de título enquanto os detalhes estão expandidos, ações secundárias que saem do primeiro botão ao receber foco e ação de adicionar ao clicar. O bloco de título, informações, botões e sinopse foi baixado no card e nos detalhes expandidos, conforme confirmação do dono. Filmes longos limitam o deslocamento pela altura medida; temporadas/episódios conservam suas posições independentes. Manter duração, repetição nos vistos e tempo restante nos incompletos; elenco centralizado, retorno e abertura direta de detalhes pela filmografia.
- **Ajustes v2:** escala própria de 80%, 90% ou 100%, com padrão de 90%. Medição, desenho e navegação usam o mesmo fator; a opção não altera o zoom global do app.
- **Menu Moderna:** centralizado verticalmente e 20% maior. Esta é a decisão atual, substituindo a regra antiga de menu abaixo do relógio e escala fixa de 90%. Apple TV conserva sua geometria e navegação próprias; Padrão conserva o alinhamento à borda.
- **Material Glass:** menu Apple TV, folha de Fontes, painéis de categorias e addons do Guia e modal de envio ao amigo passam pelos helpers compartilhados de material, respeitando opacidade e Frost. Conferir foco, véu, contraste e modo sólido nas capturas finais.
- **Home e player:** conservar a atividade de carregamento na ilha, a confirmação de Home carregada por um segundo, o blur do próximo episódio e as correções de retomada após seek e de sobreposição com Pular créditos.
- **Social:** atualizar os dados sem reaproveitar uma geração anterior de conta; distinguir carregando, privado, erro e vazio. Presença ao vivo usa ponto verde. Reação e conclusão são estados independentes; atividades de episódios diferentes não são fundidas.

## Diagnóstico de desempenho

Os 128 MB ou 300 MB citados aqui são o orçamento do cache de imagens/texturas, não a RAM total do processo. O diagnóstico pode experimentar um orçamento maior e restaurar o anterior quando o reteste não justificar mantê-lo; portanto, um candidato de 300 MB não garante que esse será o valor final.

A correção faz ambas as apresentações consultarem o perfil realmente ativo após a decisão e a restauração. A comparação antes/depois só apresenta um aumento como aplicado quando ele foi mantido. `tests/diagnostico.sh` passou com cache real: melhora, piora, ruído, cancelamento, escolha manual, mudança posterior e persistência. No host a transição é 96→160 MB; a política isolada também cobre Android 128→300 MB. Ainda não foi medida essa transição numa TV física.

## Evidência de validação

O responsável pela integração confirmou 13 verificações direcionadas no host:

| Área | Verificação |
|---|---|
| Traduções e opções | `i18n.sh`, `ajustes_secoes.sh`, `ajustes_padroes.sh`, `acentos.sh` |
| Menu e materiais | `menu_geometry.sh`, `glass_surfaces_contract.sh`, `ctxmenu_contract.sh` |
| Ilha e novidades | `ilhafila.sh`, `novidades_fila.sh` |
| Detalhes e serviços | `detail_layout.sh`, `ondever_folha.sh` |
| Social | `recomenda_social.sh`, `socialvis.sh` |

Também passaram quatro casos do backend usando SQLite real em `servidor/recomendacoes/teste-social-semantica.mjs`. Não foi executada a suíte inteira.

Capturas temporárias do host estão em `/tmp/nuvio-180-review/`. Foram conferidos Ajustes 80/90%, menu Moderna centralizado, menu Apple TV com Streaming, Fontes, Guide e envio ao amigo em vidro/Frost e sólido, além do menu contextual fora do card selecionado. As verificações de material e tradução foram repetidas após os ajustes. A nova posição Apple TV passou pelo teste de geometria e pelas capturas finais de série e filme, inclusive título localizado longo, sinopse em cinco linhas, retomada e recomendação: os elementos permanecem dentro da margem inferior. Compilação e captura no Mac não comprovam comportamento em LG, Samsung ou Android TV.

## Limites antes da release

- A identidade Social canônica e a unificação completa dos canais continuam pendentes. A implementação atual combina Nuvio e Trakt; ainda não há adaptadores completos de Simkl e Letterboxd. Não anunciar todos os canais ou estatísticas como unificados.
- A build vista na TV pertence a outra linha de desenvolvimento. Sua etiqueta 1.7.7/10707 foi usada apenas para instalação de teste, conforme esclarecimento do dono; não define uma release publicada. Identificar o código pelo commit e conteúdo do APK. Nenhuma instalação, empacotamento, publicação ou deploy deste candidato foi realizado nesta etapa, inclusive das alterações locais do backend.
- A troca dos arquivos de versão para 1.8.0, o fechamento do merge, os pacotes e a validação em dispositivos devem ser registrados separadamente quando concluídos. As [notas](NOTAS.md) continuam um rascunho.
