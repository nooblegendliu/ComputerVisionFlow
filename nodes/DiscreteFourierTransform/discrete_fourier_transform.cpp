#include "discrete_fourier_transform.hpp"
#include <opencv2/imgproc.hpp>

namespace DSPatch::DSPatchables {
DiscreteFourierTransform::DiscreteFourierTransform() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("DFT");
    SetComponentCategory_(Category::Category_Utility);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"dft"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("op_mode", "Operation", 0, {"Default", "Inverse", "Rows", "Complex", "Real", "Complex In"});
    properties_.AddBool("shift_center", "Shift DC to Center", true);
    properties_.AddBool("log_view", "Logarithmic View", false);
    SetEnabled(true);
}

void DiscreteFourierTransform::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    processing_error_.store(false);
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty() || !IsEnabled()) return;
    int mode = 0;
    bool shift = true, logarithmic = false;
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "op_mode") mode = std::get<int>(property.value);
        else if (property.key == "shift_center") shift = std::get<bool>(property.value);
        else if (property.key == "log_view") logarithmic = std::get<bool>(property.value);
    }
    try {
        cv::Mat dftFrame, frame;
        if (input->channels() > 1) cv::cvtColor(*input, dftFrame, cv::COLOR_BGR2GRAY);
        else input->copyTo(dftFrame);
        constexpr int flags[] = {0, cv::DFT_INVERSE + cv::DFT_SCALE, cv::DFT_ROWS,
            cv::DFT_COMPLEX_OUTPUT, cv::DFT_REAL_OUTPUT, cv::DFT_COMPLEX_INPUT};
        // FlowCV always constructs a float real plane plus zero imaginary plane.
        // The output is magnitude, including when an inverse/complex flag is selected.
        cv::Mat planes[] = {cv::Mat_<float>(dftFrame), cv::Mat::zeros(dftFrame.size(), CV_32F)};
        cv::Mat complexImage;
        cv::merge(planes, 2, complexImage);
        cv::dft(complexImage, complexImage, flags[mode]);
        cv::split(complexImage, planes);
        cv::magnitude(planes[0], planes[1], planes[0]);
        cv::Mat magnitude = planes[0];
        if (shift) {
            magnitude = magnitude(cv::Rect(0, 0, magnitude.cols & -2, magnitude.rows & -2));
            // A one-pixel dimension becomes empty after upstream's even-size crop.
            if (magnitude.empty()) return;
            const int cx = magnitude.cols / 2, cy = magnitude.rows / 2;
            cv::Mat q0(magnitude, cv::Rect(0, 0, cx, cy));
            cv::Mat q1(magnitude, cv::Rect(cx, 0, cx, cy));
            cv::Mat q2(magnitude, cv::Rect(0, cy, cx, cy));
            cv::Mat q3(magnitude, cv::Rect(cx, cy, cx, cy));
            cv::Mat temporary;
            q0.copyTo(temporary); q3.copyTo(q0); temporary.copyTo(q3);
            q1.copyTo(temporary); q2.copyTo(q1); temporary.copyTo(q2);
        }
        magnitude.copyTo(dftFrame);
        if (logarithmic) {
            magnitude += cv::Scalar::all(1);
            cv::log(magnitude, magnitude);
            cv::normalize(magnitude, magnitude, 0, 1, cv::NORM_MINMAX);
            magnitude.convertTo(frame, CV_8UC1, 255.0);
            if (!frame.empty()) outputs.SetValue(0, frame);
        } else if (!dftFrame.empty()) outputs.SetValue(0, dftFrame);
    } catch (const cv::Exception&) { processing_error_.store(true); }
}
}
