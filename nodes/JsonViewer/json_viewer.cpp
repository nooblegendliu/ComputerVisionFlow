#include "json_viewer.hpp"

namespace DSPatch::DSPatchables {
JsonViewer::JsonViewer() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Json_Viewer");
    SetComponentCategory_(Category::Category_Views);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"JSON"}, {IoType::Io_Type_JSON});
    SetOutputCount_(0);
    SetEnabled(true);
}
void JsonViewer::Process_(SignalBus const& inputs, SignalBus&)
{
    if (!IsEnabled()) SetEnabled(true);
    const auto* input = inputs.GetValue<nlohmann::json>(0);
    if (input && !input->empty()) {
        std::lock_guard<std::mutex> lock(io_mutex_);
        json_data_ = *input;
    }
}
nlohmann::json JsonViewer::GetJsonCopy() const
{
    std::lock_guard<std::mutex> lock(io_mutex_);
    return json_data_;
}
std::string JsonViewer::GetState() { return {}; }
void JsonViewer::SetState(std::string&& serialized)
{
    // Empty state is the node's own serialized representation.
    if (!serialized.empty()) (void)nlohmann::json::parse(serialized);
}
}
