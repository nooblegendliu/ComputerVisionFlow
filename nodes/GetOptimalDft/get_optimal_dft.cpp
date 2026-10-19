#include "get_optimal_dft.hpp"
#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables {
GetOptimalDft::GetOptimalDft() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Get_Optimal_DFT_Size");
    SetComponentCategory_(Category::Category_Utility);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "size"}, {IoType::Io_Type_CvMat, IoType::Io_Type_Int_Array});
    SetOutputCount_(1, {"size"}, {IoType::Io_Type_Int_Array});
    SetEnabled(true);
}

void GetOptimalDft::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    if (!IsEnabled()) return;
    const auto* frame = inputs.GetValue<cv::Mat>(0);
    const auto* size = inputs.GetValue<std::vector<int>>(1);
    // A nonempty image takes precedence over the optional [width, height] input.
    if (frame && !frame->empty())
        outputs.SetValue(0, std::vector<int>{cv::getOptimalDFTSize(frame->cols), cv::getOptimalDFTSize(frame->rows)});
    else if (size && size->size() == 2)
        outputs.SetValue(0, std::vector<int>{cv::getOptimalDFTSize(size->at(0)), cv::getOptimalDFTSize(size->at(1))});
}
std::string GetOptimalDft::GetState() { return "null"; }
void GetOptimalDft::SetState(std::string&& state) { (void)nlohmann::json::parse(state); }
}
