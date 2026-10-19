#include "transform.hpp"

#include <cmath>
#include <limits>
#include <opencv2/imgproc.hpp>

namespace DSPatch::DSPatchables {
namespace {
struct Parameters {
    int translateX = 0, translateY = 0, flip = 0, rotate = 0, scaleMode = 0, interpolation = 0;
    float angle = 0, scaleX = 100, scaleY = 100;
};
}

Transform::Transform() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Transform");
    SetComponentCategory_(Category::Category_Transform);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddInt("trans_x", "Translate X", 0, -4000, 4000, .5f);
    properties_.AddInt("trans_y", "Translate Y", 0, -4000, 4000, .5f);
    properties_.AddInt("res_x", "Frame Res X", 640, 0, 4000, 1, false);
    properties_.AddInt("res_y", "Frame Res Y", 480, 0, 4000, 1, false);
    properties_.AddFloat("aspect_ratio", "Aspect Ratio", 1.33333f, 0, 100, .1f, false);
    properties_.AddOption("flip_mode", "Flip", 0, {"None", "Horizontal", "Vertical", "Both"});
    properties_.AddOption("rot_mode", "Rotate", 0, {"0 degrees", "90 degrees CW", "90 degrees CCW", "180 degrees", "Free"});
    properties_.AddFloat("angle", "Angle", 0, -180, 180, .01f);
    properties_.AddOption("scale_mode", "Scale Mode", 0, {"Percentage", "Pixels"});
    properties_.AddFloat("scale_x", "Width", 100, 2, 1000, .1f);
    properties_.AddFloat("scale_y", "Height", 100, 2, 1000, .1f);
    properties_.AddOption("interp", "Interpolation", 0,
        {"Nearest", "Bilinear", "BiCubic", "Area", "Lanczos", "Bilinear Exact"});
    properties_.AddOption("aspect_mode", "Aspect Mode", 0, {"Free", "Lock Width", "Lock Height"}, false);
    UpdateMetadataLocked();
    SetEnabled(true);
}

void Transform::UpdateMetadataLocked()
{
    const bool pixels = properties_.GetW<int>("scale_mode") == 1;
    properties_.SetVisibility("angle", properties_.GetW<int>("rot_mode") == 4);
    properties_.SetVisibility("aspect_mode", pixels);
    for (const char* key : {"scale_x", "scale_y"}) {
        properties_.SetMax(key, pixels ? 8000.f : 1000.f);
        properties_.SetStep(key, pixels ? 1.f : .1f);
    }
}

void Transform::RefreshProperties()
{
    std::lock_guard<std::mutex> lock(parameter_mutex_);
    const int mode = properties_.GetW<int>("scale_mode");
    // Changed() is consumed by DSPatch Sync(); track the UI mode independently.
    // SetState sets this marker without applying an interactive mode reset.
    if (mode != ui_scale_mode_) {
        nlohmann::json updates;
        if (mode == 0)
            updates = {{"scale_x", 100.f}, {"scale_y", 100.f}, {"aspect_mode", 0}};
        else
            updates = {{"scale_x", static_cast<float>(properties_.GetW<int>("res_x"))},
                       {"scale_y", static_cast<float>(properties_.GetW<int>("res_y"))}};
        properties_.FromJson(updates);
        ui_scale_mode_ = mode;
    }
    UpdateMetadataLocked();
    // A newly restored node has no reliable input ratio yet. Its saved pixel
    // sizes must survive UI creation before the first frame reaches the node.
    if (!has_frame_metadata_) return;
    const float ratio = properties_.GetW<float>("aspect_ratio");
    if (properties_.GetW<int>("aspect_mode") == 1) {
        const float height = properties_.GetW<float>("scale_x") * ratio;
        if (std::isfinite(height)) properties_.Set("scale_y", height);
    } else if (properties_.GetW<int>("aspect_mode") == 2 && ratio > 0) {
        const float width = properties_.GetW<float>("scale_y") / ratio;
        if (std::isfinite(width)) properties_.Set("scale_x", width);
    }
}

std::string Transform::GetState()
{
    std::lock_guard<std::mutex> lock(parameter_mutex_);
    nlohmann::json state = nlohmann::json::object();
    // Preserve upstream ToJson's visible-fields-only state contract.
    properties_.ToJson(state);
    return state.dump();
}

void Transform::SetState(std::string&& serialized)
{
    if (serialized.empty()) return;
    const auto state = nlohmann::json::parse(serialized);
    std::lock_guard<std::mutex> lock(parameter_mutex_);
    properties_.FromJson(state);
    ui_scale_mode_ = properties_.GetW<int>("scale_mode");
    has_frame_metadata_ = false;
    UpdateMetadataLocked();
}

void Transform::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) {
        outputs.SetValue(0, *input);
        processing_error_.store(false);
        return;
    }
    Parameters parameters;
    {
        std::lock_guard<std::mutex> lock(parameter_mutex_);
        properties_.Sync();
        for (const auto& property : properties_.Snapshot().properties) {
            if (property.key == "trans_x") parameters.translateX = std::get<int>(property.value);
            else if (property.key == "trans_y") parameters.translateY = std::get<int>(property.value);
            else if (property.key == "flip_mode") parameters.flip = std::get<int>(property.value);
            else if (property.key == "rot_mode") parameters.rotate = std::get<int>(property.value);
            else if (property.key == "scale_mode") parameters.scaleMode = std::get<int>(property.value);
            else if (property.key == "interp") parameters.interpolation = std::get<int>(property.value);
            else if (property.key == "angle") parameters.angle = std::get<float>(property.value);
            else if (property.key == "scale_x") parameters.scaleX = std::get<float>(property.value);
            else if (property.key == "scale_y") parameters.scaleY = std::get<float>(property.value);
        }
        properties_.FromJson({{"res_x", input->cols}, {"res_y", input->rows},
            {"aspect_ratio", static_cast<float>(input->rows) / static_cast<float>(input->cols)}});
        has_frame_metadata_ = true;
    }
    try {
        cv::Mat frame = input->clone();
        // Keep the original labels and codes: Horizontal uses flip code 0.
        if (parameters.flip == 1) cv::flip(frame, frame, 0);
        else if (parameters.flip == 2) cv::flip(frame, frame, 1);
        else if (parameters.flip == 3) cv::flip(frame, frame, -1);
        if (parameters.rotate == 1) cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
        else if (parameters.rotate == 2) cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE);
        else if (parameters.rotate == 3) cv::rotate(frame, frame, cv::ROTATE_180);
        else if (parameters.rotate == 4 && parameters.angle != 0) {
            const cv::Point2f center(static_cast<float>(frame.cols - 1) / 2.f,
                static_cast<float>(frame.rows - 1) / 2.f);
            const auto rotation = cv::getRotationMatrix2D(center, parameters.angle, 1);
            cv::warpAffine(frame, frame, rotation, frame.size());
        }
        if (parameters.translateX != 0 || parameters.translateY != 0) {
            const cv::Mat translation = (cv::Mat_<double>(2, 3) << 1, 0,
                static_cast<float>(parameters.translateX), 0, 1, static_cast<float>(parameters.translateY));
            cv::warpAffine(frame, frame, translation, frame.size());
        }
        const bool percentage = parameters.scaleMode == 0;
        const float width = percentage ? static_cast<float>(frame.cols) * (parameters.scaleX / 100.f) : parameters.scaleX;
        const float height = percentage ? static_cast<float>(frame.rows) * (parameters.scaleY / 100.f) : parameters.scaleY;
        const bool applyScale = percentage ? parameters.scaleX != 100.f || parameters.scaleY != 100.f
            : parameters.scaleX != static_cast<float>(frame.cols) || parameters.scaleY != static_cast<float>(frame.rows);
        if (applyScale) {
            // Avoid undefined float-to-int conversion and zero-sized resize.
            if (!std::isfinite(width) || !std::isfinite(height) || width < 1 || height < 1
                || static_cast<double>(width) > (std::numeric_limits<int>::max)()
                || static_cast<double>(height) > (std::numeric_limits<int>::max)()) {
                processing_error_.store(true);
                return;
            }
            constexpr int interpolation[] = {cv::INTER_NEAREST, cv::INTER_LINEAR, cv::INTER_CUBIC,
                cv::INTER_AREA, cv::INTER_LANCZOS4, cv::INTER_LINEAR_EXACT};
            cv::resize(frame, frame, {static_cast<int>(width), static_cast<int>(height)}, 0, 0,
                interpolation[parameters.interpolation]);
        }
        outputs.SetValue(0, frame);
        processing_error_.store(false);
    } catch (const cv::Exception&) {
        processing_error_.store(true);
    }
}
}
