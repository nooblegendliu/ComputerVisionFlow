#include "Morphology.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <limits>

namespace DSPatch::DSPatchables {
namespace {
using Json = nlohmann::json;
Json Snapshot(const FlowCV::FlowCV_Properties& properties)
{
    Json state = Json::object();
    for (const auto& property : properties.Snapshot().properties)
        std::visit([&](const auto& value) { state[property.key] = value; }, property.value);
    return state;
}
void CheckInteger(const Json& state, const char* key, int low, int high)
{
    if (!state.contains(key)) return;
    const auto& value = state.at(key);
    if (!value.is_number_integer() || value.get<double>() < low || value.get<double>() > high)
        throw std::invalid_argument(std::string("Invalid Morphology parameter: ") + key);
}
void Validate(const Json& state)
{
    CheckInteger(state, "operation", 0, 6);
    CheckInteger(state, "shape", 0, 2);
    CheckInteger(state, "amt", 0, ((std::numeric_limits<int>::max)() - 1) / 2);
    CheckInteger(state, "iter", 0, (std::numeric_limits<int>::max)());
    CheckInteger(state, "border_type", 0, 5);
    for (const auto* channel : {"R", "G", "B"}) {
        const std::string key = std::string("border_color.") + channel;
        if (!state.contains(key)) continue;
        const auto& value = state.at(key);
        if (!value.is_number() || !std::isfinite(value.get<double>())
            || value.get<double>() < 0 || value.get<double>() > 1)
            throw std::invalid_argument("Invalid Morphology border color");
    }
}
}

Morphology::Morphology() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Morphology");
    SetComponentCategory_(Category::Category_Filter);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("operation", "Operation", 0, {"Erode", "Dilate", "Open", "Close", "Gradient", "Tophat", "Blackhat"});
    properties_.AddOption("shape", "Morph Shape", cv::MORPH_ELLIPSE, {"Rect", "Cross", "Ellipse"});
    properties_.AddInt("amt", "Morph Amt", 1, 1, 200, .25f);
    properties_.AddInt("iter", "Iter", 1, 1, 50, .25f);
    // These are the original raw OpenCV values. Wrap/Transparent stay selectable;
    // OpenCV may reject them for morphology, which is caught below as upstream did.
    properties_.AddOption("border_type", "Border Type", cv::BORDER_DEFAULT,
        {"Constant", "Replicate", "Reflect", "Wrap", "Default", "Transparent"});
    for (const auto* channel : {"R", "G", "B"})
        properties_.AddFloat(std::string("border_color.") + channel, channel, 0, 0, 1, .01f, false);
    SetEnabled(true);
}

std::string Morphology::GetState()
{
    auto state = Snapshot(properties_);
    for (const auto* channel : {"R", "G", "B"}) {
        const std::string key = std::string("border_color.") + channel;
        state["border_color"][channel] = state.at(key);
        state.erase(key);
    }
    return state.dump();
}

void Morphology::SetState(std::string&& serialized)
{
    if (serialized.empty()) return;
    auto state = Json::parse(serialized);
    if (!state.is_object()) throw std::invalid_argument("Morphology state must be an object");
    if (state.contains("border_color"))
        for (const auto* channel : {"R", "G", "B"})
            state[std::string("border_color.") + channel] = state.at("border_color").at(channel);
    Validate(state);
    properties_.FromJson(state);
}

void Morphology::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); processing_error_.store(false); return; }
    properties_.Sync();
    const auto state = Snapshot(properties_);
    try {
        Validate(state);
        const int amount = state.at("amt");
        const auto element = cv::getStructuringElement(state.at("shape").get<int>(), cv::Size(2 * amount + 1, 2 * amount + 1), cv::Point(amount, amount));
        cv::Mat frame;
        cv::morphologyEx(*input, frame, state.at("operation").get<int>(), element, cv::Point(-1, -1), state.at("iter").get<int>(), state.at("border_type").get<int>(),
            cv::Scalar(state.at("border_color.B").get<float>() * 255, state.at("border_color.G").get<float>() * 255,
                state.at("border_color.R").get<float>() * 255));
        if (!frame.empty()) outputs.SetValue(0, frame);
        processing_error_.store(false);
    } catch (const std::exception&) { processing_error_.store(true); }
}
}
