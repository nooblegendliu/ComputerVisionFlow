#include "sharpen.hpp"
#include <opencv2/imgproc.hpp>

namespace DSPatch::DSPatchables {
Sharpen::Sharpen() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Sharpen");
    SetComponentCategory_(Category::Category_Filter);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("sharpen_mode", "Sharpen Mode", 0, {"Mode 1", "Mode 2"});
    properties_.AddInt("sharpen_amt", "Sharpen", 0, 0, 3, .25f);
    SetEnabled(true);
}

void Sharpen::RefreshProperties()
{
    const bool firstMode = properties_.GetW<int>("sharpen_mode") == 0;
    properties_.SetMax("sharpen_amt", firstMode ? 3 : 5);
    // FlowCV reduces the amount when its UI switches back to Mode 1.
    if (firstMode && properties_.GetW<int>("sharpen_amt") > 3)
        properties_.Set("sharpen_amt", 3);
}

void Sharpen::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    processing_error_.store(false);
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;

    int mode = 0, amount = 0;
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "sharpen_mode") mode = std::get<int>(property.value);
        else if (property.key == "sharpen_amt") amount = std::get<int>(property.value);
    }

    try {
        cv::Mat frame;
        if (!IsEnabled()) {
            input->copyTo(frame);
        } else if (mode == 0) {
            if (amount > 3) {
                // A direct property update can precede the UI range refresh.
                // Upstream would otherwise filter with an uninitialized kernel.
                processing_error_.store(true);
                return;
            }
            if (amount <= 0) {
                input->copyTo(frame);
            } else {
                cv::Mat kernel;
                if (amount == 1)
                    kernel = (cv::Mat_<double>(3, 3) << 0, -1, 0, -1, 5, -1, 0, -1, 0);
                else if (amount == 2)
                    kernel = (cv::Mat_<double>(3, 3) << -1, -1, -1, -1, 9, -1, -1, -1, -1);
                else
                    kernel = (cv::Mat_<double>(3, 3) << -2, -2, -2, -2, 17, -2, -2, -2, -2);
                cv::filter2D(*input, frame, -1, kernel, cv::Point(-1, -1), 0, cv::BORDER_DEFAULT);
            }
        } else {
            cv::Mat temporary;
            if (input->channels() > 1) {
                cv::cvtColor(*input, temporary, cv::COLOR_BGR2GRAY);
                cv::Laplacian(temporary, temporary, -1, 3, -static_cast<float>(amount), 0.0, cv::BORDER_DEFAULT);
            } else {
                cv::Laplacian(*input, temporary, -1, 3, -static_cast<float>(amount), 0.0, cv::BORDER_DEFAULT);
            }
            cv::Mat color;
            cv::cvtColor(temporary, color, cv::COLOR_GRAY2BGR);
            // Preserve FlowCV's original depth and color merge, including its
            // unsupported grayscale/BGRA inputs in this mode.
            cv::addWeighted(*input, 1.0, color, static_cast<float>(amount), 0.0, frame);
        }
        if (!frame.empty()) outputs.SetValue(0, frame);
    } catch (const cv::Exception&) {
        processing_error_.store(true);
    }
}
}
