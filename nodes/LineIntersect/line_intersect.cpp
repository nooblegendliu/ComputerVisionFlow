#include "line_intersect.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace DSPatch::DSPatchables {
namespace {
float Finite(const nlohmann::json& value)
{
    const float result = value.get<float>();
    if (!std::isfinite(result)) throw std::invalid_argument("Line_Intersections requires finite numbers");
    return result;
}
int Integer(const nlohmann::json& value)
{
    const double number = value.get<double>();
    if (!std::isfinite(number) || number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max())
        throw std::invalid_argument("Line_Intersections integer is out of range");
    return static_cast<int>(number);
}
float Difference(int a, int b) { return static_cast<float>(double(a) - double(b)); }
cv::Point2f ComputeIntersect(const cv::Vec4i& a, const cv::Vec4i& b)
{
    // Keep upstream float arithmetic; widen only integer subtraction to avoid overflow.
    const float a1 = Difference(a[3], a[1]), b1 = Difference(a[0], a[2]);
    const float c1 = a1 * a[0] + b1 * a[1];
    const float a2 = Difference(b[3], b[1]), b2 = Difference(b[0], b[2]);
    const float c2 = a2 * b[0] + b2 * b[1];
    const float determinant = a1 * b2 - a2 * b1;
    if (determinant == 0) return {-1, -1};
    return {(b2 * c1 - b1 * c2) / determinant, (a1 * c2 - a2 * c1) / determinant};
}
}
LineIntersect::LineIntersect() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Line_Intersections");
    SetComponentCategory_(Category::Category_Feature_Detection);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "lines"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    SetOutputCount_(2, {"out", "points"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    SetEnabled(true);
}
std::string LineIntersect::GetState()
{
    std::lock_guard lock(mutex_);
    const auto& p = parameters_;
    return nlohmann::json{{"min_angle", p.min_angle}, {"min_dist", p.min_dist}, {"draw_points", p.draw_points},
        {"point_radius", p.point_radius}, {"point_thickness", p.point_thickness}, {"point_solid", p.point_solid},
        {"point_color", {{"R", p.point_color[0]}, {"G", p.point_color[1]}, {"B", p.point_color[2]}}}}.dump();
}
void LineIntersect::SetState(std::string&& serialized)
{
    const auto state = nlohmann::json::parse(serialized);
    if (!state.is_object()) throw std::invalid_argument("Line_Intersections state must be an object");
    std::lock_guard lock(mutex_);
    auto p = parameters_;
    if (state.contains("min_angle")) p.min_angle = Finite(state.at("min_angle"));
    if (state.contains("min_dist")) p.min_dist = Finite(state.at("min_dist"));
    if (state.contains("draw_points")) p.draw_points = state.at("draw_points").get<bool>();
    if (state.contains("point_solid")) p.point_solid = state.at("point_solid").get<bool>();
    if (state.contains("point_radius")) p.point_radius = Integer(state.at("point_radius"));
    if (state.contains("point_thickness")) p.point_thickness = Integer(state.at("point_thickness"));
    if (state.contains("point_color")) {
        int i = 0;
        for (const auto* channel : {"R", "G", "B"}) p.point_color[i++] = Finite(state.at("point_color").at(channel));
    }
    if (p.point_radius < 0 || p.point_thickness > 32767)
        throw std::invalid_argument("Line_Intersections invalid drawing dimensions");
    parameters_ = p;
}
void LineIntersect::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    line_count_ = 0; intersect_count_ = 0; processing_error_ = false;
    const auto* input = inputs.GetValue<nlohmann::json>(1);
    if (!input) return;
    Parameters p;
    { std::lock_guard lock(mutex_); p = parameters_; }
    try {
        cv::Mat image;
        if (const auto* frame = inputs.GetValue<cv::Mat>(0); frame && !frame->empty()) frame->copyTo(image);
        if (!input->empty()) {
            nlohmann::json points;
            if (!IsEnabled()) outputs.SetValue(1, points);
            else {
                if (input->contains("data_type") && input->at("data_type").get<std::string>() != "lines") return;
                if (input->contains("data")) {
                    const auto& data = input->at("data");
                    const size_t lineCount = data.size();
                    line_count_ = static_cast<int>(std::min<size_t>(lineCount, std::numeric_limits<int>::max()));
                    if (lineCount > 200) return;
                    if (!data.is_array() && !data.is_null())
                        throw std::invalid_argument("Line_Intersections data must be an array or null");
                    std::vector<cv::Vec4i> lines;
                    for (const auto& entry : data) {
                        const auto& line = entry.at("line");
                        lines.emplace_back(Integer(line.at("x0")), Integer(line.at("y0")), Integer(line.at("x1")), Integer(line.at("y1")));
                    }
                    std::vector<cv::Point2f> intersections;
                    for (size_t i = 0; i < lines.size(); ++i) for (size_t j = i + 1; j < lines.size(); ++j) {
                        const auto point = ComputeIntersect(lines[i], lines[j]);
                        if (!std::isfinite(point.x) || !std::isfinite(point.y) || point.x < 0 || point.y < 0) continue;
                        cv::Vec2f v1(Difference(lines[i][0], lines[i][2]), Difference(lines[i][1], lines[i][3]));
                        cv::Vec2f v2(Difference(lines[j][0], lines[j][2]), Difference(lines[j][1], lines[j][3]));
                        v1 = cv::normalize(v1); v2 = cv::normalize(v2);
                        // Rounding can push normalized dot products just outside acos's domain.
                        const float angle = acos(std::clamp(v1.dot(v2), -1.f, 1.f)) * 57.2957795130823208f;
                        if (angle <= p.min_angle) continue;
                        float closest = 9999999.f;
                        for (const auto& existing : intersections) {
                            const float distance = static_cast<float>(cv::norm(point - existing));
                            if (distance < p.min_dist) closest = distance;
                        }
                        if (closest <= p.min_dist) continue;
                        intersections.push_back(point);
                        // JSON retains all positive intersections, including those outside the image.
                        // Skip only drawing coordinates that cannot be represented safely by cv::circle.
                        const double limit = double(std::numeric_limits<int>::max()) - p.point_radius - 1;
                        if (!image.empty() && p.draw_points && point.x <= limit && point.y <= limit)
                            cv::circle(image, point, p.point_radius,
                                cv::Scalar(p.point_color[2] * 255, p.point_color[1] * 255, p.point_color[0] * 255),
                                p.point_solid ? -1 : p.point_thickness);
                    }
                    intersect_count_ = static_cast<int>(intersections.size());
                    if (!intersections.empty()) {
                        points["data"] = nlohmann::json::array();
                        for (const auto& point : intersections) points["data"].push_back({{"x", point.x}, {"y", point.y}});
                        points["data_type"] = "points";
                        points["ref_frame"] = input->value("ref_frame", nlohmann::json{});
                    }
                }
                if (!points.empty()) outputs.SetValue(1, points);
            }
        }
        if (!image.empty()) outputs.SetValue(0, image);
    }
    catch (const std::exception&) { processing_error_ = true; intersect_count_ = 0; }
}
}
