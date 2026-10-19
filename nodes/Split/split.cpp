#include "split.hpp"
#include <opencv2/core.hpp>
#include <vector>

namespace DSPatch::DSPatchables {
Split::Split() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Split");
    SetComponentCategory_(Category::Category_Utility);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(4, {"r", "g", "b", "a"},
        {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat, IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetEnabled(true);
}

void Split::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    if (!IsEnabled()) SetEnabled(true);
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty() || input->channels() < 3) {
        is_color_.store(false);
        return;
    }
    try {
        std::vector<cv::Mat> planes;
        cv::split(*input, planes);
        outputs.SetValue(0, planes[2]);
        outputs.SetValue(1, planes[1]);
        outputs.SetValue(2, planes[0]);
        if (planes.size() == 4) outputs.SetValue(3, planes[3]);
        is_color_.store(true);
    }
    catch (const cv::Exception&) {
        is_color_.store(false);
    }
}

std::string Split::GetState() { return "null"; }
void Split::SetState(std::string&& state) { (void)nlohmann::json::parse(state); }
}
