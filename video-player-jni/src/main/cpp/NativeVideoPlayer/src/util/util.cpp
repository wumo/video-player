#include <sstream>
#include "util.h"
namespace wumo {
int32_t lengthOfNum(int32_t v) {
  auto count = 0;
  while(v > 0) {
    v = v / 10;
    count++;
  }
  return count;
}

std::string tsToStr(int64_t ts) {
  ts /= 1000;
  auto h = ts / 3600;
  ts %= 3600;
  auto s = ts % 60;
  auto m = ts / 60;
  std::stringstream ss;
  ss << std::setfill('0');
  if(h > 0) ss << std::setw(2) << h << ":";
  ss << std::setw(2) << m << ':' << std::setw(2) << s;
  return ss.str();
}
}