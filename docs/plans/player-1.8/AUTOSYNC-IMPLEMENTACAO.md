# AutoSync temporal — engine comum e documentos de legenda

03/10/2026. Implementação C em `src/autosync.c/h` e adaptação de `src/legenda.c/h`, posterior ao merge Glass `b98bd2f5`. Este documento descreve capacidades de código e testes no host. Não comprova disponibilidade de referência embutida nem desempenho em LG, Android ou Samsung.

## O que funciona neste módulo

- Documentos imutáveis independentes do overlay, com idioma, origem, identidade opaca da faixa/arquivo, geração de sessão, flags e cues ordenadas. Contagem de referências permite compartilhar uma referência validada entre dois slots sem copiar o documento novamente.
- Parsing real SRT, WebVTT e ASS/SSA; conversão existente UTF-8, UTF-16 e Windows-1251/1252. O construtor de documento preserva o corpo válido para desenho, mas remove `COMPLETO` quando o parser descartou um timestamp inválido ou só conseguiu um prefixo após atingir o limite/falha de alocação. O comportamento do parser legado para playback permanece igual.
- Correlação de atividade temporal por intervalos unidos, sem exigir o mesmo texto entre idiomas. Texto só detecta repetição excessiva; não produz uma promessa semântica de que os arquivos pertencem ao mesmo filme.
- Dois slots de seleção e dois offsets automáticos/manuais independentes. Desfazer zera só o automático. Trocar documento/sessão cancela a análise e invalida ambos os offsets daquela seleção.
- Um worker por contexto do player, com cancelamento por geração de sessão e seleção, teto de tempo, fila alternada entre os dois slots e resultados publicados apenas se os donos continuam atuais. A UI consulta estado e offset; nenhum callback altera playback de outra thread.
- Tentar outra referência exclui a última identidade/hash naquela sessão e seleção, cancela resultados atrasados e desfaz sua correção automática. Até 16 exclusões; ao esgotar a lista a busca fica indisponível para aquela seleção, sem esquecer referências recusadas.
- Eventos novos em inglês, com motivo estável, slot, offset, score e tempo. Não registram cues, idioma, identidade da mídia, URL, headers ou credenciais.

## Aceitação e custo

O candidato vem de histogramas de diferenças entre inícios, quantizados em 25 ms. Os 24 picos mais votados recebem um Jaccard de atividade corrigido pela coincidência esperada. Não existe envio de áudio, download, troca de faixa, acesso à rede ou dependência externa neste caminho.

O mesmo limiar vale em Quick e Thorough: score global mínimo 0,78 e margem mínima 0,09 sobre outra hipótese; por região, score mínimo 0,72 e margem mínima 0,07. `confianca` é um score heurístico, **não uma probabilidade calibrada de acerto**. Alterar a tolerância não altera os limiares, o raio de busca ou a distância fixa usada para separar hipóteses.

Quick valida 3 regiões, Thorough 6, distribuídas sobre a cobertura inteira. Além da média, cada intervalo de diálogo precisa ter bordas correspondentes nos dois documentos dentro da tolerância, nos dois sentidos. Assim um corte curto no meio/fim não fica oculto por muitas falas corretas. `erroMs` informa o maior residual observado nas bordas ou discordância entre offsets regionais. Não faz escala, interpolação nem retiming por segmentos.

São recusados: documentos parciais; flags forced/sinais; excesso de cues de música/SDH ou ASS posicionado no alto; falas muito repetidas; menos de 30 cues úteis/24 intervalos; menos de 180 s de cobertura ou 40 s de fala; atividade esparsa ou quase contínua; pico ambíguo; desacordo de regiões/bordas; geração antiga; orçamento esgotado. Se durações de mídia confiáveis foram fornecidas para ambos, uma diferença maior que a tolerância também recusa o par.

Valores iniciais do código, ainda sujeitos à calibração do corpus humano: tolerância 250 ms, raio independente ±60 s, teto Quick 4 s e Thorough 16 s. A API aceita tolerância 50–1000 ms, raio 1–120 s e orçamento de 1–20000 ms. O cancelamento/orçamento é verificado entre lotes; não é uma garantia de preempção em exatamente esse milissegundo. São aceitos no máximo 7999 cues para sincronização; 8000 sugere truncamento do parser e recusa. Corpos têm teto de 16 MiB. Cada documento conserva `LegendaCue` completa para desenho; o custo depende do número de cues e pode chegar a aproximadamente 6,6 MiB por documento. A análise usa vetores compactos de intervalos/hashes e histograma, sem copiar textos para cada hipótese.

A regra de bordas é propositalmente conservadora. Uma tradução com divisão/timing diferente pode ser recusada mesmo pertencendo à mesma edição. Divisões contínuas e cues sobrepostas são unidas, mas lacunas diferentes e timings além da tolerância recusam. Melhorar recall exige corpus humano antes de relaxar o aceite global.

## Contrato de integração

```c
AutoSync *sync = autosync_criar();
autosync_iniciar(sync, sessaoDoPlayer); /* geração monotônica por mídia */

LegendaDocumentoInfo info = {
  .sessao = sessaoDoPlayer,
  .flags = LEGENDA_DOC_COMPLETO,
  .idioma = "pt",
  .origem = "OpenSubtitles",
  .identidade = "opaque-addon-subtitle-id"
};
LegendaDocumento *doc = legenda_documento_bytes(bytes, tamanho, &info);
autosync_selecionar(sync, 0, doc); /* 1 = idioma secundário */
legenda_documento_liberar(doc); /* contexto manteve sua própria referência */

/* Só entregar uma referência REAL, completa e da mesma sessão. */
AutoSyncConfig cfg = autosync_config(AUTOSYNC_QUICK);
autosync_solicitar(sync, 0, referenciaCompleta, &cfg);

/* Na thread da UI/desenho: */
AutoSyncResultado estado = autosync_estado(sync, 0);
int offsetTotal = autosync_offset_ms(sync, 0);
/* Positivo adianta no overlay atual: t = playback + offsetTotal/1000. */
```

Pontos mínimos no player/faixas:

1. Criar o contexto em um ponto que não segure o primeiro frame; iniciar nova geração ao abrir/trocar mídia e limpar a seleção ao desligar/trocar legenda. Não esperar análise antes de `video_iniciar`/play.
2. Criar documentos no worker que já baixou os bytes. Para snapshot de overlay existente, `legenda_documento_ativo(legenda_geracao(), &info)` copia somente se essa geração ainda é dona; sort/hash são feitos fora do lock do overlay. Snapshot não descobre idioma/origem/completude: o chamador deve fornecer metadados corretos.
3. Importar o ajuste manual atual com `autosync_manual`. Renderizar o offset total exatamente uma vez. O sinal corresponde a `legenda_cues` e `assrender_desenhar` atuais, onde positivo adianta; não assumir que o backend nativo usa o mesmo sinal. Não somar novamente `legEstilo.atrasoMs` quando `offsetTotal` já o contém.
4. Slot principal: passar o total ao overlay SRT/VTT ou `assrender_desenhar`. Slot secundário: `legenda_documento_cues` permite buscar os próprios cues com o próprio offset em outra área de desenho. Dois documentos/offsets estão implementados; **o segundo overlay libass/controle de colisões ainda exige ligação no player**, e o renderer libass atual é singleton.
5. Se estiver em seek/menu sensível ou houver competição de CPU/rede, usar `autosync_cancelar`; quando seguro, solicitar de novo com a referência real. A API não disputa a conexão de mídia.
6. Mapear estado `UNAVAILABLE/ANALYSING/ACCEPTED/REJECTED/CANCELLED` para mensagens traduzidas. `autosync_tentar_outra` só exclui; seleção/download da próxima referência é responsabilidade do chamador, respeitando capabilities e orçamento de bytes.
7. `autosync_desfazer` mantém o manual. `autosync_iniciar` limpa seleções, offsets e exclusões. `autosync_destruir` cancela e faz join; usar no teardown e não no início da reprodução. Não chamar funções depois da destruição.

### Referência embutida: limite real da base

`mkvass.c` hoje colhe a faixa ASS/SSA escolhida e publica no overlay dessa escolha. Não fornece um segundo documento independente da faixa selecionada. A janela parcial à frente do playhead não demonstra o restante do filme. `mkvass_iniciar` troca a colheita e portanto **não deve ser chamado para fabricar uma referência de AutoSync**. MP4/SRT embutida e a extração completa paralela não estão disponibilizados pela API atual.

Uma referência ASS completa de sidecar/coletor existente pode virar `LegendaDocumento` somente quando o chamador confirmou sua origem, faixa, sessão e completude. Uma referência externa completa pode ser usada pela engine nos testes/comparações explícitas, mas não equivale a uma referência embutida. Na ausência de referência real completa, o produto deve manter offset automático zero e mostrar indisponível, sem esperar nem mudar a seleção da pessoa.

VAD, PCM, speech model, AudioSync, download de modelo e APIs de ASR não foram implementados por este módulo. Nenhum modelo ou licença nova foi incorporado. Separar esses capabilities evita anunciar áudio sync apenas porque a engine temporal existe.

## Verificação executada

```sh
bash tests/autosync.sh
SANITIZE=1 bash tests/autosync.sh
SANITIZE=thread bash tests/autosync.sh
bash tests/legenda_ass.sh
cc -Isrc -Wall -Wextra -Werror -fsyntax-only src/autosync.c
```

562 verificações determinísticas no host: offsets ±0,25/1/5/30 s em ambos os modos; tradução com atividade compatível; SRT/VTT/ASS reais; charset; snapshots imutáveis após desligar/trocar overlay; forced/sinais/música/repetição; ambiguidade temporal periódica; pouco diálogo; edição diferente, escala 23,976→25 e deriva; corte em uma cue no meio/fim; ruído ±100 ms aceito com tolerância250 e recusado com50; raio independente; budget; duas línguas/manual/desfazer; exclusão; geração/seleção antiga; 100 timelines diferentes em cada modo com cortes adversariais; documentos7999cues e100trocasdesessão enquanto há trabalho.

ASan/UBSan e ThreadSanitizer passam sem erros observados. Os testes do parser ASS/SRT/VTT legado continuam passando. As timelines são sintéticas e os offsets têm ground truth determinístico; ainda falta um corpus de legendas legais reais com anotações humanas, medição de falsos aceites/recall e benchmark físico por plataforma. O maior tempo observado em uma execução sintética normal no Mac foi24ms, e102ms sob ThreadSanitizer; isso não estima latência de nenhuma TV.

Não houve compile integral, instalação na TV, publicação nem ligação de backend/player feita por este módulo isolado.
