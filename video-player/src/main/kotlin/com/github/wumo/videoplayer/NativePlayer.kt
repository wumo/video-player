package com.github.wumo.videoplayer

import com.github.wumo.videoplayer.NativeVideoPlayer.Frame
import kotlinx.coroutines.*
import kotlinx.coroutines.Dispatchers.IO
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.channels.Channel.Factory.CONFLATED
import kotlinx.serialization.toUtf8Bytes
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import java.awt.Desktop
import java.lang.System.currentTimeMillis
import java.net.URI
import java.util.concurrent.ConcurrentHashMap
import javax.sound.sampled.AudioFormat
import javax.sound.sampled.AudioSystem
import javax.sound.sampled.DataLine
import javax.sound.sampled.SourceDataLine
import kotlin.concurrent.thread
import kotlin.math.max

data class NativePlayTask(
  val path: InputCallback? = null,
  val path2: InputCallback? = null
)

data class EntryState(
  var localPath: String? = null,
  var remotePath: String? = null,
  var downloadFunc: () -> Unit = {},
  var ioFunc: suspend () -> NativePlayTask
)

internal data class Task(val title: String, val state: EntryState)

class NativePlayer(
  private val scope: CoroutineScope,
  private val videoQueueSize: Int = 5,
  private val audioQueueSize: Int = 10,
  val onWindowOpen: () -> Unit = {},
  val onVolumeChange: (Double) -> Unit = {},
  val closeCallback: () -> Unit = {}
) : CoroutineScope by scope {
  private val native = NativeVideoPlayer.FrameFetcher(videoQueueSize.toLong(), audioQueueSize.toLong())
  private val player = NativeVideoPlayer.VideoPlayer(native)
  private val dst_rate = NativeVideoPlayer.FrameFetcher.dst_rate
  private val dst_channels = NativeVideoPlayer.FrameFetcher.dst_channels
  private val dst_sample_fmt = NativeVideoPlayer.FrameFetcher.dst_sample_fmt
  
//  init {
//    AudioSystem.getMixerInfo().forEach {
//      AudioSystem.getMixer(it).also {
//        it.sourceLineInfo.forEach {
//          if (it is DataLine.Info) {
//            it.formats.forEach {
//              println(it)
//            }
//          }
//        }
//      }
//    }
//  }
  
  private val af = AudioFormat(dst_rate.toFloat(), 16, dst_channels, true, false)
  private val info = DataLine.Info(SourceDataLine::class.java, af)
  private val line = AudioSystem.getLine(info) as SourceDataLine
  private var stopDecoding = false
  private var pauseDecoding = false
  private var seekPos = -1L
  private val decodeTasks = Channel<NativePlayTask>(CONFLATED)
  private val videoFrames = Channel<Frame>(videoQueueSize)
  private val audioFrames = Channel<Frame>(audioQueueSize)
  private val closedToken = Frame(Pointer())
  private val pauseDecodingSignal = Channel<Unit>(CONFLATED)
  private val syncSignal = Channel<Unit>(CONFLATED)
  private val sync2Signal = Channel<Unit>(CONFLATED)
  private val finishUsingVideoFrameSignal = Channel<Unit>(CONFLATED)
  private val finishUsingAudioFrameSignal = Channel<Unit>(CONFLATED)
  private val decodeStopSignal = Channel<Unit>()
  private val audioStopSignal = Channel<Unit>()
  private val videoStopSignal = Channel<Unit>()
  private var clockOrigin = 0L
  private var videoClock = Clock("video")
  private var audioClock = Clock("audio")
  
  private val tasks = ConcurrentHashMap<Int, Task>()
  
  var volume: Double = 0.5
    get() = native.volume()
    set(value) {
      native.volume(value)
      player.updateVolume(value.toFloat())
      field = value
    }
  
  fun setIcon(bytes: ByteArray) {
    player.setIcon(bytes, bytes.size)
  }
  
  private fun setTitle(title: String) {
    val bytes = BytePointer(*title.toUtf8Bytes(), 0)
    player.setTitle(bytes)
    bytes.deallocate()
  }
  
  
  fun addToPlayList(title: String, task: EntryState) {
    val idx = tasks.size
    tasks[idx] = Task(title, task)
    player.addPlayEntry(title.codePoints().toArray())
  }
  
  fun updateCurrentState() {
    tasks[player.currentPlay()]?.also {
      player.setCurrentHasLocal(it.state.localPath != null)
      player.setCurrentHasRemote(it.state.remotePath != null)
    }
  }
  
  fun play(path: InputCallback? = null, path2: InputCallback? = null) {
    stopDecoding()
    decodeTasks.offer(NativePlayTask(path, path2))
  }
  
  fun play(path: String? = null, path2: String? = null) {
    stopDecoding()
    decodeTasks.offer(
      NativePlayTask(
        path?.let { FileCallback(it) },
        path2?.let { FileCallback(it) })
    )
  }
  
  fun play(index: Int) {
    tasks[index]?.also { (title, task) ->
      stopDecoding()
      scope.launch {
        decodeTasks.offer(task.ioFunc())
      }
      player.setCurrentPlay(index)
      updateCurrentState()
      setTitle(title)
    }
  }
  
  
  private fun switchPause() {
    if (pauseDecoding) {
      pauseDecoding = false
      pauseDecodingSignal.offer(Unit)
    } else pauseDecoding = true
  }
  
  private fun unpause() {
    pauseDecoding = false
    pauseDecodingSignal.offer(Unit)
  }
  
  private fun seek(timestampMilis: Long) {
    seekPos = timestampMilis
    unpause()
  }
  
  private inline fun <T> Channel<T>?.drain(destruct: (T) -> Unit = {}) {
    this?.apply {
      while (true) {
        val t = poll() ?: break
        destruct(t)
      }
    }
  }
  
  private suspend fun emptyFrames() {
    audioFrames.drain {
      native.recycleFrame(it)
    }
    audioFrames.offer(closedToken)
    videoFrames.drain {
      native.recycleFrame(it)
    }
    videoFrames.offer(closedToken)
    finishUsingVideoFrameSignal.receive()
    finishUsingAudioFrameSignal.receive()
    line.flush()
  }
  
  private fun stopDecoding() {
    stopDecoding = true
    pauseDecoding = false
    pauseDecodingSignal.offer(Unit)
  }
  
  fun start() {
    thread {
      player.init()
      onWindowOpen()
      player.detachContext()
      scope.launch(IO) {
        player.makeContextCurrent()
        while (!player.shouldClose()) {
          val defaultSleep = (1000 / max(60.0, native.fps())).toLong()
          while (true) {
            if (player.currentPlay() == -1)
              play(0)
            val frame = videoFrames.poll()
            if (frame === closedToken) {
              finishUsingVideoFrameSignal.send(Unit)
              Thread.sleep(defaultSleep)//opengl context require the thread to be the same or crash!
              player.render()
              player.present()
              break
            } else if (frame == null) {
              Thread.sleep(defaultSleep)//opengl context require the thread to be the same or crash!
              player.render()
              player.present()
              break
            }
            val sleep = videoClock.timeToSleep(frame.pts())
            if (sleep >= 0) {
              Thread.sleep(sleep)
              player.updateProgress(frame.pts(), native.duration())
              player.updateTex(frame)
              native.recycleFrame(frame)
              player.render()
              player.present()
              videoClock.backward()
              break
            } else {
              println("video skip $sleep")
              native.recycleFrame(frame)
            }
          }
        }
        player.emptyEvent()
        stopDecoding()
        // prevent decode thread from suspending due to sending video frames
        videoFrames.drain {
          if (it !== closedToken)
            native.recycleFrame(it)
        }
        finishUsingVideoFrameSignal.send(Unit)
        decodeTasks.close()
        decodeStopSignal.receive()
        audioFrames.close()
        audioStopSignal.receive()
        videoStopSignal.send(Unit)
        closedToken.deallocate()
      }
      while (!player.shouldClose()) {
        player.waitEvents()
        if (player.requestPause()) {
          player.finishPause()
          switchPause()
        }
        if (player.requestNext()) {
          player.finishNext()
          val nextIdx = player.currentPlay() + 1
          play(nextIdx)
        }
        if (player.requestPlayIndex() >= 0) {
          val idx = player.requestPlayIndex()
          player.finishRequestPlayIndex()
          play(idx)
        }
        if (player.requestReplay()) {
          player.finishedReplay()
          play(player.currentPlay())
        }
        val seek = player.requestSeek()
        if (seek >= 0) {
          seek((native.duration() * seek).toLong())
          player.finishedSeek()
        }
        val v = player.requestVolume()
        if (v != 0f) {
          native.volume((native.volume() + v * 0.01).coerceIn(0.0, 1.0))
          onVolumeChange(native.volume())
          player.finishedVolume()
          player.updateVolume(native.volume().toFloat())
        }
        if (player.requestOpenLocal()) {
          player.finishOpenLocal()
          val pagePath = tasks[player.currentPlay()]!!.state.localPath!!
          Runtime.getRuntime().exec("""explorer.exe /select,"$pagePath"""")
        }
        if (player.requestOpenRemote()) {
          player.finishOpenRemote()
          val state = tasks[player.currentPlay()]!!.state
          if (state.localPath == null)
            state.downloadFunc()
          else {
            val pageUrl = state.remotePath!!
            if (Desktop.isDesktopSupported() && Desktop.getDesktop().isSupported(Desktop.Action.BROWSE))
              Desktop.getDesktop().browse(URI(pageUrl))
          }
        }
      }
      runBlocking {
        videoStopSignal.receive()
      }
      native.stop()
      native.deallocate()
      player.stop()
      player.deallocate()
      closeCallback()
      println("close video")
    }
    scope.launch(IO) {
      try {
        line.open(af)
        line.start()
        for (frame in audioFrames) {
          if (frame === closedToken) {
            finishUsingAudioFrameSignal.send(Unit)
            continue
          }
          val queuedDelay = 1000 * (line.bufferSize - line.available()) / 2 / dst_channels / dst_rate
          val sleep = audioClock.timeToSleep(frame.pts() - queuedDelay)
          if (sleep >= 0) {
            delay(max(sleep, 0))
            if (!native.hasVideo())
              player.updateProgress(frame.pts(), native.duration())
            val bytes = ByteArray(frame.bufsize())
            frame.data(0).position(0).limit(frame.bufsize().toLong()).get(bytes)
            line.write(bytes, 0, bytes.size)
          } else
            println("audio skip $sleep")
          native.recycleFrame(frame)
        }
        line.flush()
        line.stop()
        line.close()
        audioStopSignal.send(Unit)
      } catch (e: Exception) {
        e.printStackTrace()
      }
    }.invokeOnCompletion {
      println("close audio")
    }
    scope.launch(IO) {
      for ((_fileCallback, _fileCallback2) in decodeTasks) {
        if (_fileCallback == null && _fileCallback2 == null) continue
        val fileCallback = _fileCallback ?: _fileCallback2!!
        val fileCallback2 = if (_fileCallback == null) null else _fileCallback2
        
        native.open(fileCallback, fileCallback2)
        player.clearPauseState()
        
        stopDecoding = false
        pauseDecoding = false
        seekPos = -1L
        pauseDecodingSignal.drain()
        finishUsingVideoFrameSignal.drain()
        finishUsingAudioFrameSignal.drain()
        decodeStopSignal.drain()
        syncSignal.drain()
        sync2Signal.drain()
        
        clockOrigin = 0L
        videoClock = Clock("video")
        audioClock = Clock("audio")
        if (clockOrigin == 0L) {
          clockOrigin = currentTimeMillis()
          videoClock.reset(clockOrigin)
          audioClock.reset(clockOrigin)
        }
        if (fileCallback2 != null)
          scope.launch(IO) {
            while (!stopDecoding) {
              if (seekPos != -1L) {
                syncSignal.send(Unit)
                sync2Signal.receive()
              }
              if (pauseDecoding) {
                syncSignal.send(Unit)
                sync2Signal.receive()
              }
              val frame = native.read_frame2() ?: break
              when (frame.type()) {
                Frame.TYPE_VIDEO -> videoFrames.send(frame)
                Frame.TYPE_AUDIO -> audioFrames.send(frame)
              }
            }
            syncSignal.send(Unit)
            sync2Signal.receive()
          }
        var playFinished = false
        while (!stopDecoding) {
          if (seekPos != -1L) {
            if (fileCallback2 != null)
              syncSignal.receive()
            val pos = seekPos
            seekPos = -1L
            emptyFrames()
            native.seek(pos)
            videoClock.shouldCompensateSeek()
            audioClock.shouldCompensateSeek()
            if (fileCallback2 != null)
              sync2Signal.send(Unit)
          }
          if (pauseDecoding) {
            if (fileCallback2 != null)
              syncSignal.receive()
            videoClock.pause()
            audioClock.pause()
            emptyFrames()
            pauseDecodingSignal.receive()
            videoClock.unpause()
            audioClock.unpause()
            if (fileCallback2 != null)
              sync2Signal.send(Unit)
          }
          val frame = native.read_frame()
          if (frame == null) {
            playFinished = true
            break
          }
          when (frame.type()) {
            Frame.TYPE_VIDEO -> videoFrames.send(frame)
            Frame.TYPE_AUDIO -> audioFrames.send(frame)
          }
        }
        if (fileCallback2 != null) {
          syncSignal.receive()
          sync2Signal.send(Unit)
        }
        emptyFrames()
        native.stop()
        fileCallback.stop()
        fileCallback.deallocate()
        fileCallback2?.stop()
        fileCallback2?.deallocate()
        if (playFinished) {
          val nextIdx = player.currentPlay() + 1
          val nextPlay = tasks[nextIdx]
          if (nextPlay != null) play(nextIdx)
          else player.setFinishedPlay()
        }
      }
      decodeStopSignal.send(Unit)
    }.invokeOnCompletion {
      println("close decoding")
    }
  }
  
}