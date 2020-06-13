import com.github.wumo.videoplayer.NativePlayer
import kotlinx.coroutines.Dispatchers.IO
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import okhttp3.internal.toHexString
import kotlin.concurrent.thread

fun main() {
  runBlocking {
    var player = NativePlayer(this, audioQueueSize = 10)
    player.start()
//    player.play("d.video", "d.audio")
    player.play("c.mkv")
  }
}
