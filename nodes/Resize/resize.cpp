#include "resize.hpp"

#include <vector>

#include <nlohmann/json.hpp>
#include <opencv2/imgproc.hpp>

namespace DSPatch::DSPatchables
{
    Resize::Resize()
        : FlowCV::Node(DSPatch::Component::ProcessOrder::OutOfOrder)
    {
        SetComponentName_("Resize");
        SetComponentCategory_(DSPatch::Category::Category_Transform);
        SetComponentAuthor_("Richard");
        SetComponentVersion_("0.1.0");

        // Original FlowCV has three input ports; preserve their order and types.
        SetInputCount_(3, { "in", "ref", "size" },
            { DSPatch::IoType::Io_Type_CvMat,
             DSPatch::IoType::Io_Type_CvMat,
             DSPatch::IoType::Io_Type_Int_Array });
        SetOutputCount_(1, { "out" }, { DSPatch::IoType::Io_Type_CvMat });

        properties_.AddInt("width", "Width", 512, 1, 10000, 0.5f);
        properties_.AddInt("height", "Height", 512, 1, 10000, 0.5f);
        properties_.AddOption("interp_mode", "Interpolation", 2,
            { "Nearest", "Bilinear", "BiCubic", "Area", "Lanczos", "Bilinear Exact" });

        RefreshProperties();
        SetEnabled(true);
    }

    void Resize::RefreshProperties()
    {
        // In the original UI, width/height controls are hidden while a
        // valid reference frame or size-array input supplies dimensions.
        const bool useInputSize = has_ref_frame_.load() || has_size_input_.load();
        properties_.SetVisibility("width", !useInputSize);
        properties_.SetVisibility("height", !useInputSize);
    }

    void Resize::Process_(SignalBus const& inputs, SignalBus& outputs)
    {
        const auto* in = inputs.GetValue<cv::Mat>(0);
        const auto* ref = inputs.GetValue<cv::Mat>(1);
        const auto* size = inputs.GetValue<std::vector<int>>(2);

        const bool hasRef = ref && !ref->empty();
        const bool hasSize = size && size->size() == 2;
        has_ref_frame_.store(hasRef);
        has_size_input_.store(hasSize);

        if (!in || in->empty()) {
            return;
        }
        if (!IsEnabled()) {
            outputs.SetValue(0, *in);
            return;
        }

        // Like the original, reference-image dimensions take precedence over
        // the size-array input. Store selected dimensions as editable state.
        if (hasRef) {
            properties_.Set("width", ref->cols);
            properties_.Set("height", ref->rows);
        }
        else if (hasSize) {
            properties_.Set("width", size->at(0));
            properties_.Set("height", size->at(1));
        }

        properties_.Sync();

        const int width = properties_.Get<int>("width");
        const int height = properties_.Get<int>("height");
        const int mode = properties_.Get<int>("interp_mode");

        // Invalid external size input must not reach cv::resize(), which
        // otherwise reports an OpenCV assertion failure.
        if (width <= 0 || height <= 0) {
            return;
        }

        cv::Mat frame;
        switch (mode) {
        case 0:
            cv::resize(*in, frame, cv::Size(width, height), 0, 0, cv::INTER_NEAREST);
            break;
        case 1:
            cv::resize(*in, frame, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
            break;
        case 2:
            cv::resize(*in, frame, cv::Size(width, height), 0, 0, cv::INTER_CUBIC);
            break;
        case 3:
            cv::resize(*in, frame, cv::Size(width, height), 0, 0, cv::INTER_AREA);
            break;
        case 4:
            cv::resize(*in, frame, cv::Size(width, height), 0, 0, cv::INTER_LANCZOS4);
            break;
        case 5:
            cv::resize(*in, frame, cv::Size(width, height), 0, 0, cv::INTER_LINEAR_EXACT);
            break;
        default:
            return;
        }

        if (!frame.empty()) {
            outputs.SetValue(0, frame);
        }
    }

    std::string Resize::GetState()
    {
        nlohmann::json state = {
            {"width", properties_.GetW<int>("width")},
            {"height", properties_.GetW<int>("height")},
            {"mode", properties_.GetW<int>("interp_mode")}
        };
        return state.dump(4);
    }

    void Resize::SetState(std::string&& serialized)
    {
        if (serialized.empty()) {
            return;
        }

        const auto state = nlohmann::json::parse(serialized);
        if (!state.is_object()) {
            return;
        }

        nlohmann::json updates = nlohmann::json::object();
        if (state.contains("width")) {
            updates["width"] = state.at("width");
        }
        if (state.contains("height")) {
            updates["height"] = state.at("height");
        }
        if (state.contains("mode")) {
            updates["interp_mode"] = state.at("mode");
        }

        properties_.FromJson(updates);
        RefreshProperties();
    }
}
