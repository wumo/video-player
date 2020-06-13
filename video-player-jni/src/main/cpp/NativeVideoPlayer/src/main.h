#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}
#include <iostream>
#include <string>
#include <vector>
#include "syntactic_sugar.h"
#include <queue>
#include "MPMCQueue.h"

namespace wumo {

class InputStreamCallback {
public:
  virtual int read(uint8_t *buf, int buf_size);
  virtual int64_t seek(int64_t offset, int whence);
  virtual void stop();
};

class Frame {
public:
  const static int TYPE_UNKNOWN = AVMEDIA_TYPE_UNKNOWN;
  const static int TYPE_VIDEO = AVMEDIA_TYPE_VIDEO;
  const static int TYPE_AUDIO = AVMEDIA_TYPE_AUDIO;
  const static int TYPE_DATA = AVMEDIA_TYPE_DATA;
  const static int TYPE_SUBTITLE = AVMEDIA_TYPE_SUBTITLE;
  const static int TYPE_ATTACHMENT = AVMEDIA_TYPE_ATTACHMENT;
  const static int TYPE_NB = AVMEDIA_TYPE_NB;

private:
  friend class FrameFetcher;
  explicit Frame(int frameQueueIdx, int id);
  explicit Frame();

private:
  int frameQueueIdx_;
  int id_;
  int64_t pts_{};
  AVMediaType type_{AVMEDIA_TYPE_UNKNOWN};
  uint8_t *data_[AV_NUM_DATA_POINTERS]{};
  int linesize_[AV_NUM_DATA_POINTERS]{};
  int width_{}, height_{};
  int bufsize_{};

public:
  int64_t pts() const;
  int type() const;
  int frameQueueIdx() const;
  int id() const;
  int width() const;
  int height() const;
  int bufsize() const;
  uint8_t *data(int idx);
  int linesize(int idx);
};

class FrameFetcher {
  friend class VideoPlayer;

public:
  const static int dst_rate = 48000;
  const static int dst_channels = 2;
  const static int dst_sample_fmt = AV_SAMPLE_FMT_S16;

  const static int seek_set = SEEK_SET;
  const static int seek_cur = SEEK_CUR;
  const static int seek_end = SEEK_END;
  const static int seek_size = AVSEEK_SIZE;

public:
  explicit FrameFetcher(size_t videoQueueSize = 5, size_t audioQueueSize = 10);
  virtual ~FrameFetcher();

  void open(InputStreamCallback *read1, InputStreamCallback *read2 = nullptr);
  void open(const char *url);
  void stop();

  const int dst_picture_fmt;
  bool hasVideo();
  bool hasAudio();
  int64_t duration() const;
  double fps() const;
  int width() const;
  int height() const;

  int srcRate() const;
  int srcChannels() const;
  int srcSampleFmt() const;

  Frame *read_frame(Frame *frame = nullptr);
  Frame *read_frame2(Frame *frame = nullptr);

  void recycleFrame(Frame *frame);

  double volume = 1.0;
  void seek(int64_t timestampMilis);
  void setPreciseSeek(bool preciseSeek);

private:
  void openInternal();

  Frame *read_frame_for(
    AVFormatContext *fmtCtxPtr, AVPacket *pktPtr, bool &shouldReadPktSignal,
    bool &pktRecvVideoSignal, int streamIndexOffset, Frame *frame);

  bool recv_video_frame(Frame *frame);
  bool recv_audio_frame(Frame *frame);
  
private:
  size_t videoQueueSize, audioQueueSize;
  bool isClosed{true};

  const int avio_ctx_buffer_size = 1024 * 8;
  AVIOContext *avio_ctx{nullptr}, *avio2_ctx{nullptr};
  AVPacket *pkt{nullptr}, *pkt2{nullptr};
  AVFrame *videoFrame_{nullptr}, *audioFrame_{nullptr};
  bool shouldReadPkt{true}, pktRecvVideo{false};
  bool shouldReadPkt2{true}, pkt2RecvVideo{false};

  rigtorp::MPMCQueue<Frame *> frames[2];

  AVFormatContext *fmt_ctx{nullptr}, *fmt2_ctx{nullptr};
  int video_stream_index{-1};
  int audio_stream_index{-1};
  AVRational video_timebase;
  AVRational audio_timebase;
  long video_timestamp_{0L};
  long audio_timestamp_{0L};
  AVCodecContext *videoCodecCtx{nullptr};
  AVCodecContext *audioCodecCtx{nullptr};
  int64_t duration_{0L};
  double fps_{};
  int width_{}, height_{};
  SwsContext *swscale_ctx{nullptr};
  int src_rate{};
  int src_channels{};
  int src_sample_fmt{};
  SwrContext *swresample_ctx{nullptr};
  int dst_ch_layout;
  bool preciseSeek_{false};
};
}