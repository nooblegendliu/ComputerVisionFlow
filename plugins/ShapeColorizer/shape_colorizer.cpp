// Adapted from FlowCV Plugins/ShapeColorizer (GPL-3.0).
#include "shape_colorizer.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace DSPatch::DSPatchables {
namespace {
int Coordinate(const nlohmann::json& value)
{
    if (!value.is_number()) throw std::invalid_argument("Shape coordinate must be numeric");
    const double number = value.get<double>();
    if (!std::isfinite(number) || number < std::numeric_limits<int>::min()
        || number > std::numeric_limits<int>::max()) throw std::invalid_argument("Shape coordinate out of range");
    return static_cast<int>(number);
}
uint32_t ColorPart(double value)
{
    if (!std::isfinite(value) || value < std::numeric_limits<int>::min()
        || value > std::numeric_limits<int>::max())
        throw std::invalid_argument("Shape color out of integer range");
    // Keep upstream's integer truncation and low six hex digits, including
    // non-8-bit input values, without signed shift/overflow undefined behavior.
    return static_cast<uint32_t>(static_cast<int>(value));
}
// Exact ImGui RGB/HSV conversion, with no UI dependency in the computation SDK.
void ColorConvertRGBtoHSV(float r, float g, float b, float& h, float& s, float& v)
{
    float K = 0.0f;
    if (g < b) { std::swap(g,b); K = -1.0f; }
    if (r < g) { std::swap(r,g); K = -2.0f / 6.0f - K; }
    const float chroma = r - (g < b ? g : b);
    h = std::abs(K + (g-b) / (6.0f * chroma + 1e-20f));
    s = chroma / (r + 1e-20f); v = r;
}
void ColorConvertHSVtoRGB(float h, float s, float v, float& r, float& g, float& b)
{
    if (s == 0.0f) { r = g = b = v; return; }
    h = std::fmod(h, 1.0f) / (60.0f / 360.0f);
    const int i = (int)h;
    const float f = h - (float)i, p = v * (1.0f-s), q = v * (1.0f-s*f), t = v*(1.0f-s*(1.0f-f));
    switch(i) {
    case 0: r=v; g=t; b=p; break; case 1: r=q; g=v; b=p; break;
    case 2: r=p; g=v; b=t; break; case 3: r=p; g=q; b=v; break;
    case 4: r=t; g=p; b=v; break; default: r=v; g=p; b=q; break;
    }
}
template<typename I> std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1)
{
    static const char *digits = "0123456789ABCDEF";
    std::string rc(hex_len, '0');
    for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
        rc[i] = digits[(w >> j) & 0x0f];
    return rc;
}

cv::Vec3f ColorToLab(float R, float G, float B)
{
    // http://www.brucelindbloom.com

    double r, g, b, X, Y, Z, fx, fy, fz, xr, yr, zr;
    double Ls, fas, fbs;
    double eps = 216.0f / 24389.0f;
    double k = 24389.0f / 27.0f;

    double Xr = 0.964221f;  // reference white D50
    double Yr = 1.0f;
    double Zr = 0.825211f;

    // RGB to XYZ
    r = R;  // R 0..1
    g = G;  // G 0..1
    b = B;  // B 0..1

    // assuming sRGB (D65)
    if (r <= 0.04045)
        r = r / 12;
    else
        r = (float)pow((r + 0.055) / 1.055, 2.4);

    if (g <= 0.04045)
        g = g / 12;
    else
        g = (float)pow((g + 0.055) / 1.055, 2.4);

    if (b <= 0.04045)
        b = b / 12;
    else
        b = (float)pow((b + 0.055) / 1.055, 2.4);

    X = 0.436052025f * r + 0.385081593f * g + 0.143087414f * b;
    Y = 0.222491598f * r + 0.71688606f * g + 0.060621486f * b;
    Z = 0.013929122f * r + 0.097097002f * g + 0.71418547f * b;

    // XYZ to Lab
    xr = X / Xr;
    yr = Y / Yr;
    zr = Z / Zr;

    if (xr > eps)
        fx = (float)pow(xr, 1 / 3.0);
    else
        fx = (float)((k * xr + 16.0) / 116.0);

    if (yr > eps)
        fy = (float)pow(yr, 1 / 3.0);
    else
        fy = (float)((k * yr + 16.0) / 116.0);

    if (zr > eps)
        fz = (float)pow(zr, 1 / 3.0);
    else
        fz = (float)((k * zr + 16.0) / 116);

    Ls = (116 * fy) - 16;
    fas = 500 * (fx - fy);
    fbs = 200 * (fy - fz);

    cv::Vec3f lab;
    lab[0] = (2.55 * Ls + 0.5);
    lab[1] = (fas + 0.5);
    lab[2] = (fbs + 0.5);

    return lab;
}

}
internal::ShapeColorizerParameters internal::ShapeColorizerState::Snapshot() const
{ std::lock_guard lock(mutex); return parameters; }
void internal::ShapeColorizerState::Apply(const nlohmann::json& patch)
{
    if (!patch.is_object()) throw std::invalid_argument("Shape_Colorizer state must be an object");
    std::lock_guard lock(mutex);
    auto next = parameters;
    if (patch.contains("method")) next.method = Coordinate(patch.at("method"));
    if (patch.contains("threshold")) next.threshold = Coordinate(patch.at("threshold"));
    if (patch.contains("dist_method")) next.dist_method = Coordinate(patch.at("dist_method"));
    if (next.method < 0 || next.method > 2 || next.dist_method < 0 || next.dist_method > 1)
        throw std::invalid_argument("Unknown color/distance method");
    if (patch.contains("overlay")) next.overlay = patch.at("overlay").get<bool>();
    if (patch.contains("show_labels")) next.show_labels = patch.at("show_labels").get<bool>();
    if (patch.contains("ignore_none")) next.ignore_none = patch.at("ignore_none").get<bool>();
    if (patch.contains("color_match")) {
        const auto& list = patch.at("color_match");
        if (!list.is_array() && !list.is_null()) throw std::invalid_argument("Color matches must be an array");
        next.colors.clear();
        for (const auto& value : list) {
            ColorMatch match;
            for (size_t i = 0; i < 3; ++i) {
                match.color[i] = value.at("color").at(std::array<const char*,3>{"R","G","B"}[i]).get<float>();
                if (!std::isfinite(match.color[i]) || match.color[i] < 0 || match.color[i] > 1)
                    throw std::invalid_argument("Color component outside [0,1]");
            }
            // std::string removes upstream's unchecked sprintf into a 32-byte buffer.
            match.label = value.at("label").get<std::string>();
            next.colors.push_back(std::move(match));
        }
        ++next.listRevision;
    }
    parameters = std::move(next);
}
nlohmann::json internal::ShapeColorizerState::Serialize() const
{
    const auto p = Snapshot();
    nlohmann::json result = {{"method",p.method},{"threshold",p.threshold},{"dist_method",p.dist_method},
        {"overlay",p.overlay},{"show_labels",p.show_labels},{"ignore_none",p.ignore_none}};
    for (const auto& c : p.colors)
        result["color_match"].push_back({{"color",{{"R",c.color[0]},{"G",c.color[1]},{"B",c.color[2]}}},{"label",c.label}});
    return result;
}
ShapeColorizer::ShapeColorizer() : FlowCV::Node(ProcessOrder::OutOfOrder),
    state_(std::make_shared<internal::ShapeColorizerState>())
{
    SetComponentName_("Shape_Colorizer");
    SetComponentCategory_(Category::Category_Feature_Detection);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in","shapes"}, {IoType::Io_Type_CvMat,IoType::Io_Type_JSON});
    SetOutputCount_(2, {"out","shapes"}, {IoType::Io_Type_CvMat,IoType::Io_Type_JSON});
    SetEnabled(true);
}
std::string ShapeColorizer::GetState() { return state_->Serialize().dump(4); }
void ShapeColorizer::SetState(std::string&& serialized) { state_->Apply(nlohmann::json::parse(serialized)); }
void ShapeColorizer::Process_(SignalBus const &inputs, SignalBus &outputs)
{
    try {
    const auto settings = state_->Snapshot();
    { std::lock_guard lock(state_->mutex); state_->error.clear(); }
    // Input 1 Handler
    auto shapes1 = inputs.GetValue<nlohmann::json>(1);
    auto in1 = inputs.GetValue<cv::Mat>(0);
    if (!shapes1 || !in1) {
        return;
    }

    nlohmann::json json_data_;
    json_data_ = *shapes1;

    // Process Image
    int w = 640;
    int h = 480;
    if (!json_data_.empty()) {
        if (json_data_.contains("ref_frame")) {
            w = Coordinate(json_data_["ref_frame"]["w"]);
            h = Coordinate(json_data_["ref_frame"]["h"]);
        }
    }

    if (IsEnabled()) {
        if (w <= 0 || h <= 0) return;
        if (in1->empty()) {
            cv::Mat frame(h, w, CV_8UC3, cv::Scalar(0, 0, 0));

            cv::Point pos;
            pos.x = (w - 265) / 2;
            pos.y = h / 2;
            cv::putText(frame, "Need to supply input frame for reference", pos, cv::HersheyFonts::FONT_HERSHEY_SIMPLEX, 0.4f, cv::Scalar(0, 0, 255), 1);
            if (!frame.empty())
                outputs.SetValue(0, frame);
        }
        else {
            cv::Mat frame;
            in1->copyTo(frame);
            cv::Mat matFrame(h, w, CV_8UC3, cv::Scalar(0, 0, 0));

            if (!json_data_.empty()) {

                if (json_data_.contains("data")) {
                    for (auto &d : json_data_["data"]) {
                        if (d.contains("x") && d.contains("y") && d.contains("size")) {
                            cv::Point pos;
                            pos.x = Coordinate(d["x"]);
                            pos.y = Coordinate(d["y"]);
                            int rad = Coordinate(d["size"]) / 2;
                            if (rad <= 0) continue;
                            // Preserve upstream's exclusive right/bottom crop edge, but
                            // skip zero/outside ROIs and bound both image and reference mask.
                            const int right = std::min(frame.cols, matFrame.cols) - 1;
                            const int bottom = std::min(frame.rows, matFrame.rows) - 1;
                            const auto x0 = std::max<int64_t>(0, int64_t(pos.x) - rad);
                            const auto y0 = std::max<int64_t>(0, int64_t(pos.y) - rad);
                            const auto x1 = std::min<int64_t>(right, int64_t(pos.x) + rad);
                            const auto y1 = std::min<int64_t>(bottom, int64_t(pos.y) + rad);
                            if (x1 <= x0 || y1 <= y0) continue;
                            cv::Rect crop(int(x0), int(y0), int(x1-x0), int(y1-y0));
                            cv::circle(matFrame, pos, rad, cv::Scalar(255, 255, 255), -1);
                            cv::Mat mat_8U;
                            cv::cvtColor(matFrame, mat_8U, cv::COLOR_BGR2GRAY);
                            cv::Mat roi = frame(crop);
                            cv::Mat roi_mat = mat_8U(crop);
                            cv::Scalar m = cv::mean(roi, roi_mat);

                            if (settings.method == 1) {
                                cv::Vec3f hsv;
                                cv::Vec3f rgb;
                                ColorConvertRGBtoHSV(m[2] / 255.0f, m[1] / 255.0f, m[0] / 255.0f, hsv[0], hsv[1], hsv[2]);
                                hsv[1] = 1.0f;
                                hsv[2] = 1.0f;
                                ColorConvertHSVtoRGB(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);
                                m[2] = rgb[0] * 255;
                                m[1] = rgb[1] * 255;
                                m[0] = rgb[2] * 255;
                            }
                            else if (settings.method == 2) {
                                bool foundMatch = false;
                                for (auto &c : settings.colors) {
                                    cv::Scalar match(c.color[2] * 255, c.color[1] * 255, c.color[0] * 255);
                                    double distMeasure = 255;

                                    if (settings.dist_method == 0) {
                                        cv::Vec4d diff = match - m;
                                        distMeasure = cv::norm(diff);
                                    }
                                    else if (settings.dist_method == 1) {
                                        cv::Vec3f lab1 = ColorToLab(c.color[0], c.color[1], c.color[2]);
                                        cv::Vec3f lab2 = ColorToLab(m[2] / 255.0f, m[1] / 255.0f, m[0] / 255.0f);
                                        distMeasure = sqrt(pow(lab2[0] - lab1[0], 2) + pow(lab2[1] - lab1[1], 2) + pow(lab2[2] - lab1[2], 2));
                                    }
                                    if (abs(distMeasure) < settings.threshold) {
                                        if (d.contains("dist")) {
                                            float oldDist = d["dist"].get<float>();
                                            if (distMeasure < oldDist) {
                                                m = match;
                                                d["dist"] = (float)distMeasure;
                                                d["class_id"] = c.label;
                                                foundMatch = true;
                                            }
                                        }
                                        else {
                                            m = match;
                                            d["dist"] = distMeasure;
                                            d["class_id"] = c.label;
                                            foundMatch = true;
                                        }
                                    }
                                }
                                if (!foundMatch) {
                                    d["dist"] = -1;
                                    d["class_id"] = "None";
                                }
                            }
                            uint32_t col = (ColorPart(m[2]) << 16) + (ColorPart(m[1]) << 8) + ColorPart(m[0]);
                            std::string hex = "#";
                            hex += n2hexstr<uint32_t>(col, 6);
                            d["color"] = hex;
                            if (settings.overlay) {
                                cv::circle(frame, pos, rad, m, -1);
                                if (settings.show_labels) {
                                    if (settings.method != 2) {
                                        cv::putText(frame, hex, cv::Point(pos.x - rad, pos.y), cv::FONT_HERSHEY_PLAIN, 0.65, cv::Scalar(255, 255, 255), 1.0);
                                    }
                                    else {
                                        if (d.contains("class_id")) {
                                            if (settings.ignore_none) {
                                                if (d["class_id"].get<std::string>() == "None")
                                                    continue;
                                            }
                                            cv::putText(frame, d["class_id"].get<std::string>(), cv::Point(pos.x - (rad * 0.5f), pos.y), cv::FONT_HERSHEY_PLAIN,
                                                0.65, cv::Scalar(255, 255, 255), 1.0);
                                        }
                                    }
                                }
                            }
                            else {
                                cv::circle(matFrame, pos, rad, m, -1);
                                if (settings.show_labels) {
                                    if (settings.method != 2) {
                                        cv::putText(matFrame, hex, cv::Point(pos.x - rad, pos.y), cv::FONT_HERSHEY_PLAIN, 0.65, cv::Scalar(255, 255, 255), 1.0);
                                    }
                                    else {
                                        if (d.contains("class_id")) {
                                            if (settings.ignore_none) {
                                                if (d["class_id"].get<std::string>() == "None")
                                                    continue;
                                            }
                                            cv::putText(matFrame, d["class_id"].get<std::string>(), cv::Point(pos.x - (rad * 0.5f), pos.y),
                                                cv::FONT_HERSHEY_PLAIN, 0.65, cv::Scalar(255, 255, 255), 1.0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                outputs.SetValue(1, json_data_);
            }
            if (settings.overlay) {
                if (!frame.empty())
                    outputs.SetValue(0, frame);
            }
            else {
                if (!matFrame.empty())
                    outputs.SetValue(0, matFrame);
            }
        }
    }
    else {
        outputs.SetValue(0, *in1);
    }
    } catch (const std::exception& error) {
        std::lock_guard lock(state_->mutex);
        state_->error = error.what();
    }
}


#ifndef SHAPE_COLORIZER_WITH_QT
EXPORT_PLUGIN(ShapeColorizer)
#endif
}
