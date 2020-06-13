#include "player.h"
#include <fstream>
#include <thread>
#include <chrono>

using namespace wumo;

class FileRead: public InputStreamCallback {

public:
  explicit FileRead(const std::string &path): fs{path, std::ios::in | std::ios::binary} {
    fs.seekg(0, std::fstream::end);
    size = fs.tellg();
    fs.seekg(0, std::fstream::beg);
  }

  int read(uint8_t *buf, int buf_size) override {
    fs.read((char *)(buf), buf_size);
    return fs.gcount();
  }

  int64_t seek(int64_t offset, int whence) override {
    switch(whence) {
      case SEEK_SET:
        if(fs.eof()) fs.clear();
        fs.seekg(offset, std::fstream::beg);
        break;
      case SEEK_CUR: fs.seekg(offset, std::fstream::cur); break;
      case SEEK_END: fs.seekg(offset, std::fstream::end); break;
      case AVSEEK_SIZE: return size;
    }
    return 0;
  }

  void stop() override { fs.close(); }

private:
  std::fstream fs;
  int64_t size;
};

int main() {

  rigtorp::MPMCQueue<Frame *> videoFrames{10};

  FileRead file1{"./assets/b.mkv"};
  FrameFetcher native{};
  auto decode = [&] {
    native.open(&file1);
    while(true) {
      auto frame = native.read_frame();
      if(frame == nullptr) break;
      if(frame->type() == Frame::TYPE_VIDEO) videoFrames.push(frame);
      else if(frame->type() == Frame::TYPE_AUDIO) {
        native.recycleFrame(frame);
      }
    }
  };
  std::thread decodeT{decode};

  VideoPlayer player{native};
  player.init();
  player.addPlayEntry({0x3010});
  player.setCurrentPlay(1);
  player.setCurrentHasLocal(true);
  player.setCurrentHasRemote(true);
  auto i = 0;
  while(!player.shouldClose()) {
    if(i++ > 10000) player.requestClose();
    player.pollEvents();
    Frame *frame;
    if(videoFrames.try_pop(frame)) {
      player.updateTex(frame);
      player.updateProgress(100000, 20000000);
      player.updateVolume(0.6);
      native.recycleFrame(frame);
    }
    player.render();
    std::this_thread::sleep_for(std::chrono::milliseconds{1000 / 60});
    player.present();
  }
  player.stop();
  println("stop");
  //
  //  std::thread uiT{ui};
  //

  return 0;
}