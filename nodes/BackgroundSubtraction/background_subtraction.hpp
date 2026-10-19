#pragma once
#include <flowNode.h>
#include <opencv2/video/background_segm.hpp>
#include <atomic>
#include <mutex>

namespace DSPatch::DSPatchables {
class BackgroundSubtraction final : public FlowCV::Node {
public:
    BackgroundSubtraction();
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    cv::Ptr<cv::BackgroundSubtractorMOG2> mog2_;
    cv::Ptr<cv::BackgroundSubtractorKNN> knn_;
    std::mutex model_mutex_;
    bool settings_applied_ = false;
    int applied_history_ = 500;
    float applied_threshold_ = 40;
    bool applied_shadows_ = false;
    std::atomic<bool> processing_error_{false};
};
}
