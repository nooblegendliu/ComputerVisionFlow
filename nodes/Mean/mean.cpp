#include "mean.hpp"
#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables {
Mean::Mean() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Mean");
    SetComponentCategory_(Category::Category_Utility);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"json"}, {IoType::Io_Type_JSON});
    properties_.AddBool("calc_mean", "Calc Mean", true);
    properties_.AddBool("calc_std_dev", "Calc Std Dev", false);
    properties_.AddFloat("scale", "Scale", 1.0f, .001f, 10000.0f, .001f);
    SetEnabled(true);
}

void Mean::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty() || !IsEnabled()) return;
    properties_.Sync();
    bool calcMean = true, calcStdDev = false;
    float scale = 1.0f;
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "calc_mean") calcMean = std::get<bool>(property.value);
        if (property.key == "calc_std_dev") calcStdDev = std::get<bool>(property.value);
        if (property.key == "scale") scale = std::get<float>(property.value);
    }
    try {
        nlohmann::json result, data;
        result["data_type"] = "mean";
        result["ref_frame"] = {{"w", input->cols}, {"h", input->rows}};
        if (calcStdDev) {
            cv::Mat mean, stdDev;
            cv::meanStdDev(*input, mean, stdDev);
            mean *= scale;
            stdDev *= scale;
            // Preserve upstream float rounding in the meanStdDev branch.
            for (int i = 0; i < stdDev.rows; ++i)
                data["std_dev"].emplace_back(static_cast<float>(stdDev.at<double>(i, 0)));
            if (calcMean)
                for (int i = 0; i < mean.rows; ++i)
                    data["mean"].emplace_back(static_cast<float>(mean.at<double>(i, 0)));
        }
        if (calcMean && !calcStdDev) {
            cv::Scalar mean = cv::mean(*input);
            mean *= scale;
            // This branch deliberately retains the original double values.
            for (int i = 0; i < input->channels(); ++i)
                data["mean"].emplace_back(mean.val[i]);
        }
        data["scale"] = scale;
        result["data"].emplace_back(data);
        outputs.SetValue(0, result);
    }
    catch (const cv::Exception&) {
        // Unsupported OpenCV types must not terminate the AutoTick worker.
    }
}
}
