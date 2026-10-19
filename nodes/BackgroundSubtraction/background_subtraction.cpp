#include "background_subtraction.hpp"
#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables {
BackgroundSubtraction::BackgroundSubtraction() : FlowCV::Node(ProcessOrder::OutOfOrder),
    mog2_(cv::createBackgroundSubtractorMOG2()), knn_(cv::createBackgroundSubtractorKNN())
{
    SetComponentName_("Background_Subtraction");
    SetComponentCategory_(Category::Category_Filter);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(2, {"out", "mask"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    properties_.AddOption("mode", "Subtraction Mode", 0, {"MOG2", "KNN"});
    properties_.AddInt("history", "History", 500, 1, 5000, .5f);
    properties_.AddFloat("threshold", "Threshold", 40, .01f, 1000, .01f);
    properties_.AddBool("detect_shadows", "Detect Shadows", false);
    SetEnabled(true);
}

void BackgroundSubtraction::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); processing_error_.store(false); return; }
    // Preserve OutOfOrder scheduling, but serialize access to the learned models.
    std::lock_guard lock(model_mutex_);
    int mode = 0, history = 500;
    float threshold = 40;
    bool shadows = false;
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "mode") mode = std::get<int>(property.value);
        else if (property.key == "history") history = std::get<int>(property.value);
        else if (property.key == "threshold") threshold = std::get<float>(property.value);
        else if (property.key == "detect_shadows") shadows = std::get<bool>(property.value);
    }
    // KNN's CPU implementation reads byte samples directly; MOG2 converts depths.
    if (input->dims != 2 || (mode == 1 && input->depth() != CV_8U)) {
        processing_error_.store(true);
        return;
    }
    try {
        if (!settings_applied_ || history != applied_history_ || threshold != applied_threshold_
            || shadows != applied_shadows_) {
            mog2_->setHistory(history); mog2_->setVarThreshold(threshold); mog2_->setDetectShadows(shadows);
            knn_->setHistory(history); knn_->setDist2Threshold(threshold); knn_->setDetectShadows(shadows);
            applied_history_ = history; applied_threshold_ = threshold; applied_shadows_ = shadows;
            settings_applied_ = true;
        }
        cv::Mat frame, mask, masked;
        input->copyTo(frame);
        if (mode == 0) mog2_->apply(frame, mask);
        else knn_->apply(frame, mask);
        // Every nonzero mask byte (including a 127 shadow) selects input pixels.
        cv::bitwise_and(frame, frame, masked, mask);
        if (!masked.empty()) outputs.SetValue(0, masked);
        if (!mask.empty()) outputs.SetValue(1, mask);
        processing_error_.store(false);
    } catch (const cv::Exception&) { processing_error_.store(true); }
}
}
