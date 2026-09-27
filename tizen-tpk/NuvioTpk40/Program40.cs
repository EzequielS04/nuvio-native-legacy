// Host do Nuvio para Tizen 4.0/5.0 (TVs 2018-2019).
//
// Essas APIs nao tem GLWindow. O jeito da Samsung e a TVGLApplication
// (pacote Tizen.NET.TV): ela cria a janela GL e chama OnUpdate a cada quadro
// com o contexto corrente; devolvendo true, ela mesma troca os buffers. E o
// molde do JuvoPlayer.OpenGL (github.com/SamsungDForum/JuvoPlayer), inclusive
// no video: uma janela ElmSharp separada, mostrada e rebaixada (Lower), so
// para o player — o GL fica por cima e o C abre o furo onde o video aparece.
// Dai para frente e o mesmo protocolo do Tizen 6+ (src/tpk.c, Video.cs).
//
// Se a TV nao deixar a .so carregar, a tela diz o motivo com o erro real. Com
// assinatura Public a TV recusou (spike.2, cara de UEP, a checagem de
// assinatura de executavel da Samsung); o manifesto declara um privilegio
// Partner para o Apps2Samsung assinar como Partner.
//
// O dotnet-launcher do Tizen 4/5 nao procura DllImport no lib/ do pacote: a
// .so e aberta por caminho absoluto primeiro, e o DllImport("libnuvio.so")
// casa pelo soname. dlerror e chamado uma vez antes, porque o CLR resolve o
// P/Invoke na primeira chamada e isso zerava a mensagem do dlopen.
using System;
using System.IO;
using System.Runtime.InteropServices;
using ElmSharp;
using Tizen.System;
using Tizen.TV.NUI.GLApplication;
using GLKey = Tizen.TV.NUI.GLApplication.Key;

namespace NuvioTpk
{
    class Program : TVGLApplication
    {
        [DllImport("libdl.so.2")] static extern IntPtr dlopen(string path, int flags);
        [DllImport("libdl.so.2")] static extern IntPtr dlerror();
        [DllImport("libnuvio.so")] static extern int nv_tpk_iniciar(string arte, string dados, int w, int h);
        [DllImport("libnuvio.so")] static extern void nv_tpk_config(int esperaMs, int swapZero);
        [DllImport("libnuvio.so")] static extern int nv_tpk_quadro();
        [DllImport("libnuvio.so")] static extern IntPtr nv_tpk_erro();
        [DllImport("libnuvio.so")] static extern void nv_tpk_tecla(string nome, int apertou);
        const int RTLD_NOW = 2, RTLD_GLOBAL = 0x100;
        const int W = 1920, H = 1080;

        Window janelaVideo, janelaErro;
        Video video;
        bool rodando, fim;

        protected override void OnCreate()
        {
            base.OnCreate();
            var dir = Tizen.Applications.Application.Current.DirectoryInfo;
            string dados = dir.Data;
            string raiz = Path.GetFullPath(Path.Combine(dir.Resource, ".."));
            string so = Path.Combine(raiz, "lib", "libnuvio.so");
            string falha = Carrega(so);
            if (falha != null)
            {
                // Mesma .so em data/: se o bloqueio for so a pasta do pacote, passa.
                string copia = Path.Combine(dados, "libnuvio.so");
                string falha2;
                try { File.Copy(so, copia, true); falha2 = Carrega(copia); }
                catch (Exception e) { falha2 = "copia falhou: " + e.Message; }
                if (falha2 != null)
                {
                    Erro("A TV nao deixou o Nuvio nativo carregar.",
                         "pasta do app: " + falha, "pasta de dados: " + falha2, "montagem: " + Montagem(raiz));
                    return;
                }
            }

            try
            {
                // Janela do video, atras da janela GL (JuvoPlayer.OpenGL faz igual).
                janelaVideo = new Window("NuvioVideo") { Geometry = new Rect(0, 0, W, H) };
                janelaVideo.Show();
                janelaVideo.Lower();
                video = new Video(() => new Tizen.Multimedia.Display(janelaVideo), a => EcoreMainloop.PostAndWakeUp(a), dados, W, H);

                // OnUpdate roda no fio principal: nunca segura mais que 50 ms, e
                // devolver false pula a troca de buffers.
                nv_tpk_config(50, 0);
                if (nv_tpk_iniciar(Path.Combine(dir.Resource, "art"), dados, W, H) != 0)
                {
                    Erro("O Nuvio nao conseguiu iniciar.", Marshal.PtrToStringAnsi(nv_tpk_erro()) ?? "nv_tpk_iniciar falhou");
                    return;
                }
            }
            catch (Exception e)
            {
                Erro("O Nuvio nao conseguiu iniciar.", e.GetType().Name + ": " + e.Message);
                return;
            }
            rodando = true;
            EcoreMainloop.AddTimer(0.25, () =>
            {
                if (!fim) { video.Tique(); return true; }
                video.Parar();
                string motivo = Marshal.PtrToStringAnsi(nv_tpk_erro());
                if (string.IsNullOrEmpty(motivo)) Exit();
                else Erro("O Nuvio abriu, mas nao conseguiu desenhar na tela.", motivo,
                          "log: " + Path.Combine(dados, "nuvio.log"));
                return false;
            });
        }

        // Fio principal, contexto GL corrente. true = a TVGLApplication troca
        // os buffers; false = nada novo neste quadro.
        protected override bool OnUpdate()
        {
            if (!rodando || fim) return false;
            int r = nv_tpk_quadro();
            if (r < 0) fim = true;
            return r > 0;
        }

        protected override void OnKeyEvent(GLKey k)
        {
            if (janelaErro != null)
            {
                if (k.State == GLKey.StateType.Down && (k.KeyPressedName == "XF86Back" || k.KeyPressedName == "Escape")) Exit();
                return;
            }
            if (rodando) nv_tpk_tecla(k.KeyPressedName, k.State == GLKey.StateType.Down ? 1 : 0);
        }

        protected override void OnPause()
        {
            video?.PausarPeloSistema();
            base.OnPause();
        }

        static string Carrega(string caminho)
        {
            dlerror();
            if (dlopen(caminho, RTLD_NOW | RTLD_GLOBAL) != IntPtr.Zero) return null;
            string e = Marshal.PtrToStringAnsi(dlerror());
            return string.IsNullOrEmpty(e) ? "recusado sem mensagem" : e;
        }

        static string Montagem(string raiz)
        {
            try
            {
                string melhor = "", linha = "?";
                foreach (var l in File.ReadAllLines("/proc/self/mounts"))
                {
                    var p = l.Split(' ');
                    if (p.Length > 3 && raiz.StartsWith(p[1]) && p[1].Length > melhor.Length) { melhor = p[1]; linha = p[1] + " " + p[3]; }
                }
                return linha;
            }
            catch (Exception e) { return "? (" + e.Message + ")"; }
        }

        // Tela de erro numa janela ElmSharp por cima de tudo: o motivo, a TV e
        // o pedido de foto.
        void Erro(string titulo, params string[] detalhes)
        {
            rodando = false;
            janelaErro = new Window("NuvioErro") { Geometry = new Rect(0, 0, W, H) };
            janelaErro.BackButtonPressed += (s, e) => Exit();
            var fundo = new Background(janelaErro) { Color = Color.Black };
            fundo.Show();
            janelaErro.AddResizeObject(fundo);
            var coluna = new Box(janelaErro) { AlignmentX = -1, AlignmentY = 0, WeightX = 1, WeightY = 1 };
            coluna.Show();
            janelaErro.AddResizeObject(coluna);
            Information.TryGetValue<string>("http://tizen.org/feature/platform.version", out string versao);
            Information.TryGetValue<string>("http://tizen.org/system/model_name", out string modelo);
            Linha(coluna, titulo, 36);
            foreach (var d in detalhes) Linha(coluna, d, 26);
            Linha(coluna, $"TV {modelo ?? "?"} / Tizen {versao ?? "?"} / {RuntimeInformation.FrameworkDescription}", 26);
            Linha(coluna, "Mande uma FOTO desta tela na issue #137 do GitHub (iqui27/nuvio-native-legacy). Voltar sai.", 26);
            janelaErro.Show();
        }

        void Linha(Box coluna, string texto, int tam)
        {
            var l = new Label(janelaErro)
            {
                Text = $"<font_size={tam} color=#FFFFFF>" + texto.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;") + "</font>",
                AlignmentX = -1, WeightX = 1, LineWrapType = WrapType.Word
            };
            l.Show();
            coluna.PackEnd(l);
        }

        protected override void OnTerminate()
        {
            video?.Parar();
            base.OnTerminate();
        }

        static void Main(string[] args)
        {
            new Program().Run(args);
        }
    }
}
