#include "syntactic_sugar.h"
#include <fmt/chrono.h>

namespace wumo {
std::mt19937 rng{std::random_device()()};

float random() {
  static std::uniform_real_distribution<float> dist{0.f, 1.f}; // rage 0 - 1
  return dist(rng);
}

float guassian() {
  static std::normal_distribution<float> dist{0.f, 1.f};
  return dist(rng);
}

int uniform(int min, int max) { return static_cast<int>(min + random() * (max - min)); }

float uniform(float min, float max) { return min + random() * (max - min); }

std::time_t currentTime() { return std::time(nullptr); }

double measure(std::function<void()> block) {
  auto start = std::chrono::high_resolution_clock::now();
  block();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> ms = end - start;
  return ms.count();
}

void printMeasure(std::function<void()> block, std::string msg) {
  auto duration = measure(block);
  println(msg, "duration: ", duration, "ms");
}

bool endWith(std::string const &fullString, std::string const &ending) {
  if(fullString.length() >= ending.length())
    return 0 == fullString.compare(
                  fullString.length() - ending.length(), ending.length(), ending);
  else
    return false;
}
}
