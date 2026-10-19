#include "color_reduce.hpp"
#include <opencv2/core.hpp>
#include <algorithm>

namespace DSPatch::DSPatchables {
ColorReduce::ColorReduce() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Color_Reduce");
    SetComponentCategory_(Category::Category_Color);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddInt("div", "Reduce", 64, 1, 255, .5f);
    SetEnabled(true);
}

void ColorReduce::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); return; }
    // The upstream quantizer works on bytes; reject other depths rather than
    // interpreting only part of each sample as a color value.
    if (input->depth() != CV_8U) return;
    const int divisor = std::max(1, properties_.GetW<int>("div"));
    try {
        cv::Mat frame;
        input->copyTo(frame);
        const int count = frame.cols * frame.channels();
        for (int row = 0; row < frame.rows; ++row) {
            auto* data = frame.ptr<uchar>(row);
            for (int col = 0; col < count; ++col)
                data[col] = static_cast<uchar>(data[col] / divisor * divisor + divisor / 2);
        }
        if (!frame.empty()) outputs.SetValue(0, frame);
    }
    catch (const cv::Exception&) {
        // Keep an unsupported OpenCV input from terminating the AutoTick worker.
    }
}

// FlowCV intentionally does not serialize the runtime Reduce control.
std::string ColorReduce::GetState() { return "null"; }
void ColorReduce::SetState(std::string&& state) { (void)nlohmann::json::parse(state); }
}
