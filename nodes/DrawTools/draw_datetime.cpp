#include "draw_datetime.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
int Integer(const nlohmann::json& value)
{
    if (!value.is_number_integer() || value.get<double>() < std::numeric_limits<int>::min()
        || value.get<double>() > std::numeric_limits<int>::max())
        throw std::invalid_argument("Draw_Date_Time integer is out of range");
    return value.get<int>();
}
float Number(const nlohmann::json& value)
{
    if (!value.is_number() || !std::isfinite(value.get<double>())
        || std::abs(value.get<double>()) > std::numeric_limits<float>::max())
        throw std::invalid_argument("Draw_Date_Time requires finite numeric values");
    return value.get<float>();
}
#ifdef _WIN32
thread_local bool invalidTimeFormat = false;
void InvalidTimeParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
    invalidTimeFormat = true;
}
struct TimeParameterGuard {
    _invalid_parameter_handler previous = _set_thread_local_invalid_parameter_handler(InvalidTimeParameter);
    TimeParameterGuard() { invalidTimeFormat = false; }
    ~TimeParameterGuard() { _set_thread_local_invalid_parameter_handler(previous); }
};
#endif
bool FormatPart(const std::string& format, const std::tm& localTime, std::string& result)
{
    // A sentinel also distinguishes valid empty expansions (e.g. a timezone)
    // from strftime's failure return. Never consume an uninitialized buffer.
    const auto effective = format.substr(0, format.find('\0'));
    size_t trailingPercents = 0;
    for (auto i = effective.rbegin(); i != effective.rend() && *i == '%'; ++i) ++trailingPercents;
    if (trailingPercents % 2) return false;
    const auto marked = effective + "#";
#ifdef _WIN32
    TimeParameterGuard guard;
#endif
    const size_t maximum = std::max<size_t>(256, marked.size() * 256);
    for (size_t size = 128;; size = std::min(size * 2, maximum)) {
        std::vector<char> buffer(size, '\0');
        const auto length = std::strftime(buffer.data(), buffer.size(), marked.c_str(), &localTime);
        if (length) { result.assign(buffer.data(), length - 1); return true; }
#ifdef _WIN32
        if (invalidTimeFormat) return false;
#endif
        if (size >= maximum) return false;
    }
}
}
namespace DSPatch::DSPatchables {
DrawDateTime::DrawDateTime() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Draw_Date_Time"); SetComponentCategory_(Category::Category_Draw);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat}); SetEnabled(true);
}
DrawDateTime::Settings DrawDateTime::GetSettings() const
{
    const std::lock_guard<std::mutex> lock(settings_mutex_); return settings_;
}
void DrawDateTime::UpdateSettings(const nlohmann::json& changes)
{
    if (!changes.is_object()) throw std::invalid_argument("Draw_Date_Time state must be an object");
    const std::lock_guard<std::mutex> lock(settings_mutex_);
    auto next = settings_;
    if (changes.contains("position")) {
        next.position.x = Integer(changes.at("position").at("x"));
        next.position.y = Integer(changes.at("position").at("y"));
    }
    if (changes.contains("color")) {
        next.color[0] = Number(changes.at("color").at("R"));
        next.color[1] = Number(changes.at("color").at("G"));
        next.color[2] = Number(changes.at("color").at("B"));
    }
    if (changes.contains("scale")) next.scale = Number(changes.at("scale"));
    if (changes.contains("thickness")) next.thickness = Integer(changes.at("thickness"));
    if (changes.contains("font")) {
        next.font = Integer(changes.at("font"));
        if ((next.font & 15) > cv::FONT_HERSHEY_SCRIPT_COMPLEX) throw std::invalid_argument("Invalid Draw_Date_Time font");
    }
    if (changes.contains("show_date")) next.show_date = changes.at("show_date").get<bool>();
    if (changes.contains("show_time")) next.show_time = changes.at("show_time").get<bool>();
    if (changes.contains("date_fmt")) { next.date_fmt = changes.at("date_fmt").get<std::string>(); next.date_draft = next.date_fmt; next.valid_date = true; }
    if (changes.contains("time_fmt")) { next.time_fmt = changes.at("time_fmt").get<std::string>(); next.time_draft = next.time_fmt; next.valid_time = true; }
    settings_ = std::move(next);
}
void DrawDateTime::SetState(std::string&& state) { UpdateSettings(nlohmann::json::parse(state)); }
std::string DrawDateTime::GetState()
{
    const auto s = GetSettings();
    nlohmann::json state{{"position", {{"x", s.position.x}, {"y", s.position.y}}}, {"scale", s.scale},
        {"font", s.font}, {"thickness", s.thickness}, {"color", {{"R", s.color[0]}, {"G", s.color[1]}, {"B", s.color[2]}}},
        {"show_date", s.show_date}, {"show_time", s.show_time}};
    if (s.show_date) state["date_fmt"] = s.date_fmt;
    if (s.show_time) state["time_fmt"] = s.time_fmt;
    return state.dump();
}
bool DrawDateTime::IsFormatValid(int mode, const std::string& format)
{
    const std::string allowed = mode == 0 ? "YyCGgbhBmUWVjdeaAwuxDF" : mode == 1 ? "HIMScXrRTpzZ" : "";
    if (allowed.empty() || format.size() < 2 || format.back() == '%') return false;
    int found = 0, matched = -1;
    for (size_t i = 0; i + 1 < format.size(); ++i) {
        if (format[i] == '%') {
            ++found;
            if (allowed.find(format[i + 1]) != std::string::npos) { matched = matched == -1 ? 1 : matched + 1; ++i; }
        }
    }
    return found == matched;
}
bool DrawDateTime::CommitFormat(int mode, std::string format)
{
    if (mode != 0 && mode != 1) return false;
    const bool valid = IsFormatValid(mode, format);
    const std::lock_guard<std::mutex> lock(settings_mutex_);
    if (mode == 0) { settings_.date_draft = std::move(format); settings_.valid_date = valid; if (valid) settings_.date_fmt = settings_.date_draft; }
    else { settings_.time_draft = std::move(format); settings_.valid_time = valid; if (valid) settings_.time_fmt = settings_.time_draft; }
    return valid;
}
bool DrawDateTime::FormatTime(const Settings& settings, const std::tm& localTime, std::string& text)
{
    text.clear(); std::string part;
    if (settings.show_date) { if (!FormatPart(settings.date_fmt, localTime, part)) return false; text += part; }
    if (settings.show_time) { if (!FormatPart(settings.time_fmt, localTime, part)) { text.clear(); return false; } text += part; }
    return true;
}
void DrawDateTime::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    processing_error_.store(false);
    const auto input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); return; }
    const auto settings = GetSettings();
    const auto now = std::time(nullptr); std::tm localTime{};
#ifdef _WIN32
    if (localtime_s(&localTime, &now) != 0) { processing_error_.store(true); return; }
#else
    if (!localtime_r(&now, &localTime)) { processing_error_.store(true); return; }
#endif
    std::string text;
    if (!FormatTime(settings, localTime, text)) { processing_error_.store(true); return; }
    try {
        if (input->dims != 2) { processing_error_.store(true); return; }
        auto frame = input->clone();
        cv::putText(frame, text, settings.position, settings.font, settings.scale,
            cv::Scalar(settings.color[2] * 255, settings.color[1] * 255, settings.color[0] * 255), settings.thickness);
        outputs.SetValue(0, frame);
    } catch (const cv::Exception&) { processing_error_.store(true); }
}
}
