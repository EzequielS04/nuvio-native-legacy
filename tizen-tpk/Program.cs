// Host .NET do Nuvio .tpk no Tizen 6+. Nao tem tela propria: abre um GLWindow
// de tela cheia e, a cada quadro, entrega o contexto GL ao C (libnuvio.so,
// src/tpk.c), que roda o app inteiro num fio seu. Teclas do controle vao pelo
// nome (XF86Back, Up, ...) e o C traduz para SDL. O player esta em Video.cs.
//
// Por que GLWindow e nao GLView: GLView so existe a partir da API11 e o alvo
// comeca na API8 (Tizen 6.0). O spike mediu GLWindow + .so a ~45 fps no Tizen 6.
//
// A janela padrao do NUI (Window.Instance) fica transparente por baixo do
// GLWindow: e nela que o player prende o video.
#pragma warning disable CS0618
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using Tizen.Multimedia;
using Tizen.NUI;
using IOPath = System.IO.Path;
using NuiWindow = Tizen.NUI.Window;
using NuiRect = Tizen.NUI.Rectangle;
using NuiColor = Tizen.NUI.Color;
using NuiTimer = Tizen.NUI.Timer;

namespace NuvioTpk
{
    class Program : NUIApplication
    {
        [DllImport("libnuvio.so")] static extern int nv_tpk_iniciar(string arte, string dados, int w, int h);
        [DllImport("libnuvio.so")] static extern int nv_tpk_quadro();
        [DllImport("libnuvio.so")] static extern void nv_tpk_config(int esperaMs, int swapZero);
        [DllImport("libnuvio.so")] static extern void nv_tpk_tecla(string nome, int apertou);
        [DllImport("libdl.so.2")] static extern IntPtr dlopen(string path, int flags);
        [DllImport("libdl.so.2")] static extern IntPtr dlerror();

        const int W = 1920, H = 1080;

        GLWindow gl;
        volatile bool fim;
        NuiTimer vigia;
        Video video;

        protected override void OnCreate()
        {
            base.OnCreate();
            var principal = SynchronizationContext.Current;
            NuiWindow.Instance.BackgroundColor = NuiColor.Transparent;

            string dados = DirectoryInfo.Data;
            string arte = IOPath.Combine(DirectoryInfo.Resource, "art");

            // A .so aberta por caminho absoluto ANTES do primeiro DllImport, como
            // no pacote do Tizen 4/5: se o launcher desta TV nao procurar no lib/
            // do pacote (4/5 nao procurava; no 6.5 ninguem mediu, #170), o
            // DllImport("libnuvio.so") casa pelo soname com a ja carregada.
            string so = IOPath.Combine(IOPath.GetFullPath(IOPath.Combine(DirectoryInfo.Resource, "..")), "lib", "libnuvio.so");
            dlerror();
            if (dlopen(so, 2 | 0x100) == IntPtr.Zero)
            {
                string e = Marshal.PtrToStringAnsi(dlerror());
                Erro("The TV did not let Nuvio load its native library.", so + ": " + (string.IsNullOrEmpty(e) ? "refused without a message" : e));
                return;
            }
            try
            {
                video = new Video(() => new Display(NuiWindow.Instance),
                                  a => { if (principal != null) principal.Post(_ => a(), null); else a(); },
                                  dados, W, H);
            }
            catch (Exception e) { Erro("Nuvio could not start the player.", e.GetType().Name + ": " + e.Message); return; }

            // API8: o callback roda no fio principal e o DALi troca os buffers
            // mesmo em quadro pulado, entao espera o app terminar o quadro.
            // API9+: fio de render proprio, respeita o 0 de "pular", e o swap
            // com vsync e que derrubava o Tizen 9 a 3 fps no spike.
            try
            {
#if NV_API8
                nv_tpk_config(-1, 0);
#else
                nv_tpk_config(50, 1);
#endif
                if (nv_tpk_iniciar(arte, dados, W, H) != 0) throw new Exception("nv_tpk_iniciar failed");
            }
            catch (Exception e)
            {
                try { File.WriteAllText(IOPath.Combine(dados, "tpk-erro.txt"), e.ToString()); } catch { }
                Erro("Nuvio could not start.", e.GetType().Name + ": " + e.Message);
                return;
            }

            try
            {
                CriaJanelaGL();
            }
            catch (Exception e) { Erro("Nuvio could not open its window.", e.GetType().Name + ": " + e.Message); return; }
        }

        bool tvLogada;
        int tiques;

        void LogaTv()
        {
            string versao = null, modelo = null;
            try { Tizen.System.Information.TryGetValue<string>("http://tizen.org/feature/platform.version", out versao); } catch { }
            try { Tizen.System.Information.TryGetValue<string>("http://tizen.org/system/model_name", out modelo); } catch { }
#if NV_API8
            const string host = "api8";
#elif NV_API9
            const string host = "api9";
#else
            const string host = "api11";
#endif
            video.Log("[tv] modelo=" + (modelo ?? "?") + " tizen=" + (versao ?? "?") + " host=" + host +
                      " dotnet=" + RuntimeInformation.FrameworkDescription + " tela=" + W + "x" + H);
        }

        void CriaJanelaGL()
        {
            gl = new GLWindow("nuvio", new NuiRect(0, 0, W, H), true);
#if NV_API8
            gl.SetEglConfig(false, false, 0, GLWindow.GLESVersion.Version_2_0);
            gl.RegisterGlCallback(() => { }, () => { Quadro(); }, () => { });
#elif NV_API9
            gl.SetEglConfig(false, false, 0, GLESVersion.Version20);
            gl.RegisterGlCallback(() => { }, () => Quadro(), () => { });
            gl.RenderingMode = GLRenderingMode.Continuous;
#else
            gl.SetGraphicsConfig(false, false, 0, GLESVersion.Version20);
            gl.RegisterGLCallbacks(() => { }, () => Quadro(), () => { });
            gl.RenderingMode = GLRenderingMode.Continuous;
#endif
            // As duas janelas repassam tecla: qual delas fica com o foco depende
            // do firmware, e so uma recebe de cada vez.
            gl.KeyEvent += (s, e) => Tecla(e.Key);
            NuiWindow.Instance.KeyEvent += (s, e) => Tecla(e.Key);
            gl.Show();

            // Exit() tem de sair do fio principal, e Quadro() roda no de desenho.
            vigia = new NuiTimer(250);
            vigia.Tick += (s, e) =>
            {
                if (fim) { video.Parar(); Exit(); return false; }
                video.Tique();
                // ~3 s depois de abrir, quando o main() do app ja redirecionou
                // o stdout para o nuvio.log: uma linha dizendo que TV e esta,
                // para o D1 separar os relatos por versao da Tizen.
                if (!tvLogada && ++tiques >= 12) { tvLogada = true; LogaTv(); }
                return true;
            };
            vigia.Start();
        }

        // Em vez de fechar em silencio: o motivo na tela, com a TV, para foto.
        void Erro(string titulo, string detalhe)
        {
            // Sem `fim`: o relogio (se ja existir) fecharia o app antes da foto.
            if (gl != null) { try { gl.Hide(); } catch { } }
            var w = NuiWindow.Instance;
            w.BackgroundColor = NuiColor.Black;
            string versao = "?", modelo = "?";
            try { Tizen.System.Information.TryGetValue<string>("http://tizen.org/feature/platform.version", out versao); } catch { }
            try { Tizen.System.Information.TryGetValue<string>("http://tizen.org/system/model_name", out modelo); } catch { }
            var t = new Tizen.NUI.BaseComponents.TextLabel
            {
                Text = titulo + "\n\n" + detalhe + "\n\nTV " + modelo + " / Tizen " + versao + " / " + RuntimeInformation.FrameworkDescription +
                       "\n\nPlease post a PHOTO of this screen in issue #137 on GitHub (iqui27/nuvio-native-legacy). Back closes.",
                MultiLine = true, TextColor = NuiColor.White, PointSize = 20,
                Size2D = new Size2D(W - 160, H - 160), Position2D = new Position2D(80, 80),
            };
            w.Add(t);
            w.KeyEvent += (s, e) => { if (e.Key.State == Key.StateType.Down && (e.Key.KeyPressedName == "XF86Back" || e.Key.KeyPressedName == "Escape")) Exit(); };
        }

        void Tecla(Key k)
        {
            nv_tpk_tecla(k.KeyPressedName, k.State == Key.StateType.Down ? 1 : 0);
        }

        // Fio de desenho do NUI. 1 = troca, 0 = pula, -1 = o app acabou.
        int Quadro()
        {
            if (fim) return 0;
            int r = nv_tpk_quadro();
            if (r < 0) fim = true;
            return r > 0 ? 1 : 0;
        }

        protected override void OnPause()
        {
            video?.PausarPeloSistema();
            base.OnPause();
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
