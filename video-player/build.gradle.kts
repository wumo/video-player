plugins {
  kotlin("jvm")
  kotlin("plugin.serialization")
}

dependencies {
  api(project(":video-player-jni"))
  val coroutineVer = "1.3.5"
  val serializationVer = "0.20.0"
//  api("org.bytedeco:javacpp:1.5.3")
//  implementation("org.bytedeco:javacpp-platform:1.5.3")
  implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:$coroutineVer")
  testImplementation("no.tornado:tornadofx:2.0.0-SNAPSHOT")
  testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-javafx:$coroutineVer")
  implementation("org.jetbrains.kotlinx:kotlinx-serialization-runtime:$serializationVer")
  testImplementation("org.openjfx:javafx-swing:13:win")
  testImplementation("org.openjfx:javafx-controls:13:win")
  testImplementation("org.openjfx:javafx-graphics:13:win")
  implementation("com.squareup.okhttp3:okhttp:4.4.1")
  implementation("com.squareup.okhttp3:logging-interceptor:4.4.1")
  implementation("com.squareup.okhttp3:okhttp-urlconnection:4.4.1")
}

tasks {
  compileKotlin {
    kotlinOptions.jvmTarget = "9"
  }
}

dependencies {
  implementation(kotlin("stdlib-jdk8"))
}

val kotlinSourcesJar by tasks

publishing {
  publications {
    create<MavenPublication>("maven") {
      from(components["kotlin"])
      artifact(kotlinSourcesJar)
    }
  }
}