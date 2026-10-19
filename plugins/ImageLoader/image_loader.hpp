// Adapted from FlowCV Plugins/ImageLoader (GPL-3.0); see ../README.md.
#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <flowNode.h>
#include <opencv2/core/mat.hpp>

namespace DSPatch::DSPatchables
{
class ImageLoader final : public FlowCV::Node
{
public:
    ImageLoader();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;

protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;

private:
    void LoadImage(const std::string& path);
    cv::Mat frame_;
    std::string image_file_;
    int fps_ = 30;
    int fps_index_ = 7;
    std::chrono::steady_clock::time_point last_time_;
    std::mutex io_mutex_;
};
}
