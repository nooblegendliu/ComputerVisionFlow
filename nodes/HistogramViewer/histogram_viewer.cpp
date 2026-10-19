#include "histogram_viewer.hpp"
#include <opencv2/imgproc.hpp>

namespace DSPatch::DSPatchables {
HistogramViewer::HistogramViewer() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Histogram"); SetComponentCategory_(Category::Category_Views);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat}); SetOutputCount_(0); SetEnabled(true);
}
HistogramViewer::Snapshot HistogramViewer::GetSnapshot() const
{ const std::lock_guard<std::mutex> lock(mutex_); return snapshot_; }
void HistogramViewer::Process_(SignalBus const& inputs, SignalBus&)
{
    if (!IsEnabled()) SetEnabled(true);
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input) return; // Original viewer retains the last histogram on disconnection.
    Snapshot next;
    if (!input->empty()) {
        try {
            if (input->dims != 2 || (input->channels() != 1 && input->channels() != 3 && input->channels() != 4))
                CV_Error(cv::Error::StsUnsupportedFormat, "Histogram requires gray, BGR or BGRA");
            next.is_color = input->channels() > 1;
            std::vector<cv::Mat> planes; cv::split(*input, planes);
            const int bins = 256; const float range[]{0,256}; const float* ranges[]{range};
            std::vector<float>* values[]{&next.blue, &next.green, &next.red};
            for (int c = 0; c < (next.is_color ? 3 : 1); ++c) {
                cv::Mat hist;
                cv::calcHist(&planes[c], 1, nullptr, {}, hist, 1, &bins, ranges, true, false);
                cv::normalize(hist, hist, 0, planes[c].rows, cv::NORM_MINMAX);
                // Preserve FlowCV's 255 displayed samples: x=1..255, bins=0..254.
                for (int i = 1; i < bins; ++i) values[c]->push_back(hist.at<float>(i-1));
            }
            for (int i = 1; i < bins; ++i) next.x.push_back(float(i));
            next.has_input = true;
        } catch (const cv::Exception&) { next = {}; next.error = true; }
    }
    const std::lock_guard<std::mutex> lock(mutex_); snapshot_ = std::move(next);
}
}
