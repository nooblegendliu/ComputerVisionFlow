#include "hough_lines.hpp"
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
struct Line
{
    cv::Point start;
    cv::Point end;
};

static std::optional<cv::Point> GetLineIntersection(const Line &l1, const Line &l2)
{
    cv::Point pInter = {0, 0};

    double a1 = double(l1.end.y) - l1.start.y;
    double b1 = double(l1.start.x) - l1.end.x;
    double c1 = a1 * (l1.start.x) + b1 * (l1.start.y);

    double a2 = double(l2.end.y) - l2.start.y;
    double b2 = double(l2.start.x) - l2.end.x;
    double c2 = a2 * (l2.start.x) + b2 * (l2.start.y);

    double determinant = a1 * b2 - a2 * b1;

    if (determinant != 0) {
        const double x = (b2 * c1 - b1 * c2) / determinant;
        const double y = (a1 * c2 - a2 * c1) / determinant;
        if (!std::isfinite(x) || !std::isfinite(y)
            || x < (std::numeric_limits<int>::min)() || x > (std::numeric_limits<int>::max)()
            || y < (std::numeric_limits<int>::min)() || y > (std::numeric_limits<int>::max)()) return std::nullopt;
        pInter.x = static_cast<int>(x);
        pInter.y = static_cast<int>(y);
        return pInter;
    }

    return std::nullopt;
}

static std::optional<Line> GetInsideLine(const Line &l1, int width, int height)
{
    Line innerLine;

    // Define Image Border Lines
    Line bLeft = {cv::Point(0, 0), cv::Point(0, height)};
    Line bTop = {cv::Point(0, 0), cv::Point(width, 0)};
    Line bBot = {cv::Point(0, height), cv::Point(width, height)};
    Line bRight = {cv::Point(width, 0), cv::Point(width, height)};

    bool foundStart = false;

    // Left
    if (auto pt = GetLineIntersection(l1, bLeft)) {
        if (pt->y >= 0 && pt->y <= height) {
            innerLine.start.x = pt->x;
            innerLine.start.y = pt->y;
            foundStart = true;
        }
    }

    // Top
    if (auto pt = GetLineIntersection(l1, bTop)) {
        if (pt->x >= 0 && pt->x <= width) {
            if (!foundStart) {
                innerLine.start.x = pt->x;
                innerLine.start.y = pt->y;
                foundStart = true;
            }
            else {
                innerLine.end.x = pt->x;
                innerLine.end.y = pt->y;
                return innerLine;
            }
        }
    }

    // Bottom
    if (auto pt = GetLineIntersection(l1, bBot)) {
        if (pt->x >= 0 && pt->x <= width) {
            if (!foundStart) {
                innerLine.start.x = pt->x;
                innerLine.start.y = pt->y;
                foundStart = true;
            }
            else {
                innerLine.end.x = pt->x;
                innerLine.end.y = pt->y;
                return innerLine;
            }
        }
    }

    // Right
    if (foundStart) {
        if (auto pt = GetLineIntersection(l1, bRight)) {
            if (pt->y >= 0 && pt->y <= height) {
                innerLine.end.x = pt->x;
                innerLine.end.y = pt->y;
                return innerLine;
            }
        }
    }

    return std::nullopt;
}


namespace DSPatch::DSPatchables {
HoughLines::HoughLines() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Hough_Lines");
    SetComponentCategory_(Category::Category_Feature_Detection);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(2, {"out", "lines"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    properties_.AddOption("hough_mode", "Hough Line Mode", 0, {"Standard", "Probabilistic"});
    properties_.AddFloat("rho", "Accum Dist Res", 1, .01f, 10, .01f);
    properties_.AddInt("thresh", "Threshold", 150, 1, 1000, .1f);
    properties_.AddFloat("min_theta", "Min Theta", 0, 0, 360, .1f);
    properties_.AddFloat("max_theta", "Max Theta", 360, 0, 360, .1f);
    properties_.AddFloat("min_line_len", "Min Line Length", 100, 1, 1000, .1f);
    properties_.AddFloat("max_line_gap", "Max Line Gap", 10, 1, 1000, .1f);
    properties_.AddBool("draw_lines", "Draw Lines", false);
    properties_.AddInt("line_thickness", "Line Thickness", 3, 1, 25, .1f);
    for (const auto* channel : {"R", "G", "B"})
        properties_.AddFloat(std::string("line_color.") + channel, channel, std::string(channel) == "R" ? 1.f : 0.f, 0, 1, .01f, false);
    RefreshProperties();
    SetEnabled(true);
}
void HoughLines::RefreshProperties()
{
    const auto state = Snapshot(properties_);
    const bool standard = state.at("hough_mode").get<int>() == 0;
    properties_.SetVisibility("min_theta", standard);
    properties_.SetVisibility("max_theta", standard);
    properties_.SetVisibility("min_line_len", !standard);
    properties_.SetVisibility("max_line_gap", !standard);
    properties_.SetVisibility("line_thickness", state.at("draw_lines").get<bool>());
}
std::string HoughLines::GetState()
{
    return PackColors(Snapshot(properties_), {"line_color"}).dump();
}
void HoughLines::SetState(std::string&& serialized)
{
    if (serialized.empty()) return;
    properties_.FromJson(UnpackColors(serialized, {"line_color"}));
    RefreshProperties();
}
void HoughLines::Process_(SignalBus const &inputs, SignalBus &outputs)
{
    processing_error_.store(false);
    const auto state = Snapshot(properties_);
    const auto draw_lines_ = state.at("draw_lines").get<bool>();
    const auto hough_mode_ = state.at("hough_mode").get<int>();
    const auto rho_ = state.at("rho").get<float>();
    const auto thresh_ = state.at("thresh").get<int>();
    const auto min_theta_ = state.at("min_theta").get<float>();
    const auto max_theta_ = state.at("max_theta").get<float>();
    const auto min_line_len_ = state.at("min_line_len").get<float>();
    const auto max_line_gap_ = state.at("max_line_gap").get<float>();
    const auto line_thickness_ = state.at("line_thickness").get<int>();
    try {
        static const float DEG_TO_RAD = CV_PI / 180;

        // Input 1 Handler
        auto in1 = inputs.GetValue<cv::Mat>(0);
        if (!in1) {
            return;
        }

        if (!in1->empty()) {
            if (IsEnabled()) {
                if (in1->dims != 2 || in1->type() != CV_8UC1) { processing_error_.store(true); return; }
                {
                    cv::Mat frame, outFrame;
                    in1->copyTo(frame);
                    std::vector<cv::Vec2f> found_lines;
                    std::vector<cv::Vec4i> found_p_lines;

                    if (hough_mode_ == 0)
                        cv::HoughLines(frame, found_lines, rho_, DEG_TO_RAD, thresh_, 0, 0, min_theta_ * DEG_TO_RAD, max_theta_ * DEG_TO_RAD);
                    else if (hough_mode_ == 1)
                        cv::HoughLinesP(frame, found_p_lines, rho_, DEG_TO_RAD, thresh_, min_line_len_, max_line_gap_);

                    nlohmann::json json_out;
                    nlohmann::json jLines;
                    json_out["data_type"] = "lines";
                    nlohmann::json ref;
                    ref["w"] = frame.cols;
                    ref["h"] = frame.rows;
                    json_out["ref_frame"] = ref;

                    cv::cvtColor(frame, outFrame, cv::COLOR_GRAY2BGR);
                    if (hough_mode_ == 0) {
                        for (auto &found_line : found_lines) {
                            float rho = found_line[0], theta = found_line[1];
                            double a = cos(theta), b = sin(theta);
                            double x0 = a * rho, y0 = b * rho;
                            float w2 = (float)outFrame.cols * 2.0f;
                            Line l1;
                            l1.start.x = cvRound(x0 + w2 * (-b));
                            l1.start.y = cvRound(y0 + w2 * (a));
                            l1.end.x = cvRound(x0 - w2 * (-b));
                            l1.end.y = cvRound(y0 - w2 * (a));
                            if (auto lInt = GetInsideLine(l1, outFrame.cols, outFrame.rows)) {
                                nlohmann::json cTmp;
                                nlohmann::json jLine;
                                cTmp["x0"] = lInt->start.x;
                                cTmp["y0"] = lInt->start.y;
                                cTmp["x1"] = lInt->end.x;
                                cTmp["y1"] = lInt->end.y;
                                jLine["line"] = cTmp;
                                jLines.emplace_back(jLine);
                                if (draw_lines_)
                                    line(outFrame, lInt->start, lInt->end, Color(state, "line_color"),
                                        line_thickness_, cv::LINE_AA);
                            }
                        }
                    }
                    else if (hough_mode_ == 1) {
                        for (auto &found_p_line : found_p_lines) {
                            nlohmann::json cTmp;
                            nlohmann::json jLine;
                            cTmp["x0"] = found_p_line[0];
                            cTmp["y0"] = found_p_line[1];
                            cTmp["x1"] = found_p_line[2];
                            cTmp["y1"] = found_p_line[3];
                            jLine["line"] = cTmp;
                            jLines.emplace_back(jLine);
                            if (draw_lines_) {
                                line(outFrame, cv::Point(found_p_line[0], found_p_line[1]), cv::Point(found_p_line[2], found_p_line[3]),
                                    Color(state, "line_color"), line_thickness_, cv::LINE_AA);
                            }
                        }
                    }
                    if (!outFrame.empty()) {
                        outputs.SetValue(0, outFrame);
                        json_out["data"] = jLines;
                        outputs.SetValue(1, json_out);
                    }
                }
            }
            else {
                outputs.SetValue(0, *in1);
            }
        }
    } catch (const std::exception&) { processing_error_.store(true); }
}
}
