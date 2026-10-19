#include "ColorCorrect.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace DSPatch::DSPatchables {
Colorcorrect::Colorcorrect() : FlowCV::Node(ProcessOrder::OutOfOrder), lut_(MakeLut(1))
{
    SetComponentName_("Color_Correct");
    SetComponentCategory_(Category::Category_Color);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "mask"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    SetEnabled(true);
}

std::array<unsigned char, 256> Colorcorrect::MakeLut(float gamma)
{
    std::array<unsigned char, 256> result;
    for (int i = 0; i < 256; ++i)
        // Gamma zero is allowed by upstream Controls: its limiting LUT is 0 except at 255.
        result[i] = gamma == 0 ? (i == 255 ? 255 : 0)
            : cv::saturate_cast<uchar>(pow(float(i) / 255.f, 1.f / gamma) * 255.f);
    return result;
}

void Colorcorrect::Reset()
{
    std::lock_guard lock(mutex_);
    parameters_ = Parameters{};
    lut_ = MakeLut(parameters_.gamma);
}

std::string Colorcorrect::GetState()
{
    std::lock_guard lock(mutex_);
    const auto& p = parameters_;
    return nlohmann::json{{"brightness", p.brightness}, {"contrast", p.contrast},
        {"gamma", p.gamma}, {"gain", p.gain}, {"saturation", p.saturation},
        {"low_color", {{"R", p.low[0]}, {"G", p.low[1]}, {"B", p.low[2]}}},
        {"high_color", {{"R", p.high[0]}, {"G", p.high[1]}, {"B", p.high[2]}}}}.dump();
}

void Colorcorrect::SetState(std::string&& serialized)
{
    const auto state = nlohmann::json::parse(serialized);
    if (!state.is_object()) throw std::invalid_argument("Color_Correct state must be an object");
    std::lock_guard lock(mutex_);
    auto p = parameters_;
    const auto read = [](const nlohmann::json& value) {
        const auto number = value.get<float>();
        if (!std::isfinite(number)) throw std::invalid_argument("Color_Correct requires finite parameters");
        return number;
    };
    if (state.contains("brightness")) p.brightness = read(state.at("brightness"));
    if (state.contains("contrast")) p.contrast = read(state.at("contrast"));
    if (state.contains("gamma")) p.gamma = read(state.at("gamma"));
    if (state.contains("gain")) p.gain = read(state.at("gain"));
    if (state.contains("saturation")) p.saturation = read(state.at("saturation"));
    const std::array<const char*, 3> channels{"R", "G", "B"};
    for (int i = 0; i < 3; ++i) {
        if (state.contains("low_color")) p.low[i] = read(state.at("low_color").at(channels[i]));
        if (state.contains("high_color")) p.high[i] = read(state.at("high_color").at(channels[i]));
    }
    const auto lut = MakeLut(p.gamma);
    parameters_ = p;
    lut_ = lut;
}

void Colorcorrect::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    processing_error_.store(false);
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled() || input->channels() < 3) { outputs.SetValue(0, *input); return; }
    Parameters p;
    std::array<unsigned char, 256> lut;
    { std::lock_guard lock(mutex_); p = parameters_; lut = lut_; }
    try {
        // Upstream's pixel loop only initializes Vec3b/Vec4b; reject other color formats.
        if (input->type() != CV_8UC3 && input->type() != CV_8UC4)
            throw std::invalid_argument("Color_Correct requires 8-bit BGR/BGRA");
        const auto* inputMask = inputs.GetValue<cv::Mat>(1);
        cv::Mat mask;
        if (!inputMask) mask = cv::Mat(input->size(), CV_8UC1, cv::Scalar(255));
        else {
            if (inputMask->empty() || inputMask->type() != CV_8UC1)
                throw std::invalid_argument("Color_Correct mask must be 8-bit single-channel");
            if (inputMask->size() != input->size()) cv::resize(*inputMask, mask, input->size());
            else inputMask->copyTo(mask);
        }
        cv::Mat result(input->size(), input->type());
        const int channels = input->channels();
        for (int y = 0; y < input->rows; ++y) {
            const auto* src = input->ptr<uchar>(y);
            auto* dst = result.ptr<uchar>(y);
            for (int x = 0; x < input->cols; ++x) {
                const auto* pixel = src + x * channels;
                const float lum = (float(pixel[0] + pixel[1] + pixel[2]) / 3.f) / 255.f;
                for (int c = 0; c < 3; ++c) {
                    float val = float(pixel[c]) / 255.f;
                    if (mask.at<uchar>(y, x) == 255 && val >= p.low[2-c] && val <= p.high[2-c]) {
                        if (p.saturation != 1) val = (1.f - p.saturation) * lum + p.saturation * val;
                        if (p.gamma != 1) val = float(lut[cv::saturate_cast<uchar>(val * 255.f)]) / 255.f;
                        if (p.contrast != 1) val = p.contrast * (val - .5f) + .5f;
                        if (p.gain != 1) val *= p.gain;
                        if (p.brightness != 0) val += p.brightness;
                    }
                    dst[x * channels + c] = cv::saturate_cast<uchar>(val * 255.f);
                }
                if (channels == 4) dst[x * channels + 3] = pixel[3];
            }
        }
        outputs.SetValue(0, result);
    } catch (const std::exception&) { processing_error_.store(true); }
}
}
