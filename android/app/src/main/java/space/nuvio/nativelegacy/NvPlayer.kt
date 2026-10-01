package space.nuvio.nativelegacy

import android.app.Activity
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
import android.view.Gravity
import android.view.SurfaceView
import android.view.View
import android.widget.FrameLayout
import androidx.media3.common.AudioAttributes
import androidx.media3.common.C
import androidx.media3.common.MediaItem
import androidx.media3.common.MimeTypes
import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import androidx.media3.common.Timeline
import androidx.media3.common.TrackSelectionOverride
import androidx.media3.common.Tracks
import androidx.media3.common.VideoSize
import androidx.media3.common.text.CueGroup
import androidx.media3.common.util.UnstableApi
import androidx.media3.datasource.DefaultDataSource
import androidx.media3.datasource.DefaultHttpDataSource
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.exoplayer.analytics.AnalyticsListener
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory

// Player do Nuvio no Android TV: Media3 ExoPlayer numa SurfaceView ATRAS da
// SDLSurface (o C abre um furo transparente por onde ela aparece, ver
// src/video_android.c). Mesma ABI do host .NET do .tpk (tizen-tpk/Video.cs):
// o C pede por abrir/parar/pausar/buscar/volume/janela/escolher de QUALQUER
// fio e tudo aqui passa ao fio principal; o que acontece volta ao C pelos
// natives, sempre do fio principal.
//
// Sessao monotonica: cada abrir/parar sobe `sessao`, e todo callback confere o
// numero em que nasceu. Um player velho nao fala mais depois de liberado.
//
// Eventos (nativeEvento): 1 PRONTO(durMs) 2 TOCANDO 3 PAUSADO 4 FIM 5 ERRO(cod)
// 6 TAMANHO(w,h) 7 BUFFER(pct) 8 PRIMEIRO_QUADRO, e a extensao 9 = a fonte tem
// audio mas nenhuma faixa tem decoder aqui (o C responde video_audio_nao_suportado).
//
// Tipo 2 do escolher() = atraso de legenda em ms: fica GUARDADO aqui e vale
// adiando a entrega de cada cue ao C (so atraso positivo; adiantar um cue que
// ainda nao chegou nao da). O estilo inteiro e do C.
@androidx.annotation.OptIn(UnstableApi::class)
object NvPlayer {
    private const val TAG = "NvPlayer"
    private const val EV_PRONTO = 1
    private const val EV_TOCANDO = 2
    private const val EV_PAUSADO = 3
    private const val EV_FIM = 4
    private const val EV_ERRO = 5
    private const val EV_TAMANHO = 6
    private const val EV_BUFFER = 7
    private const val EV_PRIMEIRO_QUADRO = 8
    private const val EV_AUDIO_SEM_DECODER = 9

    private const val TELA_W = 1920   // coordenadas de layout do app
    private const val TELA_H = 1080
    private const val FAIXAS_MAX = 32 // NV_FAIXA_MAX do C
    private const val TIQUE_MS = 250L
    private const val RETRY_DECODER_MS = 5000L

    private val principal = Handler(Looper.getMainLooper())
    private var activity: Activity? = null
    private var camada: FrameLayout? = null
    private var superficie: SurfaceView? = null

    // Tudo abaixo so no fio principal.
    private var player: ExoPlayer? = null
    private var sessao = 0
    private var urlAtual = ""
    private var cabAtual = ""
    private var abriuEm = 0L
    private var retentou = false
    private var pronto = false
    private var duracaoEnviada = -1
    private var videoW = 0
    private var videoH = 0
    private var temJanela = false
    private var jx = 0
    private var jy = 0
    private var jw = TELA_W
    private var jh = TELA_H
    private var atrasoMs = 0
    private var audios = ArrayList<Pair<Tracks.Group, Int>>()
    private var legendas = ArrayList<Pair<Tracks.Group, Int>>()
    private var assinatura = ""
    private var decoderDv = false
    private var ultHdr = ""
    private var ultDv = -1
    private var ultAtmos = -1
    private var avisouSemAudio = false

    // --- natives (src/video_android.c, Java_space_nuvio_nativelegacy_NvPlayer_*) ---
    @JvmStatic external fun nativeIniciar()
    @JvmStatic external fun nativeEvento(tipo: Int, a: Int, b: Int)
    @JvmStatic external fun nativeFaixa(tipo: Int, idx: Int, lingua: String)
    @JvmStatic external fun nativeFaixasFim(selAudio: Int, selLeg: Int)
    @JvmStatic external fun nativeLegenda(texto: String, durMs: Int)
    @JvmStatic external fun nativePos(ms: Int)
    @JvmStatic external fun nativeHdr(hdr: String, dv: Int, atmos: Int)

    private fun ev(tipo: Int, a: Int = 0, b: Int = 0) {
        try { nativeEvento(tipo, a, b) } catch (e: UnsatisfiedLinkError) { Log.w(TAG, "evento $tipo sem lib: $e") }
    }

    // --- ciclo de vida (NuvioActivity) ---------------------------------------

    // onCreate, DEPOIS do super.onCreate (a libmain.so ja carregada pelo SDL).
    @JvmStatic
    fun iniciar(activity: Activity, camada: FrameLayout) {
        this.activity = activity
        this.camada = camada
        val sv = SurfaceView(activity)
        sv.visibility = View.GONE
        camada.addView(sv, FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT))
        superficie = sv
        // A camada so tem tamanho depois do layout: reaplica a janela quando mudar.
        camada.addOnLayoutChangeListener { _, l, t, r, b, ol, ot, or2, ob ->
            if (r - l != or2 - ol || b - t != ob - ot) aplicarJanela()
        }
        try { nativeIniciar() } catch (e: UnsatisfiedLinkError) {
            Log.w(TAG, "nativeIniciar sem a lib ainda; o C acha a classe pelo ClassLoader da Activity: $e")
        }
    }

    // onPause: pausa (o C fica sabendo pelo evento 3) e guarda a posicao; o
    // player continua vivo para o onResume nao precisar de nada.
    @JvmStatic
    fun pausarPeloSistema() {
        try { player?.pause() } catch (e: Exception) { Log.w(TAG, "pausarPeloSistema: $e") }
    }

    // onDestroy: solta player e superficie.
    @JvmStatic
    fun encerrar() {
        liberar()
        val sv = superficie
        if (sv != null) (sv.parent as? FrameLayout)?.removeView(sv)
        superficie = null
        camada = null
        activity = null
    }

    // --- chamadas do C (qualquer fio) -----------------------------------------

    @JvmStatic fun abrir(url: String, cabecalhos: String) { principal.post { abrirMain(url, cabecalhos, false) } }
    @JvmStatic fun parar() { principal.post { liberar() } }
    @JvmStatic fun pausar(p: Int) { principal.post { player?.playWhenReady = (p == 0) } }
    @JvmStatic fun buscar(ms: Int) { principal.post { player?.seekTo(ms.toLong()) } }
    @JvmStatic fun volume(pct: Int) { principal.post { player?.volume = pct.coerceIn(0, 100) / 100f } }
    @JvmStatic fun janela(x: Int, y: Int, w: Int, h: Int, encaixa: Int) { principal.post { definirJanela(x, y, w, h, encaixa != 0) } }
    @JvmStatic fun escolher(tipo: Int, idx: Int) { principal.post { escolherMain(tipo, idx) } }

    // --- abrir / liberar ------------------------------------------------------

    private fun abrirMain(url: String, cabecalhos: String, reabrindo: Boolean) {
        val act = activity ?: return
        liberar()
        val minha = sessao
        urlAtual = url
        cabAtual = cabecalhos
        abriuEm = SystemClock.elapsedRealtime()
        if (!reabrindo) retentou = false
        try {
            val http = DefaultHttpDataSource.Factory()
                .setAllowCrossProtocolRedirects(true)
                .setConnectTimeoutMs(15000)
                .setReadTimeoutMs(20000)
            val props = HashMap<String, String>()
            var ua: String? = null
            for (linha in cabecalhos.split('\n')) {
                val i = linha.indexOf(':')
                if (i <= 0) continue
                val nome = linha.substring(0, i).trim()
                val valor = linha.substring(i + 1).trim()
                if (nome.isEmpty()) continue
                if (nome.equals("User-Agent", ignoreCase = true)) { http.setUserAgent(valor); ua = valor } else props[nome] = valor
            }
            http.setDefaultRequestProperties(props)
            // Arquivo progressivo vem em varias conexoes (ParaleloDataSource.kt:
            // o Android limita a janela TCP de cada uma); HLS/DASH seguem na unica.
            val rede = ParaleloDataSource.Factory(ua, props, http)

            val p = ExoPlayer.Builder(act)
                .setMediaSourceFactory(DefaultMediaSourceFactory(act)
                    // DefaultDataSource e nao so http: o trailer da Apple chega como
                    // file:// (master reduzido a uma variante em dados/trailer, trailerapple.c).
                    .setDataSourceFactory(DefaultDataSource.Factory(act, rede)))
                .build()
            player = p
            // Foco de audio GAIN; perder o foco pausa (o C ve o evento 3).
            p.setAudioAttributes(
                AudioAttributes.Builder().setUsage(C.USAGE_MEDIA).setContentType(C.AUDIO_CONTENT_TYPE_MOVIE).build(), true)
            // Legenda desligada ate o app escolher: quem desenha e o C.
            p.trackSelectionParameters = p.trackSelectionParameters.buildUpon()
                .setTrackTypeDisabled(C.TRACK_TYPE_TEXT, true).build()
            p.addListener(ouvinte(minha))
            p.addAnalyticsListener(analitico(minha))

            val sv = superficie
            if (sv != null) {
                sv.visibility = View.VISIBLE
                p.setVideoSurfaceView(sv)
            }
            temJanela = false
            aplicarEncaixe()

            val item = MediaItem.Builder().setUri(url)
            val baixa = url.lowercase()
            if (baixa.contains(".m3u8") || baixa.contains("m3u8?")) item.setMimeType(MimeTypes.APPLICATION_M3U8)
            else if (baixa.contains(".mpd")) item.setMimeType(MimeTypes.APPLICATION_MPD)
            p.setMediaItem(item.build())
            p.playWhenReady = true
            p.prepare()
            principal.postDelayed(tique(minha), TIQUE_MS)
        } catch (e: Exception) {
            Log.w(TAG, "abrir: $e")
            ev(EV_ERRO, -1, 0)
        }
    }

    // Sobe a sessao e solta tudo; qualquer callback pendente do player velho
    // morre na conferencia do numero.
    private fun liberar() {
        sessao++
        val p = player
        player = null
        pronto = false
        duracaoEnviada = -1
        videoW = 0; videoH = 0
        temJanela = false
        assinatura = ""
        audios = ArrayList(); legendas = ArrayList()
        decoderDv = false
        ultHdr = ""; ultDv = -1; ultAtmos = -1
        avisouSemAudio = false
        if (p != null) {
            try { p.stop() } catch (e: Exception) { }
            try { p.clearVideoSurface() } catch (e: Exception) { }
            try { p.release() } catch (e: Exception) { Log.w(TAG, "release: $e") }
        }
        superficie?.visibility = View.GONE
    }

    // Tique de 250 ms: a posicao que o C le sem esperar ninguem.
    private fun tique(minha: Int): Runnable = object : Runnable {
        override fun run() {
            if (minha != sessao) return
            val p = player ?: return
            try { nativePos(p.currentPosition.coerceIn(0L, Int.MAX_VALUE.toLong()).toInt()) } catch (e: UnsatisfiedLinkError) { }
            principal.postDelayed(this, TIQUE_MS)
        }
    }

    // --- janela ---------------------------------------------------------------

    // Retangulo em coordenadas de layout 1920x1080; aceita origem NEGATIVA e
    // tamanho maior que a tela (zoom): a camada recorta o excedente.
    // `encaixa`: janela LISA do nucleo — o quadro encaixa no retangulo com
    // tarja, como o plano da LG e o LetterBox da Samsung. Sem isso o trailer
    // em "Original" (Apple 2,4:1) saia esticado em 16:9. Recorte (encaixa =
    // false) ja vem com a proporcao certa e e aplicado exato.
    private var pedX = 0; private var pedY = 0; private var pedW = 0; private var pedH = 0
    private var pedEncaixa = true

    private fun definirJanela(x: Int, y: Int, w: Int, h: Int, encaixa: Boolean) {
        if (player == null) return   // como o Video.cs: o C repete depois do videoInfo
        pedX = x; pedY = y; pedW = w; pedH = h; pedEncaixa = encaixa
        temJanela = true
        calcularJanela()
    }

    private fun calcularJanela() {
        if (pedEncaixa && videoW > 0 && videoH > 0 && pedW > 0 && pedH > 0) {
            val esc = minOf(pedW.toFloat() / videoW, pedH.toFloat() / videoH)
            jw = (videoW * esc + 0.5f).toInt()
            jh = (videoH * esc + 0.5f).toInt()
            jx = pedX + (pedW - jw) / 2
            jy = pedY + (pedH - jh) / 2
        } else {
            jx = pedX; jy = pedY; jw = pedW; jh = pedH
        }
        aplicarJanela()
    }

    // Sem janela pedida ainda: o quadro inteiro encaixado (letterbox) na tela.
    private fun aplicarEncaixe() {
        if (temJanela) return
        if (videoW > 0 && videoH > 0) {
            val esc = minOf(TELA_W.toFloat() / videoW, TELA_H.toFloat() / videoH)
            jw = (videoW * esc + 0.5f).toInt()
            jh = (videoH * esc + 0.5f).toInt()
            jx = (TELA_W - jw) / 2
            jy = (TELA_H - jh) / 2
        } else {
            jx = 0; jy = 0; jw = TELA_W; jh = TELA_H
        }
        aplicarJanela()
    }

    private fun aplicarJanela() {
        val c = camada ?: return
        val sv = superficie ?: return
        var cw = c.width
        var ch = c.height
        if (cw < 1 || ch < 1) {
            val m = c.resources.displayMetrics
            cw = m.widthPixels; ch = m.heightPixels
        }
        val ex = cw.toFloat() / TELA_W
        val ey = ch.toFloat() / TELA_H
        // As BORDAS arredondam e o tamanho sai da diferenca: tela cheia cai
        // exatamente em 0,0,cw,ch, sem fresta.
        val x0 = Math.round(jx * ex)
        val y0 = Math.round(jy * ey)
        val x1 = Math.round((jx + jw) * ex)
        val y1 = Math.round((jy + jh) * ey)
        val lp = FrameLayout.LayoutParams(maxOf(1, x1 - x0), maxOf(1, y1 - y0))
        lp.gravity = Gravity.TOP or Gravity.START
        lp.leftMargin = x0
        lp.topMargin = y0
        val antes = sv.layoutParams as? FrameLayout.LayoutParams
        val mudou = antes == null || antes.width != lp.width || antes.height != lp.height ||
            antes.leftMargin != lp.leftMargin || antes.topMargin != lp.topMargin
        sv.layoutParams = lp
        // A TCL PRENDE A GEOMETRIA DO PLANO DE VIDEO: aplica o primeiro tamanho
        // e posicao da superficie e ignora as mudancas seguintes (o dono: "entra
        // recortado e nao sai; entra no esticar e fica esticado"). Recriar a
        // Surface (GONE -> VISIBLE) faz o compositor montar camada nova com a
        // geometria nova; o ExoPlayer troca a saida do decoder sem recarregar.
        if (mudou && player != null && sv.visibility == View.VISIBLE && sv.holder.surface?.isValid == true) {
            sv.visibility = View.GONE
            principal.post { if (player != null) sv.visibility = View.VISIBLE }
        }
    }

    // --- escolha de faixa ----------------------------------------------------

    private fun escolherMain(tipo: Int, idx: Int) {
        if (tipo == 2) { atrasoMs = idx; return }
        val p = player ?: return
        try {
            val par = p.trackSelectionParameters.buildUpon()
            if (tipo == 0) {
                val (g, i) = audios.getOrNull(idx) ?: return
                par.setOverrideForType(TrackSelectionOverride(g.mediaTrackGroup, i))
            } else if (tipo == 1) {
                if (idx < 0) {
                    par.setTrackTypeDisabled(C.TRACK_TYPE_TEXT, true).clearOverridesOfType(C.TRACK_TYPE_TEXT)
                } else {
                    val (g, i) = legendas.getOrNull(idx) ?: return
                    par.setTrackTypeDisabled(C.TRACK_TYPE_TEXT, false)
                        .setOverrideForType(TrackSelectionOverride(g.mediaTrackGroup, i))
                }
            } else return
            p.trackSelectionParameters = par.build()
        } catch (e: Exception) { Log.w(TAG, "escolher $tipo/$idx: $e") }
    }

    // --- eventos do player ----------------------------------------------------

    private fun ouvinte(minha: Int) = object : Player.Listener {
        override fun onPlaybackStateChanged(state: Int) {
            if (minha != sessao) return
            val p = player ?: return
            when (state) {
                Player.STATE_BUFFERING -> ev(EV_BUFFER, 0)
                Player.STATE_READY -> {
                    ev(EV_BUFFER, 100)
                    if (!pronto) {
                        pronto = true
                        duracaoEnviada = duracaoMs(p)
                        ev(EV_PRONTO, duracaoEnviada)
                        if (p.isPlaying) ev(EV_TOCANDO)
                    }
                }
                Player.STATE_ENDED -> ev(EV_FIM)
                else -> {}
            }
        }

        override fun onIsPlayingChanged(isPlaying: Boolean) {
            if (minha != sessao) return
            val p = player ?: return
            if (isPlaying) { if (pronto) ev(EV_TOCANDO) }
            // Parar de tocar por buffer nao e pausa (o evento 7 ja disse).
            else if (!p.playWhenReady && p.playbackState != Player.STATE_ENDED) ev(EV_PAUSADO)
        }

        override fun onTimelineChanged(timeline: Timeline, reason: Int) {
            if (minha != sessao || !pronto) return
            val p = player ?: return
            val d = duracaoMs(p)
            if (d != duracaoEnviada) { duracaoEnviada = d; ev(EV_PRONTO, d) }   // so a duracao muda
        }

        override fun onRenderedFirstFrame() {
            if (minha != sessao) return
            ev(EV_PRIMEIRO_QUADRO)
        }

        override fun onVideoSizeChanged(v: VideoSize) {
            if (minha != sessao) return
            // Pixel anamorfico entra na largura: e a proporcao que o zoom do C usa.
            videoW = (v.width * v.pixelWidthHeightRatio + 0.5f).toInt()
            videoH = v.height
            ev(EV_TAMANHO, videoW, videoH)
            if (temJanela) calcularJanela() else aplicarEncaixe()
        }

        override fun onTracksChanged(tracks: Tracks) {
            if (minha != sessao) return
            publicarFaixas(tracks)
            publicarHdr(tracks)
        }

        override fun onCues(cueGroup: CueGroup) {
            if (minha != sessao) return
            val texto = cueGroup.cues.mapNotNull { it.text?.toString()?.trim() }
                .filter { it.isNotEmpty() }.joinToString("\n")
            // O Media3 nao diz quando o cue acaba: estimativa pelo tamanho, e o
            // proximo grupo (inclusive o vazio) substitui antes disso.
            val dur = if (texto.isEmpty()) 0 else (800 + texto.length * 60).coerceIn(2000, 7000)
            val entregar = Runnable {
                if (minha == sessao) try { nativeLegenda(texto, dur) } catch (e: UnsatisfiedLinkError) { }
            }
            if (atrasoMs > 0) principal.postDelayed(entregar, atrasoMs.toLong()) else entregar.run()
        }

        override fun onPlayerError(error: PlaybackException) {
            if (minha != sessao) return
            Log.w(TAG, "erro ${error.errorCodeName} (${error.errorCode}): ${error.message}")
            // Decoder que falha nos primeiros 5 s: o recurso pode estar sendo
            // solto por outro app (ResourceConflict do Tizen); reabre uma vez.
            val cedo = SystemClock.elapsedRealtime() - abriuEm < RETRY_DECODER_MS
            val decoder = error.errorCode == PlaybackException.ERROR_CODE_DECODER_INIT_FAILED ||
                error.errorCode == PlaybackException.ERROR_CODE_DECODING_FAILED
            if (decoder && cedo && !retentou) {
                retentou = true
                val u = urlAtual
                val c = cabAtual
                principal.postDelayed({ if (minha == sessao) abrirMain(u, c, true) }, 400)
                return
            }
            ev(EV_ERRO, error.errorCode, 0)
        }
    }

    private fun analitico(minha: Int) = object : AnalyticsListener {
        override fun onVideoDecoderInitialized(
            eventTime: AnalyticsListener.EventTime, decoderName: String,
            initializedTimestampMs: Long, initializationDurationMs: Long
        ) {
            if (minha != sessao) return
            // DV so conta com decoder DV de verdade (OMX.dolby.* / c2.dolby.*).
            decoderDv = decoderName.lowercase().contains("dolby")
            player?.let { publicarHdr(it.currentTracks) }
        }
    }

    private fun duracaoMs(p: ExoPlayer): Int {
        val d = p.duration
        return if (d == C.TIME_UNSET || p.isCurrentMediaItemLive) 0 else d.coerceIn(0L, Int.MAX_VALUE.toLong()).toInt()
    }

    // Lista para o C: audio com decoder e legenda de texto suportada. So manda
    // quando a LISTA muda; troca de selecao nao reenvia (o C reiniciaria a
    // legenda). Legenda sobe sempre desligada.
    private fun publicarFaixas(tracks: Tracks) {
        val a = ArrayList<Pair<Tracks.Group, Int>>()
        val l = ArrayList<Pair<Tracks.Group, Int>>()
        var temAudio = false
        for (g in tracks.groups) {
            for (i in 0 until g.length) {
                if (g.type == C.TRACK_TYPE_AUDIO) {
                    temAudio = true
                    if (g.isTrackSupported(i) && a.size < FAIXAS_MAX) a.add(g to i)
                } else if (g.type == C.TRACK_TYPE_TEXT && g.isTrackSupported(i) && l.size < FAIXAS_MAX) {
                    l.add(g to i)
                }
            }
        }
        // Audio no arquivo e nenhum com decoder: o video segue mudo e o C avisa.
        if (temAudio && a.isEmpty() && !avisouSemAudio) { avisouSemAudio = true; ev(EV_AUDIO_SEM_DECODER) }

        val sig = StringBuilder()
        for ((g, i) in a) sig.append('a').append(g.getTrackFormat(i).id).append(g.getTrackFormat(i).language).append(';')
        for ((g, i) in l) sig.append('l').append(g.getTrackFormat(i).id).append(g.getTrackFormat(i).language).append(';')
        val nova = sig.toString()
        if (nova == assinatura) return
        val tinhaLegenda = legendas.isNotEmpty()
        assinatura = nova
        audios = a
        legendas = l
        if (a.isEmpty() && l.isEmpty()) return
        // Lista de legendas mudou: a escolha antiga nao vale (o C zera legAtual).
        if (tinhaLegenda || l.isNotEmpty()) {
            player?.let {
                it.trackSelectionParameters = it.trackSelectionParameters.buildUpon()
                    .setTrackTypeDisabled(C.TRACK_TYPE_TEXT, true).clearOverridesOfType(C.TRACK_TYPE_TEXT).build()
            }
        }
        try {
            var sel = -1
            for ((n, par) in a.withIndex()) {
                val (g, i) = par
                if (sel < 0 && g.isTrackSelected(i)) sel = n
                nativeFaixa(0, n, g.getTrackFormat(i).language ?: "")
            }
            for ((n, par) in l.withIndex()) nativeFaixa(1, n, par.first.getTrackFormat(par.second).language ?: "")
            nativeFaixasFim(sel, -1)
        } catch (e: UnsatisfiedLinkError) { Log.w(TAG, "faixas sem lib: $e") }
    }

    // HDR do Format do video selecionado; Atmos do audio selecionado (E-AC-3
    // JOC). Dolby Vision so quando o decoder escolhido e DV. Manda so se mudou.
    private fun publicarHdr(tracks: Tracks) {
        var hdr = "none"
        var dv = 0
        var atmos = 0
        for (g in tracks.groups) {
            for (i in 0 until g.length) {
                if (!g.isTrackSelected(i)) continue
                val f = g.getTrackFormat(i)
                if (g.type == C.TRACK_TYPE_VIDEO) {
                    val t = f.colorInfo?.colorTransfer
                    if (f.sampleMimeType == MimeTypes.VIDEO_DOLBY_VISION && decoderDv) { hdr = "DolbyVision"; dv = 1 }
                    else if (t == C.COLOR_TRANSFER_ST2084) hdr = "HDR10"
                    else if (t == C.COLOR_TRANSFER_HLG) hdr = "HLG"
                } else if (g.type == C.TRACK_TYPE_AUDIO && f.sampleMimeType == MimeTypes.AUDIO_E_AC3_JOC) {
                    atmos = 1
                }
            }
        }
        if (hdr == ultHdr && dv == ultDv && atmos == ultAtmos) return
        ultHdr = hdr; ultDv = dv; ultAtmos = atmos
        try { nativeHdr(hdr, dv, atmos) } catch (e: UnsatisfiedLinkError) { }
    }
}
