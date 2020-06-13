#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "main.h"
#include "input.h"
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "MPMCQueue.h"
#include "util/util.h"

namespace wumo {

class VideoPlayer {
private:
  struct Character {
    GLuint tex;         // ID handle of the glyph texture
    glm::ivec2 size;    // Size of glyph
    glm::ivec2 bearing; // Offset from baseline to left/top of glyph
    GLuint advance;     // Horizontal offset to advance to next glyph
    glm::ivec2 offset;  //offset of glyph in texture coordinates
  };

public:
  explicit VideoPlayer(
    FrameFetcher &frameFetcher, float width = 1080, float height = 720);
  void init();
  void stop();
  void setTitle(const char *title);
  void setIcon(uint8_t *pixels, int32_t sizeOfBytes);
  void detachContext();
  void makeContextCurrent();
  bool shouldClose();
  void updateTex(Frame *frame);
  void updateProgress(int64_t pts, int64_t total);
  void updateVolume(float volume);
  void render();
  void present();
  void pollEvents();
  void waitEvents();
  void emptyEvent();

  bool requestPause();
  void finishPause();
  void clearPauseState();
  bool requestNext();
  void finishNext();
  float requestSeek();
  void finishedSeek();
  float requestVolume();
  void finishedVolume();
  void requestClose();
  void setFinishedPlay();
  bool requestReplay();
  void finishedReplay();

  bool requestOpenLocal() const;
  void finishOpenLocal();
  bool requestOpenRemote() const;
  void finishOpenRemote();

  void addPlayEntry(std::vector<int32_t> entry);
  int32_t requestPlayIndex();
  void finishRequestPlayIndex();
  void setCurrentPlay(int32_t idx);
  int32_t currentPlay();
  void setCurrentHasLocal(bool local);
  void setCurrentHasRemote(bool remote);

private:
  FrameFetcher &native;
  Input input_;
  float width_, height_;
  GLFWwindow *window{nullptr};
  GLuint yuvShader{0}, colorShader{0}, texShader{0}, texAtlasShader{0};
  GLuint VBO{0}, VAO{0};
  GLuint y_tex{0}, u_tex{0}, v_tex{0};
  Character iconPlayTex{}, iconPauseTex{}, iconNextTex{}, iconVolumeTex{}, iconListTex{},
    iconReplayTex{}, iconDownloadTex{}, iconFolderTex{}, iconBrowserTex{};

  bool requestClear{false};
  double lastCheckFPS{-10};
  int fps{0}, fpsCount{0};

  bool expandBar{false};
  bool volumeOnHover{false};
  float frameWidth_, frameHeight_;
  int64_t pts_, totalTS_;
  float lastXPos{0}, lastYPos{0};
  double lastShowBarTS{-10.f};
  bool requestNext_{false};

  bool isRequestPause{false};
  bool isPaused{false};

  float requestVolume_{0.f};
  bool requestShowVolume_{false};
  double lastShowVolumeTS{-10.f};
  float volume_{0.0};

  float requestSeek_{-1.f};
  const float pauseRange{80};
  const float seekRange{50};

  bool finishedPlay{false};
  bool requestReplay_{false};

  bool toggleShowInfo{false};

  bool showList{false};
  bool hoverOnList{false};
  bool clickOnList{false};
  double lastShowListTS{-10.f};
  float listScrollYOffset{0.0};

  int32_t requestPlayIndex_{-1};
  int32_t currentPlay_{-1};
  bool currentHasLocal{false};
  bool currentHasRemote{false};
  bool requestOpenLocal_{false};
  bool requestOpenRemote_{false};

  FT_Library ft{nullptr};
  FT_Face face{nullptr};

  GLuint charTex;
  std::unordered_map<int32_t, Character> characters;

  std::vector<std::vector<int32_t>> entries;
  rigtorp::MPMCQueue<std::vector<int32_t>> entryQueue{1000};

  GLuint currentShader{0};
  GLuint currentVAO{0};
  GLuint currentTexture0{0};
  GLuint currentTexture1{0};
  GLuint currentTexture2{0};

private:
  const int32_t icon_play = 0xe800;
  const int32_t icon_pause = 0xe801;
  const int32_t icon_next = 0xe802;
  const int32_t icon_volume = 0xe803;
  const int32_t icon_list = 0xe804;
  const int32_t icon_replay = 0xe805;
  const int32_t icon_download = 0xe806;
  const int32_t icon_folder = 0xe807;
  const int32_t icon_browser = 0xe843;

  const int iconWidth = 24;
  const int textHeight = 16;
  const int maxCharPerLine = 100;
  const int texWidth = iconWidth * maxCharPerLine;

  const float barHeight = 60.f;
  const float barIconWidth = 24.f;
  const float barMarginLeft = 20.f;
  const float barMarginRight = 20.f;
  const float barMarginBottom = barHeight / 2 - barIconWidth / 2;
  const float barSpacing = 40.f;

  const float floatWindowYOffset = 60.f;

  const float volumeBackHeight = 120.f;
  const float volumeBackWidth = 30.f;
  const float volumeInnerMarginTop = 10.f;
  const float volumeInnerHeight = 80.f;
  const float volumeInnerWidth = 3.f;
  const float volumePinWidth = 10.f;
  const float volumeTextMarginTop = 8.f;

  const float listWidth = 300.f;
  const float listMaxHeight = 600.f;
  const float listMarginRight = 5.f;
  const float listInnerMarginH = 10.f;
  const float listInnerMarginV = 8.f;
  const float listLineHeight = listInnerMarginV * 2 + textHeight;

  const glm::vec4 listBackColor{0.176, 0.176, 0.173, 0.8};
  const glm::vec4 listHoverColor{0.327, 0.327, 0.327, 0.8};
  const glm::vec4 grayLightColor{0.459, 0.459, 0.459, 1.0};
  const glm::vec4 textColor{0.831, 0.831, 0.831, 1.0};
  const glm::vec4 textBlueColor{0, 0.596, 0.792, 1.0};

  const glm::vec4 barBackColor{0.16f, 0.157f, 0.165f, 0.5f};

private:
  static void resize_callback(GLFWwindow *window, int width, int height);
  static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
  static void mouse_move_callback(GLFWwindow *window, double xpos, double ypos);
  static void VideoPlayer::mouse_enter_callback(GLFWwindow *window, int entered);
  static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
  static void key_callback(GLFWwindow *window, int key, int, int action, int);

  void initGLFW();
  void createQuad();
  void createShader();
  void createTex();
  void uploadTex(Frame *frame);
  static int compileShader(const char *vert, const char *frag);
  bool mouseInRegion(float x, float y, float w, float h);
  void renderList();
  void loadFont();
  void renderUI();
  void renderPic();
  void renderVolume();
  void renderProgressBar();
  void renderPlayButton();
  void renderPlayNext();
  VideoPlayer::Character findOrLoadFontTex(int32_t c);
  void renderChar(
    Character &ch, float x, float y, const glm::vec4 &color, float scale = 1.0);
  bool renderChar(
    Character &ch, float x, float y, float minX, float maxX, float minY, float maxY,
    glm::vec4 &color, float scale = 1.0);
  void renderSimpleString(
    const std::string &str, float x, float y, float height, const glm::vec4 &color,
    float scale = 1.0);
  void renderInfoText();

  void useProgram(GLuint shader);
  void bindVAO(GLuint vao);
  void bindTexture0(GLuint tex, GLint align = 1, GLint row = 0);
  void bindTexture1(GLuint tex, GLint align = 1, GLint row = 0);
  void bindTexture2(GLuint tex, GLint align = 1, GLint row = 0);
  void renderRect(float x, float y, float w, float h, glm::vec4 color);
  void handleClear();
  void renderLocalRemoteButton();
  void handleLocalRemoteButtonClickEvent();
};
}