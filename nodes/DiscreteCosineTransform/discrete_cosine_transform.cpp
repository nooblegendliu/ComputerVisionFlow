#include "discrete_cosine_transform.hpp"
#include <opencv2/imgproc.hpp>

namespace DSPatch::DSPatchables {
DiscreteCosineTransform::DiscreteCosineTransform() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("DCT");
    SetComponentCategory_(Category::Category_Utility);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("op_mode", "Operation", 0, {"Default", "Inverse", "Rows"});
    SetEnabled(true);
}

void DiscreteCosineTransform::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    processing_error_.store(false);
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty() || !IsEnabled()) return;
    const int mode = properties_.GetW<int>("op_mode");
    try {
        cv::Mat dctFrame, frame;
        if (input->channels() > 1) cv::cvtColor(*input, dctFrame, cv::COLOR_BGR2GRAY);
        else input->copyTo(dctFrame);
        // Keep upstream's asymmetric scaling: CV_32F is already in the working
        // domain, whereas every other input depth (including CV_64F) is /255.
        if (dctFrame.type() != CV_32F) dctFrame.convertTo(dctFrame, CV_32F, 1.0 / 255);
        cv::dct(dctFrame, frame, mode == 2 ? cv::DFT_ROWS : mode);
        if (!frame.empty()) outputs.SetValue(0, frame);
    } catch (const cv::Exception&) { processing_error_.store(true); }
}

std::string DiscreteCosineTransform::GetState() { return "null"; }
void DiscreteCosineTransform::SetState(std::string&& state) { (void)nlohmann::json::parse(state); }
}
