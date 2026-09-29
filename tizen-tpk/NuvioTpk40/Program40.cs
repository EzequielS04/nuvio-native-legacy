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
// ------------------------------------------------------------------------
// CARGA DA libnuvio.so NA TIZEN 4/5 (UEP)
// ------------------------------------------------------------------------
// Numa TV Tizen 4.0 real (optiman QE55Q6FNA) o NvUepProbe/NvMemfd (spike,
// #137) provou:
//   A. dlopen de ARQUIVO do pacote (lib/) -> BLOQUEADO pela UEP
//      ("failed to map segment from shared object")
//   B. dlopen de ARQUIVO em data/         -> tambem BLOQUEADO
//   C. memfd_create por NOME (DllImport)  -> nao existe na libc do Tizen 4/5
//   D. mmap +EXEC de memoria ANONIMA      -> PERMITIDO
//
// Ou seja: mapear PROT_EXEC de um inode de arquivo e barrado, mas memoria
// anonima executavel passa. Dai as duas rotas deste host, em ordem:
//
//   1. memfd (rota preferida): syscall(385) cria um fd anonimo, escrevemos a
//      libnuvio.so nele e damos dlopen("/proc/self/fd/N"). O fd fica ABERTO a
//      vida toda do processo (o /proc/self/fd/N so existe enquanto ele vive).
//      Se passar, o handle e um handle dlopen NORMAL e a lib entra no link map
//      do loader — entao um [DllImport("libnuvio.so")] resolveria pelo soname
//      sozinho. Mesmo assim resolvemos os simbolos por dlsym(handle, ...) e
//      montamos o mesmo despacho por ponteiro de funcao da rota 2, para o
//      resto do app ter UM SO caminho de chamada nas duas rotas. (Ver NvLib.)
//
//   2. Carregador de ELF proprio (rota D): mmap anon RW, mapeia os PT_LOAD,
//      aplica relocacoes R_ARM_RELATIVE/GLOB_DAT/JUMP_SLOT/ABS32, resolve
//      externos por dlsym(RTLD_DEFAULT) (com libGLESv2/libEGL pre-carregadas
//      GLOBAL para os simbolos de GL aparecerem), roda DT_INIT_ARRAY,
//      mprotect +EXEC por segmento e resolve os nv_tpk_* pelo dynsym. Aqui a
//      lib NAO esta no link map: DllImport-por-soname NAO resolveria. Por isso
//      TODO ponto de entrada do libnuvio e chamado por ponteiro de funcao
//      (NvLib), obtido do dynsym do carregador. A memoria fica viva a vida
//      toda do processo (nunca munmap na rota vencedora).
//
//   3. Se as DUAS falharem, a tela de erro mostra as mensagens reais.
//
// Os alvos Tizen 6+ (NuvioTpk/60/65) nao mudam: la o dlopen da .so do pacote e
// permitido e o host usa DllImport direto (tizen-tpk/Program.cs). So este
// arquivo, compilado unicamente no pacote NuvioTpk40, muda.
using System;
using System.IO;
using System.Runtime.InteropServices;
using ElmSharp;
using Tizen.System;
using Tizen.TV.NUI.GLApplication;
using GLKey = Tizen.TV.NUI.GLApplication.Key;

namespace NuvioTpk
{
    // Despacho unico para os pontos de entrada do libnuvio. Nas duas rotas de
    // carga (memfd e carregador ELF) estes delegates sao apontados para o
    // codigo nativo por ponteiro de funcao, entao Program40/Video chamam sempre
    // por aqui, sem depender de o DllImport-por-soname resolver.
    static class NvLib
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int IniciarDel(string arte, string dados, int w, int h);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ConfigDel(int esperaMs, int swapZero);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int QuadroDel();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr ErroDel();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void TeclaDel(string nome, int apertou);

        // Guardados em campos estaticos: seguram o delegate vivo (o GC nao o
        // recolhe) e o ponteiro nativo e estavel pela vida do processo.
        public static IniciarDel Iniciar;
        public static ConfigDel Config;
        public static QuadroDel Quadro;
        public static ErroDel Erro;
        public static TeclaDel Tecla;

        public static bool Pronto => Iniciar != null && Config != null && Quadro != null && Erro != null && Tecla != null;
    }

    class Program : TVGLApplication
    {
        [DllImport("libdl.so.2")] static extern IntPtr dlopen(string path, int flags);
        [DllImport("libdl.so.2")] static extern IntPtr dlsym(IntPtr h, string sym);
        [DllImport("libdl.so.2")] static extern IntPtr dlerror();
        // libc por NUMERO de syscall (a libc do Tizen 4/5 nao exporta
        // memfd_create como simbolo, so o dispatcher generico syscall()).
        [DllImport("libc.so.6", SetLastError = true, EntryPoint = "syscall")]
        static extern int syscall_memfd(int number, IntPtr name, int flags);
        [DllImport("libc.so.6", SetLastError = true, EntryPoint = "syscall")]
        static extern int syscall_cache(int number, IntPtr start, IntPtr end, int flags);
        [DllImport("libc.so.6", SetLastError = true)] static extern IntPtr write(int fd, IntPtr buf, IntPtr count);
        [DllImport("libc.so.6", SetLastError = true)] static extern IntPtr mmap(IntPtr addr, IntPtr len, int prot, int flags, int fd, IntPtr off);
        [DllImport("libc.so.6", SetLastError = true)] static extern int mprotect(IntPtr addr, IntPtr len, int prot);
        [DllImport("libc.so.6", SetLastError = true)] static extern int munmap(IntPtr addr, IntPtr len);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void InitFn();

        const int RTLD_NOW = 2, RTLD_GLOBAL = 0x100, RTLD_LAZY = 1;
        static readonly IntPtr RTLD_DEFAULT = IntPtr.Zero;   // glibc
        const int PROT_READ = 1, PROT_WRITE = 2, PROT_EXEC = 4;
        const int MAP_PRIVATE = 2, MAP_ANONYMOUS = 0x20;    // ARM/Linux
        const int SYS_memfd_create = 385;                    // ARM EABI (armv7)
        const int ARM_NR_cacheflush = 0x0f0002;              // __ARM_NR_cacheflush
        const int PAGE = 4096;
        const int W = 1920, H = 1080;

        // Mantidos vivos pela vida do processo: o fd do memfd (o /proc/self/fd/N
        // depende dele) e o mapeamento anonimo do carregador ELF.
        static int memfdFd = -1;
        static IntPtr elfMap = IntPtr.Zero;
        static long elfSpan = 0;

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

            string falhaMemfd, falhaElf;
            if (!CarregaNativo(so, out falhaMemfd, out falhaElf))
            {
                Erro("A TV nao deixou o Nuvio nativo carregar.",
                     "memfd (syscall 385): " + falhaMemfd,
                     "carregador ELF: " + falhaElf,
                     "montagem: " + Montagem(raiz));
                return;
            }

            try
            {
                // Janela do video, atras da janela GL (JuvoPlayer.OpenGL faz igual).
                janelaVideo = new Window("NuvioVideo") { Geometry = new Rect(0, 0, W, H) };
                janelaVideo.Show();
                janelaVideo.Lower();
                video = new Video(() => new Tizen.Multimedia.Display(janelaVideo), a => EcoreMainloop.PostAndWakeUp(a), dados, W, H);

                // OnUpdate SEMPRE devolve true, como o JuvoPlayer.OpenGL: com
                // false no arranque (app ainda sem quadro) a TVGLApplication
                // parava de chamar, o app nunca recebia o contexto e ficava
                // tela preta sem log (Tizen 5.0, 28/09). Espera sem limite:
                // no arranque limpa para preto, depois espera o quadro do app.
                NvLib.Config(-1, 0);
                if (NvLib.Iniciar(Path.Combine(dir.Resource, "art"), dados, W, H) != 0)
                {
                    Erro("O Nuvio nao conseguiu iniciar.", Marshal.PtrToStringAnsi(NvLib.Erro()) ?? "nv_tpk_iniciar falhou");
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
                string motivo = Marshal.PtrToStringAnsi(NvLib.Erro());
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
            // Nunca false enquanto o app vive: a TVGLApplication para de chamar.
            if (fim) return false;
            if (!rodando) return true;
            int r = NvLib.Quadro();
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
            if (rodando) NvLib.Tecla(k.KeyPressedName, k.State == GLKey.StateType.Down ? 1 : 0);
        }

        protected override void OnPause()
        {
            video?.PausarPeloSistema();
            base.OnPause();
        }

        // ================= CARGA NATIVA =================

        // Tenta as duas rotas; na que vencer, deixa NvLib.* apontando para o
        // codigo nativo. Devolve true se o app pode rodar.
        static bool CarregaNativo(string so, out string falhaMemfd, out string falhaElf)
        {
            falhaMemfd = falhaElf = null;
            byte[] elf;
            try { elf = File.ReadAllBytes(so); }
            catch (Exception e)
            {
                falhaMemfd = falhaElf = "nao consegui ler " + so + ": " + e.Message;
                return false;
            }

            // Rota 1: memfd + dlopen(/proc/self/fd/N). Handle dlopen normal.
            try
            {
                if (Metodo1_Memfd(elf, out falhaMemfd)) return true;
            }
            catch (Exception e) { falhaMemfd = "EXCECAO " + e.GetType().Name + ": " + e.Message; }

            // Rota 2: carregador de ELF em memoria anonima.
            try
            {
                if (Metodo2_CarregadorElf(elf, out falhaElf)) return true;
            }
            catch (Exception e) { falhaElf = "EXCECAO " + e.GetType().Name + ": " + e.Message; }

            return false;
        }

        // ---- Rota 1: memfd_create(385) + dlopen(/proc/self/fd/N) ----
        static bool Metodo1_Memfd(byte[] elf, out string falha)
        {
            falha = null;
            IntPtr nome = Marshal.StringToHGlobalAnsi("nuvio");
            int fd;
            try { fd = syscall_memfd(SYS_memfd_create, nome, 0); }
            finally { Marshal.FreeHGlobal(nome); }
            if (fd < 0) { falha = "syscall(385) falhou (errno=" + Marshal.GetLastWin32Error() + ")"; return false; }

            GCHandle pin = GCHandle.Alloc(elf, GCHandleType.Pinned);
            try
            {
                IntPtr baseP = pin.AddrOfPinnedObject();
                int off = 0;
                while (off < elf.Length)
                {
                    int n = (int)write(fd, baseP + off, (IntPtr)(elf.Length - off));
                    if (n <= 0) { falha = "write falhou em " + off + "/" + elf.Length + " (errno=" + Marshal.GetLastWin32Error() + ")"; return false; }
                    off += n;
                }
            }
            finally { pin.Free(); }

            // O fd fica ABERTO a vida toda: /proc/self/fd/N so existe com ele vivo.
            string path = "/proc/self/fd/" + fd;
            dlerror();
            IntPtr h = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
            if (h == IntPtr.Zero) { falha = "dlopen(" + path + ") -> " + Err(); return false; }
            memfdFd = fd;   // segura o fd

            // A lib entrou no link map: [DllImport("libnuvio.so")] resolveria
            // pelo soname sozinho. Mesmo assim ligamos por dlsym(handle) para o
            // resto do app ter o mesmo caminho de chamada da rota 2.
            if (!LigaSimbolos(s => dlsym(h, s), out string qual)) { falha = "handle ok mas simbolo faltou: " + qual; return false; }
            return true;
        }

        // ---- Rota 2: carregador de ELF32 ARM em memoria anonima (rota D) ----
        static bool Metodo2_CarregadorElf(byte[] e, out string falha)
        {
            falha = null;
            if (e.Length < 52 || e[0] != 0x7F || e[1] != (byte)'E' || e[2] != (byte)'L' || e[3] != (byte)'F') { falha = "nao e ELF"; return false; }
            if (e[4] != 1) { falha = "nao e ELFCLASS32"; return false; }
            if (e[5] != 1) { falha = "nao e little-endian"; return false; }
            ushort machine = U16(e, 18);
            if (machine != 40) { falha = "e_machine=" + machine + ", esperado 40 (ARM)"; return false; }

            // Externos de GL/EGL so aparecem no dlsym(RTLD_DEFAULT) se ja
            // estiverem carregados GLOBAL. Pre-carrega os que o libnuvio precisa.
            foreach (var lib in new[] { "libEGL.so.1", "libGLESv2.so.2", "libEGL.so", "libGLESv2.so" })
                dlopen(lib, RTLD_NOW | RTLD_GLOBAL);

            uint phoff = U32(e, 28);
            ushort phentsize = U16(e, 42);
            ushort phnum = U16(e, 44);

            long minVa = long.MaxValue, maxVa = long.MinValue;
            uint dynVa = 0;
            for (int i = 0; i < phnum; i++)
            {
                int p = (int)(phoff + (uint)i * phentsize);
                uint ptype = U32(e, p);
                uint pvaddr = U32(e, p + 8);
                uint pmemsz = U32(e, p + 20);
                if (ptype == 1) { if (pvaddr < minVa) minVa = pvaddr; if (pvaddr + pmemsz > maxVa) maxVa = pvaddr + pmemsz; }
                else if (ptype == 2) dynVa = pvaddr; // PT_DYNAMIC
            }
            if (minVa == long.MaxValue) { falha = "sem PT_LOAD"; return false; }

            long baseVa = minVa & ~(long)(PAGE - 1);
            long span = ((maxVa - baseVa) + PAGE - 1) & ~(long)(PAGE - 1);

            IntPtr map = mmap(IntPtr.Zero, (IntPtr)span, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, IntPtr.Zero);
            if (map == (IntPtr)(-1) || map == IntPtr.Zero) { falha = "mmap(" + span + ") falhou (errno=" + Marshal.GetLastWin32Error() + ")"; return false; }
            long delta = map.ToInt64() - baseVa;

            for (int i = 0; i < phnum; i++)
            {
                int p = (int)(phoff + (uint)i * phentsize);
                if (U32(e, p) != 1) continue;
                uint poff = U32(e, p + 4);
                uint pvaddr = U32(e, p + 8);
                uint pfilesz = U32(e, p + 16);
                Marshal.Copy(e, (int)poff, (IntPtr)(delta + pvaddr), (int)pfilesz);
            }

            long dynAddr = delta + dynVa;
            long symtab = 0, strtab = 0, hash = 0;
            long rel = 0, relsz = 0, relent = 8;
            long jmprel = 0, pltrelsz = 0;
            long initArray = 0, initArraySz = 0, initFn = 0;
            for (long d = dynAddr; ; d += 8)
            {
                int tag = Marshal.ReadInt32((IntPtr)d);
                uint val = (uint)Marshal.ReadInt32((IntPtr)(d + 4));
                if (tag == 0) break;
                switch (tag)
                {
                    case 4: hash = delta + val; break;        // DT_HASH
                    case 5: strtab = delta + val; break;      // DT_STRTAB
                    case 6: symtab = delta + val; break;      // DT_SYMTAB
                    case 17: rel = delta + val; break;        // DT_REL
                    case 18: relsz = val; break;              // DT_RELSZ
                    case 19: relent = val; break;             // DT_RELENT
                    case 23: jmprel = delta + val; break;     // DT_JMPREL
                    case 2: pltrelsz = val; break;            // DT_PLTRELSZ
                    case 12: initFn = delta + val; break;     // DT_INIT
                    case 25: initArray = delta + val; break;  // DT_INIT_ARRAY
                    case 27: initArraySz = val; break;        // DT_INIT_ARRAYSZ
                }
            }
            if (symtab == 0 || strtab == 0) { munmap(map, (IntPtr)span); falha = "sem DT_SYMTAB/DT_STRTAB"; return false; }

            RelocaBloco(rel, relsz, relent, symtab, strtab, delta);
            RelocaBloco(jmprel, pltrelsz, 8, symtab, strtab, delta);

            for (int i = 0; i < phnum; i++)
            {
                int p = (int)(phoff + (uint)i * phentsize);
                if (U32(e, p) != 1) continue;
                uint pvaddr = U32(e, p + 8);
                uint pmemsz = U32(e, p + 20);
                uint pflags = U32(e, p + 24);   // PF_X=1 PF_W=2 PF_R=4
                long segStart = (delta + pvaddr) & ~(long)(PAGE - 1);
                long segEnd = ((delta + pvaddr + pmemsz) + PAGE - 1) & ~(long)(PAGE - 1);
                int prot = ((pflags & 4) != 0 ? PROT_READ : 0) | ((pflags & 2) != 0 ? PROT_WRITE : 0) | ((pflags & 1) != 0 ? PROT_EXEC : 0);
                int rc = mprotect((IntPtr)segStart, (IntPtr)(segEnd - segStart), prot);
                if (rc != 0 && (pflags & 1) != 0)
                { munmap(map, (IntPtr)span); falha = "mprotect +EXEC negado (errno=" + Marshal.GetLastWin32Error() + ") -> UEP tambem barra anon exec"; return false; }
            }
            try { syscall_cache(ARM_NR_cacheflush, map, (IntPtr)(map.ToInt64() + span), 0); } catch { }

            try
            {
                if (initFn != 0) Marshal.GetDelegateForFunctionPointer<InitFn>((IntPtr)initFn)();
                for (long a = 0; a < initArraySz; a += 4)
                {
                    long fnp = (uint)Marshal.ReadInt32((IntPtr)(initArray + a));
                    if (fnp != 0 && fnp != -1) Marshal.GetDelegateForFunctionPointer<InitFn>((IntPtr)fnp)();
                }
            }
            catch (Exception ex) { /* init pode chamar GL antes do contexto; segue */ System.Diagnostics.Debug.WriteLine(ex.Message); }

            // Liga NvLib.* pelo dynsym local. Guarda o mapa vivo antes, para o
            // Libera nao rodar se um simbolo faltar (mantemos evidencia).
            elfMap = map; elfSpan = span;
            if (!LigaSimbolos(s => (IntPtr)ResolveLocal(s, symtab, strtab, hash, delta), out string qual))
            {
                falha = "carregado, mas simbolo faltou no dynsym: " + qual;
                return false;
            }
            return true;
        }

        // Aponta os 5 delegates de NvLib para o codigo nativo. resolve(nome)
        // devolve o endereco de cada simbolo (dlsym na rota 1, dynsym na rota 2).
        static bool LigaSimbolos(Func<string, IntPtr> resolve, out string qualFaltou)
        {
            qualFaltou = null;
            IntPtr pIni = resolve("nv_tpk_iniciar");
            IntPtr pCfg = resolve("nv_tpk_config");
            IntPtr pQd = resolve("nv_tpk_quadro");
            IntPtr pErr = resolve("nv_tpk_erro");
            IntPtr pTec = resolve("nv_tpk_tecla");
            if (pIni == IntPtr.Zero) { qualFaltou = "nv_tpk_iniciar"; return false; }
            if (pCfg == IntPtr.Zero) { qualFaltou = "nv_tpk_config"; return false; }
            if (pQd == IntPtr.Zero) { qualFaltou = "nv_tpk_quadro"; return false; }
            if (pErr == IntPtr.Zero) { qualFaltou = "nv_tpk_erro"; return false; }
            if (pTec == IntPtr.Zero) { qualFaltou = "nv_tpk_tecla"; return false; }
            NvLib.Iniciar = Marshal.GetDelegateForFunctionPointer<NvLib.IniciarDel>(pIni);
            NvLib.Config = Marshal.GetDelegateForFunctionPointer<NvLib.ConfigDel>(pCfg);
            NvLib.Quadro = Marshal.GetDelegateForFunctionPointer<NvLib.QuadroDel>(pQd);
            NvLib.Erro = Marshal.GetDelegateForFunctionPointer<NvLib.ErroDel>(pErr);
            NvLib.Tecla = Marshal.GetDelegateForFunctionPointer<NvLib.TeclaDel>(pTec);
            // Os pontos de entrada de video/log (chamados de Video.cs) tambem
            // passam a apontar para o mesmo codigo nativo. Na rota memfd isto e
            // redundante (a lib esta no link map e o DllImport ja resolveria),
            // mas mantem UM caminho de chamada nas duas rotas; na rota ELF e
            // obrigatorio, pois o DllImport-por-soname nao resolveria.
            NvVid.Ligar(resolve);
            return NvLib.Pronto;
        }

        static void RelocaBloco(long tabela, long tamBytes, long ent, long symtab, long strtab, long delta)
        {
            if (tabela == 0 || tamBytes == 0) return;
            if (ent < 8) ent = 8;
            for (long o = 0; o < tamBytes; o += ent)
            {
                long r = tabela + o;
                uint rOffset = (uint)Marshal.ReadInt32((IntPtr)r);
                uint rInfo = (uint)Marshal.ReadInt32((IntPtr)(r + 4));
                int type = (int)(rInfo & 0xff);
                int symidx = (int)(rInfo >> 8);
                IntPtr where = (IntPtr)(delta + rOffset);
                switch (type)
                {
                    case 23: // R_ARM_RELATIVE: *where += B
                        Marshal.WriteInt32(where, (int)((uint)Marshal.ReadInt32(where) + (uint)delta));
                        break;
                    case 2:  // R_ARM_ABS32: S + A
                    case 21: // R_ARM_GLOB_DAT: S (+A)
                    {
                        uint s = (uint)ResolveSimbolo(symidx, symtab, strtab, delta);
                        uint a = (uint)Marshal.ReadInt32(where);
                        Marshal.WriteInt32(where, (int)(s + a));
                        break;
                    }
                    case 22: // R_ARM_JUMP_SLOT: S
                    {
                        uint s = (uint)ResolveSimbolo(symidx, symtab, strtab, delta);
                        Marshal.WriteInt32(where, (int)s);
                        break;
                    }
                }
            }
        }

        static long ResolveSimbolo(int symidx, long symtab, long strtab, long delta)
        {
            long sym = symtab + (long)symidx * 16;
            uint stName = (uint)Marshal.ReadInt32((IntPtr)sym);
            uint stValue = (uint)Marshal.ReadInt32((IntPtr)(sym + 4));
            ushort stShndx = (ushort)Marshal.ReadInt16((IntPtr)(sym + 14));
            if (stShndx != 0) return delta + stValue;   // definido aqui
            string nome = LeCStr(strtab + stName);
            if (string.IsNullOrEmpty(nome)) return 0;
            IntPtr p = dlsym(RTLD_DEFAULT, nome);
            return p.ToInt64();                          // 0 se nao achou (weak)
        }

        static long ResolveLocal(string nome, long symtab, long strtab, long hash, long delta)
        {
            int count = 0;
            if (hash != 0) count = Marshal.ReadInt32((IntPtr)(hash + 4)); // nchain
            if (count <= 0 || count > 100000) count = 4096;              // fallback
            for (int i = 0; i < count; i++)
            {
                long sym = symtab + (long)i * 16;
                uint stName = (uint)Marshal.ReadInt32((IntPtr)sym);
                uint stValue = (uint)Marshal.ReadInt32((IntPtr)(sym + 4));
                ushort stShndx = (ushort)Marshal.ReadInt16((IntPtr)(sym + 14));
                if (stShndx == 0 || stValue == 0) continue;
                if (LeCStr(strtab + stName) == nome) return delta + stValue;
            }
            return 0;
        }

        static string LeCStr(long addr)
        {
            var sb = new System.Text.StringBuilder();
            for (int i = 0; i < 256; i++)
            {
                byte b = Marshal.ReadByte((IntPtr)(addr + i));
                if (b == 0) break;
                sb.Append((char)b);
            }
            return sb.ToString();
        }

        static uint U32(byte[] b, int o) => (uint)(b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | (b[o + 3] << 24));
        static ushort U16(byte[] b, int o) => (ushort)(b[o] | (b[o + 1] << 8));

        static string Err()
        {
            IntPtr e = dlerror();
            string s = e == IntPtr.Zero ? null : Marshal.PtrToStringAnsi(e);
            return string.IsNullOrEmpty(s) ? "recusado sem mensagem" : s;
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
