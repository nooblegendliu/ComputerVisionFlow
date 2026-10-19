// Preserve FlowCV's sequential nearest-position matching and track history.
#include "simple_blob_tracker.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>

namespace DSPatch::DSPatchables {
namespace {
cv::Vec3b RandomColor()
{
    // A local source avoids sharing an unsynchronized random_device across nodes.
    std::random_device random;
    std::mt19937 engine(random());
    std::uniform_int_distribution<int> distribution(10, 255);
    return {static_cast<uchar>(distribution(engine)), static_cast<uchar>(distribution(engine)),
        static_cast<uchar>(distribution(engine))};
}
int Coordinate(const nlohmann::json& value)
{
    const float number = value.get<float>();
    if (!std::isfinite(number) || double(number) < std::numeric_limits<int>::min()
        || double(number) > std::numeric_limits<int>::max())
        throw std::invalid_argument("Invalid blob coordinate or size");
    return static_cast<int>(number);
}
}
SimpleBlobTracker::SimpleBlobTracker() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Simple_Blob_Tracker");
    SetComponentCategory_(Category::Category_Feature_Detection);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(3, {"in", "start", "blobs"},
        {IoType::Io_Type_CvMat, IoType::Io_Type_Bool, IoType::Io_Type_JSON});
    SetOutputCount_(2, {"vis", "tracks"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    properties_.AddBool("show_tracking", "Show Tracking", true);
    properties_.AddBool("show_ids", "Show IDs", true);
    properties_.AddInt("tracking_duration", "Tracking Duration (Frames)", 300, 2, 3000, .5f);
    properties_.AddBool("drop_lost_tracks", "Drop Lost Tracks", true);
    properties_.AddInt("lost_track_duration", "Lost Track Duration (Frames)", 120, 2, 300, .5f);
    properties_.AddInt("max_pos_var", "Max Position Diff", 8, 1, 1000, .25f);
    properties_.AddInt("max_size_var", "Max Scale Diff", 8, 1, 1000, .25f);
    SetEnabled(true);
}
void SimpleBlobTracker::RequestReset()
{
    std::lock_guard lock(mutex_);
    reset_ = true;
}
void SimpleBlobTracker::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    const auto* input = inputs.GetValue<cv::Mat>(0);
    if (!input) return;
    const auto* start = inputs.GetValue<bool>(1);
    const auto* blobs = inputs.GetValue<nlohmann::json>(2);
    std::lock_guard lock(mutex_);
    if (start && *start) reset_ = true;
    if (reset_) {
        reset_ = false; id_ = 1; frame_ = 0; tracks_.clear();
    }
    if (input->empty()) return;
    if (!IsEnabled()) { outputs.SetValue(0, *input); return; }
    try {
        nlohmann::json settings;
        properties_.ToJson(settings); // One synchronized settings snapshot per frame.
        const auto history = settings.at("tracking_duration").get<int>();
        const auto lost = settings.at("lost_track_duration").get<int>();
        const auto position = settings.at("max_pos_var").get<int>();
        const auto scale = settings.at("max_size_var").get<int>();
        cv::Mat image = input->clone();
        int width = -1, height = -1;
        struct Detection { cv::Point point; int size; };
        std::vector<Detection> detections;
        if (blobs && !blobs->empty()) {
            if (blobs->contains("ref_frame")) {
                width = blobs->at("ref_frame").at("w").get<int>();
                height = blobs->at("ref_frame").at("h").get<int>();
            }
            if (blobs->contains("data")) {
                for (const auto& item : blobs->at("data")) {
                    if (item.contains("x") && item.contains("y") && item.contains("size"))
                        detections.push_back({{Coordinate(item.at("x")), Coordinate(item.at("y"))},
                            Coordinate(item.at("size"))});
                }
            }
        }
        for (const auto& detection : detections) {
            int match = -1;
            double distance = position;
            for (std::size_t i = 0; i < tracks_.size(); ++i) {
                auto& track = tracks_[i];
                // Use wider subtraction to avoid overflow for malformed/extreme coordinates.
                const double dx = double(detection.point.x) - track.pos.back().x;
                const double dy = double(detection.point.y) - track.pos.back().y;
                const double current = std::sqrt(dx * dx + dy * dy);
                if (current < distance) {
                    // Upstream updates nearest distance even if size/age rejects this track.
                    distance = current;
                    if (track.pos.size() > 1) {
                        if (double(detection.size) > double(track.lastSize) - scale
                            && double(detection.size) < double(track.lastSize) + scale
                            && (frame_ - track.lastFrame) < static_cast<std::uint32_t>(lost))
                            match = static_cast<int>(i);
                    } else match = static_cast<int>(i);
                }
            }
            if (match < 0) {
                tracks_.push_back({{detection.point}, RandomColor(), id_++, detection.size, frame_});
            } else {
                auto& track = tracks_[match];
                track.pos.push_back(detection.point);
                track.lastSize = detection.size;
                track.lastFrame = frame_;
            }
        }
        if (settings.at("drop_lost_tracks").get<bool>()) {
            // The original index/erase loop skipped adjacent expired tracks.
            std::erase_if(tracks_, [&](const Track& track) {
                return (frame_ - track.lastFrame) > static_cast<std::uint32_t>(lost);
            });
        }
        nlohmann::json data;
        for (auto& track : tracks_) {
            if (track.pos.size() > 1) {
                // Deliberately remove only one sample per frame, as FlowCV does.
                if (track.pos.size() > static_cast<std::size_t>(history)) track.pos.pop_front();
                data.push_back({{"id", track.id}, {"x", track.pos.back().x},
                    {"y", track.pos.back().y}, {"size", track.lastSize}});
                if (settings.at("show_tracking").get<bool>()) {
                    for (std::size_t i = 1; i < track.pos.size(); ++i)
                        cv::line(image, track.pos[i - 1], track.pos[i], track.color, 2);
                }
            }
        }
        if (settings.at("show_ids").get<bool>()) {
            for (const auto& track : tracks_)
                cv::putText(image, std::to_string(track.id), track.pos.back(),
                    cv::FONT_HERSHEY_SIMPLEX, .75, track.color, 2);
        }
        nlohmann::json result{{"data_type", "tracking"}, {"data", data},
            {"ref_frame", {{"w", width != -1 && height != -1 ? width : image.cols},
                {"h", width != -1 && height != -1 ? height : image.rows}}}};
        outputs.SetValue(0, image);
        outputs.SetValue(1, result);
        ++frame_;
    } catch (const std::exception& error) {
        std::cerr << "Simple_Blob_Tracker: " << error.what() << '\n';
    }
}
#ifndef SIMPLE_BLOB_TRACKER_WITH_QT
EXPORT_PLUGIN(SimpleBlobTracker)
#endif
}
