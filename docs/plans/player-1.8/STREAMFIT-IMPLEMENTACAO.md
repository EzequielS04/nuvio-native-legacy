# StreamFit — implementação e ligação pendente

## Estado verificável

`src/streamfit.c/h` implementa a conta, o histórico em RAM por aparelho/rede/host e uma fotografia imutável. `src/streams.c` aplica a partição na folha real de fontes. **Epoch Android e diagnóstico real agora estão ligados**, com janela descartável N01, HTTP/MIME/host final e cancelamento. **Duração do player/metadados e telemetria passiva continuam pendentes**: sem runtime válido, a classificação permanece desconhecida. LG/TPK/WGT continuam sem epoch confiável e não ingerem medidas legacy. Estado, arquivos, testes e limites concretos estão em [F03-EPOCH-DIAGNOSTICO-HANDOFF](../../releases/1.8.0/F03-EPOCH-DIAGNOSTICO-HANDOFF.md). Isso não é aprendizado passivo pronto.

`Stream.tamanhoBytes` vem exclusivamente da propriedade direta `behaviorHints.videoSize`. O [contrato oficial Stremio](https://github.com/Stremio/stremio-addon-sdk/blob/master/docs/api/responses/stream.md) define essa propriedade como bytes do arquivo de vídeo. Campos soltos, aninhados em `proxyHeaders`, tamanho de torrent/pacote e texto “50 GB” não ganham essa proveniência. `tamanhoMB` permanece para a apresentação e a ordenação antiga; ele não decide peso. O leitor exato aceita inteiros decimais ou strings de dígitos, sem arredondamento, até `2^53−1`. Fração, expoente, negativo e overflow ficam desconhecidos. Um add-on que declarar incorretamente o tamanho de um pacote nesse campo continua exigindo correção do fornecedor; o cliente não adivinha um tamanho por episódio.

O engine e a folha não iniciam rede, resolução de links, teste de velocidade, thread de transferência ou acesso a disco. O consumidor de diagnóstico reutiliza somente o teste explicitamente solicitado. Não há nova dependência. Os builds que usam `src/*.c`, incluindo CMake Android, descobrem os módulos automaticamente.

## API dos consumidores

1. **Epoch de rede/aparelho**: chamar `streamfit_rede(uint64_t chave)` após detectar uma rede confiável. `0` significa desconhecida. A chave é opaca, não precisa conter SSID/IP; pertence a este aparelho/processo. Trocar a chave limpa todas as medições, inclusive voltar a uma rede anterior. Não criar uma chave constante que represente redes diferentes. Chamar `streamfit_limpar()` ao invalidar o diagnóstico/aparelho. Persistência e detecção automática de troca de rede ainda não estão ligadas.
2. **Diagnóstico real por host**: após um teste concluído de mídia, chamar `streamfit_diagnostico(epoch, urlFinal, kbpsPorSegundo, n, fimMs)`. São aceitos de 5 a 48 intervalos válidos; zero conta, negativo/valor acima de 10 Gb/s não conta. A origem é o **host final realmente medido**, e não um endpoint genérico de speed test ou o resolvedor antes do redirect. Excluir teste falho, HTML/avisos, cache hit, pausa, espera por buffer cheio e séries incompletas/canceladas. Novo teste completo substitui o anterior daquele host; entrega antiga/repetida é recusada. `fimMs` usa milissegundos de relógio civil, a mesma base de `streamfit_agora_ms()`.
3. **Duração do item**: chamar `stream_fit_duracao(alvo, segundos, SF_DUR_METADATA)` com runtime real do filme/episódio, ou `SF_DUR_MEDIA` após o backend informar duração real. O alvo é exatamente o id usado em `stream_definir_alvo`, incluindo temporada/episódio. Não enviar `PLR_DUR_PADRAO`, total de temporadas, “45 min” presumidos ou `player_duracao_seg()` antes de saber sua proveniência. Zero/origem desconhecida limpa o dado. Duração de mídia prevalece sobre metadado atrasado do mesmo alvo. Mudança de arquivo/corte exige atualizar/limpar esse runtime pelo consumidor.

Não alimentar medidas de hosts finais em fontes que só tenham URL de um resolvedor como se fossem o mesmo host. Essas fontes ficam desconhecidas até haver URL/host final identificado daquela fonte sem sondagens extras. A estrutura atual consulta a URL presente no `Stream`; não há generalização de um redirect para todos os links do add-on.

## Conta, ordem e foco

Orçamento: `vazao_resumir()` existente, `p20 × 0,75`; teto informativo `mediana × 0,9`. Demanda média: bytes exatos ×8/segundos/1000 em kbps. `SF_ADEQUADA` cabe no orçamento; `SF_PESADA` o excede; `SF_DESCONHECIDA` não tem proveniência suficiente. A demanda é **estimativa**, sem garantia contra picos de bitrate/buffering. Duração inválida, menor que 1 s ou maior que 24 h, tamanho inválido, host sem medição e rede desconhecida não classificam.

Armazenamento: no máximo 32 hosts e 48 intervalos por host. Hosts são autoridades públicas normalizadas, sem usuário/senha/caminho/query; portas não padrão são distintas. Uma medição expira após 24 h. Relógio retrocedido/amostra futura não fica disponível. Nenhuma URL, velocidade ou rede é persistida no disco. Restartar o app começa indisponível.

Ao abrir a folha, `streamfit_foto()` congela velocidade, origem, idade e runtime. As classificações por índice bruto ficam armazenadas; mudar a medição, resolver uma URL ou receber metadado enquanto aberta não reclassifica os cartões existentes. Add-ons que chegam depois usam a mesma fotografia. Outra abertura usa a evidência nova. Substituição da lista invalida identidades antigas; alvo diferente não herda runtime.

A base aprovada Glass é mantida: grupos de resolução/HDR e tamanho decrescente dentro de cada grupo, empate por ordem do add-on. Sobre cada grupo, `streamfit_particionar()` põe adequadas/desconhecidas primeiro e pesadas depois, mantendo a ordem relativa das duas subsequências. Fonte pesada não fica fixada acima de todos os grupos como “Melhor para esta TV”. Não há movimento entre grupos nem exclusão. Isso não altera a fila de autoplay/sondagem (`fonteauto.c`), a preferida ou a regra PRIMEIRA/#130 de uma única verificação. A aba “Onde ver” usa identidades próprias e não entra na partição.

`stream_fit_folha_estado(indice, &resultado)` publica classificação congelada, demanda, orçamento/teto, número de amostras, idade e razão para a UI. Deve ser consultado no fio de UI, após a montagem; fora da folha devolve desconhecida. Novo log inglês da abertura: `[stream_fit] snapshot hosts=… classified=… heavy=… origin=diagnostic`, sem URL/título/credencial.

## Testes executados

- `tests/streamfit.sh`: 45.846 checks determinísticos, incluindo 400 permutações, host/rede/portas, mínimos, zeros/stall, inválidos, expiração/rollback, reteste, limites e concorrência; normal, ASan/UBSan e ThreadSanitizer passaram.
- `tests/streamfit_parser.sh`: 33 casos de proveniência exata; ASan/UBSan/float-cast-overflow passou. Parser antigo `stream_parser.sh` também passou.
- `tests/streamfit_sheet.sh`: usa **streams.c real**, com bordas de plataforma simuladas; ASan/UBSan passou. Prende grupos/tamanho, fonte pesada sem pin, foco, respostas incrementais, snapshot, troca de título, prioridade de runtime, preservação da lista e “Onde ver”. A fila PRIMEIRA conserva ordem e uma tentativa.
- `tests/fonteauto.sh` passou. `tests/streams_idade.sh` passou com ASan/UBSan após adicionar apenas stubs do pacote opcional de selos já exigidos pela lista.
- Syntax check dos três módulos modificados passou. O warning de indentação em `badges.c:195` pertence ao código anterior e não foi modificado.

Não foi compilado o core inteiro, instalado pacote, medido dispositivo físico ou executado rede nesses testes. Stubs de plataforma não provam telemetria/latência de TV.

## Gates antes de anunciar a função

- Ligar epoch da rede/perfil por TV e invalidar após troca/reteste; impedir medida ativa concorrente com reprodução, respeitando pedido/cancelamento/orçamento de bytes.
- Ligar séries do diagnóstico por host final com identificação de mídia e runtime real por alvo; definir o caminho para URL de resolvedor ainda não associada ao host real.
- Mostrar origem “Diagnóstico”, instante/idade, Mbps sustentados, orçamento e demanda estimada na UI traduzida; informar indisponível sem dados. Ajustar selo “Melhor para esta TV” à escolha/peso real, sem prometer que autoplay foi alterado.
- Android passivo: instrumentar `TransferListener` em `ParaleloDataSource` sem dupla contagem das conexões; contar somente bytes de mídia enquanto download ativo, incluindo stalls, excluindo cache/pausa/buffer cheio. Esse adaptador não está implementado.
- LG/uMS, Samsung WGT/AVPlay e TPK: provar telemetria própria antes de dizer “aprende sua reprodução”; até lá usar somente diagnóstico. Sem inferir velocidade do buffer interno.
- Validar em LG/Android/Samsung: resultados chegando com foco em fonte, grupo/aba/filtros, resolução manual, runtime/epoch corretos, reteste e mudança de rede. Medir startup/seek/stalls/fluidez/RAM com origem e tamanho reais, em corpus separado de testes de host.
