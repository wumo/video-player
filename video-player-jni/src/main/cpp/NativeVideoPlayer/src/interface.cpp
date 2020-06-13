#include "player.h"
#include <stb_image.h>

namespace wumo {
void VideoPlayer::present() { glfwSwapBuffers(window); }
void VideoPlayer::pollEvents() { glfwPollEvents(); }
void VideoPlayer::waitEvents() { glfwWaitEvents(); }
void VideoPlayer::stop() {
  glfwDestroyWindow(window);
  glfwTerminate();
  window = nullptr;
  if(face) {
    FT_Done_Face(face);
    face = nullptr;
  }
  if(ft) {
    FT_Done_FreeType(ft);
    ft = nullptr;
  }
}
void VideoPlayer::detachContext() { glfwMakeContextCurrent(nullptr); }
void VideoPlayer::makeContextCurrent() { glfwMakeContextCurrent(window); }
void VideoPlayer::emptyEvent() { glfwPostEmptyEvent(); }
bool VideoPlayer::requestPause() { return isRequestPause; }
void VideoPlayer::finishPause() {
  isPaused = !isPaused;
  isRequestPause = false;
}
void VideoPlayer::clearPauseState() { isPaused = false; }
bool VideoPlayer::requestNext() { return requestNext_; }
void VideoPlayer::finishNext() { requestNext_ = false; }
float VideoPlayer::requestSeek() { return requestSeek_; }
void VideoPlayer::finishedSeek() { requestSeek_ = -1.f; }
float VideoPlayer::requestVolume() { return requestVolume_; }
void VideoPlayer::finishedVolume() { requestVolume_ = 0.0; }
void VideoPlayer::requestClose() {
  if(window) glfwSetWindowShouldClose(window, GL_TRUE);
}
void VideoPlayer::setFinishedPlay() { finishedPlay = true; }
bool VideoPlayer::requestReplay() { return requestReplay_; }
void VideoPlayer::finishedReplay() { requestReplay_ = false; }

void VideoPlayer::setTitle(const char *title) {
  println("settitle ", title);
  glfwSetWindowTitle(window, title);
}

void VideoPlayer::setIcon(uint8_t *pixels, int32_t sizeOfBytes) {
  int x, y, channels;
  auto data = stbi_load_from_memory(pixels, sizeOfBytes, &x, &y, &channels, 4);
  if(data) {
    GLFWimage img{x, y, data};
    glfwSetWindowIcon(window, 1, &img);
    stbi_image_free(data);
  }
}

void VideoPlayer::resize_callback(GLFWwindow *window, int width, int height) {
  auto player = static_cast<VideoPlayer *>(glfwGetWindowUserPointer(window));
  player->width_ = float(width);
  player->height_ = float(height);
}

void VideoPlayer::mouse_button_callback(
  GLFWwindow *window, int button, int action, int mods) {
  auto player = static_cast<VideoPlayer *>(glfwGetWindowUserPointer(window));
  if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    if(player->hoverOnList) {
      player->clickOnList = true;
    } else {
      float clickYPos = player->input_.mouseYPos_;
      if(!player->finishedPlay) {
        if(clickYPos < player->height_ - player->pauseRange) {
          player->isRequestPause = true;
        } else if(clickYPos < player->height_ - player->seekRange) {
          player->requestSeek_ = float(player->input_.mouseXPos_) / player->width_;
        }

        if(player->mouseInRegion(
             player->barMarginLeft,
             player->height_ - player->barMarginBottom - player->barIconWidth,
             player->barIconWidth, player->barIconWidth)) {
          player->isRequestPause = true;
        }
      } else {
        if(player->mouseInRegion(
             player->barMarginLeft,
             player->height_ - player->barMarginBottom - player->barIconWidth,
             player->barIconWidth, player->barIconWidth)) {
          player->requestReplay_ = true;
          player->finishedPlay = false;
        }
      }
      if(player->mouseInRegion(
           player->barMarginLeft + player->barIconWidth + player->barSpacing,
           player->height_ - player->barMarginBottom - player->barIconWidth,
           player->barIconWidth, player->barIconWidth)) {
        player->requestNext_ = true;
      }
    }
    player->handleLocalRemoteButtonClickEvent();
  }
}

void VideoPlayer::mouse_move_callback(GLFWwindow *window, double xpos, double ypos) {
  auto player = static_cast<VideoPlayer *>(glfwGetWindowUserPointer(window));
  player->input_.mouseXPos_ = float(xpos);
  player->input_.mouseYPos_ = float(ypos);
}

void VideoPlayer::mouse_enter_callback(GLFWwindow *window, int entered) {
  auto player = static_cast<VideoPlayer *>(glfwGetWindowUserPointer(window));
  player->input_.mouseEnterd = entered == GL_TRUE;
}

void VideoPlayer::scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  auto player = static_cast<VideoPlayer *>(glfwGetWindowUserPointer(window));
  player->input_.scrollXOffset_ += float(xoffset);
  player->input_.scrollYOffset_ += float(yoffset);
  if(player->hoverOnList) {
    player->listScrollYOffset += float(yoffset) * player->listLineHeight;
  } else {
    player->requestVolume_ += float(yoffset);
    player->requestShowVolume_ = true;
  }
}

void VideoPlayer::key_callback(GLFWwindow *window, int key, int, int action, int) {
  if(key > GLFW_KEY_LAST || key < 0) return;
  auto player = static_cast<VideoPlayer *>(glfwGetWindowUserPointer(window));

  if(action == GLFW_PRESS) player->input_.keyPressed[key] = true;
  else if(action == GLFW_RELEASE)
    player->input_.keyPressed[key] = false;

  if(player->input_.keyPressed[GLFW_KEY_TAB])
    player->toggleShowInfo = !player->toggleShowInfo;
}
}