#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <sstream>
#include <random>
#include <csignal>
#include <chrono>
#include <ctime>
#include <functional>

namespace wumo {
const auto uint32max = (std::numeric_limits<uint32_t>::max)();
template<typename Tp>
using uPtr = std::unique_ptr<Tp>;
template<typename Tp>
using sPtr = std::shared_ptr<Tp>;

using UniqueBytes = std::unique_ptr<unsigned char, std::function<void(unsigned char *)>>;
using UniqueConstBytes =
  std::unique_ptr<const unsigned char, std::function<void(const unsigned char *)>>;

float random();

int uniform(int min, int max);

float uniform(float min, float max);

float guassian();

template<typename T>
void print(T &&x) {
  std::cout << x << std::flush;
}

template<typename T>
void println(T &&x) {
  std::cout << x << std::endl;
}

template<typename T, typename... Args>
void print(T &&first, Args &&... args) {
  print(first);
  std::cout << " ";
  print(args...);
}

template<typename T, typename... Args>
void println(T &&first, Args &&... args) {
  print(first);
  std::cout << " ";
  print(args...);
  std::cout << std::endl;
}

template<typename T, typename... Args>
auto u(Args &&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename S>
bool contains(T &s, const S &k) {
  return s.find(k) != s.end();
}

template<typename T>
void append(std::vector<T> &dst, std::initializer_list<T> list) {
  dst.insert(dst.end(), list);
}

template<typename T>
void append(std::vector<T> &dst, const std::vector<T> &src) {
  dst.insert(dst.end(), src.begin(), src.end());
}

template<typename T>
void concat(std::stringstream &ss, T &&s) {
  ss << s;
}

template<typename T, typename... Args>
void concat(std::stringstream &ss, T &&s, Args &&... args) {
  ss << s;
  concat(ss, args...);
}

template<typename... Args>
std::string toString(Args &&... args) {
  std::stringstream ss;
  concat(ss, args...);
  return ss.str();
}

template<typename... Args>
void debugLog(Args &&... desc) {
#ifndef NDEBUG
  std::stringstream ss;
  concat(ss, desc...);
  std::cout << "[Debug] " << ss.str() << std::endl;
#endif
}

std::time_t currentTime();

double measure(std::function<void()> block);
void printMeasure(std::function<void()> block, const std::string msg = "");

bool endWith(std::string const &fullString, std::string const &ending);

//void signal_handler(int signum);

template<typename... Args>
void error(Args &&... desc) {
  std::stringstream ss;
  concat(ss, desc...);

  std::cerr << "[Error] " << ss.str() << std::endl;
  //    << boost::stacktrace::stacktrace() << std::endl;
  throw std::runtime_error(ss.str());
}

template<typename... Args>
void errorIf(bool p, Args &&... desc) {
  if(p) {
    std::stringstream ss;
    concat(ss, desc...);
    std::cerr << "[Error] " << ss.str() << std::endl;
    //      << boost::stacktrace::stacktrace() << std::endl;
    throw std::runtime_error(ss.str());
  }
}

template<typename E>
constexpr typename std::underlying_type<E>::type value(E e) noexcept {
  return static_cast<typename std::underlying_type<E>::type>(e);
}

#define __ptr__(ClassType) using ptr = std::unique_ptr<ClassType>;

#define __declare_only_move__(ClassType)            \
  ClassType(const ClassType &) = delete;            \
  ClassType &operator=(const ClassType &) = delete; \
  ClassType(ClassType &&other) noexcept;            \
  ClassType &operator=(ClassType &&other) noexcept;

#define __only_move__(ClassType)                    \
  ClassType(const ClassType &) = delete;            \
  ClassType &operator=(const ClassType &) = delete; \
  ClassType(ClassType &&other) = default;           \
  ClassType &operator=(ClassType &&other) = default;

#define __default_only_move__(ClassType)            \
  ClassType() = default;                            \
  ClassType(const ClassType &) = delete;            \
  ClassType &operator=(const ClassType &) = delete; \
  ClassType(ClassType &&other) = default;           \
  ClassType &operator=(ClassType &&other) = default;

#define ctor_dtor_move(ClassType)                   \
  ClassType() = default;                            \
  ~ClassType();                                     \
  ClassType(const ClassType &) = delete;            \
  ClassType &operator=(const ClassType &) = delete; \
  ClassType(ClassType &&other) = default;           \
  ClassType &operator=(ClassType &&other) = default;

}
