#include "draw_json.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
using Json = nlohmann::json;
int Integer(const Json& value)
{
    if (!value.is_number_integer() || value.get<double>() < std::numeric_limits<int>::min()
        || value.get<double>() > std::numeric_limits<int>::max())
        throw std::invalid_argument("Draw_JSON integer is out of range");
    return value.get<int>();
}
float Number(const Json& value)
{
    if (!value.is_number() || !std::isfinite(value.get<double>())
        || std::abs(value.get<double>()) > std::numeric_limits<float>::max())
        throw std::invalid_argument("Draw_JSON requires finite numeric values");
    return value.get<float>();
}
const Json* SelectedData(const Json& envelope, int index)
{
    const auto data = envelope.find("data");
    if (data == envelope.end() || !data->is_array() || index < 0 || size_t(index) >= data->size()) return nullptr;
    return &data->at(size_t(index));
}
std::string ValueText(const Json& value, int precision)
{
    if (!value.is_number_float()) return value.dump();
    std::ostringstream result;
    result.precision(precision);
    result << std::fixed << value.get<float>(); // FlowCV formats float, even for JSON doubles.
    return result.str();
}
}
namespace DSPatch::DSPatchables {
DrawJson::DrawJson() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Draw_JSON"); SetComponentCategory_(Category::Category_Draw);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "json"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    SetEnabled(true);
}
DrawJson::Settings DrawJson::GetSettings() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return settings_;
}
DrawJson::UiSnapshot DrawJson::GetUiSnapshot() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return {settings_, json_data_, list_revision_};
}
void DrawJson::UpdateSettings(const Json& state)
{
    if (!state.is_object()) throw std::invalid_argument("Draw_JSON state must be an object");
    const std::lock_guard<std::mutex> lock(mutex_);
    auto next = settings_;
    if (state.contains("color")) {
        next.color[0] = Number(state.at("color").at("R"));
        next.color[1] = Number(state.at("color").at("G"));
        next.color[2] = Number(state.at("color").at("B"));
    }
    if (state.contains("position")) {
        next.position.x = Integer(state.at("position").at("x"));
        next.position.y = Integer(state.at("position").at("y"));
    }
    if (state.contains("scale")) next.scale = Number(state.at("scale"));
    if (state.contains("thickness")) next.thickness = Integer(state.at("thickness"));
    if (state.contains("decimal_places")) next.decimal_places = Integer(state.at("decimal_places"));
    if (state.contains("font")) {
        next.font = Integer(state.at("font"));
        if ((next.font & 15) > cv::FONT_HERSHEY_SCRIPT_COMPLEX)
            throw std::invalid_argument("Invalid Draw_JSON font");
    }
    if (state.contains("json_data_index")) next.json_data_index = Integer(state.at("json_data_index"));
    if (state.contains("out_list")) {
        const auto& list = state.at("out_list");
        if (!list.is_null() && !list.is_array()) throw std::invalid_argument("Draw_JSON out_list must be an array or null");
        next.out_list.clear();
        for (const auto& item : list) {
            if (!item.is_object()) throw std::invalid_argument("Draw_JSON list entry must be an object");
            JsonOutItem entry;
            if (item.contains("is_array")) entry.is_array = item.at("is_array").get<bool>();
            if (entry.is_array && item.contains("item_index")) entry.out_item_index = Integer(item.at("item_index"));
            if (item.contains("item_path")) entry.out_key_path = item.at("item_path").get<std::string>();
            next.out_list.push_back(std::move(entry));
        }
    }
    settings_ = std::move(next);
    if (state.contains("out_list")) ++list_revision_;
}
void DrawJson::SetState(std::string&& serialized) { UpdateSettings(Json::parse(serialized)); }
std::string DrawJson::GetState()
{
    const auto s = GetSettings();
    Json list;
    for (const auto& item : s.out_list) {
        Json entry{{"is_array", item.is_array}, {"item_path", item.out_key_path}};
        if (item.is_array) entry["item_index"] = item.out_item_index;
        list.push_back(std::move(entry));
    }
    return Json{{"position", {{"x", s.position.x}, {"y", s.position.y}}}, {"scale", s.scale},
        {"font", s.font}, {"thickness", s.thickness}, {"decimal_places", s.decimal_places},
        {"color", {{"R", s.color[0]}, {"G", s.color[1]}, {"B", s.color[2]}}},
        {"json_data_index", s.json_data_index}, {"out_list", list}}.dump();
}
bool DrawJson::AddField(const std::string& path)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    const auto* selected = SelectedData(json_data_, settings_.json_data_index);
    if (!selected) return false;
    try {
        const Json::json_pointer pointer(path);
        if (!selected->contains(pointer)) return false;
        const auto& value = selected->at(pointer);
        if (value.is_object()) return false;
        if (!value.is_array() && std::any_of(settings_.out_list.begin(), settings_.out_list.end(),
            [&path](const JsonOutItem& item) { return item.out_key_path == path; })) return false;
        settings_.out_list.push_back({0, path, value.is_array()});
        ++list_revision_;
        return true;
    } catch (const Json::exception&) { return false; }
}
bool DrawJson::RemoveField(size_t index, uint64_t revision)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    if (revision != list_revision_ || index >= settings_.out_list.size()) return false;
    settings_.out_list.erase(settings_.out_list.begin() + index);
    ++list_revision_;
    return true;
}
bool DrawJson::SetItemIndex(size_t index, int value, uint64_t revision)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    if (revision != list_revision_ || index >= settings_.out_list.size() || !settings_.out_list[index].is_array) return false;
    settings_.out_list[index].out_item_index = std::max(0, value);
    return true;
}
void DrawJson::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    processing_error_.store(false);
    const auto* input = inputs.GetValue<cv::Mat>(0);
    const auto* data = inputs.GetValue<Json>(1);
    if (!input || input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); return; }
    if (!data || data->empty()) return;
    Settings s;
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        json_data_ = *data;
        s = settings_;
    }
    try {
        if (input->dims != 2) { processing_error_.store(true); return; }
        cv::Mat frame;
        input->copyTo(frame);
        auto position = s.position;
        if (const auto* selected = SelectedData(*data, s.json_data_index)) {
            for (const auto& item : s.out_list) {
                try {
                    const Json::json_pointer pointer(item.out_key_path);
                    if (!selected->contains(pointer)) continue;
                    const auto& value = selected->at(pointer);
                    std::string text = item.out_key_path;
                    if (value.is_array()) {
                        text += " [" + std::to_string(item.out_item_index) + "]: ";
                        if (item.out_item_index >= 0 && size_t(item.out_item_index) < value.size())
                            text += ValueText(value.at(size_t(item.out_item_index)), s.decimal_places);
                    } else text += ": " + ValueText(value, s.decimal_places);
                    cv::putText(frame, text, position, s.font, s.scale,
                        cv::Scalar(s.color[2] * 255, s.color[1] * 255, s.color[0] * 255), s.thickness);
                    int baseline = 0;
                    const auto size = cv::getTextSize(text, s.font, s.scale, s.thickness, &baseline);
                    const int64_t nextY = int64_t(position.y) + size.height + 14;
                    if (nextY < std::numeric_limits<int>::min() || nextY > std::numeric_limits<int>::max()) {
                        processing_error_.store(true); break;
                    }
                    position.y = int(nextY);
                } catch (const Json::exception&) { processing_error_.store(true); }
            }
        } else if (data->contains("data") && !data->at("data").empty() && !data->at("data").is_array())
            processing_error_.store(true);
        outputs.SetValue(0, frame);
    } catch (const cv::Exception&) { processing_error_.store(true); }
      catch (const Json::exception&) { processing_error_.store(true); }
}
}
