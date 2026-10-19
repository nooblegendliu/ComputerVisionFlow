#include "draw_text.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
int Integer(const nlohmann::json& value)
{
    if (!value.is_number_integer() || value.get<double>() < std::numeric_limits<int>::min()
        || value.get<double>() > std::numeric_limits<int>::max())
        throw std::invalid_argument("Draw_Text integer is out of range");
    return value.get<int>();
}
float Number(const nlohmann::json& value)
{
    if (!value.is_number() || !std::isfinite(value.get<double>())
        || std::abs(value.get<double>()) > std::numeric_limits<float>::max())
        throw std::invalid_argument("Draw_Text requires finite numeric values");
    return value.get<float>();
}
}
namespace DSPatch::DSPatchables {
DrawText::DrawText() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Draw_Text");
    SetComponentCategory_(Category::Category_Draw);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "str"}, {IoType::Io_Type_CvMat, IoType::Io_Type_String});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    SetEnabled(true);
}
DrawText::Settings DrawText::GetSettings() const
{
    const std::lock_guard<std::mutex> lock(settings_mutex_);
    return settings_;
}
void DrawText::ApplyChanges(Settings& settings, const nlohmann::json& changes)
{
    if (!changes.is_object()) throw std::invalid_argument("Draw_Text state must be an object");
    if (changes.contains("text")) settings.text = changes.at("text").get<std::string>();
    if (changes.contains("position")) {
        settings.position.x = Integer(changes.at("position").at("x"));
        settings.position.y = Integer(changes.at("position").at("y"));
    }
    if (changes.contains("color")) {
        settings.color[0] = Number(changes.at("color").at("R"));
        settings.color[1] = Number(changes.at("color").at("G"));
        settings.color[2] = Number(changes.at("color").at("B"));
    }
    if (changes.contains("scale")) settings.scale = Number(changes.at("scale"));
    if (changes.contains("font")) {
        settings.font = Integer(changes.at("font"));
        if ((settings.font & 15) > cv::FONT_HERSHEY_SCRIPT_COMPLEX)
            throw std::invalid_argument("Invalid Draw_Text font");
    }
    if (changes.contains("thickness")) settings.thickness = Integer(changes.at("thickness"));
}
void DrawText::UpdateSettings(const nlohmann::json& changes)
{
    const std::lock_guard<std::mutex> lock(settings_mutex_);
    auto next = settings_;
    ApplyChanges(next, changes);
    settings_ = std::move(next);
}
void DrawText::SetState(std::string&& serialized)
{
    const auto state = nlohmann::json::parse(serialized);
    const std::lock_guard<std::mutex> lock(settings_mutex_);
    auto next = settings_;
    next.text.clear(); // FlowCV clears local text even when text is omitted.
    ApplyChanges(next, state);
    settings_ = std::move(next);
}
std::string DrawText::GetState()
{
    const auto settings = GetSettings();
    nlohmann::json state{{"position", {{"x", settings.position.x}, {"y", settings.position.y}}},
        {"scale", settings.scale}, {"font", settings.font}, {"thickness", settings.thickness},
        {"color", {{"R", settings.color[0]}, {"G", settings.color[1]}, {"B", settings.color[2]}}}};
    if (!settings.has_string_input) state["text"] = settings.text;
    return state.dump();
}
void DrawText::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    processing_error_.store(false);
    const auto input = inputs.GetValue<cv::Mat>(0);
    const auto text = inputs.GetValue<std::string>(1);
    if (!input) return;
    Settings settings;
    {
        const std::lock_guard<std::mutex> lock(settings_mutex_);
        settings_.has_string_input = text != nullptr;
        settings = settings_;
    }
    if (input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); return; }
    try {
        if (input->dims != 2) { processing_error_.store(true); return; }
        cv::Mat frame;
        input->copyTo(frame);
        cv::putText(frame, text ? *text : settings.text, settings.position, settings.font, settings.scale,
            cv::Scalar(settings.color[2] * 255, settings.color[1] * 255, settings.color[0] * 255), settings.thickness);
        if (!frame.empty()) outputs.SetValue(0, frame);
    } catch (const cv::Exception&) { processing_error_.store(true); }
}
}
