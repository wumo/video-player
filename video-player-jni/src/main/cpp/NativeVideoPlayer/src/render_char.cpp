#include "player.h"
#include "resources/icon_ttf.h"
#include "resources/msyh_ttc.h"

namespace wumo {

void VideoPlayer::loadFont() {
  glGenTextures(1, &charTex);
  bindTexture0(charTex);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RED, texWidth, texWidth, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  auto ret = FT_Init_FreeType(&ft);
  errorIf(ret != 0, "ERROR::FREETYPE: Could not init FreeType Library");
  ret = FT_New_Memory_Face(
    ft, (const FT_Byte *)icon_ttf, icon_ttf_size * sizeof(uint32_t), 0, &face);
  errorIf(ret != 0, "ERROR::FREETYPE: Failed to load font");

  FT_Set_Pixel_Sizes(face, 0, iconWidth);

  iconPlayTex = findOrLoadFontTex(icon_play);
  iconPauseTex = findOrLoadFontTex(icon_pause);
  iconNextTex = findOrLoadFontTex(icon_next);
  iconVolumeTex = findOrLoadFontTex(icon_volume);
  iconListTex = findOrLoadFontTex(icon_list);
  iconReplayTex = findOrLoadFontTex(icon_replay);
  iconDownloadTex = findOrLoadFontTex(icon_download);
  iconFolderTex = findOrLoadFontTex(icon_folder);
  iconBrowserTex = findOrLoadFontTex(icon_browser);

  FT_Done_Face(face);

  ret = FT_New_Memory_Face(
    ft, (const FT_Byte *)msyh_ttc, msyh_ttc_size * sizeof(uint32_t), 0, &face);
  errorIf(ret != 0, "ERROR::FREETYPE: Failed to load font");

  FT_Set_Pixel_Sizes(face, 0, textHeight);
}

VideoPlayer::Character VideoPlayer::findOrLoadFontTex(int32_t c) {
  auto it = characters.find(c);
  Character ch{};
  if(it == characters.end()) { // load glyph
    auto ret = FT_Load_Char(face, c, FT_LOAD_RENDER);
    errorIf(ret, "error load glyph for ", c);

    auto nx = characters.size() % maxCharPerLine;
    auto ny = characters.size() / maxCharPerLine;
    ch.tex = charTex;
    ch.offset = {nx * iconWidth, ny * iconWidth};
    ch.size = {face->glyph->bitmap.width, face->glyph->bitmap.rows};
    ch.bearing = {face->glyph->bitmap_left, face->glyph->bitmap_top};
    ch.advance = face->glyph->advance.x >> 6;

    bindTexture0(ch.tex);
    glTexSubImage2D(
      GL_TEXTURE_2D, 0, ch.offset.x, ch.offset.y, ch.size.x, ch.size.y, GL_RED,
      GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
    characters[c] = ch;
  } else
    ch = it->second;
  return ch;
}

void VideoPlayer::renderChar(
  Character &ch, float x, float y, const glm::vec4 &color, float scale) {
  useProgram(texAtlasShader);
  bindTexture0(ch.tex);
  glUniform4f(
    glGetUniformLocation(texAtlasShader, "transform"), ch.size.x * scale / width_,
    ch.size.y * scale / height_,
    -1 + (x + (ch.bearing.x + ch.size.x / 2.f) * scale) / (width_ / 2.f),
    1 - (y - ch.bearing.y * scale + ch.size.y * scale / 2.f) / (height_ / 2.f));
  glUniform4f(
    glGetUniformLocation(texAtlasShader, "transformTex"), ch.size.x / float(texWidth),
    ch.size.y / float(texWidth), ch.offset.x / float(texWidth),
    ch.offset.y / float(texWidth));
  glUniform4fv(glGetUniformLocation(texAtlasShader, "color"), 1, (GLfloat *)(&color));
  bindVAO(VAO);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

bool VideoPlayer::renderChar(
  Character &ch, float x, float y, float minX, float maxX, float minY, float maxY,
  glm::vec4 &color, float scale) {
  if(ch.size.x == 0 && ch.size.y == 0) return true;
  auto yTop = std::max(y - ch.bearing.y * scale, minY);
  auto yBot = std::min(y - ch.bearing.y * scale + ch.size.y * scale, maxY);
  auto xLeft = std::max(x + ch.bearing.x * scale, minX);
  auto xRight = std::min(x + ch.bearing.x * scale + ch.size.x * scale, maxX);
  auto sizeX = xRight - xLeft;
  auto sizeY = yBot - yTop;
  if(sizeX <= 0) return false;
  if(sizeY <= 0) return false;

  auto texXOffset = xLeft - (x + ch.bearing.x * scale);
  auto texYOffset = yTop - (y - ch.bearing.y * scale);

  useProgram(texAtlasShader);
  bindTexture0(ch.tex);
  glUniform4f(
    glGetUniformLocation(texAtlasShader, "transform"), sizeX / width_, sizeY / height_,
    -1 + (xLeft + sizeX / 2.f) / (width_ / 2.f),
    1 - (yTop + sizeY / 2.f) / (height_ / 2.f));
  glUniform4f(
    glGetUniformLocation(texAtlasShader, "transformTex"), sizeX / scale / float(texWidth),
    sizeY / scale / float(texWidth), (ch.offset.x + texXOffset / scale) / float(texWidth),
    (ch.offset.y + texYOffset / scale) / float(texWidth));
  glUniform4fv(glGetUniformLocation(texAtlasShader, "color"), 1, (GLfloat *)(&color));
  bindVAO(VAO);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  return true;
}

void VideoPlayer::renderSimpleString(
  const std::string &str, float x, float y, float height, const glm::vec4 &color,
  float scale) {
  auto startX = x;
  y += height;
  for(char c: str) {
    if(c == '\n') {
      x = startX;
      y += height;
      continue;
    }
    auto ch = findOrLoadFontTex(c);
    renderChar(ch, x, y, color, scale);
    x += ch.advance * scale;
  }
}
}