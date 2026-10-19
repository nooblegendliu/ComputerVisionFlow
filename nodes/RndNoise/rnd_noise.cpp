#include "rnd_noise.hpp"
#include <opencv2/imgproc.hpp>
#include <array>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <thread>

namespace DSPatch::DSPatchables {
namespace {
constexpr std::array<int, 10> kFpsValues{1, 3, 5, 10, 15, 20, 25, 30, 60, 120};
}

RndNoise::RndNoise() : FlowCV::Node(ProcessOrder::OutOfOrder),
    last_time_(std::chrono::steady_clock::now())
{
    SetComponentName_("Rnd_Noise");
    SetComponentCategory_(Category::Category_Source);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(0);
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddOption("mode", "Mask Mode", 0, {"Color", "BW_8U", "BW_8UC3"});
    properties_.AddInt("width", "Width", 1280, 1, 8000, .5f);
    properties_.AddInt("height", "Height", 720, 1, 8000, .5f);
    properties_.AddOption("fps_index", "Output FPS", 7,
        {"1", "3", "5", "10", "15", "20", "25", "30", "60", "120"});
    // The original state stores FPS independently from the combo index.
    properties_.AddInt("fps", "FPS", 30, 1, 120, 1, false);
    SetEnabled(true);
}

RndNoise::Parameters RndNoise::SnapshotLocked()
{
    Parameters parameters;
    for (const auto& property : properties_.Snapshot().properties) {
        if (property.key == "width") parameters.width = std::get<int>(property.value);
        else if (property.key == "height") parameters.height = std::get<int>(property.value);
        else if (property.key == "mode") parameters.mode = std::get<int>(property.value);
        else if (property.key == "fps") parameters.fps = std::get<int>(property.value);
        else if (property.key == "fps_index") parameters.fps_index = std::get<int>(property.value);
    }
    if (parameters.fps_index != last_fps_index_) {
        parameters.fps = kFpsValues.at(static_cast<std::size_t>(parameters.fps_index));
        properties_.Set("fps", parameters.fps);
        last_fps_index_ = parameters.fps_index;
    }
    return parameters;
}

void RndNoise::Validate(const Parameters& parameters)
{
    if (parameters.width < 1 || parameters.height < 1
        || parameters.mode < 0 || parameters.mode > 2
        || parameters.fps < 1
        || parameters.fps_index < 0 || parameters.fps_index >= static_cast<int>(kFpsValues.size()))
        throw std::invalid_argument("Invalid Rnd_Noise dimensions, FPS or mode");
}

void RndNoise::RefreshProperties()
{
    std::lock_guard lock(parameter_mutex_);
    const auto parameters = SnapshotLocked();
    // Preserve saved sizes outside the original drag range without displaying
    // a different value in QSpinBox. These are UI hints, not state limits.
    properties_.SetMax("width", std::max(8000, parameters.width));
    properties_.SetMax("height", std::max(8000, parameters.height));
}

std::string RndNoise::GetState()
{
    std::lock_guard lock(parameter_mutex_);
    const auto parameters = SnapshotLocked();
    return nlohmann::json{{"mode", parameters.mode}, {"width", parameters.width},
        {"height", parameters.height}, {"fps", parameters.fps},
        {"fps_index", parameters.fps_index}}.dump(4);
}

void RndNoise::SetState(std::string&& serialized)
{
    const auto state = nlohmann::json::parse(serialized);
    if (!state.is_object()) throw std::invalid_argument("Rnd_Noise state must be an object");
    // Validate all supplied fields before touching any editable property.
    auto updates = nlohmann::json::object();
    const auto read = [&](const char* key, int minimum, int maximum) {
        if (!state.contains(key)) return;
        const auto& value = state.at(key);
        if (!value.is_number_integer() || value < minimum || value > maximum)
            throw std::invalid_argument(std::string("Invalid Rnd_Noise state field: ") + key);
        updates[key] = value.get<int>();
    };
    constexpr int maximum = (std::numeric_limits<int>::max)();
    read("width", 1, maximum); read("height", 1, maximum); read("mode", 0, 2);
    read("fps", 1, maximum); read("fps_index", 0, 9);
    std::lock_guard lock(parameter_mutex_);
    SnapshotLocked();
    properties_.FromJson(updates);
    // Restoring an index must not remap a separately restored FPS value.
    last_fps_index_ = properties_.GetW<int>("fps_index");
}

void RndNoise::Process_(SignalBus const&, SignalBus& outputs)
{
    // Preserve the source's OutOfOrder policy: competing ticks skip this frame.
    std::unique_lock ioLock(io_mutex_, std::try_to_lock);
    if (!ioLock.owns_lock()) return;
    processing_error_.store(false);
    try {
        Parameters parameters;
        {
            std::lock_guard parameterLock(parameter_mutex_);
            parameters = SnapshotLocked();
        }
        Validate(parameters);
        const auto period = std::chrono::milliseconds(static_cast<unsigned int>(
            (1.0f / static_cast<float>(parameters.fps)) * 1000.0f));
        // Keep upstream's truncated millisecond period and first-frame wait,
        // without spinning or holding the parameter lock while waiting.
        std::this_thread::sleep_until(last_time_ + period);
        cv::Mat frame(parameters.height, parameters.width,
            parameters.mode > 0 ? CV_8U : CV_8UC3, cv::Scalar::all(0));
        cv::randu(frame, cv::Scalar::all(0), cv::Scalar::all(255));
        if (parameters.mode == 2) cv::cvtColor(frame, frame, cv::COLOR_GRAY2BGR);
        // Upstream deliberately does not consult or change the enabled flag.
        if (!frame.empty()) outputs.SetValue(0, frame);
        last_time_ = std::chrono::steady_clock::now();
    } catch (const std::exception&) {
        processing_error_.store(true);
    }
}
}
