// Adapted from FlowCV Plugins/ImageLoader (GPL-3.0); see ../README.md.
#include "image_loader.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <opencv2/imgcodecs.hpp>

namespace DSPatch::DSPatchables
{
namespace
{
constexpr std::array<int, 10> kFpsValues{1, 3, 5, 10, 15, 20, 25, 30, 60, 120};
}

ImageLoader::ImageLoader() : FlowCV::Node(ProcessOrder::InOrder)
{
    SetComponentName_("Image_Loader");
    SetComponentCategory_(Category::Category_Source);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(0);
    SetOutputCount_(1, {"out"}, {IoType::Io_Type_CvMat});
    properties_.AddFilePath("image_path", "Image Path", "");
    properties_.AddOption("fps_index", "Output FPS", 7,
        {"1", "3", "5", "10", "15", "20", "25", "30", "60", "120"});
    last_time_ = std::chrono::steady_clock::now();
    SetEnabled(true);
}

void ImageLoader::LoadImage(const std::string& path)
{
    image_file_ = path;
    frame_.release();
    if (path.empty())
        return;
    try {
        // Qt paths are UTF-8. Use a native filesystem path for Windows I/O,
        // retaining upstream imread(..., true)'s 8-bit BGR decode semantics.
        std::ifstream stream(std::filesystem::u8path(path), std::ios::binary);
        const std::vector<unsigned char> bytes{
            std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
        if (!bytes.empty())
            frame_ = cv::imdecode(bytes, cv::IMREAD_COLOR);
    }
    catch (const std::exception& error) {
        std::cerr << "Image_Loader: " << error.what() << '\n';
    }
    if (frame_.empty())
        std::cerr << "Image_Loader: unable to load image: " << path << '\n';
}

void ImageLoader::Process_(SignalBus const&, SignalBus& outputs)
{
    // Preserve upstream source-node enable and frame pacing behavior.
    if (!IsEnabled())
        SetEnabled(true);
    std::unique_lock<std::mutex> lock(io_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
        return;

    const bool reloadImage = properties_.Changed("image_path");
    properties_.Sync();
    const auto path = properties_.Get<std::string>("image_path");
    if (reloadImage || path != image_file_)
        LoadImage(path);
    const int index = properties_.Get<int>("fps_index");
    if (index != fps_index_) {
        fps_ = kFpsValues.at(static_cast<std::size_t>(index));
        fps_index_ = index;
    }
    if (!frame_.empty()) {
        const auto fpsTime = static_cast<std::uint32_t>(1000.0f / fps_);
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_time_).count() < fpsTime) {
        }
        outputs.SetValue(0, frame_);
    }
    last_time_ = std::chrono::steady_clock::now();
}

std::string ImageLoader::GetState()
{
    std::lock_guard<std::mutex> lock(io_mutex_);
    nlohmann::json state;
    properties_.ToJson(state);
    const int index = state.at("fps_index").get<int>();
    state["fps"] = index == fps_index_ ? fps_ : kFpsValues.at(index);
    return state.dump(4);
}

void ImageLoader::SetState(std::string&& serialized)
{
    const auto state = nlohmann::json::parse(serialized);
    if (!state.is_object())
        throw std::invalid_argument("Image_Loader state must be an object");
    for (const char* key : {"fps", "fps_index"}) {
        if (state.contains(key) && (!state.at(key).is_number_integer()
            || state.at(key) < 0 || state.at(key) > 120))
            throw std::invalid_argument("Image_Loader FPS fields require bounded integers");
    }
    std::lock_guard<std::mutex> lock(io_mutex_);
    const int index = state.value("fps_index", properties_.GetW<int>("fps_index"));
    if (index < 0 || index >= static_cast<int>(kFpsValues.size()))
        throw std::invalid_argument("Image_Loader fps_index out of range");
    const int fps = state.value("fps", state.contains("fps_index") ? kFpsValues[index] : fps_);
    if (fps <= 0 || fps > 120)
        throw std::invalid_argument("Image_Loader fps must be between 1 and 120");
    properties_.FromJson(state);
    fps_ = fps;
    fps_index_ = index;
    if (state.contains("image_path"))
        LoadImage(properties_.GetW<std::string>("image_path"));
}

EXPORT_PLUGIN(ImageLoader)
}
