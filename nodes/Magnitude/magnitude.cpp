#include "magnitude.hpp"
#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables {
Magnitude::Magnitude() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Magnitude");
    SetComponentCategory_(Category::Category_Utility);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in1", "in2"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    SetEnabled(true);
}

void Magnitude::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* first = inputs.GetValue<cv::Mat>(0);
    const auto* second = inputs.GetValue<cv::Mat>(1);
    // Upstream requires both inputs even while disabled.
    if (!first || !second || first->empty() || second->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *first); return; }
    // Comparing type(), not depth(), deliberately preserves single-channel only.
    if ((first->type() != CV_32F && first->type() != CV_64F)
        || first->type() != second->type() || first->size != second->size) return;
    try {
        cv::Mat frame;
        cv::magnitude(*first, *second, frame);
        if (!frame.empty()) outputs.SetValue(0, frame);
    } catch (const cv::Exception&) { }
}
std::string Magnitude::GetState() { return "null"; }
void Magnitude::SetState(std::string&& state) { (void)nlohmann::json::parse(state); }
}
