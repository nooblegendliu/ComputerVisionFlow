#include "laplacian_filter.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <limits>

namespace DSPatch::DSPatchables {
LaplacianFilter::LaplacianFilter() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Laplacian");
    SetComponentCategory_(Category::Category_Filter);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("depth_out", "Depth Out", 2, {"Auto", "CV_16S", "CV_32F", "CV_64F"});
    properties_.AddInt("ksize", "Kernel Size", 3, 1, 7, 2);
    constexpr auto limit = (std::numeric_limits<float>::max)();
    properties_.AddFloat("scale", "Scale", 1, -limit, limit, .1f);
    properties_.AddFloat("delta", "Delta", 0, -limit, limit, .1f);
    SetEnabled(true);
}

void LaplacianFilter::RefreshProperties()
{
    std::lock_guard lock(parameter_mutex_);
    const int current = properties_.GetW<int>("ksize");
    if (current == last_kernel_) return;
    int kernel = std::clamp(current, 1, 7);
    if (kernel % 2 == 0) ++kernel;
    if (kernel != current) properties_.Set("ksize", kernel);
    last_kernel_ = kernel;
    properties_.SetMin("ksize", 1);
    properties_.SetMax("ksize", 7);
}

void LaplacianFilter::SetState(std::string&& state)
{
    const auto parsed = nlohmann::json::parse(state);
    std::lock_guard lock(parameter_mutex_);
    properties_.FromJson(parsed);
    // Retain saved kernels; the odd 1..7 correction belongs to interactive edits.
    last_kernel_ = properties_.GetW<int>("ksize");
    // QSpinBox must be able to display a restored value outside the edit limits.
    properties_.SetMin("ksize", std::min(1, last_kernel_));
    properties_.SetMax("ksize", std::max(7, last_kernel_));
}

void LaplacianFilter::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    processing_error_.store(false);
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); return; }
    int depth = 2, kernel = 3;
    float scale = 1, delta = 0;
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "depth_out") depth = std::get<int>(property.value);
        else if (property.key == "ksize") kernel = std::get<int>(property.value);
        else if (property.key == "scale") scale = std::get<float>(property.value);
        else if (property.key == "delta") delta = std::get<float>(property.value);
    }
    try {
        cv::Mat gray, frame;
        if (input->channels() > 1) cv::cvtColor(*input, gray, cv::COLOR_BGR2GRAY);
        else gray = *input;
        constexpr int depths[] = {-1, CV_16S, CV_32F, CV_64F};
        cv::Laplacian(gray, frame, depths[depth], kernel, scale, delta, cv::BORDER_DEFAULT);
        if (!frame.empty()) outputs.SetValue(0, frame);
    } catch (const cv::Exception&) { processing_error_.store(true); }
}
}
