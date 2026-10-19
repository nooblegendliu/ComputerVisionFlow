#include "combine.hpp"
#include <opencv2/core.hpp>
#include <vector>

namespace DSPatch::DSPatchables {
Combine::Combine() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Combine");
    SetComponentCategory_(Category::Category_Utility);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(4, {"r", "g", "b", "a"},
        {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat, IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    SetEnabled(true);
}

void Combine::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    if (!IsEnabled()) SetEnabled(true);
    const auto* r = inputs.GetValue<cv::Mat>(0);
    const auto* g = inputs.GetValue<cv::Mat>(1);
    const auto* b = inputs.GetValue<cv::Mat>(2);
    const auto* a = inputs.GetValue<cv::Mat>(3);
    if (!r || !g || !b) return;
    if (r->empty() && g->empty() && b->empty()) return;
    if (r->type() != g->type() || r->type() != b->type()) return;
    if (r->size != g->size || r->size != b->size) return;
    try {
        std::vector<cv::Mat> channels{*b, *g, *r};
        if (a && !a->empty() && r->type() == a->type() && r->size == a->size)
            channels.emplace_back(*a);
        cv::Mat output;
        cv::merge(channels, output);
        outputs.SetValue(0, output);
    }
    catch (const cv::Exception&) {
        // Keep unsupported inputs from terminating the AutoTick worker.
    }
}

std::string Combine::GetState() { return "null"; }
void Combine::SetState(std::string&& state) { (void)nlohmann::json::parse(state); }
}
