// Adapted from FlowCV Plugins/VideoLoader (GPL-3.0).
#include "video_loader.hpp"
#include <algorithm>
#include <cmath>
#include <thread>
namespace DSPatch::DSPatchables {
VideoLoaderStatus internal::VideoLoaderSession::Snapshot() const { std::lock_guard lock(mutex); return status; }
std::string internal::VideoLoaderSession::State() const {
    std::lock_guard lock(mutex);
    return nlohmann::json{{"movie_path",status.path},{"looping",status.looping},{"use_fps_speed",status.useFps}}.dump(4);
}
void internal::VideoLoaderSession::Restore(const std::string& state) {
    const auto j = nlohmann::json::parse(state);
    std::lock_guard lock(mutex);
    auto next = status;
    if (j.contains("looping")) next.looping = j.at("looping").get<bool>();
    if (j.contains("use_fps_speed")) next.useFps = j.at("use_fps_speed").get<bool>();
    if (j.contains("movie_path")) next.path = j.at("movie_path").get<std::string>();
    status = std::move(next);
    if (j.contains("movie_path")) reload = true;
}
void internal::VideoLoaderSession::Play(bool playing) { std::lock_guard lock(mutex); status.playing = playing; }
void internal::VideoLoaderSession::Seek(int frame) {
    std::lock_guard lock(mutex);
    seek = std::clamp(frame, 1, std::max(1,status.frameCount));
}
VideoLoader::VideoLoader() : FlowCV::Node(ProcessOrder::OutOfOrder),session_(std::make_shared<internal::VideoLoaderSession>()) {
    SetComponentName_("Video_Loader"); SetComponentCategory_(Category::Category_Source);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.1"); SetInputCount_(0);
    SetOutputCount_(4,{"video","start","frame","fps"},{IoType::Io_Type_CvMat,IoType::Io_Type_Bool,IoType::Io_Type_Int,IoType::Io_Type_Int});
    SetEnabled(true);
}
void VideoLoader::Process_(SignalBus const&, SignalBus& out) {
    if (!IsEnabled()) SetEnabled(true); // Source nodes retain upstream enable semantics.
    auto& s=*session_; std::lock_guard lock(s.mutex);
    try {
        if (s.reload) {
            s.reload=false; s.capture.release(); s.status.opened=false;
            s.status.frame=s.status.frameCount=s.status.fps=0; s.seek=-1; s.status.error.clear();
            if (!s.status.path.empty() && s.capture.open(s.status.path,cv::CAP_ANY)) {
                s.status.opened=true;
                s.status.frameCount=std::max(0,static_cast<int>(s.capture.get(cv::CAP_PROP_FRAME_COUNT)));
                const double fps=s.capture.get(cv::CAP_PROP_FPS);
                s.status.fps=std::isfinite(fps)&&fps>0&&fps<=100000 ? static_cast<int>(fps) : 0;
            } else s.status.error="Cannot open movie: "+s.status.path;
        }
        if (!s.capture.isOpened()) return;
        const int interval=s.status.useFps&&s.status.playing&&s.status.fps>0 ? std::max(0,1000/s.status.fps) : 16;
        std::this_thread::sleep_until(s.last+std::chrono::milliseconds(interval));
        s.last=std::chrono::steady_clock::now();
        const bool seeking=s.seek>=1;
        if (seeking) { s.capture.set(cv::CAP_PROP_POS_FRAMES,s.seek-1); s.status.frame=s.seek-1; s.seek=-1; }
        else if (s.status.playing && s.status.frameCount>0 && s.status.frame>=s.status.frameCount) {
            if (s.status.looping) { s.capture.set(cv::CAP_PROP_POS_FRAMES,0); s.status.frame=0; }
            else s.status.playing=false;
        }
        if (!s.status.playing && !seeking) s.capture.set(cv::CAP_PROP_POS_FRAMES,std::max(0,s.status.frame-1));
        const bool start=s.status.frame==0||s.status.frame==1;
        cv::Mat image;
        if (!s.capture.read(image) || image.empty()) {
            if (s.status.looping && s.status.playing) { s.capture.set(cv::CAP_PROP_POS_FRAMES,0); s.status.frame=0; }
            else s.status.playing=false;
            return;
        }
        s.status.frame=static_cast<int>(s.capture.get(cv::CAP_PROP_POS_FRAMES));
        out.SetValue(0,image); out.SetValue(1,start); out.SetValue(2,s.status.frame); out.SetValue(3,s.status.fps);
    } catch(const std::exception& e) { s.status.error=e.what(); s.capture.release(); s.status.opened=false; }
}
#ifndef VIDEO_LOADER_WITH_QT
EXPORT_PLUGIN(VideoLoader)
#endif
}
