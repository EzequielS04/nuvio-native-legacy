# Ajustes: pesquisa e proposta de UX

Base: `release/1.6.6` (399c195). Etapa de pesquisa: nada de código de produto,
nenhum texto novo de tela. Os rótulos citados abaixo são os de hoje, só para
identificar a linha; os textos novos vêm depois, numa lista à parte.

O pedido do dono: "repensar em como as pessoas usam e mexem nas configs, para
entender tudo o que cada coisa faz, para deixar do jeito da pessoa". A troca de
visual da 1.6.x foi rejeitada ("o problema tá igual, só tá com outro design").
Este documento tenta explicar por que o problema continua igual e o que mudaria
de fato.

---

## 1. Inventário (medido no código)

Fonte: `src/ajustes.c` (enum `OpcaoId`, `OPCOES[]`, `CHAVE[]`, `valor[]`,
`TELA[]`, `ajudaOpcao`, `efeitoOpcao`, `desenhaPrevia`, `somenteDesteAparelho`).
Contagem feita por script sobre o fonte, não à mão.

| | |
|---|---|
| Opções no enum `AJ_*` | **176** |
| Por tipo | 127 escolhas (97 delas são liga/desliga), 10 números, 33 ações, 6 só leitura |
| Ajustes com valor (escolha + número) | 137: 76 podem ir para a conta, 61 ficam só nesta TV |
| Chaves com "-" (não gravam) | 41 (ações, leituras, credenciais em arquivo próprio) |
| Estrutura da tela | 9 categorias › 13 grupos recolhíveis + 9 rótulos fixos › 176 linhas |
| Profundidade | Ajustes › categoria › grupo › linha › (folha/teclado/sub-tela). Ex.: "Trailer do cartaz em foco" = Ajustes › Layout › Foco no pôster › 3ª linha |
| Linhas por categoria | Layout **76**, Integrações 41, Conteúdo 14, Avançado 13, Reprodução 10, Aparência 9, Conta 6, Sobre 4, Trakt e Simkl 3 |
| Maior grupo | "Conteúdo da Home": 20 linhas misturando fileiras, barra lateral, destaque, rótulos, notas e o fundo da escolha de perfil |
| Têm frase de ajuda própria | 175 de 176 (falta só `AJ_SALVOS_DEST`, que cai no texto genérico) |
| Têm a linha "o que muda na prática" (`efeitoOpcao`) | 14 |
| Têm prévia desenhada (`desenhaPrevia`) | 8 opções, 5 desenhos distintos — todos **esquemáticos** (retângulos cinza), não a tela de verdade |

**Qualidade da ajuda.** A quantidade de texto não é o problema; o tipo de
texto é. Lendo as 175 frases, uma a uma:

- ~25 repetem o rótulo ou ficam no jargão sem dizer o que a pessoa vê:
  "Controla o arredondamento dos cantos dos pôsteres", "Quanto a borda do
  cartaz em foco acende", "Use Reduzidas para movimentos mais discretos",
  "Escolhe o próximo episódio a partir do mais avançado marcado como
  assistido", e os 8+11 toggles de nota com a mesma frase.
- Várias não dizem o que o **Desligado** faz. Ex.: Dolby Vision ("Preferência
  para fontes compatíveis") — desligar evita fonte DV? deixa tocar sem DV? A
  pessoa com TV sem DV precisa exatamente dessa resposta.
- Pelo menos uma é enganosa por omissão: "Largura do item" — o próprio
  `ajustes.h` avisa que no layout Moderna essa preferência **não muda o
  tamanho** do cartaz; a ajuda não diz.
- As longas (Layout da home: 377 caracteres; Limite de fileiras: 315)
  explicam bem, mas viram parágrafo no painel e ninguém lê do sofá.

**Mesmo assunto espalhado em vários lugares** (a causa mais forte de "não
achei"):

| Assunto | Onde está hoje |
|---|---|
| Idioma | Aparência › Idioma (interface) · Reprodução › Idiomas (áudio, legenda) · Integrações › TMDB › Idioma dos metadados · estilo da legenda só dentro do player |
| Trailer | Layout da Home (destaque, som, espera) · Foco no pôster (cartaz em foco) · Página de detalhes (automático, som, botão, qualidade, proporção, fonte) — 10 linhas em 3 grupos |
| Notas | Conteúdo da Home › Avaliações gerais · Integrações › MDBList (8 fontes) · Integrações › Notas no título (11 fontes) |
| Arte | Layout da Home (Background do hero, Destaque com outra arte) · Pôsteres personalizados (7) · Arte do addon (4) · Integrações › TMDB › Arte localizada · fanart.tv |
| Ficha do título | Página de detalhes (Priorizar metadados externos **e** Usar sempre o Cinemeta, que se sobrepõem) · TMDB (14 toggles) |

**Por que a troca de visual não resolveu.** A árvore atual é a do app web
(`SECTION_META` de `settingsScreen.js`), que organiza por **onde o ajuste mora
no código** (Layout, Integrações, Avançado), não pelo que a pessoa quer fazer.
"Layout" sozinho tem 43% de todas as linhas. Redesenhar a mesma árvore mantém
os mesmos caminhos de 4 níveis, os mesmos assuntos espalhados e as mesmas
prévias que não mostram a tela da pessoa.

O que já existe e serve de base (não reinventar):
- `TELA[]` é **separada do enum** desde a 1.4.4: reorganizar a tela não toca
  `valor[]`, `CHAVE[]` nem o arquivo.
- `inativa()` já conhece as dependências entre opções (e diz o porquê).
- `seguro.c` já faz "experimentar e voltar sozinho se cair" para 10 ajustes
  arriscados.
- `ajustes_abrir_na_cor/fonte/layout/vidro` já são atalhos que pousam o foco
  numa linha (usados pelos cartões de novidades).
- O menu de segurar OK no cartaz (`ctxmenu.c`) já tem "Estilo da fileira": um
  ajuste contextual que funciona.
- `perfiltv.c` (`PtvPerfil`) e o diagnóstico já aplicam um perfil "Qualidade"
  ou "Desempenho"; `gpunivel.c` já mede a GPU no .tpk.
- O Spotlight (`spotlight.c`) busca títulos, pessoas, canais, catálogos e
  addons — **não busca ajustes**.

---

## 2. Uso real

### 2.1 Registros das TVs (D1, só agregados)

Janela: ~9,4 dias de registros, **265 aparelhos** (webOS 162, Tizen .wgt 73,
.tpk 49, VIDAA 5, Android 3, Mac 1). Inclui as TVs de teste do dono. Viés:
só quem tem envio de registro ligado.

**Não existe hoje uma linha de log por ajuste mudado.** O que dá para medir:

| Sinal | Resultado |
|---|---|
| `[seguro] em prova` (os 10 ajustes arriscados) | **105 aparelhos (40%)** mudaram ao menos um. Vidro **93** · tema imersivo 35 · limite de fileiras 31 · interface 4K 26 · qualidade da imagem 24 · trailer no destaque 22 · itens por fileira 16 · P2P 13 · memória de imagens 11 · trailer do cartaz 4 |
| `[seguro] revertido` (o app caiu e desfez) | 11 aparelhos: vidro 5, imersivo 2, 4K 2, qualidade 1, P2P 1 |
| `[fileiras] escolha local: limite N` (último por aparelho, 201) | 130 no padrão 7 · 24 em 16 (o padrão antigo, suspeito que não foi escolha) · **47 (23%) escolheram outro**, de 3 a 40 (6 no máximo, 40) |
| `[tex] teto de N MB pedido em Ajustes` | 35 aparelhos fixaram a memória de imagens (14 no máximo, 512 MB) |
| `[gpu-modos] lento` (90 aparelhos, só sessões lentas) | vidro ligado em 54 (60%), tema dinâmico em 37 (41%), layout Dinâmica 17, Padrão 3 |
| `[ajustes] blob da conta` (último arranque, 200) | em 120 a conta mudou algum ajuste da TV; 69 aparelhos sobem ajustes da TV para a conta |
| `nao reconhecido; mantido` | `tmdb_language` com valor que a TV não conhece em 16+ aparelhos (`pt-br` 7, `ro` 4, `el`, `es-419`, `ru`, `ar`, `uk`); `selected_theme=CUSTOM/DARK` 3 |

Leitura:
- As pessoas **mexem**, e mexem no visual: vidro é o ajuste mais mudado e o que
  mais derrubou o app. Entre as sessões lentas, 60% estão com vidro.
  Recomendação por TV e "experimentar" têm demanda medida.
- Quase um quarto mudou o número de fileiras — ajuste de "como fica a
  minha home", hoje no 1º grupo de Layout, ok.
- A conta e a TV brigam por ajuste com frequência (120/200 arranques com
  mudança vinda da conta). "O que mudou e de onde veio" não é luxo.
- Achado fora do escopo, **suspeito, não provado**: `tmdb_language` "pt-br"
  da conta não casa com a lista `W_TMDB_LING` (que tem "pt"), então a TV
  ignora o idioma de metadados escolhido no web. Pode estar por trás do #209
  (títulos em inglês com português escolhido). Merece uma etapa própria.

### 2.2 Issues do GitHub (204 lidas pelo título, ~50 abertas e lidas inteiras)

| Padrão | Issues |
|---|---|
| **Pediu algo que já existia / não achou** | #124 (trailer no destaque já existia), #127 ("Separar futuros" existia), #57 (o dono pergunta se a pessoa conhece "Escolher a fonte ao reproduzir"), #183 (o dono indica Ajustes › Layout › Fonte do trailer), #201 ("Is there a setting I'm missing?" — o estilo por fileira está no segurar OK), #71 (linha de leitura confundida com ajuste) |
| **Ajuste que não fazia o que dizia / efeito invisível** | #160 ("Catálogos do destaque" nunca fez nada), #162 ("Local do Descobrir" sem efeito), #133 e #177 (desfocar não aplicava), #199 (mexeu em "não exibidos" e na fonte do CW, nada mudou), #205 (mudar a fonte do CW "conserta" a fileira), #202 (texto branco no branco na própria tela de Ajustes) |
| **Não confia que salvou** | #85, #129, #149 (voltavam ao padrão ao reabrir) |
| **Idioma em vários lugares** | #3, #8, #150, #209 — a pessoa escolhe português "no app e no TMDB" e não entende por que a sinopse vem em inglês |
| **A tela em si** | #11 ("cluttered"), #75 (pediu cabeçalhos selecionáveis para pular a lista), #144 ("mainly the settings are really bad") |
| **Pedidos de personalização** | #95, #162, #163, #90, #142, #104, #47, #91, #175 |
| **Trailer (o tema mais recorrente)** | 13 issues: #15, #60, #82, #86, #123, #124, #136, #140, #168, #178, #183, #195, #204 |

Nas respostas, o dono precisa escrever caminhos de 4 níveis
("Settings › Layout › Poster focus › Trailer on the focused poster"): sinal
direto de que achar é o problema, não entender a frase.

---

## 3. Referências (o que funciona com controle remoto)

| Produto | O que faz bem | O que levar |
|---|---|---|
| **tvOS Ajustes** | Lista curta de categorias por tarefa ("Vídeo e Áudio", "Acessibilidade"); valor à direita; OK abre a **lista de valores com ✓**, não alterna no lugar; painel com imagem do que a linha muda; legenda com **amostra ao vivo** do estilo | Escolha em lista vertical com explicação por valor; prévia ilustrada à direita |
| **Google TV / webOS (ajustes rápidos)** | Engrenagem abre um **painel lateral por cima do conteúdo**: muda imagem/som e o efeito aparece atrás, na hora; "Todos os ajustes" fica um nível abaixo | Ajuste contextual sobre a tela real é a prévia mais honesta que existe |
| **Google TV (primeira vez)** | Pergunta quais serviços a pessoa assina e personaliza a partir disso | Assistente curto de gosto, não de configuração |
| **Netflix** | Quase nenhum ajuste no app; áudio/legenda escolhidos **no player**, lembrados por perfil | O lugar certo de um ajuste é onde ele é sentido |
| **Kodi** | Nível de ajustes **Básico / Padrão / Avançado / Especialista**; texto de ajuda fixo embaixo; "restaurar padrão" por seção | Esconder o avançado por um interruptor, não por mais um nível de menu |
| **Plex** | "Mostrar avançado" na própria lista; qualidade separada por rede local/remota; ajustes por aparelho vs por conta claramente separados | Dizer na linha se vale "nesta TV" ou "na conta" |
| **Stremio (TV)** | Uma página só, seções com âncora na coluna da esquerda, poucos ajustes | Poucas seções, todas rasas |
| **Infuse** | Seções por assunto (Reprodução, Legendas, Áudio); descrição curta por linha; amostra da legenda | Legendas + áudio + idioma num lugar só |
| **iOS / macOS / Android (telefone)** | Busca nos ajustes por palavra e sinônimo | tvOS não tem; aqui o Spotlight já existe e já tem teclado de D-pad |

Regras de controle remoto que todas seguem: no máximo 2 níveis até a linha;
←/→ só para liga/desliga e números; acima de 2 valores, OK abre lista; Voltar
sempre sobe um nível; o foco nunca some.

---

## 4. Proposta

### 4.1 Arquitetura por intenção

Onze seções pela pergunta que a pessoa tem na cabeça. Toda opção do enum foi
encaixada (conferido por script: 176, nenhuma repetida, nenhuma faltando).
**116 visíveis, 60 em Avançado** (escondidas até ligar "Mostrar ajustes
avançados", como no Kodi/Plex). Agrupando os blocos de liga/desliga parecidos
numa só linha de múltipla escolha (11 notas, 4 itens da barra, 5 chaves de
serviço, Stalker/Xtream), sobram **~85 linhas visíveis**, nenhuma seção acima
de 24, e todo caminho com no máximo 2 níveis.

```
 Buscar nos ajustes            (abre o Spotlight já filtrado em Ajustes)
 O que você mudou (12)         (só aparece com algo diferente do padrão)
 ─────────────────────────────
 1  Tela inicial               estilo · fileiras · Continuar assistindo · barra lateral
 2  Cartazes e arte            cartaz em foco · forma · de onde vem a arte
 3  Trailers                   onde toca · som · fonte
 4  Página do título           o que aparece · notas · dados do título
 5  Reprodução                 escolha da fonte · imagem e som · durante e depois
 6  Idiomas e legendas         interface · áudio · legenda · metadados
 7  Aparência                  cor · vidro · fonte · animações · relógio
 8  TV ao vivo                 provedor · se o canal não abre
 9  Contas e serviços          conta e perfis · Trakt e Simkl · chaves · P2P
10  Desempenho desta TV        perfil pronto · resolução · imagens · diagnóstico
11  Sobre e ajuda              versão · atualizar · registros
```

Mapa (prefixo `AJ_` omitido; *itálico* = Avançado):

| Seção › bloco | Opções |
|---|---|
| 1 › Estilo | HOME_LAYOUT, LANDSCAPE, HERO, HERO_CHEIO, HERO_CATALOGOS, *HERO_TRANSICAO* |
| 1 › Fileiras | FIL_ORDEM, FIL_LIMITE, ITENS_FILEIRA, OCULTAR_NLANC, NOTAS_HOME, *ROTULOS, NOME_ADDON, SUFIXO_TIPO* |
| 1 › Continuar assistindo | CW_LIGADO, CW_OK, CW_ESTILO, CW_THUMB, CW_BLUR_PROX, CW_ORDEM, CW_NAO_EXIBIDOS, *CW_FURTHEST* |
| 1 › Barra lateral | RAIL_MODERNA, RAIL, MENU_EXPLORAR/GUIA/AGENDA/PERFIL (uma linha), *RAIL_BLUR, DESCOBRIR* |
| 1 › Escolha de perfil | PS_FUNDO |
| 2 › Cartaz em foco | EXPANDIR, EXPANDIR_ATRASO, BORDA_FOCO, *GRAD_CLASSICO* |
| 2 › Forma | LARGURA_DP, RAIO_DP, PROF, *PROF_BORDA, PROF_BRILHO, PROF_COBERTURA, PROF_POSTERS/CW/EPS/ELENCO/TRAILERS* |
| 2 › De onde vem a arte | HERO_FUNDO, HERO_ARTE_DIF, POSTER_PROV, *ADDON_POSTER/FUNDO/LOGO, COL_ARTE_CONTA, POSTER_INST/TOKEN/EXTRA/CHAVE/MODELO/TESTAR* |
| 3 › Onde toca | HERO_TRAILER, FOCO_TRAILER, DET_TRAILER_AUTO, DET_TRAILER |
| 3 › Som e imagem | HERO_TRAILER_SOM, DET_TRAILER_SOM, TRAILER_FONTE, *HERO_TRAILER_ESPERA, TRAILER_QUAL, TRAILER_ASPECTO* |
| 4 › O que aparece | DET_VEU, DET_DATA_CHEIA, DET_BLUR_NAO_VISTOS |
| 4 › Notas | NT_* (11, uma linha "Quais notas"), MDB_LIGADO, *MDB_TRAKT…MDB_MAL* |
| 4 › Dados do título | TMDB_LIGADO, *TMDB_ARTE…TMDB_CW (12), DET_META_EXT, DET_SO_CINEMETA* |
| 5 › Escolha da fonte | FONTE_MANUAL, FONTE_AUTO, FONTE_REPOR, *SELOS_CORES* |
| 5 › Imagem e som | QUALIDADE, DV, ATMOS |
| 5 › Durante e depois | PAUSA_OVERLAY, CW_CONCLUIDO |
| 6 | IDIOMA, AUD_LINGUA, LEG_LINGUA, TMDB_IDIOMA (+ atalho para o estilo da legenda do player) |
| 7 | TEMA, COR_LOGO, VIDRO, FONTE_UI, ANIM, RELOGIO, RELOGIO_POS, *VIDRO_CONTORNO* |
| 8 › Provedor | XTREAM_SERVIDOR/USUARIO/SENHA/CONTA/LIMPAR, STALKER_PORTAL/MAC/LIMPAR, EPG_PAIS |
| 8 › Se o canal não abre | LIVETV_DIAG, LIVETV_RES, LIVETV_ESPERA, *LIVETV_FORMATO, LIVETV_MODO, LIVETV_PROXY* |
| 9 › Conta e perfis | PERFIL_ATIVO, SYNC, ADDONS, ADDONS_PRINCIPAL, PERFIL_PESQ, PERFIL_EDITAR, SAIR |
| 9 › Trakt e Simkl | TRAKT, SIMKL, SALVOS_DEST, CW_FONTE |
| 9 › Chaves | DEBRID_RD/TB/PM/AD/AD_TESTAR, MDB_CHAVE, FANART_CHAVE (uma linha que abre a lista) |
| 9 › P2P | *P2P_LIGADO, P2P_URL, P2P_TESTAR* |
| 10 | (perfil pronto, novo), RESOLUCAO, QUALIDADE_IMG, GPU_EFEITOS (.tpk/Android), NAV_RAPIDA, DIAGNOSTICO, VELOCIDADE, ESPACO, *TEX_MB* |
| 11 | VERSAO_I, ATUALIZAR, ENVIAR_LOG, ENVIO_AUTO |

Outras regras da arquitetura:
- **Linhas de leitura saem da lista** (Versão, Perfil, Sincronização, Conta
  Xtream, Memória usada): viram o cabeçalho da seção. Resolve #71.
- **Linha que não se aplica some** em vez de ficar cinza (TV ao vivo sem
  provedor mostra só "Adicionar provedor"; Som do trailer na Samsung .wgt, que
  é sempre mudo, não aparece). O cinza com explicação de `inativa()` fica só
  para dependência que a pessoa pode resolver na mesma tela.
- **Duas linhas que se sobrepõem viram uma** com valores claros: "Priorizar
  metadados externos" + "Usar sempre o Cinemeta" → uma escolha de 3 valores
  (o mapeamento para as duas chaves continua por baixo, nada muda no arquivo).

### 4.2 A tela

```
┌──────────────┬──────────────────────────────────┬────────────────────────────┐
│ Buscar       │ Trailers                         │  ┌──────────────────────┐  │
│ O que mudou 3│                                  │  │  [home de verdade,   │  │
│──────────────│ ONDE TOCA                        │  │   cartaz em foco com │  │
│ Tela inicial │ No destaque do topo      Ligado ●│  │   o trailer no fundo]│  │
│ Cartazes     │▶No cartaz em foco      ◀ Ligado ▶│  └──────────────────────┘  │
│▶Trailers     │ Na página do título      Ligado  │  Parar o foco num cartaz   │
│ Página       │ Botão de trailer         Ligado  │  por 3 s toca o trailer    │
│ Reprodução   │                                  │  dele no fundo, sem som.   │
│ Idiomas      │ SOM E IMAGEM                     │                            │
│ Aparência    │ Som no destaque       Desligado  │  Desligado: o fundo fica   │
│ TV ao vivo   │ Fonte do trailer     Automático ›│  com a foto.               │
│ Contas       │                                  │  Precisa de: Expandir      │
│ Desempenho   │                                  │  cartaz ao focar  [ir]     │
│ Sobre        │ ○ Mostrar ajustes avançados      │  Vale: só nesta TV         │
│              │                                  │  Padrão: Desligado · você  │
│              │                                  │  mudou hoje   [Restaurar]  │
└──────────────┴──────────────────────────────────┴────────────────────────────┘
```

O painel da direita deixa de ser parágrafo e vira uma ficha fixa, sempre na
mesma ordem, cada parte curta:
1. **Prévia** (ver 4.3).
2. **Ligado:** o que a pessoa vê de diferente. **Desligado:** idem. Uma frase
   cada. Para escolhas, uma frase por valor.
3. **Precisa de / afeta:** a dependência, com atalho que leva à outra linha
   (hoje `inativa()` só diz em texto).
4. **Vale:** "só nesta TV" ou "na sua conta (também no app web e nas outras
   TVs)". Sai de `somenteDesteAparelho` + chave com/sem "-".
5. **Padrão e histórico:** valor de fábrica, se a pessoa mudou, quando, e de
   onde veio (esta TV / conta). Botão de restaurar a linha.

Escolha com mais de 2 valores (Layout da home, Fonte do trailer, Cor de
destaque, Idioma, Background do hero…): OK abre **lista vertical** com todos
os valores, cada um com sua frase e sua prévia; ↑/↓ troca a prévia, OK
confirma, Voltar desiste. ←/→ continua valendo para liga/desliga e números.
Hoje, ciclar com ←/→ mostra um valor por vez e esconde os outros.

### 4.3 Prévia ao vivo: três níveis, do mais honesto ao mais barato

1. **A própria tela (ajuste rápido).** Painel lateral de ~40% da largura por
   cima da tela onde a pessoa está (home, título, player), como os ajustes
   rápidos da webOS/Google TV. A tela de trás continua sendo desenhada e lê
   os `ajustes_*()` a cada quadro: mudar a borda, o raio, o vidro, a cor, os
   rótulos, o relógio, as notas, a profundidade aparece **na home dela, com os
   cartazes dela**. É a resposta direta ao "entender o que cada coisa faz".
   Precisa medir quais ajustes são lidos por quadro e quais exigem remontar
   (fileiras, itens, layout, idioma); estes vão para o nível 3.
2. **Miniatura com a arte de verdade** na tela cheia de Ajustes: o desenho
   de `desenhaPrevia` passa a usar texturas que já estão no `tex_cache` (os
   cartazes e o fundo do destaque atuais) em vez de retângulos cinza. Antes /
   depois lado a lado para os liga/desliga visuais.
3. **Experimentar com volta automática** para o que é pesado ou exige
   remontar (Layout da home, limite de fileiras, 4K, vidro, tema imersivo):
   aplica, volta para a home, mostra uma pílula "Manter · Voltar (15 s)" como
   a troca de resolução de um sistema operacional. Sem resposta, volta. Usa o
   diário do `seguro.c`, que já confirma/desfaz exatamente esses ajustes.

### 4.4 Busca nos ajustes

- O Spotlight ganha o grupo **Ajustes**: indexa rótulo, valores, a frase da
  ajuda e uma lista de sinônimos por linha (legenda/subtitle/CC, dublado,
  trailer, 4K/HDR/DV, lento/travando, barra/menu). No idioma da interface,
  pelo `i18n` que já existe.
- Resultado abre Ajustes com o foco na linha e o grupo aberto. Generaliza
  `ajustes_abrir_na_cor()` & cia. num `ajustes_abrir_em(op)`.
- "Buscar" é a primeira linha da coluna de categorias, e a tecla de busca/voz
  do controle dentro de Ajustes já abre filtrado.

### 4.5 "O que você mudou"

- Lista de tudo que difere do padrão de fábrica, agrupada por seção, com o
  valor de fábrica ao lado, de onde veio (esta TV / conta) e quando.
- Restaurar por linha e "Restaurar esta seção". Para chave da conta, o botão
  diz que volta também no app web (ver Riscos).
- Precisa de uma cópia constante dos padrões (`valor[]` é sobrescrito no
  arranque) e de gravar data/origem da última mudança por chave.
- Responde também ao "não confio que salvou" (#85, #129, #149): a pessoa vê
  a própria mudança registrada.

### 4.6 Perfis prontos (Desempenho desta TV)

Uma linha no topo da seção 10: **Leve / Equilibrado / Máximo / Personalizado**.

| | Leve | Equilibrado | Máximo |
|---|---|---|---|
| Vidro, tema imersivo | desligados | como a pessoa deixou | como a pessoa deixou |
| Profundidade, trailer no cartaz | desligados | desligados | ligados |
| Qualidade da imagem | Baixa | Padrão | Alta |
| Fileiras / itens | 7 / 12 | 7 / 12 | escolha da pessoa |
| Efeitos (.tpk) | Leves | Automático | Completos |
| Interface | 720p | 1080p | 4K se a TV aceita |

- Antes de aplicar, mostra a lista do que muda ("muda 6 ajustes: …"); depois,
  "Voltar ao que eu tinha" por um tempo (cópia dos valores anteriores).
- Mexer em qualquer linha da tabela depois passa o perfil a "Personalizado".
- Só toca ajustes **desta TV**; nenhum sobe para a conta (o perfil é sobre o
  aparelho, não o gosto).
- A recomendação usa o que o app já sabe: plataforma, RAM (`perfiltv.c`),
  nível de GPU medido (`gpunivel.c`), quedas recentes (`seguro.c`) e FPS da
  home (`[gpu-modos]`). Ex.: "Esta TV: webOS 4.5, 1 GB. Recomendado: Leve".

### 4.7 Assistente de primeira vez (5 perguntas, pulável)

Só em instalação nova sem ajustes da conta. Com conta que já traz ajustes, em
vez do assistente: "Trouxemos N ajustes da sua conta" com link para "O que
você mudou". Refazível em Ajustes › Sobre e ajuda.

1. Idioma da interface (já vem preenchido pelo automático; só confirma).
2. Áudio: original ou dublado? Legenda: nunca / só quando o áudio não é meu
   idioma / sempre, e em que idioma. (AUD_LINGUA, LEG_LINGUA)
3. Ao apertar Reproduzir: o app escolhe a melhor fonte / quero escolher
   sempre. (FONTE_MANUAL, FONTE_AUTO)
4. Visual da home: Moderna / Padrão / Dinâmica, com a prévia de cada.
5. Trailers sozinhos: sim / sem som / não. (HERO_TRAILER, DET_TRAILER_AUTO,
   FOCO_TRAILER, sons)

O desempenho não vira pergunta: o perfil da TV é escolhido sozinho e dito
no fim ("Esta TV ficou no Equilibrado; dá para trocar em Desempenho").

### 4.8 Atalhos contextuais

O ajuste aparece onde ele é sentido (lição da Netflix), sem roubar gesto que
já existe:
- **Segurar OK num cartaz da home**: o menu já tem "Estilo da fileira"; ganha
  "Ajustar a tela inicial…", que abre o ajuste rápido (4.3-1) com o bloco
  daquela superfície (fileira → Fileiras; Continuar assistindo → bloco dele;
  destaque → Estilo + Trailers).
- **Player**: o painel (Legendas / Áudio / Fontes…) ganha "Ajustes de
  reprodução" com qualidade, DV/Atmos, idioma padrão, estilo da legenda e
  "Escolher a fonte ao reproduzir". Responde ao #57 no lugar certo.
- **Página do título**: botão de opções com Trailers, Notas e Escurecimento.
- **Mensagens que hoje explicam sem levar**: fileira do Simkl vazia,
  trailer mudo na Samsung, canal que não abre, aviso de memória — cada uma
  com "Abrir o ajuste".
- Cartões de novidades continuam usando `ajustes_abrir_em(op)`.

### 4.9 Medir o uso (para não decidir no escuro de novo)

Uma linha de log por mudança: `[ajustes] mudou <chave> <antes> -> <depois>
via=<tela|rapido|busca|assistente|perfil|conta>`, sem nada da pessoa. Com
isso, em duas semanas se sabe o que é mudado, o que é achado pela busca e o
que ninguém toca (candidato a Avançado ou a sair).

---

## 5. Prioridade

**Fase 1 — maior ganho, menor custo (só `TELA[]`, `SECAO_AJUDA[]`, testes da
tela; nenhum enum, chave ou arquivo muda)**
1. Log de mudança (4.9). Uma linha em `ajustes.c`; mede tudo o que vem depois.
2. Nova árvore por intenção + "Mostrar ajustes avançados" (4.1).
3. Busca nos ajustes pelo Spotlight + `ajustes_abrir_em(op)` (4.4).
4. "O que você mudou" + restaurar por linha (4.5).
5. Painel da direita em ficha fixa (Ligado/Desligado, Precisa de, Vale onde,
   Padrão) — reescrita de texto, sem desenho novo (4.2).

**Fase 2 — o "entender" de verdade**
6. Ajuste rápido sobre a tela real + atalhos de segurar OK, player e título
   (4.3-1, 4.8).
7. Lista vertical de valores com frase e prévia por valor (4.2).
8. Perfis prontos com recomendação pela TV e "Experimentar · voltar em 15 s"
   via `seguro.c` (4.6, 4.3-3).

**Fase 3 — depois, com os números da Fase 1**
9. Prévias com a arte real (4.3-2).
10. Assistente de primeira vez (4.7).
11. Juntar/remover o que o log mostrar que ninguém usa.

---

## 6. Riscos (o que não pode quebrar)

- **Vetores paralelos posicionais.** `OPCOES[]`, `CHAVE[]` e `valor[]` são
  indexados pelo enum; opção nova só no fim (já houve #149 e o desalinhamento
  de 876742e). A reorganização da Fase 1 não precisa tocar o enum: só `TELA[]`.
- **Índices gravados.** `ajustes.txt` é por chave, mas grava o **índice** do
  valor: nenhum `V_*` pode ganhar valor no meio (ex.: `LING_OPC_ORIGINAL` no
  fim de propósito). A junção "metadados externos + Cinemeta" tem de ser só
  de tela, gravando as duas chaves como hoje.
- **Sync com a conta e o app web.** O blob usa os literais `W_*` e
  `ajustes_mesclar_blob` só reescreve chave que já existe. "Restaurar padrão"
  ou perfil pronto numa chave da conta **muda o app web e as outras TVs**:
  ou avisa, ou restaura para o valor da conta em vez do de fábrica. Tema
  dinâmico continua sem subir. Perfis prontos só em chaves de
  `somenteDesteAparelho`.
- **Perfis de pessoa.** `ajustes-p<N>.txt` guarda os ajustes por perfil;
  "O que você mudou" e o histórico (data/origem) têm de ser por perfil também,
  e o logout continua apagando tudo (`ajustes_perfil_esquecer`).
- **"Cada opção aparece uma vez em TELA".** `conferirTela()` e
  `tests/ajustes_secoes.sh` exigem isso. Mostrar o mesmo ajuste em dois
  lugares (ex.: idioma do áudio em Idiomas e no player) pede um tipo de item
  "atalho" que não conta como aparição, ou relaxar a regra com cuidado.
- **Testes que fixam a tela atual:** `ajustes_secoes`, `ajustes_shot`,
  `notas_ajustes_shot`, `ajustes_opcao_em_foco`/`ajustes_abrir_no_*` e os
  cartões de novidades que abrem em "Layout da home" e "Interface de vidro".
  Mudam junto, no mesmo commit.
- **Memória nas TVs de 1 GB.** Ajuste rápido sobre a tela real não pode
  segurar uma cópia extra da tela; o Spotlight já congela o fundo numa cópia
  (~8 MB a 1080p). Medir na C9 e numa Tizen 5 antes de ligar por padrão.
- **Persistência no Tizen .wgt.** Estado novo (histórico, perfil escolhido,
  assistente visto) tem de passar por `dados_gravar`, senão some como no #85.
- **Textos.** Toda frase nova passa pela tradução (30 idiomas de interface). Ficha curta
  e fixa ajuda: menos texto por linha para traduzir e para caber.
- **Modo seguro.** "Experimentar" e perfis prontos não podem disputar o
  diário de `seguro.c` (teto de 12 mudanças); a volta automática tem de ser a
  mesma peça, não uma segunda.
