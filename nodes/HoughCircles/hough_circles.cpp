#include "hough_circles.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <limits>
#include <optional>

namespace {
using Json = nlohmann::json;
Json Snapshot(const FlowCV::FlowCV_Properties& properties)
{
    Json state = Json::object();
    for (const auto& property : properties.Snapshot().properties)
        std::visit([&](const auto& value) { state[property.key] = value; }, property.value);
    return state;
}
Json PackColors(Json state, std::initializer_list<const char*> colors)
{
    for (const auto* color : colors) for (const auto* channel : {"R", "G", "B"}) {
        const auto key = std::string(color) + "." + channel;
        state[color][channel] = state.at(key);
        state.erase(key);
    }
    return state;
}
Json UnpackColors(const std::string& serialized, std::initializer_list<const char*> colors)
{
    auto state = Json::parse(serialized);
    if (!state.is_object()) throw std::invalid_argument("Hough state must be an object");
    for (const auto* color : colors) for (const auto* channel : {"R", "G", "B"}) {
        const auto key = std::string(color) + "." + channel;
        if (state.contains(color)) state[key] = state.at(color).at(channel);
        if (state.contains(key) && (!state.at(key).is_number()
            || !std::isfinite(state.at(key).get<double>())
            || state.at(key).get<double>() < 0 || state.at(key).get<double>() > 1))
            throw std::invalid_argument("Invalid Hough color");
    }
    return state;
}
cv::Scalar Color(const Json& state, const char* name)
{
    return {state.at(std::string(name) + ".B").get<float>() * 255,
        state.at(std::string(name) + ".G").get<float>() * 255,
        state.at(std::string(name) + ".R").get<float>() * 255};
}
}

namespace DSPatch::DSPatchables {
HCircles::HCircles() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Hough_Circles");
    SetComponentCategory_(Category::Category_Feature_Detection);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"orig", "canny"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(2, {"vis", "circles"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    properties_.AddFloat("dp", "Inv Acc Res", 2, 1, 100, .1f);
    properties_.AddFloat("min_dist", "Min Dist", 150, 1, 2000, 1);
    properties_.AddFloat("param1", "Param 1", 200, 1, 2000, .1f);
    properties_.AddFloat("param2", "Param 2", 100, 1, 2000, .1f);
    properties_.AddInt("min_radius", "Min Radius", 0, 0, 1000, .25f);
    properties_.AddInt("max_radius", "Max Radius", 0, 0, 1000, .25f);
    properties_.AddInt("circle_thickness", "Circle Thickness", 2, 1, 50, .1f);
    properties_.AddBool("fill_circle", "Fill Circle", false);
    properties_.AddBool("show_center", "Show Center", true);
    properties_.AddBool("show_radius", "Show Radius", true);
    properties_.AddInt("text_offset", "Text Offset", 10, 0, 300, .25f);
    properties_.AddFloat("text_scale", "Text Scale", 1, .1f, 20, .01f);
    properties_.AddInt("text_thickness", "Text Thickness", 2, 1, 30, .25f);
    for (const auto* color : {"circle_color", "center_color", "text_color"})
        for (const auto* channel : {"R", "G", "B"}) {
            const bool white = std::string(color) == "text_color";
            const bool red = std::string(color) == "circle_color" && std::string(channel) == "R";
            const bool green = std::string(color) == "center_color" && std::string(channel) == "G";
            properties_.AddFloat(std::string(color) + "." + channel, channel, white || red || green ? 1.f : 0.f, 0, 1, .01f, false);
        }
    SetEnabled(true);
}
std::string HCircles::GetState()
{
    return PackColors(Snapshot(properties_), {"circle_color", "center_color", "text_color"}).dump();
}
void HCircles::SetState(std::string&& serialized)
{
    if (serialized.empty()) return;
    properties_.FromJson(UnpackColors(serialized, {"circle_color", "center_color", "text_color"}));
}
void HCircles::Process_(SignalBus const &inputs, SignalBus &outputs)
{
    processing_error_.store(false);
    const auto state = Snapshot(properties_);
    const auto dp_ = state.at("dp").get<float>();
    const auto min_dist_ = state.at("min_dist").get<float>();
    const auto param1_ = state.at("param1").get<float>();
    const auto param2_ = state.at("param2").get<float>();
    const auto min_radius_ = state.at("min_radius").get<int>();
    const auto max_radius_ = state.at("max_radius").get<int>();
    const auto text_scale_ = state.at("text_scale").get<float>();
    const auto text_thickness_ = state.at("text_thickness").get<int>();
    const auto text_offset_ = state.at("text_offset").get<int>();
    const auto circle_thickness_ = state.at("circle_thickness").get<int>();
    const auto show_radius_ = state.at("show_radius").get<bool>();
    const auto show_center_ = state.at("show_center").get<bool>();
    const auto fill_circle_ = state.at("fill_circle").get<bool>();
    try {
        // Input 1 Handler
        auto in1 = inputs.GetValue<cv::Mat>(1);
        if (!in1) {
            return;
        }
        auto in2 = inputs.GetValue<cv::Mat>(0);

        if (!in1->empty()) {
            if (IsEnabled()) {
                if (in1->dims != 2 || (in2 && !in2->empty() && in2->dims != 2)) {
                    processing_error_.store(true); return;
                }
                cv::Mat cannyImg, visImg;

                in1->copyTo(cannyImg);

                // Process Image
                if (cannyImg.type() == CV_8UC3 && cannyImg.channels() == 3) {
                    cv::cvtColor(cannyImg, cannyImg, cv::COLOR_BGR2GRAY);
                }
                std::vector<cv::Vec3f> circles;
                cv::HoughCircles(cannyImg, circles, cv::HOUGH_GRADIENT, dp_, min_dist_, param1_, param2_, min_radius_, max_radius_);

                if (in2) {
                    if (!in2->empty()) {
                        in2->copyTo(visImg);
                    }
                    else {
                        visImg = cv::Mat(cannyImg.rows, cannyImg.cols, CV_8UC3, cv::Scalar(0, 0, 0));
                    }
                }
                else {
                    visImg = cv::Mat(cannyImg.rows, cannyImg.cols, CV_8UC3, cv::Scalar(0, 0, 0));
                }
                nlohmann::json json_out;
                nlohmann::json jCircles;
                json_out["data_type"] = "circles";
                nlohmann::json ref;
                ref["w"] = cannyImg.cols;
                ref["h"] = cannyImg.rows;
                json_out["ref_frame"] = ref;
                for (const auto &c : circles) {
                    nlohmann::json cTmp;
                    cTmp["x"] = c[0];
                    cTmp["y"] = c[1];
                    cTmp["size"] = c[2] * 2.0f;
                    jCircles.emplace_back(cTmp);
                    cv::Point center(cvRound(c[0]), cvRound(c[1]));
                    int radius = cvRound(c[2]);
                    // draw the circle outline
                    if (fill_circle_)
                        circle(visImg, center, radius, Color(state, "circle_color"), -1, 8, 0);
                    else
                        circle(visImg, center, radius, Color(state, "circle_color"), circle_thickness_, 8, 0);

                    // draw the circle center
                    if (show_center_)
                        circle(visImg, center, 3, Color(state, "center_color"), -1, 8, 0);

                    // draw radius test
                    if (show_radius_) {
                        cv::Point textPos;
                        textPos.x = (center.x - 10) + (int)(sin((float)text_offset_ / 50.0f) * 20.0f);
                        textPos.y = (center.y + 6) - (int)(cos((float)text_offset_ / 50.0f) * 20.0f);
                        cv::putText(visImg, std::to_string(radius), textPos, cv::FONT_HERSHEY_SIMPLEX, text_scale_,
                            Color(state, "text_color"), text_thickness_);
                    }
                }
                if (!visImg.empty()) {
                    outputs.SetValue(0, visImg);
                    json_out["data"] = jCircles;
                    outputs.SetValue(1, json_out);
                }
            }
            else {
                outputs.SetValue(0, *in1);
            }
        }
    } catch (const std::exception&) { processing_error_.store(true); }
}
}
