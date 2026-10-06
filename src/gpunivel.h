// NIVEL DE GPU do .tpk da Samsung: o que a GPU da TV aguenta desenhar.
//
// POR QUE EXISTE (registro D1 8825, Tizen 5.0 de 2019, 1108 MB, "Mali-TDVX |
// OpenGL ES 3.2 r11p0", 1920x1080): tela vazia a 60 fps; com a home cheia,
// 22-29 fps e ~45 trancos por janela. O pior quadro reparte assim:
//   [quadro] pior=68ms | ev=0 bomb=0 upd=0.2 clr=62.0 des=5.1 swap=0.8
// A CPU submete o quadro em 2-5 ms e o tempo vai todo no glClear: e o primeiro
// toque GL no buffer seguinte, onde o driver espera a GPU liberar um buffer da
// janela. HIPOTESE, nao prova: a GPU nao termina o quadro anterior a tempo
// (preenchimento/ALU por pixel). O mesmo app na Tizen 6 roda a 60 fps.
//
// QUATRO NIVEIS, um degrau por vez:
//   0 = como sempre (1080p nativo, efeitos cheios).
//   1 = EFEITOS LEVES (gfx_definir_efeitos_leves): dither Bayer barato no lugar do highp dos
//       degrades e sem os realces decorativos (brilho no alto do card, luz de
//       canto, a luz do chao da escolha de perfil). Resolucao nativa.
//   2 = EFEITOS MINIMOS (gfx_definir_efeitos_minimos): nivel 1 + sem a luz de
//       tela cheia do tema imersivo e sem sombra/halo. Resolucao nativa. So
//       quando o 1 ainda fica abaixo de 25 fps (Mali-400, registro 9859).
//   3 = nivel 2 + DESENHO INTERNO EM 1280x720, ampliado para a janela numa
//       unica passada (GFX_COPIA, filtro linear). 2,25x menos pixels por
//       quadro. SO FORCADO: nas Tizen 5.0 o texto ficou borrado demais.
//
// COMO O NIVEL E ESCOLHIDO (so no .tpk, NV_TPK):
//   - -DNV_TPK_NIVEL_FORCADO=N (tools/tpk.sh com NV_TPK_NIVEL=N): fixo, sem
//     medir. E o canario de comparacao; a release nao usa.
//   - senao ADAPTATIVO: nos primeiros ~20 s de home cheia mede FPS e quanto do
//     quadro e ESPERA (clr+swap) contra CPU (ev+bomb+upd+des). FPS < 45 com a
//     espera dominando = GPU presa: desce UM nivel, espera assentar, mede de
//     novo. FPS bom = fica. CPU dominando = fica (menos pixel nao ajuda). Nunca
//     sobe sozinho. O nivel alcancado fica em <dados>/gpu-nivel.txt com a
//     chave GPU+driver+modelo+versao da Tizen, e o proximo arranque COMECA nele
//     (e continua medindo: so desce). Chave diferente (firmware novo, outra TV)
//     = recomeca do 0.
//   - fora do .tpk: nivel 0 sempre (LG e .wgt nao mudam). No Mac,
//     NUVIO_GPU_NIVEL=N no ambiente forca o nivel, para ver o resultado.
//
// GLES3 / EXTENSOES: o host cria contexto GLES 2.0 (Program.cs Version20,
// Program40.cs TVGLApplication), mas o driver pode entregar um contexto maior
// (a Mali do 8825 responde "OpenGL ES 3.2"). So se usa glInvalidateFramebuffer
// se GL_VERSION disser "OpenGL ES 3" E eglGetProcAddress devolver a funcao;
// senao glDiscardFramebufferEXT se GL_EXT_discard_framebuffer estiver na
// lista; senao nada (o caminho GLES2 de sempre). Onde: antes de sobrescrever
// um alvo inteiro (ampliacao do nivel 2, passadas do desfoque) e, no fim do
// quadro, profundidade/stencil da janela quando ela os tem.
#ifndef NV_GPUNIVEL_H
#define NV_GPUNIVEL_H

// Depois do contexto GL, ANTES de gfx_iniciar e de tex_iniciar: le GL_*,
// decide o nivel inicial e marca a GPU fraca no perfil (perfiltv.h).
// `w`,`h` = drawable da janela.
void gpun_iniciar(int w, int h);

// Uma linha no log: [perfil] tpk mem=... gpu=... -> tex=... (so no .tpk).
void gpun_log_perfil(long memMB, int texMb, int fios, int heroi);

int  gpun_nivel(void);
// Troca o nivel na hora, sem medir nem gravar (captura de teste).
void gpun_definir_nivel(int n);
// Ajuste "Efeitos visuais" do .tpk: 0 automatico, 1 completos, 2 leves.
void gpun_preferencia(int p);

// Ajuste "Resolucao da interface = 720p": fixa o nivel 3 (desenho interno em
// 1280x720) e desliga a medida e o "Efeitos visuais" ate o fim da sessao.
void gpun_forcar_720(void);

// Laco de quadro: _inicio ANTES do glClear da tela (liga o alvo interno no
// nivel 2), _fim depois do ultimo desenho e antes do swap (amplia e descarta).
void gpun_quadro_inicio(void);
void gpun_quadro_fim(void);

// Uma medida por quadro, com as fases do quadro ANTERIOR (main.c). `naHome`
// = a home e a tela de frente, sem detalhe nem player; `cheia` = com artes na
// tela (a home vazia roda a 60 e nao diz nada).
void gpun_medir(double dtms, double espera, double cpu, int naHome, int cheia);

// Descarta a COR do alvo ligado agora, que vai ser sobrescrito inteiro.
// `padrao` = 1 se o ligado e a janela (FBO 0). No-op sem ES3/extensao.
void gpun_descartar_cor(int padrao);
#endif
