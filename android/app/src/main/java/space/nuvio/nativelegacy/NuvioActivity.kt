package space.nuvio.nativelegacy

import android.content.Intent
import android.graphics.PixelFormat
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Process
import android.provider.Settings
import android.speech.RecognizerIntent
import android.system.Os
import android.view.KeyEvent
import android.view.SurfaceHolder
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.core.content.FileProvider
import org.libsdl.app.SDLActivity
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

// Ponte entre o Android e o nucleo C (libmain.so): prepara ambiente e arquivos
// ANTES do SDL subir, traduz teclas de controle remoto e poe a camada de video
// do ExoPlayer atras da superficie GLES do SDL.
class NuvioActivity : SDLActivity() {

    // SDL2 e compartilhada; SDL2_image e SDL2_ttf entram estaticas na libmain.
    override fun getLibraries(): Array<String> = arrayOf("SDL2", "main")

    private var camadaVideo: FrameLayout? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        prepararAmbiente()
        super.onCreate(savedInstanceState)

        // SDLActivity.mLayout e um RelativeLayout com a SDLSurface dentro.
        // O video fica no indice 0 (atras); a SDLSurface fica por cima, em
        // formato translucido, e o C desenha alfa 0 no "furo" do player.
        val camada = FrameLayout(this)
        camadaVideo = camada
        mLayout.addView(
            camada, 0,
            ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
        )
        mSurface.setZOrderMediaOverlay(true)
        mSurface.holder.setFormat(PixelFormat.TRANSLUCENT)
        NvPlayer.iniciar(this, camada)
    }

    // Chamado pelo C (android_pedir_superficie), do fio do SDL, antes de criar a
    // janela: fixa o buffer da SDLSurface em w x h e espera a superficie nova
    // chegar (ate 2 s). Devolve true se ela veio nesse tamanho.
    fun pedirSuperficie(w: Int, h: Int): Boolean {
        val chegou = CountDownLatch(1)
        var ok = false
        runOnUiThread {
            val holder = mSurface.holder
            val atual = holder.surfaceFrame
            if (atual.width() == w && atual.height() == h) { ok = true; chegou.countDown(); return@runOnUiThread }
            holder.addCallback(object : SurfaceHolder.Callback {
                override fun surfaceCreated(hd: SurfaceHolder) {}
                override fun surfaceDestroyed(hd: SurfaceHolder) {}
                override fun surfaceChanged(hd: SurfaceHolder, f: Int, ww: Int, hh: Int) {
                    if (ww == w && hh == h) { ok = true; hd.removeCallback(this); chegou.countDown() }
                }
            })
            holder.setFixedSize(w, h)
        }
        chegou.await(2, TimeUnit.SECONDS)
        return ok
    }

    // DESPEDIDA (dados_despedida_ler, dados.c). Escondido, o app pode ser morto
    // pelo Android sem saida limpa; o arquivo diz ao proximo arranque que isso
    // nao foi queda, e o modo seguro nao desfaz ajustes por causa dela.
    private fun despedida() = File(filesDir, "dados/despedida.txt")

    private var instalando = false

    // Chamado pelo C (android_instalar_apk, atualizacao.c), do fio do SDL.
    // 1 = instalador aberto, 2 = falta a permissao "instalar apps desta fonte"
    // (abre a tela dela), 0 = falhou. Nao bloqueia: o resultado e do sistema.
    fun instalarApk(caminho: String): Int {
        return try {
            val arq = File(caminho)
            if (!arq.isFile) return 0
            if (Build.VERSION.SDK_INT >= 26 && !packageManager.canRequestPackageInstalls()) {
                startActivity(
                    Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES, Uri.parse("package:$packageName"))
                        .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                )
                return 2
            }
            val uri = FileProvider.getUriForFile(this, "space.nuvio.nativelegacy.atualizacao", arq)
            instalando = true
            startActivity(
                Intent(Intent.ACTION_VIEW)
                    .setDataAndType(uri, "application/vnd.android.package-archive")
                    .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_ACTIVITY_NEW_TASK)
            )
            1
        } catch (e: Exception) {
            instalando = false
            0
        }
    }

    // So apaga ao VOLTAR com o processo vivo: no primeiro onStart quem le (e
    // apaga) a despedida da sessao anterior e o C, no arranque.
    private var jaComecou = false

    override fun onStart() {
        super.onStart()
        if (jaComecou) {
            val f = despedida()
            if (f.exists()) {
                val t = f.readText()
                // "fim" so sobra aqui se o instalador foi cancelado (o C a grava
                // antes de entregar o APK): desfaz, senao uma queda futura
                // pareceria saida limpa.
                if (t.startsWith("oculto") || (instalando && t.startsWith("fim"))) f.delete()
            }
            instalando = false
        }
        jaComecou = true
    }

    override fun onStop() {
        try {
            val f = despedida()
            if (!(f.exists() && f.readText().startsWith("fim"))) f.writeText("oculto\n")
        } catch (_: Exception) {}
        super.onStop()
    }

    override fun onPause() {
        NvPlayer.pausarPeloSistema()
        super.onPause()
    }

    override fun onDestroy() {
        NvPlayer.encerrar()
        val saindo = isFinishing
        super.onDestroy()
        // O nucleo C guarda estado global: sair do app e matar o processo, para
        // a proxima abertura nascer limpa (o SDL ja pediu finish quando o main voltou).
        if (saindo) Process.killProcess(Process.myPid())
    }

    // Variaveis que o C le (contrato do porte Android). Tem de rodar antes do
    // super.onCreate: o SDL abre o main numa thread logo depois.
    private fun prepararAmbiente() {
        val dados = File(filesDir, "dados").apply { mkdirs() }
        val res = File(filesDir, "res")
        extrairAssets(res)
        val versao = try {
            packageManager.getPackageInfo(packageName, 0).versionName ?: ""
        } catch (e: Exception) { "" }
        fun env(k: String, v: String) = Os.setenv(k, v, true)
        env("NUVIO_DADOS", dados.path)
        env("NUVIO_LOG", File(dados, "nuvio.log").path)
        env("NUVIO_LOG_ANTERIOR", File(dados, "nuvio-anterior.log").path)
        env("HOME", dados.path)
        env("NUVIO_ARTE", File(res, "art").path)
        env("NUVIO_LOCALE", java.util.Locale.getDefault().toLanguageTag())
        env("NUVIO_TV_INFO", "${Build.MANUFACTURER} ${Build.MODEL}|${Build.VERSION.SDK_INT}|${Build.VERSION.RELEASE}|$versao")
    }

    // Copia assets art/ e fonts/ para filesDir/res, uma vez por versionCode.
    private fun extrairAssets(res: File) {
        val marca = File(res, ".versao")
        val atual = try {
            packageManager.getPackageInfo(packageName, 0).let {
                @Suppress("DEPRECATION") it.versionCode.toString()
            }
        } catch (e: Exception) { "0" }
        if (marca.exists() && marca.readText() == atual && File(res, "art").isDirectory) return
        res.deleteRecursively()
        res.mkdirs()
        for (pasta in arrayOf("art", "fonts")) copiarAsset(pasta, File(res, pasta))
        marca.writeText(atual)
    }

    private fun copiarAsset(caminho: String, destino: File) {
        val filhos = assets.list(caminho) ?: emptyArray()
        if (filhos.isEmpty()) {
            // folha: arquivo (ou pasta vazia, que assets.list tambem devolve vazia)
            try {
                destino.parentFile?.mkdirs()
                assets.open(caminho).use { i -> destino.outputStream().use { o -> i.copyTo(o, 64 * 1024) } }
            } catch (e: java.io.FileNotFoundException) {
                destino.mkdirs()
            }
            return
        }
        destino.mkdirs()
        for (f in filhos) copiarAsset("$caminho/$f", File(destino, f))
    }

    // DITADO DO SPOTLIGHT (android_ditado_iniciar/ler, src/android.c). A tela de
    // voz e a do sistema (Google na maioria das TVs): quem grava e ela, entao o
    // app nao pede RECORD_AUDIO. O resultado fica aqui ate o C ler.
    @Volatile private var ditado: String? = null
    private val PEDIDO_DITADO = 4711

    // Chamado pelo C, do fio do SDL. false = nao ha reconhecedor de voz.
    fun ditar(): Boolean {
        val i = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH)
            .putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_WEB_SEARCH)
            .putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 1)
        if (i.resolveActivity(packageManager) == null) return false
        ditado = null
        return try {
            runOnUiThread {
                @Suppress("DEPRECATION")
                try { startActivityForResult(i, PEDIDO_DITADO) } catch (e: Exception) { ditado = "!" }
            }
            true
        } catch (e: Exception) { false }
    }

    // "=texto", "!" (voltou sem nada) ou null (ainda ouvindo). Consome.
    fun ditadoLer(): String? {
        val d = ditado ?: return null
        ditado = null
        return d
    }

    @Deprecated("startActivityForResult e o que o SDLActivity (Activity) oferece")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (requestCode == PEDIDO_DITADO) {
            val t = if (resultCode == RESULT_OK)
                data?.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS)?.firstOrNull() else null
            ditado = if (t.isNullOrBlank()) "!" else "=$t"
            return
        }
        @Suppress("DEPRECATION")
        super.onActivityResult(requestCode, resultCode, data)
    }

    // Controle remoto: troca a tecla ANTES do SDL, para cair no SDLK que o app espera.
    override fun dispatchKeyEvent(ev: KeyEvent): Boolean {
        val novo = when (ev.keyCode) {
            KeyEvent.KEYCODE_DPAD_CENTER, KeyEvent.KEYCODE_NUMPAD_ENTER -> KeyEvent.KEYCODE_ENTER
            KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE, KeyEvent.KEYCODE_MEDIA_PLAY,
            KeyEvent.KEYCODE_MEDIA_PAUSE -> KeyEvent.KEYCODE_BREAK
            KeyEvent.KEYCODE_MEDIA_STOP -> KeyEvent.KEYCODE_BACK
            KeyEvent.KEYCODE_MEDIA_FAST_FORWARD, KeyEvent.KEYCODE_MEDIA_NEXT -> KeyEvent.KEYCODE_DPAD_RIGHT
            KeyEvent.KEYCODE_MEDIA_REWIND, KeyEvent.KEYCODE_MEDIA_PREVIOUS -> KeyEvent.KEYCODE_DPAD_LEFT
            KeyEvent.KEYCODE_PROG_BLUE -> KeyEvent.KEYCODE_S
            // Info (i) = registro (enviar o log), como a vermelha/verde.
            KeyEvent.KEYCODE_PROG_RED, KeyEvent.KEYCODE_PROG_GREEN,
            KeyEvent.KEYCODE_INFO -> KeyEvent.KEYCODE_F9
            // CH+/CH- vao como F7/F8 e o C decide (main.c): troca de canal com
            // canal na tela; fora disso, CH+ = AZUL e CH- = Spotlight, porque a
            // TCL e a maioria dos controles Android TV nao tem teclas coloridas
            // e o microfone da TCL manda ASSIST (abre o Gemini, nao chega aqui).
            KeyEvent.KEYCODE_CHANNEL_UP -> KeyEvent.KEYCODE_F7
            KeyEvent.KEYCODE_CHANNEL_DOWN -> KeyEvent.KEYCODE_F8
            // SPOTLIGHT (spotlight.h). O botao de microfone da maioria dos
            // controles Android TV manda SEARCH, que o sistema entrega ao app:
            // vira F6 (abrir + ditado). ASSIST e VOICE_ASSIST o sistema NAO
            // entrega (KeyEvent: "not delivered to applications"). A amarela
            // vira F5 (so abrir).
            KeyEvent.KEYCODE_SEARCH -> KeyEvent.KEYCODE_F6
            KeyEvent.KEYCODE_PROG_YELLOW -> KeyEvent.KEYCODE_F5
            else -> return super.dispatchKeyEvent(ev)
        }
        return super.dispatchKeyEvent(
            KeyEvent(
                ev.downTime, ev.eventTime, ev.action, novo, ev.repeatCount, ev.metaState,
                ev.deviceId, ev.scanCode, ev.flags, ev.source
            )
        )
    }
}
