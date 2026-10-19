// Adapted from FlowCV Plugins/ShapeCounter (GPL-3.0).
#pragma once
#include <flowNode.h>
#include <opencv2/core.hpp>
#include <memory>
#include <mutex>
namespace DSPatch::DSPatchables {
namespace internal {
struct ShapeCounterParams {
    int size_match = 100, size_thresh = 5, count_mode = 0;
    bool show_overlay = true;
    cv::Point position{20, 30};
    float scale = .35f;
    int thickness = 1, font = 0;
    cv::Vec4f color{1.f, 1.f, 1.f, .8f};
};
struct ShapeCounterState {
    mutable std::mutex mutex;
    ShapeCounterParams params;
    ShapeCounterParams Snapshot() const { std::lock_guard lock(mutex); return params; }
};
}
class ShapeCounter : public FlowCV::Node {
public:
    ShapeCounter();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
protected:
    void Process_(SignalBus const&, SignalBus&) override;
    std::shared_ptr<internal::ShapeCounterState> state_;
};
}
