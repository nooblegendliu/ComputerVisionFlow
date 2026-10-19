#include "perspective_warp.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace DSPatch::DSPatchables {
namespace {
void sortCorners(std::vector<cv::Point2f>& corners, const cv::Point2f& center)
{
    std::vector<cv::Point2f> top, bottom;
    for (const auto& point : corners) {
        if (point.y < center.y) top.push_back(point);
        else bottom.push_back(point);
    }
    corners.clear();
    if (top.size() == 2 && bottom.size() == 2) {
        corners.push_back(top[0].x > top[1].x ? top[1] : top[0]);
        corners.push_back(top[0].x > top[1].x ? top[0] : top[1]);
        corners.push_back(bottom[0].x > bottom[1].x ? bottom[0] : bottom[1]);
        corners.push_back(bottom[0].x > bottom[1].x ? bottom[1] : bottom[0]);
    }
}
}

PerspectiveWarp::PerspectiveWarp() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Perspective_Warp");
    SetComponentCategory_(Category::Category_Transform);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "points"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    SetEnabled(true);
}

PerspectiveWarp::Parameters PerspectiveWarp::GetParameters() const
{
    std::lock_guard<std::mutex> lock(parameters_mutex_);
    return parameters_;
}

void PerspectiveWarp::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    // Publish each tick's result, including malformed/missing JSON, without
    // flashing the UI warning while a valid frame is still being processed.
    struct PublishStatus {
        std::atomic<bool>& target;
        bool valid = false;
        ~PublishStatus() { target.store(valid); }
    } status{is_poly_};
    const auto* image = inputs.GetValue<cv::Mat>(0);
    const auto* points = inputs.GetValue<nlohmann::json>(1);
    if (!image || !points || image->empty() || points->empty()) return;
    if (!IsEnabled()) {
        outputs.SetValue(0, *image);
        return;
    }
    try {
        if (points->contains("data_type") && points->at("data_type").get<std::string>() != "points") return;
        if (!points->contains("data") || points->at("data").size() != 4) return;
        std::vector<cv::Point2f> corners;
        for (const auto& item : points->at("data")) {
            const cv::Point2f point(item.at("x").get<float>(), item.at("y").get<float>());
            if (!std::isfinite(point.x) || !std::isfinite(point.y)) return;
            corners.push_back(point);
        }
        std::vector<cv::Point2f> approx;
        cv::approxPolyDP(corners, approx, cv::arcLength(corners, true) * .02, true);
        if (approx.size() != 4) return;
        cv::Point2f center(0, 0);
        for (const auto& corner : corners) center += corner;
        center *= .25f;
        if (!std::isfinite(center.x) || !std::isfinite(center.y)) return;
        sortCorners(corners, center);
        if (corners.size() != 4) return;
        const auto p = GetParameters();
        int width = p.fixed_width, height = p.fixed_height;
        if (!p.use_fixed_res) {
            const auto length = [](const cv::Point2f& delta) {
                const double value = cv::norm(delta);
                if (!std::isfinite(value) || value > std::numeric_limits<int>::max())
                    throw std::invalid_argument("Perspective edge length exceeds integer range");
                return static_cast<int>(value);
            };
            const int w1 = length(corners[0] - corners[1]), w2 = length(corners[2] - corners[3]);
            const int h1 = length(corners[0] - corners[3]), h2 = length(corners[1] - corners[2]);
            width = static_cast<int>((static_cast<int64_t>(w1) + w2) / 2);
            height = static_cast<int>((static_cast<int64_t>(h1) + h2) / 2);
            if (width == 0) return; // Upstream would divide by zero before clamping.
            float ratio = static_cast<float>(height) / static_cast<float>(width);
            ratio += p.aspect_ratio_adj;
            const float adjustedHeight = static_cast<float>(width) * ratio;
            if (!std::isfinite(adjustedHeight) ||
                static_cast<double>(adjustedHeight) > std::numeric_limits<int>::max() ||
                static_cast<double>(adjustedHeight) < std::numeric_limits<int>::min()) return;
            height = static_cast<int>(adjustedHeight);
        }
        width = std::max(width, 2);
        height = std::max(height, 2);
        // Preserve upstream's edge coordinates (cols/rows, not cols-1/rows-1).
        const std::vector<cv::Point2f> destination{{0, 0}, {static_cast<float>(width), 0},
            {static_cast<float>(width), static_cast<float>(height)}, {0, static_cast<float>(height)}};
        const cv::Mat transform = cv::getPerspectiveTransform(corners, destination);
        cv::Mat frame;
        cv::warpPerspective(*image, frame, transform, cv::Size(width, height), p.interp_mode);
        outputs.SetValue(0, frame);
        status.valid = true;
    }
    catch (const std::exception&) {
        // Invalid JSON/geometry and OpenCV errors must not escape AutoTick.
    }
}

std::string PerspectiveWarp::GetState()
{
    const auto p = GetParameters();
    nlohmann::json state{{"aspect_ratio_adj", p.aspect_ratio_adj}, {"interp_mode", p.interp_mode},
        {"use_fixed_res", p.use_fixed_res}};
    if (p.use_fixed_res) {
        state["fixed_width"] = p.fixed_width;
        state["fixed_height"] = p.fixed_height;
    }
    return state.dump(4);
}

void PerspectiveWarp::SetState(std::string&& serialized)
{
    const auto state = nlohmann::json::parse(serialized);
    std::lock_guard<std::mutex> lock(parameters_mutex_);
    auto next = parameters_;
    if (state.contains("aspect_ratio_adj")) next.aspect_ratio_adj = state.at("aspect_ratio_adj").get<float>();
    if (state.contains("interp_mode")) next.interp_mode = state.at("interp_mode").get<int>();
    if (state.contains("use_fixed_res")) next.use_fixed_res = state.at("use_fixed_res").get<bool>();
    if (state.contains("fixed_width")) next.fixed_width = state.at("fixed_width").get<int>();
    if (state.contains("fixed_height")) next.fixed_height = state.at("fixed_height").get<int>();
    if (!std::isfinite(next.aspect_ratio_adj) || next.interp_mode < 0 || next.interp_mode > 4)
        throw std::invalid_argument("Invalid Perspective_Warp ratio or interpolation");
    parameters_ = next;
}
}
