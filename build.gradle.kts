plugins {
  base
  `maven-publish`
  kotlin("jvm") version "1.3.71" apply false
  kotlin("plugin.serialization") version "1.3.71" apply false
  id("com.google.osdetector") version "1.6.2" apply false
}

allprojects {
  apply(plugin = "maven-publish")
  apply(plugin = "com.google.osdetector")
  
  group = "com.github.wumo"
  version = "0.0.2"
  
  repositories {
    jcenter()
    maven("https://oss.sonatype.org/content/repositories/snapshots")
    maven(url = "https://jitpack.io")
  }
}