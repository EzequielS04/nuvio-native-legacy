# Social: contrato TV <-> servidor (redesenho de 02/10/2026)

Servidor: `servidor/recomendacoes/src/social.js` (+ `index.js`). Tabelas:
`migracao-006-social.sql`. Cliente: `src/recomenda.h` (secao "REDESENHO").
Testes: `servidor/recomendacoes/teste-social.sh`, `tests/recomenda_social.sh`.

## Identidade

- Cabecalhos de sempre: `Authorization: Bearer <token>` + `X-Nuvio-Auth: nuvio|trakt`.
- **Novo**: `X-Nuvio-Perfil: <profile_index>` so quando o perfil ativo NAO e o
  principal. Id da pessoa: `nuvio:<sub>:<indice>`. Principal (sem cabecalho)
  continua `nuvio:<sub>` — nenhum dado antigo muda de dono. Trakt ignora.
  Valor fora de 1..32 ou nao numerico = principal.
- `POST /v1/eu` aceita `{"nome": "<nome do perfil>", "avatar": "<url>"}`
  (corpo vazio continua valido). Avatar so `https` de host conhecido (host do
  `SUPABASE_URL`, Trakt, ou a lista `AVATAR_HOSTS` do worker); senao "".
  Resposta ganha `nome` (pronto), `exibicao`, `alcance`, `perfil`.
- Nome pronto (`pessoa.nome`): exibicao > nome do perfil (Nuvio) / da conta
  (Trakt) > `Amigo #<rowid>`. Nunca o id.
- `POST /v1/eu/nome {"nome": ".."}` — nome de exibicao (letras, numeros,
  espaco, `'` e `-`, ate 32). `""` apaga e volta ao nome do perfil.

## Privacidade (alcance)

`POST /v1/alcance {"nivel": 0|1|2}`. `pessoa.alcance`: -1 nao respondeu (=0),
0 ninguem, 1 so amigos, 2 amigos de amigos. Conferido em toda escrita e
leitura. Nivel 0 apaga eventos, "agora" e agregados.

Decisao: **separado** do `descobrivel` ("quem pode me achar") e do antigo
`perfil.ativ` (rota velha `/v1/amigos/atividade`). Motivo: perguntas
diferentes; e `ativ` antigo nao vira alcance (eventos novos — abandono,
reacao, tempo — sao mais do que aquela resposta aceitou).

## POST /v1/atividade (eventos do player)

Exatamente o contrato pedido; **nada mudou**:

```
{"ev":"inicio"|"progresso"|"fim"|"abandono"|"reacao"|"salvo", "imdb":"tt..",
 "midia":"movie"|"series", "titulo":"..", "poster":"https://..", "temporada":0,
 "episodio":0, "pct":0-100, "seg":<s neste trecho>, "reacao":1|0|-1, "rec":<id|0>}
```

Resposta: `{"ok":1,"guardado":0|1,"evento":0|1}` (`guardado:0` = alcance < 1).
Detalhes do servidor:
- `progresso`: so atualiza "assistindo agora" (vence 15 min) e agregados; nao
  vira evento. `inicio` tambem liga "agora"; `fim`/`abandono` desligam.
- Mesmo ev+titulo+episodio+reacao em 10 min = um evento so.
- `seg` limitado a 4 h por evento; `poster` so de host conhecido (tmdb,
  metahub, amazon, tvdb, trakt), senao vazio.
- `rec`: so altera a rec se ela foi mandada PARA quem envia
  (`inicio`/`progresso` -> comecou, `fim` -> terminou, `reacao` -> reacao).
- Limite 600 eventos/h por pessoa. Retencao 90 dias.
- **Corpo antigo** (sem `ev`: `imdb,tipo,titulo,ano,nota,agora`) continua
  aceito na mesma rota e vai para a logica antiga de `amigos.js` — TVs no ar
  e o `enviarAtividade` atual de recomenda.c (fileira "Entre amigos" via
  `perfil.ativ`) usam. A rota decide pelo campo `ev`.

## GET /v1/feed?desde=<id>

Eventos `inicio|fim|abandono|reacao|salvo` de quem eu posso ver, mais novo
primeiro, ate 50, com `ETag`/`If-None-Match` (304). O cliente pede sempre
`desde=0` (para quem baixa o nivel sumir da lista).

```
{"cursor":11,"itens":[{"id":11,"de":"nuvio:aaa"|"pub:<handle>","deNome":"..",
 "deAvatar":"..","grau":1|2,"via":"<amigo em comum, grau 2>","ev":"reacao",
 "imdb":"tt..","midia":"movie","titulo":"..","poster":"..","temporada":0,
 "episodio":0,"pct":0,"reacao":1,"criado":1790974381}]}
```

Grau 2 (amigo de amigo, so com alcance 2): id vira handle opaco, sem foto.
Bloqueio (qualquer sentido) esconde. Limitacao: o ETag e maior id + contagem.

## GET /v1/amigo?id=<id de contato | pub:handle>

```
{"id","nome","avatar","grau","via","desde","origem":"codigo|trakt|sugestao|pedido|",
 "compartilha":0|1,
 "mes":{"mes":"2026-10","seg":2700,"filmes":1,"series":1}|null,
 "agora":{imdb,midia,titulo,poster,temporada,episodio,pct,atualizado}|null,
 "gostou":[{imdb,midia,titulo,poster,criado}] (ate 10),
 "recs":[{id,imdb,tipo,titulo,poster,criado,"estado":"entregue|aberta|comecou|terminou|reacao","reacao":1|0|-1|null}] (so grau 1, ate 20),
 "gosto":{"total","iguais","pct"}|null}
```

404 para quem nao e contato nem amigo de amigo visivel (mesma resposta de
"nao existe"). `gosto` so quando os dois tem alcance >= 1. `origem` vazio =
contato de antes da migracao.

`GET /v1/contatos` ganhou `desde` e `via` em cada contato.

## Merge de fontes (cliente)

`RecEvento` (recomenda.h) e o formato unico. `rec_eventos_unir` deduplica por
pessoa + imdb + acao (reacao tambem pelo valor) com |dt| <= 1 h, ou sempre
que um dos dois nao tem hora; na fusao vence o do nosso servidor, completado
pelo outro. `recomenda_feed_unido` junta o cache do feed com os itens de
`trakt_social`. Trakt "assistindo agora" = `REC_ACAO_INICIO`, "assistiu" =
`REC_ACAO_FIM`, "avaliou" = `REC_ACAO_NOTA`.

Simkl e Letterboxd: nao implementados (ver docs/social-fontes.md). Se o dono
decidir, a UI precisa pedir: Simkl = id NUMERICO do amigo; Letterboxd =
usuario do amigo (idealmente informado pelo proprio amigo); e o Letterboxd so
traz TMDB id (precisa TMDB->IMDb antes do dedupe).

## Ligacao com o modulo do player (agente/reacao, src/atividade.c)

- Permissao: no arranque, `recomenda_ao_mudar_alcance(atividade_definir_permitido)`.
  Chama na hora e a cada mudanca de nivel, fora do mutex; nao perguntado = 0.
- Identidade: `atividade.c` monta os cabecalhos sozinho e **nao manda
  `X-Nuvio-Perfil`** — num perfil nao principal os eventos cairiam na pessoa
  do principal. No merge, trocar o `identidade()` dele por
  `recomenda_cabecalhos(cab /*[4]*/, aut, .., via, .., perfil, ..)`.
- `recomenda_atividade()` (fila propria em recomenda.c) faz o mesmo papel;
  com o atividade.c no merge, um dos dois sobra — sugestao: ficar com o
  atividade.c (tem fila em disco) e apagar `recomenda_atividade`.
