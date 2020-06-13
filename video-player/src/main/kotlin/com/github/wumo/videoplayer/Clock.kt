package com.github.wumo.videoplayer

import java.lang.System.currentTimeMillis

internal class Clock(val tag: String) {
  private val minDelay = -1000
  private val maxDelay = 1000
  private val maxSleep = 1000
  
  init {
    assert(0 in minDelay..maxDelay)
  }
  
  private val lags = Array(maxSleep + 1) {
    IntArray(maxDelay - minDelay + 1)
  }.also { lags ->
    repeat(maxSleep + 1) {
      lags[it][0 - minDelay] = 1
    }
  }
  
  private val best = IntArray(maxSleep + maxDelay - minDelay) {
    (it + minDelay).coerceIn(
      0,
      maxSleep
    )
  }
  private val SKIP = -1L
  private val INIT = -2L
  private val SKIP_THRESHOLD = -30L
  
  private var origin = 0L
  private var lastTS = 0L
  private var lastPTS = 0L
  private var lastSleep = INIT
  private var isPaused = false
  private var pauseTS = 0L
  private var shouldCompensateSeek = false
  
  fun reset(origin: Long) {
    this.origin = origin
    isPaused = false
    lastSleep = INIT
  }
  
  fun currentTime(): Long {
    if (isPaused) return pauseTS - origin
    return currentTimeMillis() - origin
  }
  
  fun pause() {
    isPaused = true
    pauseTS = currentTimeMillis()
  }
  
  fun unpause() {
    isPaused = false
    origin += currentTimeMillis() - pauseTS
  }
  
  fun timeToSleep(pts: Long): Long {
    if (shouldCompensateSeek) {
      shouldCompensateSeek = false
      origin = currentTimeMillis() - pts
    }
    val ts = currentTime()
    val expected = (pts - ts).toInt()
    lastTS = ts
    lastPTS = pts
    lastSleep = if (expected < 0) {
      if (expected < SKIP_THRESHOLD)
        expected.toLong()
      else 0L
    } else {
      val diff = expected.coerceIn(
        minDelay,
        maxDelay
      ) - minDelay
      best[diff].toLong()
    }
    return lastSleep
  }
  
  fun backward() {
    val ts = currentTime()
    val realSleep = ts - lastTS
    val delta = (realSleep - lastSleep).toInt().coerceIn(
      minDelay,
      maxDelay
    )
//    println("[$tag] delta=${ts - lastPTS},expected=${lastPTS - lastTS},choose=$lastSleep,real=$realSleep")
    if (lastSleep != INIT) {
      val i = lastSleep.toInt()
      val j = delta - minDelay
      lags[i][j]++
      val k = i + j
      if (lags[i][j] > lags[best[k]][k - best[k]])
        best[k] = i
    }
  }
  
  fun compensateLag(lag: Long) {
    origin += lag
  }
  
  fun shouldCompensateSeek() {
    shouldCompensateSeek = true
  }
}