// Host .NET do Nuvio .tpk. Nao tem tela propria: abre um GLWindow de tela
// cheia e, a cada quadro, entrega o contexto GL ao C (libnuvio.so, src/tpk.c),
// que roda o app inteiro num fio seu. Teclas do controle vao pelo nome
// (XF86Back, Up, ...) e o C traduz para SDL.
//
// Por que GLWindow e nao GLView: GLView so existe a partir da API11 e o alvo
// comeca na API8 (Tizen 6.0). O spike mediu GLWindow + .so a ~45 fps no Tizen 6.
//
// VIDEO: Tizen.Multimedia.Player no plano de video da TV, preso a janela padrao
// do NUI (Window.Instance), que fica transparente por baixo do GLWindow. O
// GLWindow e translucido e o C abre o furo (gfx_furo) onde o video aparece,
// igual a LG. O C pede pelas funcoes registradas em nv_tpk_video_registrar e
// ouve pelo nv_tpk_video_evento (src/video_tpk.c).
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
        [DllImport("libnuvio.so")] static extern void nv_tpk_tecla(string nome, int apertou);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_registrar(IntPtr abrir, IntPtr parar, IntPtr pausar,
                                                                              IntPtr buscar, IntPtr volume, IntPtr janela, IntPtr pos);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_evento(int tipo, int a, int b);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnAbrir(IntPtr url, IntPtr cabecalhos);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnSemArg();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnInt(int v);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnRet(int x, int y, int w, int h);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate int FnPos();

        const int EV_PRONTO = 1, EV_TOCANDO = 2, EV_PAUSADO = 3, EV_FIM = 4, EV_ERRO = 5, EV_TAMANHO = 6, EV_BUFFER = 7;
        const int W = 1920, H = 1080;

        GLWindow gl;
        volatile bool fim;
        NuiTimer vigia;
        SynchronizationContext principal;
        // Referencias vivas: o C guarda os ponteiros, o GC nao pode recolher.
        FnAbrir fAbrir; FnSemArg fParar; FnInt fPausar, fBuscar, fVolume; FnRet fJanela; FnPos fPos;

        Player player;
        int sessao;
        volatile int posMs;
        string dados;

        protected override void OnCreate()
        {
            base.OnCreate();
            principal = SynchronizationContext.Current;
            NuiWindow.Instance.BackgroundColor = NuiColor.Transparent;

            dados = DirectoryInfo.Data;
            string arte = IOPath.Combine(DirectoryInfo.Resource, "art");

            fAbrir = (u, c) => { string url = Marshal.PtrToStringAnsi(u), cab = Marshal.PtrToStringAnsi(c); Principal(() => Abrir(url, cab)); };
            fParar = () => Principal(Parar);
            fPausar = p => Principal(() => Pausar(p != 0));
            fBuscar = ms => Principal(() => Buscar(ms));
            fVolume = v => Principal(() => { if (player != null) player.Volume = Math.Max(0, Math.Min(100, v)) / 100f; });
            fJanela = (x, y, w, h) => Principal(() => Janela(x, y, w, h));
            fPos = () => posMs;
            nv_tpk_video_registrar(Marshal.GetFunctionPointerForDelegate(fAbrir), Marshal.GetFunctionPointerForDelegate(fParar),
                                   Marshal.GetFunctionPointerForDelegate(fPausar), Marshal.GetFunctionPointerForDelegate(fBuscar),
                                   Marshal.GetFunctionPointerForDelegate(fVolume), Marshal.GetFunctionPointerForDelegate(fJanela),
                                   Marshal.GetFunctionPointerForDelegate(fPos));

            if (nv_tpk_iniciar(arte, dados, W, H) != 0)
            {
                File.WriteAllText(IOPath.Combine(dados, "tpk-erro.txt"), "nv_tpk_iniciar falhou");
                Exit();
                return;
            }

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

            // Exit() tem de sair do fio principal, e Quadro() roda no de
            // desenho. O mesmo relogio atualiza a posicao do video, que o C le
            // do fio dele sem esperar ninguem.
            vigia = new NuiTimer(250);
            vigia.Tick += (s, e) =>
            {
                if (fim) { Parar(); Exit(); return false; }
                try { if (player != null && player.State == PlayerState.Playing) posMs = player.GetPlayPosition(); } catch { }
                return true;
            };
            vigia.Start();
        }

        void Tecla(Key k)
        {
            nv_tpk_tecla(k.KeyPressedName, k.State == Key.StateType.Down ? 1 : 0);
        }

        void Principal(Action a)
        {
            if (principal != null) principal.Post(_ => { try { a(); } catch (Exception e) { Log("principal: " + e); } }, null);
            else a();
        }

        void Log(string s)
        {
            try { File.AppendAllText(IOPath.Combine(dados, "tpk-host.log"), DateTime.Now.ToString("HH:mm:ss ") + s + "\n"); } catch { }
        }

        // Fio de desenho do NUI. 0 do C = o app pediu para sair (Voltar na home).
        int Quadro()
        {
            if (fim) return 0;
            if (nv_tpk_quadro() == 0) fim = true;
            return 1;
        }

        async void Abrir(string url, string cabecalhos)
        {
            Parar();
            int minha = ++sessao;
            posMs = 0;
            try
            {
                var p = new Player();
                player = p;
                p.PlaybackCompleted += (s, e) => { if (minha == sessao) nv_tpk_video_evento(EV_FIM, 0, 0); };
                p.ErrorOccurred += (s, e) => { if (minha == sessao) { Log("erro " + e.Error); nv_tpk_video_evento(EV_ERRO, (int)e.Error, 0); } };
                p.BufferingProgressChanged += (s, e) => { if (minha == sessao) nv_tpk_video_evento(EV_BUFFER, e.Percent, 0); };
                foreach (var linha in (cabecalhos ?? "").Split('\n'))
                {
                    int i = linha.IndexOf(':');
                    if (i <= 0) continue;
                    string nome = linha.Substring(0, i).Trim(), valor = linha.Substring(i + 1).Trim();
                    if (nome.Equals("User-Agent", StringComparison.OrdinalIgnoreCase)) p.UserAgent = valor;
                    else if (nome.Equals("Cookie", StringComparison.OrdinalIgnoreCase)) p.Cookie = valor;
                    else Log("cabecalho ignorado pelo player: " + nome);
                }
                p.SetSource(new MediaUriSource(url));
                p.Display = new Display(NuiWindow.Instance);
                p.DisplaySettings.Mode = PlayerDisplayMode.LetterBox;
                await p.PrepareAsync();
                if (minha != sessao) { p.Unprepare(); p.Dispose(); return; }
                int dur = 0;
                try { dur = p.StreamInfo.GetDuration(); } catch { }
                try { var v = p.StreamInfo.GetVideoProperties(); nv_tpk_video_evento(EV_TAMANHO, v.Size.Width, v.Size.Height); } catch { }
                nv_tpk_video_evento(EV_PRONTO, dur, 0);
                p.Start();
                nv_tpk_video_evento(EV_TOCANDO, 0, 0);
            }
            catch (Exception e)
            {
                Log("abrir: " + e);
                if (minha == sessao) nv_tpk_video_evento(EV_ERRO, -1, 0);
            }
        }

        void Parar()
        {
            sessao++;
            var p = player;
            player = null;
            if (p == null) return;
            try { if (p.State == PlayerState.Playing || p.State == PlayerState.Paused) p.Stop(); } catch { }
            try { if (p.State != PlayerState.Idle) p.Unprepare(); } catch { }
            try { p.Dispose(); } catch { }
        }

        void Pausar(bool pausa)
        {
            if (player == null) return;
            try
            {
                if (pausa && player.State == PlayerState.Playing) { player.Pause(); nv_tpk_video_evento(EV_PAUSADO, 0, 0); }
                else if (!pausa && player.State == PlayerState.Paused) { player.Start(); nv_tpk_video_evento(EV_TOCANDO, 0, 0); }
            }
            catch (Exception e) { Log("pausar: " + e.Message); }
        }

        async void Buscar(int ms)
        {
            if (player == null) return;
            try { posMs = ms; await player.SetPlayPositionAsync(ms, false); }
            catch (Exception e) { Log("buscar: " + e.Message); }
        }

        void Janela(int x, int y, int w, int h)
        {
            if (player == null) return;
            try
            {
                if (x == 0 && y == 0 && w >= W && h >= H) { player.DisplaySettings.Mode = PlayerDisplayMode.LetterBox; return; }
                player.DisplaySettings.Mode = PlayerDisplayMode.Roi;
                player.DisplaySettings.SetRoi(new Tizen.Multimedia.Rectangle(x, y, w, h));
            }
            catch (Exception e) { Log("janela: " + e.Message); }
        }

        protected override void OnTerminate()
        {
            Parar();
            base.OnTerminate();
        }

        static void Main(string[] args)
        {
            new Program().Run(args);
        }
    }
}
