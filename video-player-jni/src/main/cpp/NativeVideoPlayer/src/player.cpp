#include "player.h"

namespace wumo {
using namespace std::chrono;

VideoPlayer::VideoPlayer(FrameFetcher &frameFetcher, float width, float height)
  : native{frameFetcher},
    width_{width},
    height_{height},
    frameWidth_{width},
    frameHeight_{height} {}

void VideoPlayer::init() {
  initGLFW();
  createShader();
  createQuad();
  loadFont();
}

void VideoPlayer::initGLFW() {
  auto ret = glfwInit();
  errorIf(ret == 0, "error init glfw");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window =
    glfwCreateWindow(int(width_), int(height_), "Bilibili Player", nullptr, nullptr);
  println("create window");
  if(window == nullptr) {
    glfwTerminate();
    error("Failed to create GLFW window");
  }
  glfwSetWindowUserPointer(window, this);
  glfwMakeContextCurrent(window);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetCursorEnterCallback(window, mouse_enter_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetFramebufferSizeCallback(window, resize_callback);
  glfwSetKeyCallback(window, key_callback);

  errorIf(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to init GLAD");
}

void VideoPlayer::createQuad() {
  float vertices[] = {
    // positions     // texture coords
    1.f,  1.f,  0.0f, 1.0f, 1.0f, // top right
    1.f,  -1.f, 0.0f, 1.0f, 0.0f, // bottom right
    -1.f, -1.f, 0.0f, 0.0f, 0.0f, // bottom left
    -1.f, 1.f,  0.0f, 0.0f, 1.0f, // top left
  };

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  bindVAO(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  // texture coord attribute
  glVertexAttribPointer(
    1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
}

bool VideoPlayer::shouldClose() { return glfwWindowShouldClose(window); }

void VideoPlayer::render() {
  glViewport(0, 0, width_, height_);
  glClearColor(0.f, 0.f, 0.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  auto currentTS = glfwGetTime();
  if(currentTS - lastCheckFPS >= 1) {
    lastCheckFPS = currentTS;
    fps = fpsCount;
    fpsCount = 0;
  } else
    fpsCount++;

  handleClear();
  renderPic();
  renderUI();
}

void VideoPlayer::handleClear() {
  if(requestClear) {
    requestClear = false;
    glDeleteTextures(1, &y_tex);
    glDeleteTextures(1, &u_tex);
    glDeleteTextures(1, &v_tex);
    entries.clear();
  }
}

bool VideoPlayer::mouseInRegion(float x, float y, float w, float h) {
  return x <= input_.mouseXPos_ && input_.mouseXPos_ < x + w && y <= input_.mouseYPos_ &&
         input_.mouseYPos_ < y + h;
}

void VideoPlayer::useProgram(GLuint shader) {
  if(currentShader != shader) {
    glUseProgram(shader);
    currentShader = shader;
  }
}

void VideoPlayer::bindVAO(GLuint vao) {
  if(currentVAO != vao) {
    glBindVertexArray(vao);
    currentVAO = vao;
  }
}

void VideoPlayer::bindTexture0(GLuint tex, GLint align, GLint row) {
  if(currentTexture0 != tex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, align);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row);
    currentTexture0 = tex;
  }
}
void VideoPlayer::bindTexture1(GLuint tex, GLint align, GLint row) {
  if(currentTexture1 != tex) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, align);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row);
    currentTexture1 = tex;
  }
}
void VideoPlayer::bindTexture2(GLuint tex, GLint align, GLint row) {
  if(currentTexture2 != tex) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, align);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row);
    currentTexture2 = tex;
  }
}

}