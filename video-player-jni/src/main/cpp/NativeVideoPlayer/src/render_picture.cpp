#include "player.h"

namespace wumo {

void VideoPlayer::updateTex(Frame *frame) {
  if(
    !glIsTexture(y_tex) || frameWidth_ != frame->width() ||
    frameHeight_ != frame->height()) {
    frameWidth_ = frame->width();
    frameHeight_ = frame->height();
    createTex();
  }
  uploadTex(frame);
}

void VideoPlayer::updateProgress(int64_t pts, int64_t total) {
  pts_ = pts;
  totalTS_ = total;
}

void VideoPlayer::updateVolume(float volume) { volume_ = std::clamp(volume, 0.f, 1.f); }

void VideoPlayer::createTex() {
  glDeleteTextures(1, &y_tex);
  glDeleteTextures(1, &u_tex);
  glDeleteTextures(1, &v_tex);

  glGenTextures(1, &y_tex);
  bindTexture0(y_tex);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_R8, frameWidth_, frameHeight_, 0, GL_RED, GL_UNSIGNED_BYTE,
    nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenTextures(1, &u_tex);
  bindTexture0(u_tex);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_R8, frameWidth_ / 2, frameHeight_ / 2, 0, GL_RED,
    GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenTextures(1, &v_tex);
  bindTexture0(v_tex);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_R8, frameWidth_ / 2, frameHeight_ / 2, 0, GL_RED,
    GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void VideoPlayer::uploadTex(Frame *frame) {
  bindTexture0(y_tex, 4, frame->linesize(0));
  glTexSubImage2D(
    GL_TEXTURE_2D, 0, 0, 0, frameWidth_, frameHeight_, GL_RED, GL_UNSIGNED_BYTE,
    frame->data(0));

  bindTexture0(u_tex, 4, frame->linesize(1));
  glTexSubImage2D(
    GL_TEXTURE_2D, 0, 0, 0, frameWidth_ / 2, frameHeight_ / 2, GL_RED, GL_UNSIGNED_BYTE,
    frame->data(1));

  bindTexture0(v_tex, 4, frame->linesize(2));
  glTexSubImage2D(
    GL_TEXTURE_2D, 0, 0, 0, frameWidth_ / 2, frameHeight_ / 2, GL_RED, GL_UNSIGNED_BYTE,
    frame->data(2));
}

void VideoPlayer::renderPic() {
  if(glIsTexture(y_tex)) {
    // bind Texture
    bindTexture0(y_tex);
    bindTexture1(u_tex);
    bindTexture2(v_tex);

    // render picture
    useProgram(yuvShader);
    auto fw = float(frameWidth_);
    auto fh = float(frameHeight_);
    float ratio = fw / fh;
    fw = width_ / ratio < height_ ? width_ : height_ * ratio;
    fh = fw / ratio;

    glUniform4f(
      glGetUniformLocation(yuvShader, "transform"), fw / width_, fh / height_, 0, 0);
    bindVAO(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    bindTexture0(0);
    bindTexture1(0);
    bindTexture2(0);
  }
}

}