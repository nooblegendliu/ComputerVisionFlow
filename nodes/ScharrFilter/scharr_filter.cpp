#include "scharr_filter.hpp"
#include <opencv2/imgproc.hpp>
#include <limits>

namespace DSPatch::DSPatchables {
ScharrFilter::ScharrFilter() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Scharr");
    SetComponentCategory_(Category::Category_Filter);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("derivative_order", "Derivative Order", 0, {"X", "Y"});
    properties_.AddOption("depth_out", "Depth Out", 2, {"Auto", "CV_16S", "CV_32F", "CV_64F"});
    constexpr auto limit = (std::numeric_limits<float>::max)();
    properties_.AddFloat("scale", "Scale", 1, -limit, limit, .01f);
    properties_.AddFloat("delta", "Delta", 0, -limit, limit, .01f);
    SetEnabled(true);
}

void ScharrFilter::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    processing_error_.store(false);
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); return; }
    int order = 0, depth = 2;
    float scale = 1, delta = 0;
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "derivative_order") order = std::get<int>(property.value);
        else if (property.key == "depth_out") depth = std::get<int>(property.value);
        else if (property.key == "scale") scale = std::get<float>(property.value);
        else if (property.key == "delta") delta = std::get<float>(property.value);
    }
    try {
        cv::Mat gray, frame;
        if (input->channels() > 1) cv::cvtColor(*input, gray, cv::COLOR_BGR2GRAY);
        else gray = *input;
        constexpr int depths[] = {-1, CV_16S, CV_32F, CV_64F};
        cv::Scharr(gray, frame, depths[depth], order == 0 ? 1 : 0, order == 0 ? 0 : 1,
            scale, delta, cv::BORDER_DEFAULT);
        if (!frame.empty()) outputs.SetValue(0, frame);
    } catch (const cv::Exception&) { processing_error_.store(true); }
}
}
