# Pacotes locais 1.7.2

Não publicados. Artefatos em `/Volumes/ExternalSSD/nuvio-rel-172`; desenvolvimento continua em `/private/tmp/nv-172` (`release/1.7.2`). Cópias limpas de build no SSD externo. Nenhuma instalação ou abertura em TV nesta rodada.

## Candidata intermediária

Fonte `32e70e48b8325bf7fc7e7310273ffe446fd7faf2`. LG normal/highcache, WGT, quatro TPKs, dois núcleos de autoatualização, APK dualABI e HBrepo passaram na compilação e conferência. Preservados em `intermediaria-32e70e48/`. Esta candidata ainda não inclui a fila durável de addons após fechar/reabrir e não é o conjunto final.

Android: versão1.7.2/10702, certificado esperado e seis bibliotecas exigidas em cada ABI; configuração efetiva conferida sem expor valores. LG: versão1.7.2/ARM, libass e configuração efetiva presentes, ausência de arquivos pessoais, marcador300MB só no highcache; HBrepo confere hash/tamanho do normal. Samsung: manifesto1.7.2, pacote sem arquivos pessoais; núcleos anexos idênticos aos empacotados; Tizen4/5 semPT_TLS/relocaçõesTLS, comDT_HASH e semdrminfo; WGT comproveniência de configuração/WASM e compatibilidadeChrome69 conferidas pelo script.

## Temporários e espaço

A primeira compilação Samsung produziu os TPKs, mas a extração de conferência falhou por disco cheio. Não foi aprovada. No Mac desta rodada, `/usr/bin/mktemp` sem template escolheu o diretório temporário do sistema mesmo com `TMPDIR` no SSD (reproduzido em bash filho). Segunda execução forçou templates explícitos sob `TMPDIR` via função shell exportada e concluiu. A função não altera fonte nem identidade da build. A correção permanente agora usa templates explícitos em Android, TPK e conferência Samsung; a regressão de caminho temporário passou em um bash filho. ARM também usa template explícito. Correção comum em `fc711f6b`.

Builds antigos da worktree já publicada/mesclada `nv-166b` foram preservados em `/Volumes/ExternalSSD/nuvio-build-170-preservado`; `build/ass-wasm` permaneceu na origem para não quebrar referências. Builds descartáveis1.7.1/abrirrapido removidos conforme handoff. Gradle e temporários da build Android ficam no SSD; memória/swap do sistema ainda pressionavam o Mac na primeira rodada. O cache dos pacotes publicados da 1.7.1 foi transferido com conferência SHA256 para `/Volumes/ExternalSSD/nuvio-rel-171-cache`; os dez caminhos originais permanecem acessíveis por links válidos. Após a recuperação de espaço, o Mac tinha cerca de 10 GB disponíveis; essa disponibilidade é variável.

## Gates da release

Conferência de pacote não comprova controle remoto, reprodução, GPU, HDR nem latência nas TVs. Dono precisa validar C9/TCL/Samsung e os casos em VALIDACAO-LOCAL.md. Issues dependentes de fonte/configuração/aparelho não são declaradas resolvidas só por build. Nenhum push/tag/publicação/comentário autorizado nesta rodada.

## Conjunto final local

Geração a partir de `fc711f6b6231400416dadf102de99f5bcc8987d6`, incluindo a fila durável `d8402e54`. Worktrees limpas e separadas para Android, LG e Samsung. Destino: `/Volumes/ExternalSSD/nuvio-rel-172/final`; logs e relatórios ficam fora da pasta de anexos. Concluído: 13 anexos, com `SHA256SUMS` consolidado e conferido. Sem instalação nas TVs.

A verificação standalone TPK recebeu ainda `06d915f6`: `grep -q` encerrava cedo e SIGPIPE podia mascarar a presença de arquivo pessoal sob pipefail. Fixture com 12.000 entradas reproduziu retorno 141; leitura completa da listagem agora passa tanto no caso limpo quanto no contaminado. É uma correção da ferramenta de conferência, sem alterar o código compilado. Todos os ZIPs finais também foram conferidos por leitura completa no relatório Samsung.

### Resultado final

- Android: versão 1.7.2/10702, assinatura fixa, duas ABIs e seis bibliotecas em cada uma, configuração presente e nenhum arquivo pessoal.
- LG normal/highcache: versão 1.7.2 no manifesto e control, ARM32/libass, configuração presente; marcador de cache ampliado apenas no highcache. Metadados Homebrew conferem hash e tamanho do normal.
- Samsung: WGT e quatro TPKs 1.7.2; anexos de núcleo idênticos aos empacotados; Tizen 4/5 sem TLS e com DT_HASH. Script de release e conferência completa de ZIP/configuração/manifestações aprovados.
- Todas as worktrees de build ficaram limpas. Nenhum pacote foi instalado nem publicado nesta rodada.

Manifesto e relatórios privados apenas no disco local: `/Volumes/ExternalSSD/nuvio-rel-172/FINAL-MANIFEST.json`, `/Volumes/ExternalSSD/nv172-final-android-verification.json`, `/Volumes/ExternalSSD/nv172-final-lg-verification.json`, `/Volumes/ExternalSSD/nv172-final-samsung-verification.json`. Contêm identidade e resultados booleanos, sem valores de configuração.

| Anexo | Bytes | SHA256 |
|---|---:|---|
| `Nuvio-1.7.2-NuvioTpk.tpk` | 27489219 | `b7b46caf6e9317b0c8ad4b2b6d23bd48fbfcb07f651ae70146ecd031cab82e8f` |
| `Nuvio-1.7.2-NuvioTpk40.tpk` | 27493537 | `23269e47f5c360d6b7c1138ff8d13103a9986adc19cc4102e6921b0e349bdcc9` |
| `Nuvio-1.7.2-NuvioTpk60.tpk` | 27488264 | `8d10d35ca9c1dabc4f8b59b8dc1d305db00447315a64a374492f4b1517df6578` |
| `Nuvio-1.7.2-NuvioTpk65.tpk` | 27488447 | `638e01fe17afa8ed0a78f16d37b5b66b956b6658d06704d82c3167189a9c222b` |
| `Nuvio-1.7.2-android.apk` | 43557763 | `04510610fb86363b52fc707ce4b7db7eccd6e81802c75913cacef3fbb3c7a4df` |
| `NuvioTV-1.7.2-tizen.wgt` | 27096640 | `8060de75819de9c039e2c8d25bdf4834af2fd3097eea3e3ddf0e252f940d80b2` |
| `libnuvio-1.7.2-tpk-arm.so` | 9862820 | `9f8704f7c7426f39f1af9a6c450ade3d291909c647f1c684249613d6b7ae1222` |
| `libnuvio-1.7.2-tpk40-arm.so` | 9862804 | `e71fbf6149471ff2f95e856ccf5d820a8d58f7aa65d56e78c22660c9c179cb16` |
| `repo.json` | 1310 | `137f217d698e09bdaa3fd4d964400ce1093f75b49e8940bd79b78ba09ee85b5c` |
| `space.nuvio.native.legacy_1.7.2_arm-highcache.ipk` | 51056930 | `3acc12cfcf0a6952a32cd82f3df1d29a047f2652f4cc28b985903fcba9b2085b` |
| `space.nuvio.native.legacy_1.7.2_arm.ipk` | 51056632 | `e994e90fd260e9418f7b341178a68dfc994b8305b9ab05b5d6902308802942ff` |
| `webosbrew.manifest.json` | 633 | `33b6687ed9a63812ce8fd2471bafc5e1460c7fee4230c71491949b772d75145e` |

O décimo terceiro anexo é `SHA256SUMS`; a conferência de todos os 12 arquivos listados nele passou. Logs de build ficam fora de `final/`.
