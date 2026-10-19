// Adapted from FlowCV Plugins/VideoLoader (GPL-3.0).
#pragma once
#include "video_loader_control.hpp"
#include <flowNode.h>
#include <opencv2/videoio.hpp>
#include <chrono>
#include <memory>
#include <mutex>
namespace DSPatch::DSPatchables {
namespace internal {
struct VideoLoaderSession {
    mutable std::mutex mutex;
    VideoLoaderStatus status;
    cv::VideoCapture capture;
    bool reload = false;
    int seek = -1;
    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    VideoLoaderStatus Snapshot() const;
    std::string State() const;
    void Restore(const std::string& state);
    void Play(bool playing);
    void Seek(int frame);
};
}
class VideoLoader : public FlowCV::Node, public VideoLoaderControl {
public:
    VideoLoader();
    VideoLoaderStatus GetPlaybackStatus() const override { return session_->Snapshot(); }
    void SetPlaying(bool playing) override { session_->Play(playing); }
    void SeekFrame(int frame) override { session_->Seek(frame); }
    std::string GetState() override { return session_->State(); }
    void SetState(std::string&& state) override { session_->Restore(state); }
protected:
    void Process_(SignalBus const&, SignalBus& outputs) override;
    std::shared_ptr<internal::VideoLoaderSession> session_;
};
}
