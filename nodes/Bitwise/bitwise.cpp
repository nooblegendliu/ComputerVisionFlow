#include "bitwise.hpp"
#include <opencv2/imgproc.hpp>

namespace DSPatch::DSPatchables {
Bitwise::Bitwise() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Bitwise");
    SetComponentCategory_(Category::Category_Merge);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(3, {"in1", "in2", "mask"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat, IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("mode", "Bitwise Mode", 0, {"AND", "NOT", "OR", "XOR"});
    SetEnabled(true);
}

void Bitwise::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* in1 = inputs.GetValue<cv::Mat>(0);
    if (!in1 || in1->empty()) return;
    if (!IsEnabled()) {
        outputs.SetValue(0, *in1);
        processing_error_.store(false);
        return;
    }
    const auto* in2 = inputs.GetValue<cv::Mat>(1);
    const auto* inMask = inputs.GetValue<cv::Mat>(2);
    properties_.Sync();
    try {
        cv::Mat frame, mask, frame2;
        // Upstream's integer 255 selects Scalar conversion, not the data pointer
        // constructor. Make the same full mask explicit here.
        if (!inMask) mask = cv::Mat(in1->size(), CV_8UC1, cv::Scalar(255));
        else inMask->copyTo(mask);
        if (in2) {
            if (in1->size() != in2->size()) cv::resize(*in2, frame2, in1->size());
            else in2->copyTo(frame2);
        }
        const cv::Mat& second = in2 ? frame2 : *in1;
        switch (properties_.Get<int>("mode")) {
        case 0: cv::bitwise_and(*in1, second, frame, mask); break;
        case 1: cv::bitwise_not(*in1, frame, mask); break;
        case 2: cv::bitwise_or(*in1, second, frame, mask); break;
        case 3: cv::bitwise_xor(*in1, second, frame, mask); break;
        }
        outputs.SetValue(0, frame);
        processing_error_.store(false);
    } catch (const cv::Exception&) {
        // A malformed mask/type must not terminate a DSPatch worker.
        processing_error_.store(true);
    }
}
}
