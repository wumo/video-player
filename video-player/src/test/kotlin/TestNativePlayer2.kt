import com.github.wumo.videoplayer.NativePlayer
import kotlinx.coroutines.Dispatchers.IO
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlin.concurrent.thread

fun main() {
  runBlocking {
    var player = NativePlayer(this, audioQueueSize = 10)
    player.start()
    player.play("b.mkv")
    Thread.sleep(1000)
    player.play("b.mkv")
    delay(2000)
//    delay(2000)
//    player = NativePlayer(this)
//    player.start()
//    player.play("a.mkv")
//    delay(10000)
  }
}
