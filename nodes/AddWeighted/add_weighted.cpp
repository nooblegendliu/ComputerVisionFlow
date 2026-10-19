#include "add_weighted.hpp"
#include <opencv2/core.hpp>
#include <limits>

namespace DSPatch::DSPatchables {
AddWeighted::AddWeighted() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Add_Weighted");
    SetComponentCategory_(Category::Category_Merge);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in1", "in2"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddFloat("alpha", "Alpha", 0.5f,
        (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::max)(), 0.05f);
    properties_.AddFloat("beta", "Beta", 0.5f,
        (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::max)(), 0.05f);
    properties_.AddFloat("gamma", "Gamma", 0.0f,
        (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::max)(), 0.05f);
    SetEnabled(true);
}

void AddWeighted::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* in1 = inputs.GetValue<cv::Mat>(0);
    const auto* in2 = inputs.GetValue<cv::Mat>(1);
    if (!in1 || !in2 || in1->empty() || in2->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *in1); return; }
    if (in1->type() != in2->type() || in1->size != in2->size) return;
    properties_.Sync();
    float alpha = 0.5f;
    float beta = 0.5f;
    float gamma = 0.0f;
    // Take all editable values under one lock for this processing cycle.
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "alpha") alpha = std::get<float>(property.value);
        if (property.key == "beta") beta = std::get<float>(property.value);
        if (property.key == "gamma") gamma = std::get<float>(property.value);
    }
    try {
        cv::Mat frame;
        cv::addWeighted(*in1, alpha, *in2, beta, gamma, frame);
        if (!frame.empty()) outputs.SetValue(0, frame);
    }
    catch (const cv::Exception&) {
        // Unsupported OpenCV input must not terminate the AutoTick worker.
    }
}
}
