#include "contours.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace DSPatch::DSPatchables {
Contours::Contours() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Contours");
    SetComponentCategory_(Category::Category_Feature_Detection);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"viz", "in"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(2, {"out", "contours"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    SetEnabled(true);
}

Contours::Parameters Contours::GetParameters() const
{
    std::lock_guard<std::mutex> lock(parameters_mutex_);
    return parameters_;
}

void Contours::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(1);
    if (!input || input->empty()) return;
    if (!IsEnabled()) {
        // Upstream bypass is the analysis input, not the optional visualization.
        outputs.SetValue(0, *input);
        return;
    }
    const auto* visualization = inputs.GetValue<cv::Mat>(0);
    const auto p = GetParameters();
    try {
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        if (input->type() == CV_8UC1)
            cv::findContours(*input, contours, hierarchy, p.mode, p.method + 1);
        cv::Mat frame;
        if (visualization) visualization->copyTo(frame);
        else frame = cv::Mat(input->rows, input->cols, CV_8UC3, cv::Scalar(0, 0, 0));

        nlohmann::json jsonOut;
        nlohmann::json jsonContours;
        jsonOut["data_type"] = "contours";
        jsonOut["ref_frame"] = {{"w", frame.cols}, {"h", frame.rows}};
        const int thickness = p.fill_contours ? -1 : 2;
        if (!hierarchy.empty()) {
            // Preserve FlowCV's top-level sibling traversal (including its
            // hierarchy-dependent drawing), strict area limits and null data.
            for (int index = 0; index >= 0; index = hierarchy[index][0]) {
                if (static_cast<std::size_t>(index) >= contours.size()) continue;
                const float area = static_cast<float>(cv::contourArea(contours[index]));
                if (p.filter_by_area && !(area > p.area_min && area < p.area_max)) continue;
                nlohmann::json contour;
                if (p.draw_contours) {
                    cv::drawContours(frame, contours, index,
                        cv::Scalar(p.contour_color[2] * 255, p.contour_color[1] * 255, p.contour_color[0] * 255),
                        thickness, cv::LINE_8, hierarchy);
                    nlohmann::json points;
                    for (const auto& point : contours[index])
                        points.push_back({{"x", point.x}, {"y", point.y}});
                    contour["contour"] = points;
                }
                if (p.draw_bbox) {
                    const cv::Rect box = cv::boundingRect(contours[index]);
                    cv::rectangle(frame, box,
                        cv::Scalar(p.bbox_color[2] * 255, p.bbox_color[1] * 255, p.bbox_color[0] * 255), 2);
                    contour["bbox"] = {{"x", box.x}, {"y", box.y}, {"w", box.width}, {"h", box.height}};
                }
                jsonContours.push_back(contour);
            }
        }
        if (!frame.empty()) {
            outputs.SetValue(0, frame);
            jsonOut["data"] = jsonContours;
            outputs.SetValue(1, jsonOut);
        }
    }
    catch (const std::exception& error) {
        // Include findContours in the original exception boundary: malformed
        // input must not unwind through DSPatch's processing thread.
        std::cout << error.what() << std::endl;
    }
}

std::string Contours::GetState()
{
    const auto p = GetParameters();
    nlohmann::json state = {
        {"mode", p.mode}, {"method", p.method}, {"filter_by_area", p.filter_by_area},
        {"area_min", p.area_min}, {"area_max", p.area_max}, {"draw_contours", p.draw_contours},
        {"fill_contours", p.fill_contours}, {"draw_bbox", p.draw_bbox},
        {"contour_color", {{"R", p.contour_color[0]}, {"G", p.contour_color[1]}, {"B", p.contour_color[2]}}},
        {"bbox_color", {{"R", p.bbox_color[0]}, {"G", p.bbox_color[1]}, {"B", p.bbox_color[2]}}}
    };
    return state.dump(4);
}

void Contours::SetState(std::string&& serialized)
{
    const auto state = nlohmann::json::parse(serialized);
    std::lock_guard<std::mutex> lock(parameters_mutex_);
    auto next = parameters_;
    if (state.contains("mode")) next.mode = state.at("mode").get<int>();
    if (state.contains("method")) next.method = state.at("method").get<int>();
    if (state.contains("filter_by_area")) next.filter_by_area = state.at("filter_by_area").get<bool>();
    if (state.contains("area_min")) next.area_min = state.at("area_min").get<float>();
    if (state.contains("area_max")) next.area_max = state.at("area_max").get<float>();
    if (state.contains("draw_contours")) next.draw_contours = state.at("draw_contours").get<bool>();
    if (state.contains("fill_contours")) next.fill_contours = state.at("fill_contours").get<bool>();
    if (state.contains("draw_bbox")) next.draw_bbox = state.at("draw_bbox").get<bool>();
    for (const auto* key : {"contour_color", "bbox_color"}) {
        if (!state.contains(key)) continue;
        auto& color = std::string(key) == "contour_color" ? next.contour_color : next.bbox_color;
        color = {state.at(key).at("R").get<float>(), state.at(key).at("G").get<float>(), state.at(key).at("B").get<float>()};
    }
    if (next.mode < 0 || next.mode > 3 || next.method < 0 || next.method > 3 ||
        !std::isfinite(next.area_min) || !std::isfinite(next.area_max))
        throw std::invalid_argument("Invalid Contours mode, method or area");
    for (const auto& color : {next.contour_color, next.bbox_color})
        for (float component : color)
            if (!std::isfinite(component)) throw std::invalid_argument("Invalid Contours color");
    parameters_ = next;
}
}
