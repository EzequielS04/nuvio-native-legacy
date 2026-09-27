// Host do Nuvio para Tizen 4.0/5.0. Primeiro passo: descobrir se a TV deixa
// carregar a libnuvio.so com o pacote assinado como Partner. Com Public ela
// recusou (spike.2): dlopen NULL com dlerror vazio, e Process.Start com
// "Operation not permitted" — cara de UEP, a checagem de assinatura de
// executavel da Samsung. A tela diz o resultado com o erro real.
//
// Por que o dlerror vinha vazio: o CLR resolve o simbolo do P/Invoke dlerror
// na primeira chamada, e esse proprio dlopen/dlsym zera a mensagem pendente.
// Aqui dlerror e chamado uma vez ANTES, para ja estar resolvido.
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using ElmSharp;
using Tizen.Applications;
using Tizen.System;

namespace NuvioTpk
{
    class Program : CoreUIApplication
    {
        [DllImport("libdl.so.2")] static extern IntPtr dlopen(string path, int flags);
        [DllImport("libdl.so.2")] static extern IntPtr dlerror();
        [DllImport("libdl.so.2")] static extern IntPtr dlsym(IntPtr h, string nome);
        const int RTLD_NOW = 2, RTLD_GLOBAL = 0x100;

        Window janela;
        Box coluna;

        protected override void OnCreate()
        {
            base.OnCreate();
            janela = new Window("Nuvio");
            janela.BackButtonPressed += (s, e) => Exit();
            var fundo = new Background(janela) { Color = Color.Black };
            fundo.Show();
            janela.AddResizeObject(fundo);
            coluna = new Box(janela) { AlignmentX = -1, AlignmentY = 0, WeightX = 1, WeightY = 1 };
            coluna.Show();
            janela.AddResizeObject(coluna);
            janela.Show();

            Linha("Nuvio para TVs 2018-2019 (teste de compatibilidade)");
            Information.TryGetValue<string>("http://tizen.org/feature/platform.version", out string versao);
            Information.TryGetValue<string>("http://tizen.org/system/model_name", out string modelo);
            Linha($"TV: {modelo ?? "?"} / Tizen {versao ?? "?"} / {RuntimeInformation.FrameworkDescription}");

            string raiz = Path.GetFullPath(Path.Combine(DirectoryInfo.Resource, ".."));
            string so = Path.Combine(raiz, "lib", "libnuvio.so");
            Linha("1. libnuvio.so no pacote: " + (File.Exists(so) ? "sim" : "NAO (" + so + ")"));
            Linha("2. carregar do pacote: " + Carrega(so));

            // Mesma .so copiada para data/: se o bloqueio for so da pasta do
            // pacote (rotulo SMACK de somente leitura), aqui passa.
            string copia = Path.Combine(DirectoryInfo.Data, "libnuvio-copia.so");
            string r3;
            try { File.Copy(so, copia, true); r3 = Carrega(copia); }
            catch (Exception e) { r3 = "copia falhou: " + e.Message; }
            Linha("3. carregar de data/: " + r3);
            Linha("4. montagem da pasta do app: " + Montagem(raiz));
            Linha("Mande uma FOTO desta tela na #137 do GitHub. Voltar sai.");
        }

        static string Carrega(string caminho)
        {
            dlerror();
            IntPtr h = dlopen(caminho, RTLD_NOW | RTLD_GLOBAL);
            if (h == IntPtr.Zero)
            {
                string e = Marshal.PtrToStringAnsi(dlerror());
                return "FALHOU (" + (string.IsNullOrEmpty(e) ? "sem mensagem do dlopen" : e) + ")";
            }
            return dlsym(h, "nv_tpk_iniciar") != IntPtr.Zero ? "OK" : "carregou, mas sem nv_tpk_iniciar";
        }

        static string Montagem(string raiz)
        {
            try
            {
                string melhor = "", linhaMelhor = "?";
                foreach (var l in File.ReadAllLines("/proc/self/mounts"))
                {
                    var p = l.Split(' ');
                    if (p.Length > 3 && raiz.StartsWith(p[1]) && p[1].Length > melhor.Length) { melhor = p[1]; linhaMelhor = p[1] + " " + p[3]; }
                }
                return linhaMelhor;
            }
            catch (Exception e) { return "? (" + e.Message + ")"; }
        }

        void Linha(string texto)
        {
            var l = new Label(janela)
            {
                Text = "<font_size=28 color=#FFFFFF>" + texto.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;") + "</font>",
                AlignmentX = -1, WeightX = 1, LineWrapType = WrapType.Word
            };
            l.Show();
            coluna.PackEnd(l);
        }

        static void Main(string[] args)
        {
            Elementary.Initialize();
            Elementary.ThemeOverlay();
            new Program().Run(args);
        }
    }
}
