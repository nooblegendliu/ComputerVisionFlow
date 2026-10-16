#include "blur.hpp"

#include <opencv2/imgproc.hpp>

namespace DSPatch::DSPatchables
{
	Blur::Blur()
        : FlowCV::Node(DSPatch::Component::ProcessOrder::OutOfOrder)
    {
        SetComponentName_("Blur");
        SetComponentCategory_(DSPatch::Category::Category_Filter);
        SetComponentAuthor_("Richard");
        SetComponentVersion_("0.1.0");

        SetInputCount_(1, { "in" }, { DSPatch::IoType::Io_Type_CvMat });
        SetOutputCount_(1, { "out" }, { DSPatch::IoType::Io_Type_CvMat });

        properties_.AddOption("blur_mode", "Blur Type", 0,
            { "Box", "Gaussian", "Median", "Bilateral" });
        properties_.AddBool("lock_h_v", "Lock H & V", false);
        properties_.AddFloat("blur_amt_h", "Blur Amount H", 1.0f,
            1.0f, 100.0f, 0.1f);
        properties_.AddFloat("blur_amt_v", "Blur Amount V", 1.0f,
            1.0f, 100.0f, 0.1f);

        RefreshProperties();
        SetEnabled(true);
    }

    void Blur::RefreshProperties()
    {
        // Originally in UpdateGui(): keep the same property names and
        // mode-specific visibility, but do not depend on ImGui.
        const int mode = properties_.GetW<int>("blur_mode");
        const bool lockHV = properties_.GetW<bool>("lock_h_v");

        if (mode < 2) {
            properties_.SetDescription("blur_amt_h", "Blur Amount H");
            properties_.SetDescription("blur_amt_v", "Blur Amount V");
            properties_.SetVisibility("lock_h_v", true);
            properties_.SetVisibility("blur_amt_v", !lockHV);
        }
        else if (mode == 2) {
            properties_.SetDescription("blur_amt_h", "Blur Amount");
            properties_.SetVisibility("blur_amt_v", false);
            properties_.Set("lock_h_v", false);
            properties_.SetVisibility("lock_h_v", false);
        }
        else if (mode == 3) {
            properties_.SetDescription("blur_amt_h", "Color");
            properties_.SetDescription("blur_amt_v", "Space");
            properties_.Set("lock_h_v", false);
            properties_.SetVisibility("lock_h_v", false);
            properties_.SetVisibility("blur_amt_v", true);
        }
    }

    void Blur::Process_(SignalBus const& inputs, SignalBus& outputs)
    {
        // Retain the original lock for Blur processing.
        std::lock_guard<std::mutex> lock(mutex_lock_);

        const auto* in = inputs.GetValue<cv::Mat>(0);
        if (!in || in->empty()) {
            return;
        }

        if (!IsEnabled()) {
            outputs.SetValue(0, *in);
            return;
        }

        properties_.Sync();

        float bh = properties_.Get<float>("blur_amt_h");
        float bv = properties_.Get<float>("blur_amt_v");
        const int mode = properties_.Get<int>("blur_mode");

        if (properties_.Get<bool>("lock_h_v")) {
            bv = bh;
        }

        cv::Mat frame;
        if (mode == 0) {
            cv::blur(*in, frame, cv::Size(static_cast<int>(bh),
                static_cast<int>(bv)), cv::Point(-1, -1));
        }
        else if (mode == 1) {
            cv::GaussianBlur(*in, frame, cv::Size(0, 0), bh, bv);
        }
        else if (mode == 2) {
            int kernelSize = static_cast<int>(bh);
            if (kernelSize % 2 == 0) {
                ++kernelSize;
            }
            cv::medianBlur(*in, frame, kernelSize);
        }
        else if (mode == 3) {
            cv::bilateralFilter(*in, frame, static_cast<int>(bh),
                bh * 2.0f, bv / 2.0f);
        }

        if (!frame.empty()) {
            outputs.SetValue(0, frame);
        }
    }

}	// namespace DSPatch::DSPatchables