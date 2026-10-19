#include "blob_detector.hpp"
#include <opencv2/features2d.hpp>
#include <cmath>
#include <limits>

namespace DSPatch::DSPatchables {
namespace {
using Json = nlohmann::json;
Json Snapshot(const FlowCV::FlowCV_Properties& properties)
{
    Json state = Json::object();
    for (const auto& property : properties.Snapshot().properties)
        std::visit([&](const auto& value) { state[property.key] = value; }, property.value);
    return state;
}
void Validate(const Json& state)
{
    for (const char* channel : {"R", "G", "B"}) {
        const auto key = std::string("blob_viz_color.") + channel;
        if (state.contains(key) && (!state.at(key).is_number()
            || !std::isfinite(state.at(key).get<double>())
            || state.at(key).get<double>() < 0 || state.at(key).get<double>() > 1))
            throw std::invalid_argument("Invalid Blob_Detector visualization color");
    }
    for (const char* key : {"color", "min_repeat"}) {
        if (state.contains(key) && (!state.at(key).is_number_integer()
            || state.at(key).get<double>() < 0
            || state.at(key).get<double>() > (std::string(key) == "color" ? 255 : (std::numeric_limits<int>::max)())))
            throw std::invalid_argument("Invalid Blob_Detector integer setting");
    }
}
// Recent OpenCV rejects the original zero lower bounds, even for disabled
// filters. Preserve the saved settings and adapt only the detector's copy.
// This excludes exactly-zero degenerate circularity/inertia/convexity values;
// ordinary positive contour measurements retain their original comparisons.
float PositiveLowerBound(float value)
{
    return value == 0 ? (std::numeric_limits<float>::min)() : value;
}
}

BlobDetector::BlobDetector() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Blob_Detector");
    SetComponentCategory_(Category::Category_Feature_Detection);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(2, {"viz", "blobs"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    properties_.AddFloat("min_threshold", "Min Threshold", 10, 0, 255, .25f);
    properties_.AddFloat("max_threshold", "Max Threshold", 200, 0, 255, .25f);
    properties_.AddFloat("threshold_step", "Threshold Step", 10, 1, 255, .01f);
    properties_.AddFloat("min_dist", "Min Distance Between Blobs", 10, 0, 5000, .1f);
    properties_.AddInt("min_repeat", "Min Repeatability", 2, 1, 255, .25f);
    properties_.AddBool("filter_by_color", "Filter By Color", true);
    properties_.AddInt("color", "Color", 255, 0, 255, .25f);
    properties_.AddBool("filter_by_area", "Filter By Area", true);
    properties_.AddFloat("min_area", "Min Area", 0, 0, 5000, .1f);
    properties_.AddFloat("max_area", "Max Area", 5000, 0, 5000, .1f);
    for (const char* filter : {"circularity", "convexity", "inertia"}) {
        const std::string name(filter);
        properties_.AddBool("filter_by_" + name, "Filter By " + name, false);
        properties_.AddFloat("min_" + name, "Min " + name, 0, 0, 1, .001f, false);
        properties_.AddFloat("max_" + name, "Max " + name, 1, 0, 1, .001f, false);
    }
    for (const char* channel : {"R", "G", "B"})
        properties_.AddFloat(std::string("blob_viz_color.") + channel, channel, 0, 0, 1, .01f, false);
    SetEnabled(true);
}

void BlobDetector::RefreshProperties()
{
    const auto state = Snapshot(properties_);
    properties_.SetVisibility("color", state.at("filter_by_color").get<bool>());
    for (const char* filter : {"area", "circularity", "convexity", "inertia"}) {
        const std::string name(filter);
        const bool show = state.at("filter_by_" + name).get<bool>();
        properties_.SetVisibility("min_" + name, show);
        properties_.SetVisibility("max_" + name, show);
    }
}

std::string BlobDetector::GetState()
{
    auto state = Snapshot(properties_);
    for (const char* channel : {"R", "G", "B"}) {
        const auto key = std::string("blob_viz_color.") + channel;
        state["blob_viz_color"][channel] = state.at(key);
        state.erase(key);
    }
    return state.dump();
}

void BlobDetector::SetState(std::string&& serialized)
{
    if (serialized.empty()) return;
    auto state = Json::parse(serialized);
    if (!state.is_object()) throw std::invalid_argument("Blob_Detector state must be an object");
    if (state.contains("blob_viz_color"))
        for (const char* channel : {"R", "G", "B"})
            state[std::string("blob_viz_color.") + channel] = state.at("blob_viz_color").at(channel);
    Validate(state);
    properties_.FromJson(state);
    RefreshProperties();
}

void BlobDetector::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input || input->empty()) { processing_error_.store(false); return; }
    if (!IsEnabled()) { outputs.SetValue(0, *input); processing_error_.store(false); return; }
    const auto state = Snapshot(properties_);
    try {
        Validate(state);
        cv::SimpleBlobDetector::Params params;
        params.filterByColor = state.at("filter_by_color").get<bool>();
        params.filterByArea = state.at("filter_by_area").get<bool>();
        params.filterByCircularity = state.at("filter_by_circularity").get<bool>();
        params.filterByConvexity = state.at("filter_by_convexity").get<bool>();
        params.filterByInertia = state.at("filter_by_inertia").get<bool>();
        params.blobColor = static_cast<uchar>(state.at("color").get<int>());
        params.minThreshold = state.at("min_threshold").get<float>();
        params.maxThreshold = state.at("max_threshold").get<float>();
        params.thresholdStep = state.at("threshold_step").get<float>();
        params.minRepeatability = state.at("min_repeat").get<int>();
        params.minDistBetweenBlobs = PositiveLowerBound(state.at("min_dist").get<float>());
        params.minArea = PositiveLowerBound(state.at("min_area").get<float>());
        params.maxArea = state.at("max_area").get<float>();
        params.minCircularity = PositiveLowerBound(state.at("min_circularity").get<float>());
        params.maxCircularity = state.at("max_circularity").get<float>();
        params.minConvexity = PositiveLowerBound(state.at("min_convexity").get<float>());
        params.maxConvexity = state.at("max_convexity").get<float>();
        params.minInertiaRatio = PositiveLowerBound(state.at("min_inertia").get<float>());
        params.maxInertiaRatio = state.at("max_inertia").get<float>();
        auto detector = cv::SimpleBlobDetector::create(params);
        cv::Mat frame = input->clone();
        std::vector<cv::KeyPoint> keypoints;
        detector->detect(frame, keypoints);
        cv::drawKeypoints(frame, keypoints, frame,
            cv::Scalar(state.at("blob_viz_color.B").get<float>() * 255,
                state.at("blob_viz_color.G").get<float>() * 255,
                state.at("blob_viz_color.R").get<float>() * 255), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        if (!keypoints.empty()) {
            Json data = Json::array();
            for (const auto& point : keypoints)
                data.push_back({{"x", point.pt.x}, {"y", point.pt.y}, {"size", point.size}});
            outputs.SetValue(1, Json{{"data_type", "keypoints"},
                {"ref_frame", {{"w", frame.cols}, {"h", frame.rows}}}, {"data", data}});
        }
        if (!frame.empty()) outputs.SetValue(0, frame);
        processing_error_.store(false);
    } catch (const std::exception&) { processing_error_.store(true); }
}
}
