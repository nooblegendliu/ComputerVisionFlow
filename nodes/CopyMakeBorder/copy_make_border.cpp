#include "copy_make_border.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <cstdint>
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
void Validate(const Json& state)
{
    if (state.contains("border_type") && (!state.at("border_type").is_number_integer()
        || state.at("border_type").get<double>() < 0 || state.at("border_type").get<double>() > 4))
        throw std::invalid_argument("Invalid Copy_Make_Border border type");
    for (const auto* channel : {"R", "G", "B"}) {
        const auto key = std::string("border_color.") + channel;
        if (state.contains(key) && (!state.at(key).is_number() || !std::isfinite(state.at(key).get<double>())
            || state.at(key).get<double>() < 0 || state.at(key).get<double>() > 1))
            throw std::invalid_argument("Invalid Copy_Make_Border color");
    }
}
}

CopyMakeBorder::CopyMakeBorder() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Copy_Make_Border");
    SetComponentCategory_(Category::Category_Transform);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "size"}, {IoType::Io_Type_CvMat, IoType::Io_Type_Int_Array});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddBool("center", "Center", true);
    properties_.AddInt("add_width", "Border Width", 0, 0, 500, .5f);
    properties_.AddInt("add_height", "Border Height", 0, 0, 500, .5f);
    properties_.AddOption("border_type", "Border Type", 0, {"Constant", "Replicate", "Reflect", "Wrap", "Default"});
    for (const auto* channel : {"R", "G", "B"})
        properties_.AddFloat(std::string("border_color.") + channel, channel, 0, 0, 1, .01f, false);
    SetEnabled(true);
}

std::string CopyMakeBorder::GetState()
{
    auto state = Snapshot(properties_);
    for (const auto* channel : {"R", "G", "B"}) {
        const auto key = std::string("border_color.") + channel;
        state["border_color"][channel] = state.at(key);
        state.erase(key);
    }
    return state.dump();
}

void CopyMakeBorder::SetState(std::string&& serialized)
{
    if (serialized.empty()) return;
    auto state = Json::parse(serialized);
    if (!state.is_object()) throw std::invalid_argument("Copy_Make_Border state must be an object");
    if (state.contains("border_color"))
        for (const auto* channel : {"R", "G", "B"})
            state[std::string("border_color.") + channel] = state.at("border_color").at(channel);
    Validate(state);
    properties_.FromJson(state);
}

void CopyMakeBorder::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); processing_error_.store(false); return; }
    properties_.Sync();
    const auto state = Snapshot(properties_);
    try {
        Validate(state);
        const auto* size = inputs.GetValue<std::vector<int>>(1);
        const bool hasSize = size && size->size() == 2;
        const auto width = int64_t(hasSize ? size->at(0) : input->cols) + state.at("add_width").get<int>();
        const auto height = int64_t(hasSize ? size->at(1) : input->rows) + state.at("add_height").get<int>();
        const auto deltaX = std::abs(width - input->cols);
        const auto deltaY = std::abs(height - input->rows);
        const bool center = state.at("center").get<bool>();
        // Preserve upstream: smaller requested sizes also add borders; centered odd
        // differences are truncated independently on both sides.
        const auto borderX = center ? int64_t(static_cast<float>(deltaX) * .5f) : deltaX;
        const auto borderY = center ? int64_t(static_cast<float>(deltaY) * .5f) : deltaY;
        const auto maxInt = (std::numeric_limits<int>::max)();
        if (int64_t(input->cols) + borderX * (center ? 2 : 1) > maxInt
            || int64_t(input->rows) + borderY * (center ? 2 : 1) > maxInt)
            throw std::overflow_error("Copy_Make_Border output dimensions overflow");
        cv::Mat result;
        cv::copyMakeBorder(*input, result, center ? int(borderY) : 0, int(borderY),
            center ? int(borderX) : 0, int(borderX), state.at("border_type").get<int>(),
            cv::Scalar(state.at("border_color.B").get<float>() * 255, state.at("border_color.G").get<float>() * 255,
                state.at("border_color.R").get<float>() * 255));
        if (!result.empty()) outputs.SetValue(0, result);
        processing_error_.store(false);
    } catch (const std::exception&) { processing_error_.store(true); }
}
}
