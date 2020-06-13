#include "player.h"
#include <sstream>
#include <cmath>

namespace wumo {
void VideoPlayer::renderInfoText() {
  if(toggleShowInfo) {
    {
      std::stringstream ss;
      ss << "fps: " << fps << " source: " << std::round(native.fps_) << '\n'
         << "resolution: " << native.width_ << 'x' << native.height_ << '\n'
         << "Audio: sample rate(" << native.src_rate << "), channels("
         << native.src_channels << "), sample format(" << native.src_sample_fmt << ")";
      auto text = ss.str();
      renderSimpleString(text, 10, 10, textHeight * 2, {1.0, 0.0, 0.0, 1.0}, 2.0);
    }
  }
}
}