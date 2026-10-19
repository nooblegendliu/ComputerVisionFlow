#include "canny.hpp"

#include <algorithm>

#include <opencv2/imgproc.hpp>

namespace DSPatch::DSPatchables
{
    CannyFilter::CannyFilter()
        : FlowCV::Node(DSPatch::Component::ProcessOrder::OutOfOrder)
    {
        SetComponentName_("Canny");
        SetComponentCategory_(DSPatch::Category::Category_Feature_Detection);
        SetComponentAuthor_("Richard");
        SetComponentVersion_("0.1.0");

        SetInputCount_(1, { "in" }, { DSPatch::IoType::Io_Type_CvMat });
        SetOutputCount_(1, { "out" }, { DSPatch::IoType::Io_Type_CvMat });

        properties_.AddInt("kernel_size", "Sobel Apt Size", 3, 3, 7, 2.0f);
        properties_.AddOption("thresh_mode", "Threshold Mode", 0,
            { "Fixed", "Automatic" });
        properties_.AddOption("norm_type", "Gradient Mode", 0,
            { "L1 Norm", "L2 Norm" });
        properties_.AddFloat("low_thresh", "Low Threshold", 50.0f,
            1.0f, 5000.0f, 0.1f);
        properties_.AddFloat("high_thresh", "High Threshold", 150.0f,
            1.0f, 5000.0f, 0.1f);

        RefreshProperties();
        SetEnabled(true);
    }

    void CannyFilter::RefreshProperties()
    {
        const bool fixedThresholds =
            properties_.GetW<int>("thresh_mode") == 0;
        properties_.SetVisibility("low_thresh", fixedThresholds);
        properties_.SetVisibility("high_thresh", fixedThresholds);

        // The original ImGui editor rounded edited apertures to odd values.
        // A Qt spin box can also receive 4 or 6, including from saved JSON.
        const int current = properties_.GetW<int>("kernel_size");
        int kernel = std::clamp(current, 3, 7);
        if (kernel % 2 == 0) {
            ++kernel;
        }
        if (kernel != current) {
            properties_.Set("kernel_size", kernel);
        }
    }

    void CannyFilter::Process_(SignalBus const& inputs, SignalBus& outputs)
    {
        const auto* in = inputs.GetValue<cv::Mat>(0);
        if (!in || in->empty()) {
            return;
        }
        if (!IsEnabled()) {
            outputs.SetValue(0, *in);
            return;
        }

        properties_.Sync();

        cv::Mat gray;
        if (in->channels() > 1) {
            cv::cvtColor(*in, gray, cv::COLOR_BGR2GRAY);
        }
        else {
            gray = *in;
        }

        float low = properties_.Get<float>("low_thresh");
        float high = properties_.Get<float>("high_thresh");
        if (properties_.Get<int>("thresh_mode") == 1) {
            cv::Scalar mean, deviation;
            cv::meanStdDev(gray, mean, deviation);
            low = static_cast<float>(mean[0] - deviation[0]);
            high = static_cast<float>(mean[0] + deviation[0]);
        }

        // Controls step through odd values. Normalize at execution as well
        // for externally supplied even apertures.
        const int currentKernel = properties_.Get<int>("kernel_size");
        int kernel = std::clamp(currentKernel, 3, 7);
        if (kernel % 2 == 0) {
            ++kernel;
        }
        if (currentKernel != kernel) {
            properties_.Set("kernel_size", kernel);
        }

        cv::Mat frame;
        cv::Canny(gray, frame, low, high, kernel,
            static_cast<bool>(properties_.Get<int>("norm_type")));
        if (!frame.empty()) {
            outputs.SetValue(0, frame);
        }
    }
}
