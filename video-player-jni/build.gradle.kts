import org.bytedeco.javacpp.tools.Info

plugins {
  `java-library`
  id("com.github.wumo.javacpp") version "1.0.9"
}

java {
  withSourcesJar()
}

publishing {
  publications {
    create<MavenPublication>("maven") {
      from(components["java"])
    }
  }
}

javacpp {
  include = listOf("main.h", "player.h", "input.h")
  preload = listOf("glfw3")
  link = listOf("NativeVideoPlayer")
  target = "com.github.wumo.videoplayer.NativeVideoPlayer"
  infoMap = {
    it.put(Info("wumo::InputStreamCallback").virtualize())
      .put(Info("wumo::FrameGetter").virtualize())
  }
  cppSourceDir = "${project.projectDir}/src/main/cpp/NativeVideoPlayer"
  cppIncludeDir = "$cppSourceDir/src"
}