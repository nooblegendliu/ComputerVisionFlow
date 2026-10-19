#include "normalize.hpp"
#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables {
namespace {
constexpr int NormTypeValues[] = {cv::NORM_INF, cv::NORM_L1, cv::NORM_L2, cv::NORM_MINMAX};
}
Normalize::Normalize() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Normalize");
    SetComponentCategory_(Category::Category_Color);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "mask"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("norm_type", "Normal Type", 0, {"Inf", "L1", "L2", "Min Max"});
    properties_.AddFloat("alpha", "Alpha (Min)", 1, 0, 500, .01f);
    properties_.AddFloat("beta", "Beta (Max)", 10, 0, 500, .01f, false);
    SetEnabled(true);
}

void Normalize::RefreshProperties()
{
    properties_.SetVisibility("beta", properties_.GetW<int>("norm_type") == 3);
}

std::string Normalize::GetState()
{
    // Upstream always saves beta, including while the UI hides it.
    nlohmann::json state = nlohmann::json::object();
    for (const auto& property : properties_.Snapshot().properties)
        std::visit([&](const auto& value) { state[property.key] = value; }, property.value);
    return state.dump();
}

void Normalize::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) {
        outputs.SetValue(0, *input);
        processing_error_.store(false);
        return;
    }
    const auto* mask = inputs.GetValue<cv::Mat>(1);
    properties_.Sync();
    int normType = 0;
    float alpha = 1, beta = 10;
    // Read the complete parameter group under one properties lock.
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "norm_type") normType = std::get<int>(property.value);
        else if (property.key == "alpha") alpha = std::get<float>(property.value);
        else if (property.key == "beta") beta = std::get<float>(property.value);
    }
    try {
        cv::Mat frame;
        // Min Max deliberately ignores even a connected mask in FlowCV.
        if (mask && !mask->empty() && normType != 3)
            cv::normalize(*input, frame, alpha, beta, NormTypeValues[normType], -1, *mask);
        else
            cv::normalize(*input, frame, alpha, beta, NormTypeValues[normType]);
        if (!frame.empty()) outputs.SetValue(0, frame);
        processing_error_.store(false);
    } catch (const cv::Exception&) {
        processing_error_.store(true);
    }
}
}
