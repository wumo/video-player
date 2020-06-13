#include "player.h"

namespace wumo {

void VideoPlayer::renderRect(float x, float y, float w, float h, glm::vec4 color) {
  useProgram(colorShader);
  bindVAO(VAO);
  glUniform4f(
    glGetUniformLocation(colorShader, "transform"), w / width_, h / height_,
    -1.f + (x + w / 2) / (width_ / 2.f), 1 - (y + h / 2) / (height_ / 2.f));
  glUniform4fv(glGetUniformLocation(colorShader, "color"), 1, (GLfloat *)(&color));
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void VideoPlayer::renderUI() {
  auto currentTS = glfwGetTime();
  expandBar = false;
  if(
    input_.mouseEnterd &&
    (input_.mouseXPos_ != lastXPos || input_.mouseYPos_ != lastYPos ||
     mouseInRegion(0, height_ - pauseRange, width_, pauseRange) || hoverOnList)) {
    lastXPos = input_.mouseXPos_;
    lastYPos = input_.mouseYPos_;

    lastShowBarTS = currentTS;
    expandBar = true;
  } else
    expandBar = currentTS - lastShowBarTS < 1;
  renderProgressBar();
  renderPlayButton();
  renderPlayNext();
  renderVolume();
  renderList();
  renderInfoText();
  renderLocalRemoteButton();
}

void VideoPlayer::renderProgressBar() {
  if(expandBar) {
    renderRect(0, height_ - barHeight, width_, barHeight, barBackColor);
    auto progress = tsToStr(pts_) + " / " + tsToStr(totalTS_);
    auto x = barMarginLeft + iconWidth * 2.f + barSpacing * 2.f;
    auto y = height_ - barMarginBottom;
    renderSimpleString(progress, x, y - iconWidth, iconWidth, textColor);
  }

  auto barYOffset = expandBar ? barHeight : 0.f;
  renderRect(0, height_ - barYOffset - 5, width_, 5, {0.443f, 0.443f, 0.443f, 1.f});
  renderRect(
    0, height_ - barYOffset - 5, float(pts_) / totalTS_ * width_, 5,
    {0.137f, 0.678f, 0.898f, 1.f});
}

void VideoPlayer::renderPlayNext() {
  if(expandBar) {
    auto color =
      (mouseInRegion(
        barMarginLeft + barIconWidth + barSpacing,
        height_ - barMarginBottom - barIconWidth, barIconWidth, barIconWidth)) ?
        textBlueColor :
        textColor;
    renderChar(
      iconNextTex, barMarginLeft + barIconWidth + barSpacing, height_ - barMarginBottom,
      color);
  }
}

void VideoPlayer::renderPlayButton() {
  if(expandBar) {
    auto tex = isPaused ? iconPlayTex : iconPauseTex;
    if(finishedPlay) tex = iconReplayTex;

    auto color = mouseInRegion(
                   barMarginLeft, height_ - barMarginBottom - barIconWidth, barIconWidth,
                   barIconWidth) ?
                   textBlueColor :
                   textColor;
    renderChar(tex, barMarginLeft, height_ - barMarginBottom, color);
  }
}

void VideoPlayer::handleLocalRemoteButtonClickEvent() {
  if(expandBar) {
    auto x = width_ - barMarginRight - (barIconWidth + barSpacing) * 2 - barIconWidth;
    auto y = height_ - barMarginBottom;
    if(currentHasRemote) {
      if(mouseInRegion(x, y - barIconWidth, barIconWidth, barIconWidth)) {
        requestOpenRemote_ = true;
      }
      x -= barIconWidth + barSpacing;
    }

    if(currentHasLocal) {
      if(mouseInRegion(x, y - barIconWidth, barIconWidth, barIconWidth)) {
        requestOpenLocal_ = true;
      }
    }
  }
}

void VideoPlayer::renderLocalRemoteButton() {
  if(expandBar) {
    auto x = width_ - barMarginRight - (barIconWidth + barSpacing) * 2 - barIconWidth;
    auto y = height_ - barMarginBottom;
    if(currentHasRemote) {
      auto color = (mouseInRegion(x, y - barIconWidth, barIconWidth, barIconWidth)) ?
                     textBlueColor :
                     textColor;
      auto ch = currentHasLocal ? iconBrowserTex : iconDownloadTex;
      renderChar(ch, x, y, color);
      x -= barIconWidth + barSpacing;
    }

    if(currentHasLocal) {
      auto color = (mouseInRegion(x, y - barIconWidth, barIconWidth, barIconWidth)) ?
                     textBlueColor :
                     textColor;
      renderChar(iconFolderTex, x, y, color);
    }
  }
}
}