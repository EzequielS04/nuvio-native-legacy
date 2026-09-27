# Fontes distribuídas no Nuvio

Inter Display continua sendo o padrão da interface. As famílias adicionais são Montserrat, Roboto e Atkinson Hyperlegible Next. A seleção da interface e a seleção das legendas são independentes. A primeira fica em Aparência → Fonte da interface; a segunda permanece no painel de estilo da legenda do player. Legendas ASS renderizadas pelo libass mantêm a tipografia definida no arquivo.

Montserrat é uma alternativa de estética geométrica para a proposta visual solicitada. Não é Netflix Sans e não é apresentada como fonte oficial da Netflix.

Os arquivos adicionais são estáticos, fornecidos pelos autores, nos pesos Regular (400), Medium (500) e Bold (700). Nenhum peso foi simulado ou gerado nesta inclusão. As licenças SIL OFL 1.1 acompanham cada família em `deploy/app/fonts/*-OFL.txt`.

## Procedência

- [JulietaUla/Montserrat](https://github.com/JulietaUla/Montserrat/tree/555facfb2a18c72c3c0380f0d9c0f060453a9058) — commit `555facfb2a18c72c3c0380f0d9c0f060453a9058`.
- [googlefonts/atkinson-hyperlegible-next](https://github.com/googlefonts/atkinson-hyperlegible-next/tree/7925f50f649b3813257faf2f4c0b381011f434f1) — commit `7925f50f649b3813257faf2f4c0b381011f434f1`.
- [Roboto v3.016](https://github.com/googlefonts/roboto-3-classic/releases/tag/v3.016) — arquivos `hinted/static/` do pacote oficial.
- [Atkinson Hyperlegible, Braille Institute](https://www.brailleinstitute.org/freefont/) — apresentação da família pelos responsáveis.

## Integridade

| Arquivo | Bytes | SHA-256 |
|---|---:|---|
| `AtkinsonHyperlegibleNext-Bold.ttf` | 66792 | `994414047df66bb4998d01c1cb1eeb4a2ddc4622d1aa56bbb8adbeca7645b041` |
| `AtkinsonHyperlegibleNext-Medium.ttf` | 67080 | `dd50b08b3c560846097d23baaaf6a97ffa20dd077115d23c59df68083b9ea05e` |
| `AtkinsonHyperlegibleNext-Regular.ttf` | 65068 | `88ed5c31a71584c7772963b02d04bef1eb7e3d2e9c8b9cb204339b1f82cf432c` |
| `Montserrat-Bold.ttf` | 454864 | `bc6e854971cea46b463be6f9eef4d9cd52f51cfc1fc0dd90c9d3e6483dc0ec61` |
| `Montserrat-Medium.ttf` | 447320 | `dae47428bb041f9716604e0e07b5b0c8585b3bdd8183362f75c69fe7bb3cfaf4` |
| `Montserrat-Regular.ttf` | 445928 | `3e8abe50c44c82e2242e97d1ec8c0d385c4890cdc50447bcdb8605c81a38cfb2` |
| `Roboto-Bold.ttf` | 465944 | `44f13e570a8b3fc42240abfcb3d72fe298f3dd6a25db0c78d0c2495dbd0e9121` |
| `Roboto-Medium.ttf` | 464100 | `80ce163ec5ecd91a883aa20e80d31dc74971b55cf658dd7c5e8d21f4e9fcb417` |
| `Roboto-Regular.ttf` | 463712 | `9f300202f482ad59f8b13bc3131a295744d14fd530fa3765a0a11ec87264d203` |

Total dos nove arquivos adicionais: 2.940.808 bytes.

## Empacotamento

Os pipelines existentes incluem a pasta `deploy/app/fonts` no pacote nativo e no preload WebAssembly. As licenças devem acompanhar os arquivos de fonte. A inclusão dos arquivos no repositório não comprova instalação nem comportamento na TV.
