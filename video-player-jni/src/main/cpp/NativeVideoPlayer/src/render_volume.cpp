#include "player.h"

namespace wumo {
void VideoPlayer::renderVolume() {
  auto currentTS = glfwGetTime();
  if(requestShowVolume_) {
    requestShowVolume_ = false;
    lastShowVolumeTS = currentTS;
    volumeOnHover = true;
  } else if(
    input_.mouseEnterd &&
    mouseInRegion(
      width_ - barMarginRight - barIconWidth, height_ - barMarginBottom - barIconWidth,
      barIconWidth, barIconWidth)) {
    lastShowVolumeTS = currentTS;
    volumeOnHover = true;
  } else
    volumeOnHover = currentTS - lastShowVolumeTS < 1;

  if(expandBar) {
    auto color = volumeOnHover ? textBlueColor : textColor;
    renderChar(
      iconVolumeTex, width_ - barMarginRight - iconWidth, height_ - barMarginBottom,
      color);
  }

  if(volumeOnHover) {
    // backboard
    useProgram(colorShader);
    bindVAO(VAO);
    renderRect(
      width_ - barMarginRight - barIconWidth / 2 - volumeBackWidth / 2,
      height_ - floatWindowYOffset - volumeBackHeight, volumeBackWidth, volumeBackHeight,
      {0.16f, 0.157f, 0.165f, 1.f});

    //total
    renderRect(
      width_ - barMarginRight - barIconWidth / 2 - volumeInnerWidth / 2,
      height_ - floatWindowYOffset + volumeInnerMarginTop - volumeBackHeight / 2 -
        volumeInnerHeight / 2,
      volumeInnerWidth, volumeInnerHeight, {0.9f, 0.9f, 0.9f, 1.f});

    //progress
    renderRect(
      width_ - barMarginRight - barIconWidth / 2 - volumeInnerWidth / 2,
      height_ - floatWindowYOffset + volumeInnerMarginTop -
        (volumeBackHeight / 2 - volumeInnerHeight / 2) - volume_ * volumeInnerHeight,
      volumeInnerWidth, volume_ * volumeInnerHeight, {0.137f, 0.678f, 0.898f, 1.f});

    //progress pin
    renderRect(
      width_ - barMarginRight - barIconWidth / 2 - volumePinWidth / 2,
      height_ - floatWindowYOffset + volumeInnerMarginTop -
        (volumeBackHeight / 2 - volumeInnerHeight / 2) - volume_ * volumeInnerHeight -
        volumePinWidth / 2,
      volumePinWidth, volumePinWidth, {0.137f, 0.678f, 0.898f, 1.f});

    std::vector<Character> ints;
    float scale = 0.8;
    float rw{0}, rh{0};
    auto v = int(volume_ * 100);
    if(v == 0) {
      auto ch = findOrLoadFontTex('0');
      ints.push_back(ch);
      rw = ch.advance;
      rh = ch.bearing.y;
    } else
      while(v > 0) {
        auto c = v % 10;
        auto ch = findOrLoadFontTex(c + '0');
        rw += ch.advance;
        rh = std::max(rh, float(ch.bearing.y));
        ints.push_back(ch);
        v = v / 10;
      }
    rw *= scale;
    rh *= scale;
    auto x = width_ - barMarginRight - barIconWidth / 2 - rw / 2;
    auto y = height_ - floatWindowYOffset - volumeBackHeight + volumeTextMarginTop + rh;
    for(int i = ints.size() - 1; i >= 0; --i) {
      auto ch = ints[i];
      renderChar(ch, x, y, {0.9f, 0.9f, 0.9f, 1.f}, scale);
      x += ch.advance * scale;
    }
  }
}
}