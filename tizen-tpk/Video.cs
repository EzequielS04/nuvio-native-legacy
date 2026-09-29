// Player do Nuvio .tpk, comum aos dois hosts (NUI no Tizen 6+, ElmSharp no
// 4/5). Tizen.Multimedia.Player no plano de video da TV; a janela GL fica por
// cima, translucida, e o C abre o furo onde o video aparece. O C pede pelas
// funcoes registradas em nv_tpk_video_registrar e ouve pelo
// nv_tpk_video_evento (src/video_tpk.c).
//
// Toda chamada ao player passa pelo fio principal (Principal), porque o C
// chama do fio dele. A posicao e lida pelo relogio do host (Tique) e o C so le
// o numero guardado, sem esperar ninguem.
using System;
using System.IO;
using System.Runtime.InteropServices;
using Tizen.Multimedia;

namespace NuvioTpk
{
    class Video
    {
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_registrar(IntPtr abrir, IntPtr parar, IntPtr pausar,
                                                                              IntPtr buscar, IntPtr volume, IntPtr janela, IntPtr pos);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_evento(int tipo, int a, int b);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_registrar_faixas(IntPtr escolher);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_faixa(int tipo, int idx, string lingua);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_faixas_fim(int selAudio, int selLeg);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_legenda(string texto, int durMs);
        [DllImport("libnuvio.so")] static extern void nv_tpk_log(string linha);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnAbrir(IntPtr url, IntPtr cabecalhos);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnSemArg();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnInt(int v);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnRet(int x, int y, int w, int h);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate int FnPos();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnEscolher(int tipo, int idx);

        const int EV_PRONTO = 1, EV_TOCANDO = 2, EV_PAUSADO = 3, EV_FIM = 4, EV_ERRO = 5, EV_TAMANHO = 6, EV_BUFFER = 7;

        readonly Func<Display> fazDisplay;
        readonly Action<Action> principal;
        readonly string logArq;
        readonly int telaW, telaH;
        // Referencias vivas: o C guarda os ponteiros, o GC nao pode recolher.
        FnAbrir fAbrir; FnSemArg fParar; FnInt fPausar, fBuscar, fVolume; FnRet fJanela; FnPos fPos; FnEscolher fEscolher;

        Player player;
        int sessao;
        volatile int posMs;

        public Video(Func<Display> fazDisplay, Action<Action> principal, string dados, int telaW, int telaH)
        {
            this.fazDisplay = fazDisplay;
            this.principal = principal;
            this.telaW = telaW;
            this.telaH = telaH;
            logArq = Path.Combine(dados, "tpk-host.log");
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
            fEscolher = (tipo, idx) => Principal(() => Escolher(tipo, idx));
            nv_tpk_video_registrar_faixas(Marshal.GetFunctionPointerForDelegate(fEscolher));
        }

        // 0 = audio, 1 = legenda embutida, 2 = atraso da legenda (ms).
        void Escolher(int tipo, int idx)
        {
            if (player == null) return;
            try
            {
                if (tipo == 0) player.AudioTrackInfo.Selected = idx;
                else if (tipo == 1) player.SubtitleTrackInfo.Selected = idx;
                else player.SetSubtitleOffset(idx);
            }
            catch (Exception e) { if (tipo != 2) Log("escolher " + tipo + "/" + idx + ": " + e.Message); }
        }

        // Lista de faixas para o C, logo depois do prepare. Devolve quantas
        // faixas de audio o player listou.
        int Faixas(Player p, bool soAudio = false)
        {
            int selA = -1, selL = -1, nA = 0;
            try
            {
                var a = p.AudioTrackInfo;
                nA = a.GetCount();
                for (int i = 0; i < nA; i++) nv_tpk_video_faixa(0, i, Lingua(() => a.GetLanguageCode(i)));
                try { selA = a.Selected; } catch { }
            }
            catch (Exception e) { Log("faixas de audio: " + e.Message); }
            // #165: legenda listada e audio nao. Se o video tem som, a faixa que
            // toca aparece como unica, em vez de "nenhuma faixa".
            if (nA == 0)
            {
                try
                {
                    var ap = p.StreamInfo.GetAudioProperties();
                    if (ap.Channels > 0) { nv_tpk_video_faixa(0, 0, ""); selA = 0; Log($"audio sem lista do player: {ap.Channels} canais, {ap.SampleRate} Hz"); }
                }
                catch (Exception e) { Log("propriedades de audio: " + e.Message); }
            }
            if (soAudio) { nv_tpk_video_faixas_fim(selA, -1); return nA; }
            try
            {
                var l = p.SubtitleTrackInfo;
                int n = l.GetCount();
                for (int i = 0; i < n; i++) nv_tpk_video_faixa(1, i, Lingua(() => l.GetLanguageCode(i)));
                try { selL = l.Selected; } catch { }
            }
            catch (Exception e) { Log("faixas de legenda: " + e.Message); }
            Log($"faixas do player: {nA} audio (sel={selA}), legenda sel={selL}");
            nv_tpk_video_faixas_fim(selA, selL);
            return nA;
        }

        static string Lingua(Func<string> f)
        {
            try { return f() ?? ""; } catch { return ""; }
        }

        // App foi para segundo plano: o player pausa (e o C fica sabendo).
        public void PausarPeloSistema() { Pausar(true); }

        // Relogio do host, no fio principal.
        public void Tique()
        {
            try { if (player != null && player.State == PlayerState.Playing) posMs = player.GetPlayPosition(); } catch { }
        }

        public void Log(string s)
        {
            try { nv_tpk_log(s); } catch { }
            try { File.AppendAllText(logArq, DateTime.Now.ToString("HH:mm:ss ") + s + "\n"); } catch { }
        }

        void Principal(Action a)
        {
            principal(() => { try { a(); } catch (Exception e) { Log("principal: " + e); } });
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
                p.PlaybackInterrupted += (s, e) => { if (minha == sessao) { Log("interrompido: " + e.Reason); nv_tpk_video_evento(EV_PAUSADO, 0, 0); } };
                p.SubtitleUpdated += (s, e) => { if (minha == sessao) nv_tpk_video_legenda(e.Text ?? "", (int)e.Duration); };
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
                p.Display = fazDisplay();
                p.DisplaySettings.Mode = PlayerDisplayMode.LetterBox;
                await p.PrepareAsync();
                if (minha != sessao) { p.Unprepare(); p.Dispose(); return; }
                int dur = 0;
                try { dur = p.StreamInfo.GetDuration(); } catch { }
                try { var v = p.StreamInfo.GetVideoProperties(); nv_tpk_video_evento(EV_TAMANHO, v.Size.Width, v.Size.Height); } catch { }
                int nAudio = Faixas(p);
                nv_tpk_video_evento(EV_PRONTO, dur, 0);
                p.Start();
                nv_tpk_video_evento(EV_TOCANDO, 0, 0);
                // Alguns contêineres/HLS so publicam as faixas de audio depois
                // que a reproducao comeca: le de novo, uma vez.
                if (nAudio == 0)
                {
                    await System.Threading.Tasks.Task.Delay(2000);
                    if (minha == sessao && player == p) Faixas(p, true);
                }
            }
            catch (Exception e)
            {
                Log("abrir: " + e);
                if (minha == sessao) nv_tpk_video_evento(EV_ERRO, -1, 0);
            }
        }

        public void Parar()
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
                // QUADRO CHEIO SEM ZOOM: LetterBox, o mesmo caminho da reproducao
                // normal nos 4 hosts. A reproducao cheia pede exatamente a tela
                // (0,0,telaW,telaH), entao esta guarda continua sendo um no-op
                // para ela — 6+ e o host 4/5 nao mudam.
                //
                // ZOOM (#178): o recorte de fonte emulado (src/video_tpk.c
                // video_janela_fonte) manda um ROI que RECUA a origem para
                // negativo ou ESTOURA a tela, para que a fatia desejada do quadro
                // preencha o destino. Antes, `w >= telaW && h >= telaH` engolia um
                // ROI ampliado ancorado em 0,0 de volta para LetterBox, matando o
                // zoom. Agora so o quadro EXATO da tela vira LetterBox; qualquer
                // ROI de zoom (origem negativa OU maior que a tela) passa cru ao
                // SetRoi.
                if (x == 0 && y == 0 && w == telaW && h == telaH) { player.DisplaySettings.Mode = PlayerDisplayMode.LetterBox; return; }
                player.DisplaySettings.Mode = PlayerDisplayMode.Roi;
                player.DisplaySettings.SetRoi(new Rectangle(x, y, w, h));
            }
            catch (Exception e) { Log("janela: " + e.Message); }
        }
    }
}
