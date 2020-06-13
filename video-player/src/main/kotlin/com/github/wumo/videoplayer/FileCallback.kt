package com.github.wumo.videoplayer

import com.github.wumo.videoplayer.NativeVideoPlayer.FrameFetcher.*
import org.bytedeco.javacpp.BytePointer
import java.io.RandomAccessFile

open class InputCallback : NativeVideoPlayer.InputStreamCallback()

class FileCallback(private val vf_path: String) : InputCallback() {
  private val input = RandomAccessFile(vf_path, "r")
  
  override fun read(buf: BytePointer, buf_size: Int): Int {
    buf.position(0).capacity(buf_size.toLong())
    return input.channel.read(buf.asByteBuffer())
  }
  
  override fun seek(offset: Long, whence: Int): Long {
    when (whence) {
      seek_set -> input.seek(offset)
      seek_cur -> input.seek(input.filePointer + offset)
      seek_end -> input.seek(input.length())
      seek_size -> return input.length()
    }
    return 0
  }
  
  override fun stop() {
    input.close()
  }
  
  
}