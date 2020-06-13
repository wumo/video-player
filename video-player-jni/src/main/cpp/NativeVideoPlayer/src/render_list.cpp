#include "player.h"

namespace wumo {

void VideoPlayer::addPlayEntry(std::vector<int32_t> entry) {
  entryQueue.emplace(std::move(entry));
}

int32_t VideoPlayer::requestPlayIndex() { return requestPlayIndex_; }
void VideoPlayer::finishRequestPlayIndex() { requestPlayIndex_ = -1; }
void VideoPlayer::setCurrentPlay(int32_t idx) { currentPlay_ = idx; }
int32_t VideoPlayer::currentPlay() { return currentPlay_; }
void VideoPlayer::setCurrentHasLocal(bool local) { currentHasLocal = local; }
void VideoPlayer::setCurrentHasRemote(bool remote) { currentHasRemote = remote; }
bool VideoPlayer::requestOpenLocal() const { return requestOpenLocal_; }
bool VideoPlayer::requestOpenRemote() const { return requestOpenRemote_; }
void VideoPlayer::finishOpenLocal() { requestOpenLocal_ = false; }
void VideoPlayer::finishOpenRemote() { requestOpenRemote_ = false; }

void VideoPlayer::renderList() {
  {
    std::vector<int32_t> entry;
    while(entryQueue.try_pop(entry))
      entries.push_back(std::move(entry));
  }

  auto currentTS = glfwGetTime();
  if(mouseInRegion(
       width_ - barMarginRight - barIconWidth - barSpacing - barIconWidth,
       height_ - barMarginBottom - barIconWidth, barIconWidth, barIconWidth)) {
    lastShowListTS = currentTS;
    showList = true;
  } else {
    showList = currentTS - lastShowListTS < 1;
    hoverOnList = false;
  }
  if(showList) {
    if(
      input_.mouseEnterd &&
      mouseInRegion(
        width_ - listMarginRight - listWidth,
        height_ - floatWindowYOffset - listMaxHeight, listWidth, listMaxHeight)) {
      lastShowListTS = currentTS;
      hoverOnList = true;
    }
  }

  if(expandBar) {
    auto color = showList ? textBlueColor : textColor;
    renderChar(
      iconListTex, width_ - barMarginRight - iconWidth - barSpacing - barIconWidth,
      height_ - barMarginBottom, color);
  }
  if(!showList) return;

  renderRect(
    width_ - listMarginRight - listWidth, height_ - floatWindowYOffset - listMaxHeight,
    listWidth, listMaxHeight, listBackColor);

  useProgram(texAtlasShader);
  auto minScrollYoffset =
    std::min(-(listLineHeight * entries.size() - listMaxHeight + listInnerMarginV), 0.f);
  listScrollYOffset = std::clamp(listScrollYOffset, minScrollYoffset, 0.f);
  auto startX = width_ - listMarginRight - listWidth + listInnerMarginH;
  auto startY = height_ - floatWindowYOffset - listMaxHeight + float(listLineHeight) -
                listInnerMarginV + listScrollYOffset;
  auto minX = width_ - listMarginRight - listWidth + listInnerMarginH;
  auto maxX = width_ - listMarginRight - listInnerMarginH;
  auto minY = height_ - floatWindowYOffset - listMaxHeight + listInnerMarginV;
  auto maxY = height_ - floatWindowYOffset - listInnerMarginV;
  auto y = startY;
  auto len = lengthOfNum(entries.size());
  for(int idx = 0; idx < entries.size(); ++idx) {
    auto &entry = entries[idx];
    auto color = idx == currentPlay_ ? textBlueColor : textColor;
    auto x = startX;

    if(mouseInRegion(
         width_ - listMarginRight - listWidth, y - listLineHeight + listInnerMarginV,
         listWidth, listLineHeight)) {
      auto yTop = std::max(y - listLineHeight + listInnerMarginV, minY);
      auto yBot = std::min(y + listInnerMarginV, maxY);
      auto yH = yBot - yTop;
      if(yH >= 0) {
        renderRect(
          width_ - listMarginRight - listWidth, yTop, listWidth, yH, listHoverColor);
        if(clickOnList) {
          clickOnList = false;
          requestPlayIndex_ = idx;
          println("request play ", idx);
        }
      }
    }

    if(idx == currentPlay_) {
      auto scale = float(textHeight) / iconWidth;
      renderChar(iconPlayTex, x, y, minX, maxX, minY, maxY, color, scale);
      x += iconPlayTex.advance * scale + 4.0;
    }

    //render index number
    std::vector<Character> ints;
    auto v = idx + 1;
    while(v > 0) {
      auto c = v % 10;
      Character ch = findOrLoadFontTex(c + '0');
      ints.push_back(ch);
      v = v / 10;
    }
    for(int i = len - ints.size(); i > 0; --i) {
      Character ch = findOrLoadFontTex('0');
      ints.push_back(ch);
    }
    //render leading zero
    for(int i = ints.size() - 1; i >= 0; --i) {
      auto ch = ints[i];
      renderChar(ch, x, y, minX, maxX, minY, maxY, color);
      x += ch.advance;
    }
    Character ch = findOrLoadFontTex('.');
    renderChar(ch, x, y, minX, maxX, minY, maxY, color);
    x += ch.advance;

    for(auto &c: entry) {
      Character ch = findOrLoadFontTex(c);

      bindTexture0(ch.tex);

      if(!renderChar(ch, x, y, minX, maxX, minY, maxY, color)) break;
      x += ch.advance;
    }
    y += listLineHeight;
  }
}

}