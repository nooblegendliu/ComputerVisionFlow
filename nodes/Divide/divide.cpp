#include "divide.hpp"
#include <opencv2/core.hpp>
#include <limits>

namespace DSPatch::DSPatchables {
Divide::Divide() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Divide");
    SetComponentCategory_(Category::Category_Merge);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in1", "in2"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddFloat("scale", "Scale", 1.0f,
        (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::max)(), 0.001f);
    SetEnabled(true);
}

void Divide::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* in1 = inputs.GetValue<cv::Mat>(0);
    const auto* in2 = inputs.GetValue<cv::Mat>(1);
    if (!in1 || !in2 || in1->empty() || in2->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *in1); return; }
    if (in1->type() != in2->type() || in1->size != in2->size) return;
    properties_.Sync();
    float scale = 1.0f;
    // Take all editable values under one lock for this processing cycle.
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "scale") scale = std::get<float>(property.value);
    }
    try {
        cv::Mat frame;
        cv::divide(*in1, *in2, frame, scale);
        if (!frame.empty()) outputs.SetValue(0, frame);
    }
    catch (const cv::Exception&) {
        // Unsupported OpenCV input must not terminate the AutoTick worker.
    }
}
}
