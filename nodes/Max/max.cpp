#include "max.hpp"
#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables {
Max::Max() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Max");
    SetComponentCategory_(Category::Category_Merge);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in1", "in2"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    SetEnabled(true);
}

void Max::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* in1 = inputs.GetValue<cv::Mat>(0);
    const auto* in2 = inputs.GetValue<cv::Mat>(1);
    if (!in1 || !in2 || in1->empty() || in2->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *in1); return; }
    if (in1->type() != in2->type() || in1->size != in2->size) return;
    try {
        cv::Mat frame;
        cv::max(*in1, *in2, frame);
        if (!frame.empty()) outputs.SetValue(0, frame);
    }
    catch (const cv::Exception&) {
        // Unsupported OpenCV input must not terminate the AutoTick worker.
    }
}

std::string Max::GetState() { return "null"; }
void Max::SetState(std::string&& state)
{
    (void)nlohmann::json::parse(state);
}
}
