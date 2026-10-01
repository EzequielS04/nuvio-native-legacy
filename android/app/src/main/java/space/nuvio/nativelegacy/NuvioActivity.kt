package space.nuvio.nativelegacy

import android.graphics.PixelFormat
import android.os.Build
import android.os.Bundle
import android.os.Process
import android.system.Os
import android.view.KeyEvent
import android.view.ViewGroup
import android.widget.FrameLayout
import org.libsdl.app.SDLActivity
import java.io.File

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

    // Controle remoto: troca a tecla ANTES do SDL, para cair no SDLK que o app espera.
    override fun dispatchKeyEvent(ev: KeyEvent): Boolean {
        val novo = when (ev.keyCode) {
            KeyEvent.KEYCODE_DPAD_CENTER, KeyEvent.KEYCODE_NUMPAD_ENTER -> KeyEvent.KEYCODE_ENTER
            KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE, KeyEvent.KEYCODE_MEDIA_PLAY,
            KeyEvent.KEYCODE_MEDIA_PAUSE -> KeyEvent.KEYCODE_BREAK
            KeyEvent.KEYCODE_MEDIA_STOP -> KeyEvent.KEYCODE_BACK
            KeyEvent.KEYCODE_MEDIA_FAST_FORWARD, KeyEvent.KEYCODE_MEDIA_NEXT -> KeyEvent.KEYCODE_DPAD_RIGHT
            KeyEvent.KEYCODE_MEDIA_REWIND, KeyEvent.KEYCODE_MEDIA_PREVIOUS -> KeyEvent.KEYCODE_DPAD_LEFT
            KeyEvent.KEYCODE_PROG_BLUE, KeyEvent.KEYCODE_CHANNEL_UP -> KeyEvent.KEYCODE_S
            // CH- abre o painel de registro: a TCL e a maioria dos controles
            // Android TV nao tem as teclas coloridas (pedido do dono, 30/09).
            KeyEvent.KEYCODE_PROG_RED, KeyEvent.KEYCODE_PROG_GREEN,
            KeyEvent.KEYCODE_CHANNEL_DOWN -> KeyEvent.KEYCODE_F9
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
