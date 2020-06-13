#include "player.h"

namespace wumo {

const char *transformVert =
  R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec4 transform;

void main()
{
  vec2 scale=transform.xy;
  vec2 offset=transform.zw;
  vec2 pos=aPos.xy;
  gl_Position = vec4(pos*scale+offset,aPos.z, 1.0);
  TexCoord = vec2(aTexCoord.x, 1-aTexCoord.y);
}
)";

const char *transformTexVert =
  R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec4 transform;
uniform vec4 transformTex;

void main()
{
  vec2 scale=transform.xy;
  vec2 offset=transform.zw;
  gl_Position = vec4(aPos.xy * scale + offset, aPos.z, 1.0);
  TexCoord = vec2(aTexCoord.x, 1-aTexCoord.y);
  vec2 texScale=transformTex.xy;
  vec2 texOffset=transformTex.zw;
  TexCoord=TexCoord*texScale+texOffset;
}
)";

const char *videoFrag =
  R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D y_tex;
uniform sampler2D u_tex;
uniform sampler2D v_tex;

const vec3 R_cf = vec3(1.164383,  0.000000,  1.596027);
const vec3 G_cf = vec3(1.164383, -0.391762, -0.812968);
const vec3 B_cf = vec3(1.164383,  2.017232,  0.000000);
const vec3 offset = vec3(-0.0625, -0.5, -0.5);

void main()
{
  float y = texture(y_tex, TexCoord).r;
  float u = texture(u_tex, TexCoord).r;
  float v = texture(v_tex, TexCoord).r;
  vec3 yuv = vec3(y,u,v);
  yuv += offset;
  FragColor.r = dot(yuv, R_cf);
  FragColor.g = dot(yuv, G_cf);
  FragColor.b = dot(yuv, B_cf);
  FragColor.a = 1.0;
}
)";

const char *colorFrag =
  R"(
#version 330 core
out vec4 FragColor;

uniform vec4 color;

void main(){ FragColor = color; }
)";

const char *textureFrag =
  R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D tex;
uniform vec4 color;

void main(){
  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(tex, TexCoord).r);
  FragColor = color * sampled;
}
)";

int VideoPlayer::compileShader(
  const char *vert, const char *frag) { // build and compile our shader program
  // ------------------------------------
  // vertex shader
  int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vert, nullptr);
  glCompileShader(vertexShader);
  // check for shader compile errors
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if(!success) {
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  // fragment shader
  int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &frag, nullptr);
  glCompileShader(fragmentShader);
  // check for shader compile errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if(!success) {
    glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  // link shaders
  auto shader = glCreateProgram();
  glAttachShader(shader, vertexShader);
  glAttachShader(shader, fragmentShader);
  glLinkProgram(shader);
  // check for linking errors
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if(!success) {
    glGetProgramInfoLog(shader, 512, nullptr, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
  }
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  return shader;
}

void VideoPlayer::createShader() {
  yuvShader = compileShader(transformVert, videoFrag);
  colorShader = compileShader(transformVert, colorFrag);
  texShader = compileShader(transformVert, textureFrag);
  texAtlasShader = compileShader(transformTexVert, textureFrag);

  glUseProgram(yuvShader);
  glUniform1i(glGetUniformLocation(yuvShader, "y_tex"), 0);
  glUniform1i(glGetUniformLocation(yuvShader, "u_tex"), 1);
  glUniform1i(glGetUniformLocation(yuvShader, "v_tex"), 2);

  glUseProgram(texShader);
  glUniform1i(glGetUniformLocation(texShader, "tex"), 0);

  glUseProgram(texAtlasShader);
  glUniform1i(glGetUniformLocation(texAtlasShader, "tex"), 0);
}

}