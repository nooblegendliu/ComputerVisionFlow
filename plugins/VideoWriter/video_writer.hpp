// Adapted from FlowCV Plugins/VideoWriter (GPL-3.0).
#pragma once
#include "video_writer_control.hpp"
#include <flowNode.h>
#include <opencv2/videoio.hpp>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>
namespace DSPatch::DSPatchables {
const std::vector<std::pair<std::string,std::string>>& VideoCodecs();
namespace internal {
struct VideoWriterSession {
    mutable std::mutex mutex;
    VideoWriterStatus status;
    cv::VideoWriter writer;
    bool reopen=false;
    int lastFps=30,lastCodec=0;
    cv::Size frameSize;
    VideoWriterStatus Snapshot() const;
    std::string State() const;
    void Restore(const std::string& state);
    void Start(); void Stop();
    void Open(const cv::Mat& frame);
};
}
class VideoWriter : public FlowCV::Node, public VideoWriterControl {
public:
    VideoWriter();
    VideoWriterStatus GetWriteStatus() const override { return session_->Snapshot(); }
    void StartWriting() override { session_->Start(); }
    void StopWriting() override { session_->Stop(); }
    std::string GetState() override { return session_->State(); }
    void SetState(std::string&& state) override { session_->Restore(state); }
protected:
    void Process_(SignalBus const& inputs,SignalBus&) override;
    std::shared_ptr<internal::VideoWriterSession> session_;
};
}
