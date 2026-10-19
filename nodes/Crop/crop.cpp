// Adapted from FlowCV Internal_Nodes/Crop (GPL-3.0).
#include "crop.hpp"
#include <opencv2/core.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace DSPatch::DSPatchables {
namespace {
int Integer(const nlohmann::json& value)
{
    if (!value.is_number() || !std::isfinite(value.get<double>()) ||
        value.get<double>() < (std::numeric_limits<int>::min)() ||
        value.get<double>() > (std::numeric_limits<int>::max)())
        throw std::invalid_argument("Invalid Crop coordinate");
    return value.get<int>();
}
}
Crop::Crop() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Crop");
    SetComponentCategory_(Category::Category_Transform);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "bbox"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    SetEnabled(true);
}
Crop::Parameters Crop::GetParameters() const
{
    std::lock_guard lock(mutex_); return parameters_;
}
void Crop::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* image = inputs.GetValue<cv::Mat>(0);
    const auto* json = inputs.GetValue<nlohmann::json>(1);
    if (!image || image->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *image); return; }
    // The upstream crop rectangle is persistent runtime state and is saved.
    // Serialize its updates with GUI edits and other buffered processing calls.
    std::lock_guard lock(mutex_);
    const auto& p = parameters_;
    int64_t x = p.x, y = p.y, width = p.width, height = p.height;
    try {
        has_json_data_.store(json && !json->empty() && json->contains("data"));
        if (has_json_data_.load()) {
            const auto& data = json->at("data");
            if (p.calc_total_bbox) {
                int64_t left = image->cols, top = image->rows, right = 0, bottom = 0;
                for (const auto& item : data) {
                    if (!item.contains("bbox")) continue;
                    const auto& bbox = item.at("bbox");
                    const int64_t bx = Integer(bbox.at("x")), by = Integer(bbox.at("y"));
                    left = (std::min)(left, bx); top = (std::min)(top, by);
                    right = (std::max)(right, bx + Integer(bbox.at("w")));
                    bottom = (std::max)(bottom, by + Integer(bbox.at("h")));
                }
                if (left < right && top < bottom) {
                    x = left - p.adjust_size_x / 2; y = top - p.adjust_size_y / 2;
                    // Preserve the upstream total-box padding formula, including
                    // its use of the already adjusted origin when computing size.
                    width = right - x + p.adjust_size_x;
                    height = bottom - y + p.adjust_size_y;
                }
            } else if (p.json_bbox_index >= 0 && static_cast<size_t>(p.json_bbox_index) < data.size()) {
                const auto& item = data.at(p.json_bbox_index);
                if (item.contains("bbox")) {
                    const auto& bbox = item.at("bbox");
                    x = static_cast<int64_t>(Integer(bbox.at("x"))) - p.adjust_size_x / 2;
                    y = static_cast<int64_t>(Integer(bbox.at("y"))) - p.adjust_size_y / 2;
                    width = static_cast<int64_t>(Integer(bbox.at("w"))) + p.adjust_size_x;
                    height = static_cast<int64_t>(Integer(bbox.at("h"))) + p.adjust_size_y;
                }
            }
        }
        x = std::clamp<int64_t>(x, 0, image->cols - 1);
        y = std::clamp<int64_t>(y, 0, image->rows - 1);
        width = std::clamp<int64_t>(width, 1, image->cols - x);
        height = std::clamp<int64_t>(height, 1, image->rows - y);
        cv::Mat frame = image->clone();
        const cv::Rect area(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height));
        outputs.SetValue(0, cv::Mat(frame(area)));
        parameters_.x = area.x; parameters_.y = area.y;
        parameters_.width = area.width; parameters_.height = area.height;
        processing_error_.store(false);
    } catch (const std::exception&) {
        // Malformed JSON/coordinates must not unwind through AutoTick.
        processing_error_.store(true);
    }
}
std::string Crop::GetState()
{
    const auto p = GetParameters();
    return nlohmann::json{{"bbox", {{"X", p.x}, {"Y", p.y}, {"W", p.width}, {"H", p.height}}},
        {"calc_total_bbox", p.calc_total_bbox}, {"json_bbox_index", p.json_bbox_index},
        {"adjust_size_x", p.adjust_size_x}, {"adjust_size_y", p.adjust_size_y}}.dump();
}
void Crop::SetState(std::string&& serialized)
{
    const auto state = nlohmann::json::parse(serialized);
    std::lock_guard lock(mutex_);
    auto next = parameters_;
    if (state.contains("bbox")) {
        const auto& bbox = state.at("bbox");
        next.x = Integer(bbox.at("X")); next.y = Integer(bbox.at("Y"));
        next.width = Integer(bbox.at("W")); next.height = Integer(bbox.at("H"));
    }
    if (state.contains("calc_total_bbox")) next.calc_total_bbox = state.at("calc_total_bbox").get<bool>();
    if (state.contains("json_bbox_index")) next.json_bbox_index = Integer(state.at("json_bbox_index"));
    if (state.contains("adjust_size_x")) next.adjust_size_x = Integer(state.at("adjust_size_x"));
    if (state.contains("adjust_size_y")) next.adjust_size_y = Integer(state.at("adjust_size_y"));
    parameters_ = next;
}
}
