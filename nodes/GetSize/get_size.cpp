#include "get_size.hpp"
#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables {
GetSize::GetSize() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Get_Size");
    SetComponentCategory_(Category::Category_Utility);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"size"}, {IoType::Io_Type_Int_Array});
    SetEnabled(true);
}
void GetSize::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (input && !input->empty() && IsEnabled())
        outputs.SetValue(0, std::vector<int>{input->cols, input->rows});
}
std::string GetSize::GetState() { return "null"; }
void GetSize::SetState(std::string&& serialized)
{
    (void)nlohmann::json::parse(serialized);
}
}
