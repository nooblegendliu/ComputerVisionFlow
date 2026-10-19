#include "threshold.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace DSPatch::DSPatchables {
namespace {
using Json = nlohmann::json;
// The stored range is RGB, despite the upstream member names hsv_low_/high_.
// Preserve ImGui's conversion arithmetic without a GUI dependency.
cv::Scalar RgbToHsv(float r, float g, float b)
{
    float k = 0;
    if (g < b) { std::swap(g, b); k = -1; }
    if (r < g) { std::swap(r, g); k = -2.f / 6.f - k; }
    const float chroma = r - (g < b ? g : b);
    const float h = std::fabs(k + (g - b) / (6.f * chroma + 1e-20f));
    const float s = chroma / (r + 1e-20f);
    return cv::Scalar(h * 179, s * 255, r * 255);
}
Json Snapshot(const FlowCV::FlowCV_Properties& properties)
{
    Json state = Json::object();
    for (const auto& property : properties.Snapshot().properties)
        std::visit([&](const auto& value) { state[property.key] = value; }, property.value);
    return state;
}
void CheckInteger(const Json& state, const char* key, int low, int high, bool odd = false)
{
    if (!state.contains(key)) return;
    const auto& value = state.at(key);
    if (!value.is_number_integer() || value.get<double>() < low || value.get<double>() > high
        || (odd && value.get<int>() % 2 == 0))
        throw std::invalid_argument(std::string("Invalid Threshold parameter: ") + key);
}
void Validate(const Json& state)
{
    CheckInteger(state, "thresh_method", 0, 3);
    CheckInteger(state, "thresh_type", 0, 4);
    CheckInteger(state, "thresh_amt", 0, 255);
    CheckInteger(state, "adapt_type", 0, 1);
    CheckInteger(state, "adapt_thresh", 0, 255);
    CheckInteger(state, "adapt_block", 3, (std::numeric_limits<int>::max)(), true);
    for (const auto* group : {"low_range", "high_range"})
        for (const auto* channel : {"R", "G", "B"}) {
            const std::string key = std::string(group) + "." + channel;
            if (!state.contains(key)) continue;
            const auto& value = state.at(key);
            if (!value.is_number() || !std::isfinite(value.get<double>())
                || value.get<double>() < 0 || value.get<double>() > 1)
                throw std::invalid_argument("Invalid Threshold RGB range");
        }
}
}

Threshold::Threshold() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Threshold");
    SetComponentCategory_(Category::Category_Color);
    SetComponentAuthor_("richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("thresh_method", "Method", 0, {"Simple", "Color Range", "Adaptive", "Otsu"});
    properties_.AddOption("thresh_type", "Type", 0, {"Binary", "Binary Inv", "Trunc", "ToZero", "ToZero Inv"});
    properties_.AddInt("thresh_amt", "Thresh", 127, 1, 255);
    properties_.AddOption("adapt_type", "Adapt Type", 0, {"Mean", "Gaussian"});
    properties_.AddInt("adapt_thresh", "Adapt Thresh", 2, 0, 255);
    properties_.AddInt("adapt_block", "Block Size", 11, 3, (std::numeric_limits<int>::max)(), 2);
    for (const auto* channel : {"R", "G", "B"}) {
        properties_.AddFloat(std::string("low_range.") + channel, channel, 0, 0, 1, .01f, false);
        properties_.AddFloat(std::string("high_range.") + channel, channel, 1, 0, 1, .01f, false);
    }
    SetEnabled(true);
}

std::string Threshold::GetState()
{
    auto state = Snapshot(properties_);
    for (const auto* group : {"low_range", "high_range"})
        for (const auto* channel : {"R", "G", "B"}) {
            const std::string key = std::string(group) + "." + channel;
            state[group][channel] = state.at(key);
            state.erase(key);
        }
    return state.dump();
}

void Threshold::SetState(std::string&& serialized)
{
    if (serialized.empty()) return;
    auto state = Json::parse(serialized);
    if (!state.is_object()) throw std::invalid_argument("Threshold state must be an object");
    for (const auto* group : {"low_range", "high_range"})
        if (state.contains(group))
            for (const auto* channel : {"R", "G", "B"})
                state[std::string(group) + "." + channel] = state.at(group).at(channel);
    Validate(state);
    // Includes hidden color channels, with all fields validated before one commit.
    properties_.FromJson(state);
}

void Threshold::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); processing_error_.store(false); return; }
    properties_.Sync();
    const auto state = Snapshot(properties_);
    try {
        Validate(state);
        const int method = state.at("thresh_method"), type = state.at("thresh_type");
        cv::Mat frame;
        if (method == 0)
            cv::threshold(*input, frame, state.at("thresh_amt").get<int>(), 255, type);
        else if (method == 1) {
            const auto low = RgbToHsv(state.at("low_range.R"), state.at("low_range.G"), state.at("low_range.B"));
            const auto high = RgbToHsv(state.at("high_range.R"), state.at("high_range.G"), state.at("high_range.B"));
            if (input->channels() > 1) cv::cvtColor(*input, frame, cv::COLOR_BGR2HSV);
            else { cv::cvtColor(*input, frame, cv::COLOR_GRAY2BGR); cv::cvtColor(frame, frame, cv::COLOR_BGR2HSV); }
            cv::inRange(frame, low, high, frame);
        }
        else {
            if (input->channels() > 1) cv::cvtColor(*input, frame, cv::COLOR_BGR2GRAY);
            else input->copyTo(frame);
            if (method == 2)
                cv::adaptiveThreshold(frame, frame, 255, state.at("adapt_type").get<int>(), (std::min)(type, 1),
                    state.at("adapt_block").get<int>(), state.at("adapt_thresh").get<int>());
            else cv::threshold(frame, frame, 0, 255, type + cv::THRESH_OTSU);
        }
        if (!frame.empty()) outputs.SetValue(0, frame);
        processing_error_.store(false);
    } catch (const std::exception&) {
        // Invalid input depth/channel combinations must not escape the AutoTick worker.
        processing_error_.store(true);
    }
}
}
