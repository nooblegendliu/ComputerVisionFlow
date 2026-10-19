#include "scale_abs.hpp"
#include <opencv2/core.hpp>
#include <limits>

namespace DSPatch::DSPatchables {
ScaleAbs::ScaleAbs() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Scale_Abs");
    SetComponentCategory_(Category::Category_Filter);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddFloat("alpha", "Alpha", 1.0f,
        (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::max)(), 0.01f);
    properties_.AddFloat("beta", "Beta", 0.0f,
        (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::max)(), 0.1f);
    SetEnabled(true);
}

void ScaleAbs::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* in1 = inputs.GetValue<cv::Mat>(0);
    if (!in1 || in1->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *in1); return; }
    properties_.Sync();
    float alpha = 1.0f;
    float beta = 0.0f;
    // Take all editable values under one lock for this processing cycle.
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "alpha") alpha = std::get<float>(property.value);
        if (property.key == "beta") beta = std::get<float>(property.value);
    }
    try {
        cv::Mat frame;
        cv::convertScaleAbs(*in1, frame, alpha, beta);
        if (!frame.empty()) outputs.SetValue(0, frame);
    }
    catch (const cv::Exception&) {
        // Unsupported OpenCV input must not terminate the AutoTick worker.
    }
}
}
