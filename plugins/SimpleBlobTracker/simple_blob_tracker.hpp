// Adapted from FlowCV Plugins/SimpleBlobTracker (GPL-3.0).
#pragma once
#include <flowNode.h>
#include <opencv2/core.hpp>
#include <deque>
#include <mutex>

namespace DSPatch::DSPatchables {
class SimpleBlobTracker : public FlowCV::Node
{
public:
    SimpleBlobTracker();
    void RequestReset();
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    struct Track {
        std::deque<cv::Point> pos;
        cv::Vec3b color;
        std::uint32_t id;
        int lastSize;
        std::uint32_t lastFrame;
    };
    std::mutex mutex_;
    std::vector<Track> tracks_;
    bool reset_ = false;
    std::uint32_t id_ = 1;
    std::uint32_t frame_ = 0;
};
}
