#include "main.h"
#include <string>
#include <vector>
#include <chrono>
#include <thread>
namespace wumo {

int InputStreamCallback::read(uint8_t *buf, int buf_size) {
  errorIf(true, "not implemented");
  return 0;
}
int64_t InputStreamCallback::seek(int64_t offset, int whence) {
  errorIf(true, "not implemented");
  return 0;
}
void InputStreamCallback::stop() { errorIf(true, "not implemented"); }

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
  auto me = static_cast<InputStreamCallback *>(opaque);
  return me->read(buf, buf_size);
}
static int64_t seek_packet(void *opaque, int64_t offset, int whence) {
  auto me = static_cast<InputStreamCallback *>(opaque);
  return me->seek(offset, whence);
}

Frame::Frame(int frameQueueIdx, int id): frameQueueIdx_(frameQueueIdx), id_{id} {}
int64_t Frame::pts() const { return pts_; }
int Frame::type() const { return type_; }

uint8_t *Frame::data(int idx) { return data_[idx]; }
int Frame::linesize(int idx) { return linesize_[idx]; }
int Frame::width() const { return width_; }
int Frame::height() const { return height_; }
int Frame::bufsize() const { return bufsize_; }
int Frame::frameQueueIdx() const { return frameQueueIdx_; }
int Frame::id() const { return id_; }
Frame::Frame(): frameQueueIdx_{-1}, id_{-1} {}

FrameFetcher::FrameFetcher(size_t videoQueueSize, size_t audioQueueSize)
  : dst_picture_fmt{AV_PIX_FMT_YUV420P},
    videoQueueSize{videoQueueSize},
    audioQueueSize{audioQueueSize},
    frames{
      rigtorp::MPMCQueue<Frame *>{videoQueueSize},
      rigtorp::MPMCQueue<Frame *>{audioQueueSize}} {
  for(int i = 0; i < videoQueueSize; ++i)
    frames[0].push(new Frame{0, i});
  for(int i = 0; i < audioQueueSize; ++i)
    frames[1].push(new Frame{1, i});
}
FrameFetcher::~FrameFetcher() { stop(); }

void FrameFetcher::open(InputStreamCallback *read1, InputStreamCallback *read2) {
  errorIf(read1 == nullptr, "first input callback shouldn't be null!");
  auto avio_ctx_buffer = static_cast<uint8_t *>(av_malloc(avio_ctx_buffer_size));
  avio_ctx = avio_alloc_context(
    avio_ctx_buffer, avio_ctx_buffer_size, 0, read1, read_packet, nullptr, seek_packet);
  errorIf(!avio_ctx, "cannot alloc avio ctx");
  fmt_ctx = avformat_alloc_context();
  errorIf(!fmt_ctx, "cannot alloc fmt ctx");
  fmt_ctx->pb = avio_ctx;
  auto ret = avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr);
  errorIf(ret < 0, "Could not open file ", ret);

  if(read2 != nullptr) {
    auto avio2_ctx_buffer = static_cast<uint8_t *>(av_malloc(avio_ctx_buffer_size));
    avio2_ctx = avio_alloc_context(
      avio2_ctx_buffer, avio_ctx_buffer_size, 0, read2, read_packet, nullptr,
      seek_packet);
    errorIf(!avio2_ctx, "cannot alloc avio ctx");
    fmt2_ctx = avformat_alloc_context();
    errorIf(!fmt2_ctx, "cannot alloc fmt ctx");
    fmt2_ctx->pb = avio2_ctx;
    ret = avformat_open_input(&fmt2_ctx, nullptr, nullptr, nullptr);
    errorIf(ret < 0, "Could not open file ", ret);
  }
  openInternal();
}

void FrameFetcher::open(const char *url) {
  auto ret = avformat_open_input(&fmt_ctx, url, nullptr, nullptr);
  errorIf(ret < 0, "Could not open file ", ret);
  openInternal();
}

void FrameFetcher::openInternal() {
  isClosed = false;
  auto ret = avformat_find_stream_info(fmt_ctx, nullptr);
  errorIf(ret < 0, "Could not find stream information ");
  if(fmt2_ctx) {
    ret = avformat_find_stream_info(fmt2_ctx, nullptr);
    errorIf(ret < 0, "Could not find stream information ");
  }
  duration_ = fmt_ctx->duration / 1000;
  for(auto i = 0; i < fmt_ctx->nb_streams; i++) {
    auto stream = fmt_ctx->streams[i];
    auto codecpar = stream->codecpar;
    if(codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1) {
      fps_ = av_q2d(stream->r_frame_rate);
      video_timebase = stream->time_base;
      video_stream_index = i;
      auto codec = avcodec_find_decoder(codecpar->codec_id);
      errorIf(!codec, "Unsupported codec!");
      videoCodecCtx = avcodec_alloc_context3(codec);
      ret = avcodec_parameters_to_context(videoCodecCtx, codecpar);
      errorIf(ret != 0, "Could not copy codec context.");
      ret = avcodec_open2(videoCodecCtx, codec, nullptr);
      errorIf(ret < 0, "Could not open codec.");
    } else if(codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1) {
      audio_timebase = stream->time_base;
      audio_stream_index = i;
      auto codec = avcodec_find_decoder(codecpar->codec_id);
      errorIf(!codec, "Unsupported codec!");
      audioCodecCtx = avcodec_alloc_context3(codec);
      ret = avcodec_parameters_to_context(audioCodecCtx, codecpar);
      errorIf(ret != 0, "Could not copy codec context.");
      ret = avcodec_open2(audioCodecCtx, codec, nullptr);
      errorIf(ret < 0, "Could not open codec.");
    }
  }
  int idxOffset = int(fmt_ctx->nb_streams);
  if(fmt2_ctx)
    for(auto i = 0; i < fmt2_ctx->nb_streams; i++) {
      auto stream = fmt2_ctx->streams[i];
      auto codecpar = stream->codecpar;
      if(codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1) {
        fps_ = av_q2d(stream->r_frame_rate);
        video_timebase = stream->time_base;
        video_stream_index = idxOffset + i;
        auto codec = avcodec_find_decoder(codecpar->codec_id);
        errorIf(!codec, "Unsupported codec!");
        videoCodecCtx = avcodec_alloc_context3(codec);
        ret = avcodec_parameters_to_context(videoCodecCtx, codecpar);
        errorIf(ret != 0, "Could not copy codec context.");
        ret = avcodec_open2(videoCodecCtx, codec, nullptr);
        errorIf(ret < 0, "Could not open codec.");
      } else if(codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1) {
        audio_timebase = stream->time_base;
        audio_stream_index = idxOffset + i;
        auto codec = avcodec_find_decoder(codecpar->codec_id);
        errorIf(!codec, "Unsupported codec!");
        audioCodecCtx = avcodec_alloc_context3(codec);
        ret = avcodec_parameters_to_context(audioCodecCtx, codecpar);
        errorIf(ret != 0, "Could not copy codec context.");
        ret = avcodec_open2(audioCodecCtx, codec, nullptr);
        errorIf(ret < 0, "Could not open codec.");
      }
    }
  pkt = av_packet_alloc();
  errorIf(!pkt, "Could not allocate packet");
  videoFrame_ = av_frame_alloc();
  errorIf(!videoFrame_, "Could not allocate frame");
  audioFrame_ = av_frame_alloc();
  errorIf(!audioFrame_, "Could not allocate frame");

  if(fmt2_ctx) {
    pkt2 = av_packet_alloc();
    errorIf(!pkt2, "Could not allocate packet");
  }

  if(video_stream_index != -1) {
    swscale_ctx = sws_getContext(
      videoCodecCtx->width, videoCodecCtx->height, videoCodecCtx->pix_fmt,
      videoCodecCtx->width, videoCodecCtx->height, (AVPixelFormat)dst_picture_fmt,
      SWS_BILINEAR, nullptr, nullptr, nullptr);
    width_ = videoCodecCtx->width;
    height_ = videoCodecCtx->height;
  }

  if(audio_stream_index != -1) {
    auto src_ch_layout = audioCodecCtx->channels == av_get_channel_layout_nb_channels(
                                                      audioCodecCtx->channel_layout) ?
                           audioCodecCtx->channel_layout :
                           av_get_default_channel_layout(audioCodecCtx->channels);
    errorIf(src_ch_layout <= 0, "invalid src ch layout");
    switch(dst_channels) {
      case 1: dst_ch_layout = AV_CH_LAYOUT_MONO; break;
      case 2: dst_ch_layout = AV_CH_LAYOUT_STEREO; break;
      default: dst_ch_layout = AV_CH_LAYOUT_SURROUND; break;
    }
    src_rate = audioCodecCtx->sample_rate;
    src_channels = audioCodecCtx->channels;
    src_sample_fmt = audioCodecCtx->sample_fmt;
    swresample_ctx = swr_alloc_set_opts(
      nullptr, dst_ch_layout, AVSampleFormat(dst_sample_fmt), dst_rate, src_ch_layout,
      AVSampleFormat(src_sample_fmt), src_rate, 0, nullptr);
    ret = swr_init(swresample_ctx);
    errorIf(ret < 0, "error init swr ctx");
  }
}

void FrameFetcher::stop() {
  if(isClosed) return;
  shouldReadPkt = true;
  shouldReadPkt2 = true;
  if(videoFrame_) {
    av_frame_free(&videoFrame_);
    videoFrame_ = nullptr;
  }
  if(audioFrame_) {
    av_frame_free(&audioFrame_);
    audioFrame_ = nullptr;
  }
  for(auto &queue: frames) {
    Frame *frame;
    std::vector<Frame *> temp;
    while(queue.try_pop(frame)) {
      temp.push_back(frame);
      av_freep(&frame->data_[0]);
      for(auto &p: frame->data_)
        p = nullptr;
      for(auto &p: frame->linesize_)
        p = 0;
    }
    for(auto &f: temp)
      queue.push(f);
  }
  if(pkt) {
    av_packet_unref(pkt);
    av_packet_free(&pkt);
    pkt = nullptr;
  }
  if(pkt2) {
    av_packet_unref(pkt2);
    av_packet_free(&pkt2);
    pkt2 = nullptr;
  }
  if(videoCodecCtx) {
    avcodec_free_context(&videoCodecCtx);
    videoCodecCtx = nullptr;
    video_stream_index = -1;
  }
  if(audioCodecCtx) {
    avcodec_free_context(&audioCodecCtx);
    audioCodecCtx = nullptr;
    audio_stream_index = -1;
  }
  if(fmt_ctx) avformat_close_input(&fmt_ctx);
  if(fmt2_ctx) avformat_close_input(&fmt2_ctx);
  if(swscale_ctx) {
    sws_freeContext(swscale_ctx);
    swscale_ctx = nullptr;
  }
  if(swresample_ctx) {
    swr_free(&swresample_ctx);
    swresample_ctx = nullptr;
  }
  if(avio_ctx) {
    if(avio_ctx->buffer) av_freep(&avio_ctx->buffer);
    avio_context_free(&avio_ctx);
    avio_ctx = nullptr;
  }
  if(fmt_ctx) {
    avformat_free_context(fmt_ctx);
    fmt_ctx = nullptr;
  }
  if(avio2_ctx) {
    if(avio2_ctx->buffer) av_freep(&avio2_ctx->buffer);
    avio_context_free(&avio2_ctx);
    avio2_ctx = nullptr;
  }
  if(fmt2_ctx) {
    avformat_free_context(fmt2_ctx);
    fmt2_ctx = nullptr;
  }
  fps_ = 0.0;
  isClosed = true;
}

bool FrameFetcher::hasVideo() { return video_stream_index != -1; }
bool FrameFetcher::hasAudio() { return audio_stream_index != -1; }
double FrameFetcher::fps() const { return fps_; }
int FrameFetcher::width() const { return width_; }
int FrameFetcher::height() const { return height_; }
int64_t FrameFetcher::duration() const { return duration_; }

Frame *FrameFetcher::read_frame(Frame *frame) {
  return read_frame_for(fmt_ctx, pkt, shouldReadPkt, pktRecvVideo, 0, frame);
}

Frame *FrameFetcher::read_frame2(Frame *frame) {
  return read_frame_for(
    fmt2_ctx, pkt2, shouldReadPkt2, pkt2RecvVideo, fmt_ctx->nb_streams, frame);
}

Frame *FrameFetcher::read_frame_for(
  AVFormatContext *fmtCtxPtr, AVPacket *pktPtr, bool &shouldReadPktSignal,
  bool &pktRecvVideoSignal, int streamIndexOffset, Frame *frame) {

  while(true) {
    if(shouldReadPktSignal) {
      shouldReadPktSignal = false;
      while(true) {
        auto ret = av_read_frame(fmtCtxPtr, pktPtr);
        pktPtr->stream_index += streamIndexOffset;
        if(ret < 0) return nullptr;
        if(pktPtr->stream_index == video_stream_index) {
          errorIf(!videoCodecCtx, "no video codec!");
          ret = avcodec_send_packet(videoCodecCtx, pktPtr);
          errorIf(ret < 0, "error send video packet");
          pktRecvVideoSignal = true;
          break;
        } else if(pktPtr->stream_index == audio_stream_index) {
          errorIf(!audioCodecCtx, "no video codec!");
          ret = avcodec_send_packet(audioCodecCtx, pktPtr);
          errorIf(ret < 0, "error send audio packet");
          pktRecvVideoSignal = false;
          break;
        } else
          av_packet_unref(pktPtr);
      }
    }

    Frame *frame_;
    if(!frame) {
      auto queueIndex = pktRecvVideoSignal ? 0 : 1;
      frames[queueIndex].pop(frame_);
    } else
      frame_ = frame;

    bool recvd = pktRecvVideoSignal ? recv_video_frame(frame_) : recv_audio_frame(frame_);
    if(recvd) return frame_;
    else {
      if(frame_->frameQueueIdx_ >= 0) recycleFrame(frame_);
      shouldReadPktSignal = true;
      av_packet_unref(pktPtr);
    }
  }
}

void FrameFetcher::recycleFrame(Frame *frame) {
  frames[frame->frameQueueIdx_].push(frame);
}

bool FrameFetcher::recv_video_frame(Frame *frame) {
  errorIf(!videoCodecCtx, "no video codec!");
  auto ret = avcodec_receive_frame(videoCodecCtx, videoFrame_);
  if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return false;
  errorIf(ret < 0, "error while receiving a frame");

  auto pts = videoFrame_->best_effort_timestamp;
  frame->pts_ = long(1000.0 * pts * video_timebase.num / video_timebase.den);

  if(frame->frameQueueIdx_ < 0) return true;
  if(frame->data_[0] == nullptr) {
    auto pictureBufSize_ = av_image_get_buffer_size(
      (AVPixelFormat)dst_picture_fmt, videoCodecCtx->width, videoCodecCtx->height, 32);
    auto pictureBuf_ =
      static_cast<uint8_t *>(av_malloc(pictureBufSize_ * sizeof(uint8_t)));
    av_image_fill_arrays(
      frame->data_, frame->linesize_, pictureBuf_, (AVPixelFormat)dst_picture_fmt,
      videoCodecCtx->width, videoCodecCtx->height, 32);
  }
  sws_scale(
    swscale_ctx, videoFrame_->data, videoFrame_->linesize, 0, videoCodecCtx->height,
    frame->data_, frame->linesize_);
  frame->type_ = AVMEDIA_TYPE_VIDEO;
  frame->width_ = videoCodecCtx->width;
  frame->height_ = videoCodecCtx->height;
  return true;
}

bool FrameFetcher::recv_audio_frame(Frame *frame) {
  errorIf(!audioCodecCtx, "no video codec!");
  auto ret = avcodec_receive_frame(audioCodecCtx, audioFrame_);
  if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return false;
  errorIf(ret < 0, "error while receiving a frame");
  auto pts = audioFrame_->best_effort_timestamp;
  frame->pts_ = long(1000.0 * pts * audio_timebase.num / audio_timebase.den);

  if(frame->frameQueueIdx_ < 0) return true;
  auto src_nb_samples = audioFrame_->nb_samples;
  auto dst_nb_samples = av_rescale_rnd(
    swr_get_delay(swresample_ctx, src_rate) + src_nb_samples, dst_rate, src_rate,
    AV_ROUND_UP);
  auto dst_bufsize = av_samples_get_buffer_size(
    nullptr, dst_channels, dst_nb_samples, (AVSampleFormat)dst_sample_fmt, 1);
  if(dst_bufsize > frame->linesize_[0]) {
    av_freep(&frame->data_[0]);
    ret = av_samples_alloc(
      frame->data_, &frame->linesize_[0], dst_channels, dst_nb_samples,
      (AVSampleFormat)dst_sample_fmt, 1);
    errorIf(ret < 0, "error samples_alloc");
  }
  auto rt_nb_Samples = swr_convert(
    swresample_ctx, frame->data_, dst_nb_samples,
    (const uint8_t **)(audioFrame_->extended_data), src_nb_samples);
  errorIf(ret < 0, "swr convert error");
  dst_bufsize = av_samples_get_buffer_size(
    nullptr, dst_channels, rt_nb_Samples, (AVSampleFormat)dst_sample_fmt, 1);
  errorIf(dst_bufsize < 0, "Could not get sample buffer size");
  auto dst = frame->data_[0];
  auto sdst = (short *)dst;
  for(auto i = 0; i < dst_bufsize / 2; i++) {
    auto v = (short)std::clamp(sdst[i] * volume, -32768.0, 32767.0);
    sdst[i] = v;
  }

  frame->type_ = AVMEDIA_TYPE_AUDIO;
  frame->bufsize_ = dst_bufsize;
  return true;
}

void FrameFetcher::seek(int64_t timestampMilis) {
  int64_t seek_target = timestampMilis * 1000;
  if(fmt_ctx->start_time != AV_NOPTS_VALUE) seek_target += fmt_ctx->start_time;
  auto ret = av_seek_frame(fmt_ctx, -1, seek_target, AVSEEK_FLAG_BACKWARD);
  errorIf(ret < 0, "error seeking ", ret);
  if(fmt2_ctx) {
    ret = av_seek_frame(fmt2_ctx, -1, seek_target, AVSEEK_FLAG_BACKWARD);
    errorIf(ret < 0, "error seeking ", ret);
  }
  if(video_stream_index >= 0) avcodec_flush_buffers(videoCodecCtx);
  if(audio_stream_index >= 0) avcodec_flush_buffers(audioCodecCtx);
  if(pkt) av_packet_unref(pkt);
  if(pkt2) av_packet_unref(pkt2);
  shouldReadPkt = true;
  shouldReadPkt2 = true;

  if(preciseSeek_) {
    Frame frame;
    while(true) {
      if(read_frame(&frame) == nullptr) break;
      if(frame.pts_ >= timestampMilis - 1) { break; }
    }
    if(fmt2_ctx)
      while(true) {
        if(read_frame2(&frame) == nullptr) break;
        if(frame.pts_ >= timestampMilis - 1) { break; }
      }
  }
}
void FrameFetcher::setPreciseSeek(bool preciseSeek) {
  FrameFetcher::preciseSeek_ = preciseSeek;
}
int FrameFetcher::srcRate() const { return src_rate; }
int FrameFetcher::srcChannels() const { return src_channels; }
int FrameFetcher::srcSampleFmt() const { return src_sample_fmt; }
}