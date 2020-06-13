cmake_minimum_required(VERSION 3.12)
project(NativeVideoPlayer LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/symlink.cmake)
symlink(assets bin/assets)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets/public/NativeVideoPlayer)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan.cmake)
conan_cmake_run(
  BASIC_SETUP
  CONANFILE conanfile.py
  BUILD missing)

option(BUILD_TEST "Build test" ON)
option(BUILD_SHARED "Build shared lib" ON)

set(sources src/main.cpp
  src/syntactic_sugar.cpp
  src/player.cpp
  src/impl.cpp
  src/render_list.cpp
  src/compile_shader.cpp
  src/render_picture.cpp
  src/render_ui.cpp
  src/interface.cpp
  src/shader.cpp
  src/render_volume.cpp src/render_char.cpp src/util/util.cpp src/render_info.cpp)

if(BUILD_SHARED)
  add_library(NativeVideoPlayer SHARED ${sources})
else()
  add_library(NativeVideoPlayer STATIC ${sources})
endif()

resources(${CMAKE_CURRENT_LIST_DIR}/resources
  ${CMAKE_CURRENT_LIST_DIR}/src/resources)

target_include_directories(NativeVideoPlayer PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/src)
target_compile_definitions(NativeVideoPlayer
  PRIVATE
  $<$<CONFIG:DEBUG>:DEBUG>)
target_link_libraries(NativeVideoPlayer
  PUBLIC
  ${CONAN_LIBS}
  resources
  $<$<PLATFORM_ID:Linux>:dl>)

if(BUILD_TEST)
  file(GLOB_RECURSE test-sources CONFIGURE_DEPENDS "test/*.cpp")
  foreach(file ${test-sources})
    get_filename_component(comp ${file} NAME_WE)
    add_executable(${comp} ${file})
    target_include_directories(${comp} PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/test)
    target_link_libraries(${comp} PRIVATE NativeVideoPlayer resources)
  endforeach()
endif()
