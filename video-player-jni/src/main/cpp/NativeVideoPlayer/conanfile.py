from conans import ConanFile, CMake, tools

class NativeVideoPlayerConan(ConanFile):
  name = "NativeVideoPlayer"
  version = "0.0.1"
  settings = "os", "compiler", "build_type", "arch"
  requires = (
    "ffmpeg/4.2.1@bincrafters/stable",
    "fmt/5.3.0@bincrafters/stable",
    "glm/0.9.9.6",
    "glfw/3.3.2@bincrafters/stable",
    "glad/0.1.33",
    "stb/20200203",
    "file2header/0.0.5@wumo/stable",
    "freetype/2.10.1",
  )
  generators = "cmake"
  scm = {
    "type": "git",
    "subfolder": name,
    "url": "auto",
    "revision": "auto"
  }
  options = {
    "shared": [True, False],
  }
  default_options = {
    "shared": True,
    "glfw:shared": True
  }

  def configure(self):
    if self.settings.compiler != 'Visual Studio':
      self.options["ffmpeg"].qsv = False

  def build(self):
    cmake = CMake(self)
    cmake.definitions["BUILD_TEST"] = False
    cmake.definitions["BUILD_SHARED"] = self.options.shared
    cmake.configure(source_folder=self.name)
    cmake.build()

  def imports(self):
    self.copy("*.dll", dst="bin", src="bin")
    self.copy("*.dll", dst="bin", src="lib")
    self.copy("*.dylib*", dst="bin", src="lib")
    self.copy("*.pdb", dst="bin", src="bin")
    self.copy("*", dst="bin/assets/public", src="resources")

  def package(self):
    self.copy("*", dst="bin", src="bin", keep_path=False)
    self.copy("*.h", dst="include", src=f"{self.name}/src")
    self.copy("*.so", dst="lib", src="lib", keep_path=False)
    self.copy("*.a", dst="lib", src="lib", keep_path=False)
    self.copy("*.lib", dst="lib", src="lib", keep_path=False)

  def package_info(self):
    self.cpp_info.libs = tools.collect_libs(self)
