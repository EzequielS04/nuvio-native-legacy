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
//
// TIZEN 9 (#170, #137): aberto pelo menu da TV, o app sobe, desenha o PRIMEIRO
// quadro e para (log "anterior" da UN75CU7700GXZD no D1: "[t] ... primeiro
// quadro na tela" e nada mais ate DALI_FRAMEWORK_DESTROY); aberto pelo
// Apps2Samsung, roda. Suspeita (nao provada): o lancador do Tizen 9 sobe a
// janela principal (opaca para o gerenciador de janelas) por cima do GLWindow
// quando o arranque termina; o GLWindow coberto e iconificado e o fio de
// desenho dele para. Por isso: o vigia sobe o GLWindow de volta se os quadros
// pararem com a janela principal visivel e o app em primeiro plano, e, se nem
// assim voltar, poe o motivo na tela para foto. O rastro de etapas diz onde o
// arranque anterior parou.
#pragma warning disable CS0618
using System;
using System.Collections.Generic;
using System.Diagnostics;
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
using NuiLabel = Tizen.NUI.BaseComponents.TextLabel;

namespace NuvioTpk
{
    class Program : NUIApplication
    {
        [DllImport("libnuvio.so")] static extern int nv_tpk_iniciar(string arte, string dados, int w, int h);
        [DllImport("libnuvio.so")] static extern int nv_tpk_quadro();
        [DllImport("libnuvio.so")] static extern void nv_tpk_config(int esperaMs, int swapZero);
        [DllImport("libnuvio.so")] static extern void nv_tpk_tecla(string nome, int apertou);
        [DllImport("libnuvio.so")] static extern void nv_tpk_log(string linha);
        [DllImport("libdl.so.2")] static extern IntPtr dlopen(string path, int flags);
        [DllImport("libdl.so.2")] static extern IntPtr dlerror();

        const int W = 1920, H = 1080;
#if NV_API8
        const string Host = "api8";
#elif NV_API9
        const string Host = "api9";
#else
        const string Host = "api11";
#endif

        // CANARIO (#137): toca res/silencio.mp4 pelo player do filme logo que a
        // janela sobe, para a TV tirar o audio do Samsung TV Plus que segue por
        // baixo (ver Video.PrimeAudio). false = desliga, o host volta a ser o de
        // antes. Espera PRIME_ESPERA_MS depois do gl.Show() para nao disputar
        // com a abertura.
        const bool PRIME_AUDIO = true;
        const int PRIME_ESPERA_MS = 1500;

        GLWindow gl;
        volatile bool fim;
        NuiTimer vigia, prime;
        Video video;
        bool soCarregada;

        protected override void OnCreate()
        {
            base.OnCreate();
            relogio.Start();
            string dados = DirectoryInfo.Data;
            // Rastro primeiro: se algo abaixo derrubar o processo, a linha ja esta
            // no disco para o proximo arranque mostrar.
            etapaAnterior = RodarEtapas(dados);
            Etapa("note launch host=" + Host + " " + RuntimeInformation.FrameworkDescription + " tv=" + Tv());
            AppDomain.CurrentDomain.UnhandledException += (s, e) => Etapa("note unhandled " + e.ExceptionObject);
            try { Arranca(dados); }
            catch (Exception e)
            {
                Etapa("note launch threw " + e.GetType().Name + ": " + e.Message);
                Erro("Nuvio could not start.", e.GetType().Name + ": " + e.Message);
            }
        }

        void Arranca(string dados)
        {
            var principal = SynchronizationContext.Current;
            var janela = NuiWindow.Instance;
            janela.BackgroundColor = NuiColor.Transparent;
            // Rastro nunca derruba o arranque: sem esses eventos, o app segue.
            try
            {
                janela.VisibilityChanged += (s, e) => { principalVisivel = e.Visibility; Etapa("note main-window visible=" + e.Visibility + Contagem()); };
                janela.FocusChanged += (s, e) => { Etapa("note main-window focus=" + e.FocusGained + Contagem()); if (e.FocusGained) SobeGlSePreciso("main-window focus"); };
            }
            catch (Exception e) { Etapa("note main-window events unavailable " + e.GetType().Name + ": " + e.Message); }

            string arte = IOPath.Combine(DirectoryInfo.Resource, "art");

            // A .so aberta por caminho absoluto ANTES do primeiro DllImport, como
            // no pacote do Tizen 4/5: se o launcher desta TV nao procurar no lib/
            // do pacote (4/5 nao procurava; no 6.5 ninguem mediu, #170), o
            // DllImport("libnuvio.so") casa pelo soname com a ja carregada.
            string so = IOPath.Combine(IOPath.GetFullPath(IOPath.Combine(DirectoryInfo.Resource, "..")), "lib", "libnuvio.so");
            Etapa("begin dlopen-so");
            dlerror();
            if (dlopen(so, 2 | 0x100) == IntPtr.Zero)
            {
                string e = Marshal.PtrToStringAnsi(dlerror());
                Etapa("fail dlopen-so " + e);
                Erro("The TV did not let Nuvio load its native library.", so + ": " + (string.IsNullOrEmpty(e) ? "refused without a message" : e));
                return;
            }
            soCarregada = true;
            Etapa("ok dlopen-so");

            Etapa("begin video-init");
            try
            {
                video = new Video(() => new Display(NuiWindow.Instance),
                                  a => { if (principal != null) principal.Post(_ => a(), null); else a(); },
                                  dados, W, H);
            }
            catch (Exception e)
            {
                Etapa("fail video-init " + e.GetType().Name + ": " + e.Message);
                Erro("Nuvio could not start the player.", e.GetType().Name + ": " + e.Message);
                return;
            }
            Etapa("ok video-init");

            // API8: o callback roda no fio principal e o DALi troca os buffers
            // mesmo em quadro pulado, entao espera o app terminar o quadro.
            // API9+: fio de render proprio, respeita o 0 de "pular", e o swap
            // com vsync e que derrubava o Tizen 9 a 3 fps no spike.
            Etapa("begin nv_tpk_iniciar");
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
                Etapa("fail nv_tpk_iniciar " + e.GetType().Name + ": " + e.Message);
                Erro("Nuvio could not start.", e.GetType().Name + ": " + e.Message);
                return;
            }
            Etapa("ok nv_tpk_iniciar");

            Etapa("begin gl-window");
            try
            {
                CriaJanelaGL();
            }
            catch (Exception e)
            {
                Etapa("fail gl-window " + e.GetType().Name + ": " + e.Message);
                Erro("Nuvio could not open its window.", e.GetType().Name + ": " + e.Message);
                return;
            }
            Etapa("ok gl-window");
            Etapa("begin first-frame");
            if (etapaAnterior != null) Aviso("Previous launch stopped at: " + etapaAnterior);
        }

        bool tvLogada;
        int tiques;

        static string Tv()
        {
            string versao = null, modelo = null;
            try { Tizen.System.Information.TryGetValue<string>("http://tizen.org/feature/platform.version", out versao); } catch { }
            try { Tizen.System.Information.TryGetValue<string>("http://tizen.org/system/model_name", out modelo); } catch { }
            return (modelo ?? "?") + " tizen=" + (versao ?? "?");
        }

        void LogaTv()
        {
            video.Log("[tv] modelo=" + Tv() + " host=" + Host +
                      " dotnet=" + RuntimeInformation.FrameworkDescription + " tela=" + W + "x" + H);
            // O rastro do arranque anterior entra no nuvio.log deste, que o envio
            // automatico sobe: e assim que um arranque que morreu chega ao D1.
            try
            {
                string ant = IOPath.Combine(DirectoryInfo.Data, "tpk-etapas-anterior.txt");
                if (!File.Exists(ant)) return;
                var linhas = File.ReadAllLines(ant);
                int de = Math.Max(0, linhas.Length - 80);
                for (int i = de; i < linhas.Length; i++) video.Log("[etapa-anterior] " + linhas[i]);
            }
            catch { }
        }

        void CriaJanelaGL()
        {
            gl = new GLWindow("nuvio", new NuiRect(0, 0, W, H), true);
            // O vigia conta "parado" a partir daqui ate a primeira chamada.
            Interlocked.Exchange(ref ultimaChamadaMs, relogio.ElapsedMilliseconds);
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
            gl.KeyEvent += (s, e) => Tecla(e.Key, "gl");
            NuiWindow.Instance.KeyEvent += (s, e) => Tecla(e.Key, "main");
            try
            {
                gl.VisibilityChanged += (s, e) => { glVisivel = e.Visibility; Etapa("note gl-window visible=" + e.Visibility + Contagem()); };
                gl.FocusChanged += (s, e) => Etapa("note gl-window focus=" + e.FocusGained + Contagem());
            }
            catch (Exception e) { Etapa("note gl-window events unavailable " + e.GetType().Name + ": " + e.Message); }
            gl.Show();

            // Exit() tem de sair do fio principal, e Quadro() roda no de desenho.
            vigia = new NuiTimer(250);
            vigia.Tick += (s, e) =>
            {
                if (fim) { Etapa("note app ended (main returned)"); video.Parar(); Exit(); return false; }
                video.Tique();
                // ~3 s depois de abrir, quando o main() do app ja redirecionou
                // o stdout para o nuvio.log: uma linha dizendo que TV e esta,
                // para o D1 separar os relatos por versao da Tizen.
                if (!tvLogada && ++tiques >= 12) { tvLogada = true; LogaTv(); }
                Vigia();
                return true;
            };
            vigia.Start();

            AgendaPrime();
        }

        // Nunca derruba nada: qualquer falha vira linha no log e o app segue.
        void AgendaPrime()
        {
            if (!PRIME_AUDIO) return;
            try
            {
                string arq = IOPath.Combine(DirectoryInfo.Resource, "silencio.mp4");
                prime = new NuiTimer(PRIME_ESPERA_MS);
                prime.Tick += (s, e) =>
                {
                    try { if (!fim && video != null) video.PrimeAudio(arq); }
                    catch (Exception ex) { video?.Log("[audio] prime fail " + ex.GetType().Name + ": " + ex.Message); }
                    return false;
                };
                prime.Start();
            }
            catch (Exception e) { video?.Log("[audio] prime fail agendar " + e.GetType().Name + ": " + e.Message); }
        }

        // ================= VIGIA DO DESENHO (Tizen 9) =================

        // Contadores do fio de desenho: chamadas do framework e quadros trocados.
        int chamadas, trocados;
        readonly Stopwatch relogio = new Stopwatch();
        long ultimaChamadaMs = -1;
        volatile bool pausado;
        bool principalVisivel = true, glVisivel = true;
        int subidas, subidasFoco;
        long paradoDesdeMs = -1;
        NuiLabel telaParado;

        string Contagem()
        {
            return " t=" + (relogio.ElapsedMilliseconds / 1000.0).ToString("0.0") + "s calls=" + chamadas + " frames=" + trocados +
                   " paused=" + pausado;
        }

        // Sobe o GLWindow por cima da janela principal. So quando o app esta em
        // primeiro plano e a principal visivel: com a tela inicial da TV por
        // cima (tecla Home), as duas ficam invisiveis e o app nao se intromete.
        void SobeGl(string porque)
        {
            if (gl == null || erroNaTela) return;
            subidas++;
            Etapa("note raise gl-window #" + subidas + " (" + porque + ")" + Contagem() + " mainVisible=" + principalVisivel + " glVisible=" + glVisivel);
            try { gl.Show(); gl.Raise(); } catch (Exception e) { Etapa("note raise failed " + e.GetType().Name + ": " + e.Message); }
        }

        void SobeGlSePreciso(string porque)
        {
#if NV_API8
            // Tizen 6.0 funciona hoje: so registra.
#else
            if (gl == null || pausado || subidasFoco >= 5) return;
            if (!glVisivel || (trocados > 0 && ParadoMs() > 1000)) { subidasFoco++; SobeGl(porque); }
#endif
        }

        long ParadoMs()
        {
            long u = Interlocked.Read(ref ultimaChamadaMs);
            return u < 0 ? relogio.ElapsedMilliseconds : relogio.ElapsedMilliseconds - u;
        }

        // A cada 250 ms, no fio principal. Quadros parados ha mais de 1,5 s com o
        // app em primeiro plano e a janela principal visivel = o GLWindow foi
        // coberto. Sobe ele ate 3 vezes; se nem assim voltar, a tela diz o que
        // houve (com os numeros) para uma foto.
        void Vigia()
        {
            if (gl == null || erroNaTela) return;
            long parado = ParadoMs();
            if (parado < 1500 || pausado || !principalVisivel)
            {
                if (paradoDesdeMs >= 0 && parado < 1500)
                {
                    Etapa("note frames resumed after " + (relogio.ElapsedMilliseconds - paradoDesdeMs) + " ms" + Contagem());
                    paradoDesdeMs = -1;
                    subidas = 0;
                    TiraTelaParado();
                }
                return;
            }
            if (paradoDesdeMs < 0)
            {
                paradoDesdeMs = relogio.ElapsedMilliseconds;
                Etapa("note frames stalled" + Contagem() + " mainVisible=" + principalVisivel + " glVisible=" + glVisivel);
            }
            long ha = relogio.ElapsedMilliseconds - paradoDesdeMs;
            if (subidas < 3 && ha >= subidas * 1000L) { SobeGl("frames stalled " + parado + " ms"); return; }
            if (telaParado == null && ha >= 4000 && trocados < 30) MostraTelaParado(parado);
        }

        // Nao fatal: se os quadros voltarem, some sozinha.
        void MostraTelaParado(long parado)
        {
            try
            {
                Etapa("note showing stalled screen" + Contagem());
                var w = NuiWindow.Instance;
                w.BackgroundColor = NuiColor.Black;
                telaParado = new NuiLabel
                {
                    Text = "Nuvio started, but the TV stopped drawing its window.\n\n" +
                           "Drawn " + trocados + " frame(s), " + chamadas + " draw call(s), none for " + (parado / 1000) + " s. " +
                           "Main window visible=" + principalVisivel + ", Nuvio window visible=" + glVisivel + ", raised " + subidas + " time(s).\n" +
                           "TV " + Tv() + " / host " + Host + " / " + RuntimeInformation.FrameworkDescription +
                           (etapaAnterior != null ? "\nPrevious launch stopped at: " + etapaAnterior : "") +
                           "\n\nPlease post a PHOTO of this screen in issue #170 on GitHub (iqui27/nuvio-native-legacy). Back closes.",
                    MultiLine = true, TextColor = NuiColor.White, PointSize = 20,
                    Size2D = new Size2D(W - 160, H - 160), Position2D = new Position2D(80, 80),
                };
                w.Add(telaParado);
                w.Raise();
            }
            catch (Exception e) { Etapa("note stalled screen failed " + e.GetType().Name + ": " + e.Message); }
        }

        void TiraTelaParado()
        {
            var t = telaParado;
            telaParado = null;
            if (t == null) return;
            try { NuiWindow.Instance.Remove(t); t.Dispose(); NuiWindow.Instance.BackgroundColor = NuiColor.Transparent; gl?.Raise(); } catch { }
        }

        // ================= ERRO FATAL NA TELA =================

        bool erroNaTela;

        // Em vez de fechar em silencio: o motivo na tela, com a TV, para foto.
        void Erro(string titulo, string detalhe)
        {
            // Sem `fim`: o relogio (se ja existir) fecharia o app antes da foto.
            erroNaTela = true;
            Etapa("note error screen: " + titulo + " " + detalhe);
            if (gl != null) { try { gl.Hide(); } catch { } }
            try
            {
                var w = NuiWindow.Instance;
                w.BackgroundColor = NuiColor.Black;
                var t = new NuiLabel
                {
                    Text = titulo + "\n\n" + detalhe + "\n\nTV " + Tv() + " / host " + Host + " / " + RuntimeInformation.FrameworkDescription +
                           (etapaAnterior != null ? "\nPrevious launch stopped at: " + etapaAnterior : "") +
                           "\n\nPlease post a PHOTO of this screen in issue #137 on GitHub (iqui27/nuvio-native-legacy). Back closes.",
                    MultiLine = true, TextColor = NuiColor.White, PointSize = 20,
                    Size2D = new Size2D(W - 160, H - 160), Position2D = new Position2D(80, 80),
                };
                w.Add(t);
                w.KeyEvent += (s, e) => { if (e.Key.State == Key.StateType.Down && (e.Key.KeyPressedName == "XF86Back" || e.Key.KeyPressedName == "Escape")) Exit(); };
                // No Tizen 9 outra janela pode estar por cima: esta vem para a frente.
                w.Show();
                w.Raise();
            }
            catch (Exception e) { Etapa("note error screen failed " + e.GetType().Name + ": " + e.Message); }
        }

        // ================= RASTRO DE ETAPAS =================
        // data/tpk-etapas.txt: "host begin X" / "host ok X" / "host fail X" /
        // "host note ...", uma linha por append, sem handler de sinal (o CoreCLR
        // e dono deles). No arranque seguinte vira tpk-etapas-anterior.txt; se
        // acabou numa etapa sem ok, a tela mostra qual por 30 s, e as linhas
        // entram no nuvio.log (LogaTv) para subir com o envio automatico.

        static string etapasArq = "";
        static readonly object travaEtapa = new object();
        string etapaAnterior;
        int etapasEscritas;

        void Etapa(string linha)
        {
            if (string.IsNullOrEmpty(etapasArq)) return;
            // Teto: o vigia e os eventos de janela nao podem encher a pasta.
            if (Interlocked.Increment(ref etapasEscritas) > 400) return;
            string l = "host " + linha.Replace('\n', ' ') + " @" + (relogio.ElapsedMilliseconds / 1000.0).ToString("0.00") + "s";
            try { lock (travaEtapa) File.AppendAllText(etapasArq, l + "\n"); } catch { }
            if (soCarregada) { try { nv_tpk_log("[etapa] " + l); } catch { } }
        }

        static string RodarEtapas(string dados)
        {
            string atual = IOPath.Combine(dados, "tpk-etapas.txt");
            string anterior = IOPath.Combine(dados, "tpk-etapas-anterior.txt");
            string parou = null;
            try
            {
                if (File.Exists(atual))
                {
                    if (File.Exists(anterior)) File.Delete(anterior);
                    File.Move(atual, anterior);
                }
                if (File.Exists(anterior)) parou = EtapaAberta(File.ReadAllLines(anterior));
            }
            catch { }
            etapasArq = atual;
            return parou;
        }

        // "begin X" abre, "ok X"/"fail X" fecham. Devolve a ULTIMA aberta, com os
        // numeros da ultima nota do vigia (se houver); null = nada ficou aberto.
        // first-key sozinha numa sessao que terminou normalmente nao conta: e so
        // alguem que abriu e fechou sem apertar tecla.
        static string EtapaAberta(string[] linhas)
        {
            var abertas = new List<string>();
            bool terminou = false;
            string ultimaNota = null;
            foreach (var l in linhas)
            {
                var p = l.Split(new[] { ' ' }, 4, StringSplitOptions.RemoveEmptyEntries);
                if (p.Length < 3) continue;
                string verbo = p[1], etapa = p[2];
                if (verbo == "begin") { abertas.Remove(etapa); abertas.Add(etapa); }
                else if (verbo == "ok" || verbo == "fail") abertas.Remove(etapa);
                else if (verbo == "note")
                {
                    if (etapa == "terminate") terminou = true;
                    if (l.Contains("calls=")) ultimaNota = l.Substring(l.IndexOf("note ") + 5);
                }
            }
            if (abertas.Count == 0) return null;
            if (abertas.Count == 1 && abertas[0] == "first-key" && terminou) return null;
            string r = abertas[abertas.Count - 1];
            if (terminou) r += " (then the TV closed it)";
            if (ultimaNota != null) r += " — last: " + ultimaNota;
            return r;
        }

        // Aviso NAO fatal do arranque anterior: faixa preta no topo, numa janela
        // propria por cima do GLWindow, por 30 s ou ate a primeira tecla.
        NuiWindow janelaAviso;

        void Aviso(string texto)
        {
            try
            {
                Etapa("note showing notice: " + texto);
                janelaAviso = new NuiWindow("NuvioAviso", new NuiRect(0, 0, W, 240), false);
                janelaAviso.BackgroundColor = NuiColor.Black;
                janelaAviso.Add(new NuiLabel
                {
                    Text = texto + "\nPlease photograph this and post it in GitHub issue #170 (iqui27/nuvio-native-legacy). Any key hides it.",
                    MultiLine = true, TextColor = NuiColor.White, PointSize = 16,
                    Size2D = new Size2D(W - 80, 220), Position2D = new Position2D(40, 10),
                });
                janelaAviso.KeyEvent += (s, e) => { FechaAviso(); Tecla(e.Key, "notice"); };
                janelaAviso.Show();
                var t = new NuiTimer(30000);
                t.Tick += (s, e) => { FechaAviso(); return false; };
                t.Start();
            }
            catch (Exception e) { Etapa("note notice failed " + e.GetType().Name + ": " + e.Message); janelaAviso = null; }
        }

        void FechaAviso()
        {
            var j = janelaAviso;
            janelaAviso = null;
            if (j == null) return;
            try { j.Hide(); } catch { }
            try { gl?.Raise(); } catch { }
        }

        // ================= TECLAS, QUADRO, CICLO DE VIDA =================

        bool primeiraTecla;

        void Tecla(Key k, string janela)
        {
            if (!primeiraTecla && k.State == Key.StateType.Down)
            {
                primeiraTecla = true;
                Etapa("ok first-key " + k.KeyPressedName + " via " + janela + Contagem());
            }
            if (telaParado != null && k.State == Key.StateType.Down && (k.KeyPressedName == "XF86Back" || k.KeyPressedName == "Escape")) { Exit(); return; }
            nv_tpk_tecla(k.KeyPressedName, k.State == Key.StateType.Down ? 1 : 0);
        }

        // Fio de desenho do NUI. 1 = troca, 0 = pula, -1 = o app acabou.
        int Quadro()
        {
            Interlocked.Exchange(ref ultimaChamadaMs, relogio.ElapsedMilliseconds);
            if (Interlocked.Increment(ref chamadas) == 1) Etapa("note first draw call");
            if (fim) return 0;
            int r = nv_tpk_quadro();
            // De novo na saida: na API8 esta chamada roda no fio principal e pode
            // esperar o app terminar um quadro longo; o vigia (mesmo fio) nao
            // pode ler isso como "o framework parou de chamar".
            Interlocked.Exchange(ref ultimaChamadaMs, relogio.ElapsedMilliseconds);
            if (r < 0) fim = true;
            if (r > 0)
            {
                int n = Interlocked.Increment(ref trocados);
                if (n == 1) { Etapa("ok first-frame" + Contagem()); Etapa("begin first-key"); Etapa("begin steady-frames"); }
                else if (n == 30) Etapa("ok steady-frames" + Contagem());
            }
            return r > 0 ? 1 : 0;
        }

        protected override void OnPause()
        {
            pausado = true;
            Etapa("note pause" + Contagem());
            video?.PausarPeloSistema();
            base.OnPause();
        }

        protected override void OnResume()
        {
            pausado = false;
            Etapa("note resume" + Contagem());
            base.OnResume();
        }

        protected override void OnTerminate()
        {
            Etapa("note terminate" + Contagem());
            video?.Parar();
            base.OnTerminate();
        }

        static void Main(string[] args)
        {
            new Program().Run(args);
        }
    }
}
