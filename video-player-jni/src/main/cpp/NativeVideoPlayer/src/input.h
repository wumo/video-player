#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>

namespace wumo {
class Input {
  friend class VideoPlayer;

public:
  bool isMouseLeftPressed() { return mouseButtonPressed[GLFW_MOUSE_BUTTON_LEFT]; }
  bool isMouseRightPressed() { return mouseButtonPressed[GLFW_MOUSE_BUTTON_RIGHT]; }
  float mouseXPos() { return mouseXPos_; }
  float mouseYPos() { return mouseYPos_; }

  void setMouseLeftPressed(bool pressed) {
    mouseButtonPressed[GLFW_MOUSE_BUTTON_LEFT] = pressed;
  }
  void setMouseRightPressed(bool pressed) {
    mouseButtonPressed[GLFW_MOUSE_BUTTON_RIGHT] = pressed;
  }

  float mouseXPos_{0.0};
  float mouseYPos_{0.0};
  bool mouseEnterd{false};
  float scrollXOffset_{0.0};
  float scrollYOffset_{0.0};

private:
  std::array<bool, GLFW_MOUSE_BUTTON_LAST> mouseButtonPressed{};
  std::array<bool, GLFW_KEY_LAST> keyPressed{};
};
}
