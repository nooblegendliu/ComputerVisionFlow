#include "subtract.hpp"
#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables {
Subtract::Subtract() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Subtract");
    SetComponentCategory_(Category::Category_Merge);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(3, {"in1", "in2", "mask"},
        {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("mask_mode", "Mask Mode", 0, {"Default", "In1", "In2"});
    SetEnabled(true);
}

void Subtract::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* in1 = inputs.GetValue<cv::Mat>(0);
    const auto* in2 = inputs.GetValue<cv::Mat>(1);
    const auto* mask = inputs.GetValue<cv::Mat>(2);
    if (!in1 || !in2 || in1->empty() || in2->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *in1); return; }
    if (in1->type() != in2->type() || in1->size != in2->size) return;
    properties_.Sync();
    int maskMode = 0;
    for (const auto& property : properties_.Snapshot().properties)
        if (property.key == "mask_mode") maskMode = std::get<int>(property.value);
    // Match upstream: multi-channel masks are ignored. OpenCV also accepts
    // signed 8-bit masks; do not narrow the accepted path to CV_8UC1.
    const bool hasMask = mask && !mask->empty() && mask->channels() == 1;
    try {
        cv::Mat frame;
        if (hasMask) {
            if (maskMode == 1) in1->copyTo(frame);
            else if (maskMode == 2) in2->copyTo(frame);
            cv::subtract(*in1, *in2, frame, *mask);
        }
        else cv::subtract(*in1, *in2, frame);
        if (!frame.empty()) outputs.SetValue(0, frame);
    }
    catch (const cv::Exception&) {
        // Invalid mask depth/size must not terminate the AutoTick worker.
    }
}
}
