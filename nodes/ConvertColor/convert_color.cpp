#include "convert_color.hpp"
#include "color_types.hpp"
#include <opencv2/imgproc.hpp>
#include <iterator>
#include <iostream>

namespace DSPatch::DSPatchables {
ConvertColor::ConvertColor() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Convert_Color");
    SetComponentCategory_(Category::Category_Color);
    SetComponentAuthor_("richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    // FlowCV saves the table index, not the OpenCV conversion enum.
    properties_.AddOption("conv_code", "Conversion Type", 0, ConvertColorNames);
    SetEnabled(true);
}

void ConvertColor::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) return;
    if (!IsEnabled()) {
        outputs.SetValue(0, *input);
        channel_error_.store(false);
        return;
    }
    properties_.Sync();
    try {
        const int index = properties_.Get<int>("conv_code");
        // A malformed saved index must not read beyond the original table.
        if (index < 0 || index >= static_cast<int>(std::size(ConvertColorCodes)))
            throw std::out_of_range("Incompatible Conversion Type");
        cv::Mat frame;
        cv::cvtColor(*input, frame, ConvertColorCodes[index]);
        channel_error_.store(false);
        if (!frame.empty()) outputs.SetValue(0, frame);
    }
    catch (const std::exception& error) {
        std::cout << error.what();
        outputs.SetValue(0, *input);
        channel_error_.store(true);
    }
}
}
