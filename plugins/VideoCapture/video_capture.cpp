// Adapted from FlowCV Plugins/VideoCapture (GPL-3.0).
#include "video_capture.hpp"
#include <cmath>
#include <stdexcept>
namespace DSPatch::DSPatchables {
const std::vector<cv::Size>& CameraModes() {
    static const std::vector<cv::Size> modes={{320,240},{640,480},{800,600},{1024,768},{1280,720},{1280,960},
        {1600,1200},{1920,1080},{2048,1536},{2592,1944},{3840,2160}};
    return modes;
}
namespace {
const std::map<int,std::string>& Properties() {
    static const std::map<int,std::string> properties={{10,"Brightness"},{11,"Contrast"},{12,"Saturation"},{13,"Hue"},
        {14,"Gain"},{15,"Exposure"},{20,"Sharpness"},{21,"Auto Exposure"},{22,"Gamma"},{27,"Zoom"},{28,"Focus"},
        {30,"ISO Speed"},{31,"Backlight"},{35,"Iris"}};
    return properties;
}
std::string CameraName(const VideoCaptureStatus& s) {
    return s.listIndex>=0&&s.listIndex<static_cast<int>(s.cameras.size()) ? s.cameras[s.listIndex].second : "";
}
void MatchName(VideoCaptureStatus& s,const std::string& name) {
    if(CameraName(s)==name) return;
    s.listIndex=0; s.sourceIndex=0;
    for(size_t i=0;i<s.cameras.size();++i) if(s.cameras[i].second==name) {
        s.listIndex=static_cast<int>(i); s.sourceIndex=s.cameras[i].first; break;
    }
}
}
internal::VideoCaptureSession::VideoCaptureSession() { Enumerate(); }
void internal::VideoCaptureSession::Enumerate() {
    enumerator.RefreshCameraList(); status.cameras.clear();
    for(int i=0;i<enumerator.GetCameraCount();++i) status.cameras.emplace_back(enumerator.GetCameraIndex(i),enumerator.GetCameraName(i));
}
VideoCaptureStatus internal::VideoCaptureSession::Snapshot() const { std::lock_guard lock(mutex); return status; }
std::string internal::VideoCaptureSession::State() const {
    std::lock_guard lock(mutex); nlohmann::json settings;
    for(const auto& [id,p]:status.properties) settings[p.name]=p.value;
    return nlohmann::json{{"src_index",status.sourceIndex},{"list_index",status.listIndex},{"force_index",status.forceIndex},
        {"src_name",CameraName(status)},{"src_mode",status.mode},{"fps",status.fps},{"fps_index",status.fpsIndex},
        {"set_load_man",status.restoreSettings},{"camera_settings",settings}}.dump(4);
}
void internal::VideoCaptureSession::Restore(const std::string& state) {
    const auto j=nlohmann::json::parse(state); std::lock_guard lock(mutex); auto next=status;
    if(j.contains("list_index")) next.listIndex=j.at("list_index").get<int>();
    if(j.contains("force_index")) next.forceIndex=j.at("force_index").get<bool>();
    if(j.contains("src_index")) next.sourceIndex=j.at("src_index").get<int>();
    if(j.contains("src_mode")) next.mode=j.at("src_mode").get<int>();
    if(j.contains("fps")&&j.contains("fps_index")) { next.fps=j.at("fps").get<int>(); next.fpsIndex=j.at("fps_index").get<int>(); }
    if(j.contains("set_load_man")) next.restoreSettings=j.at("set_load_man").get<bool>();
    auto settings=loadedSettings;
    if(j.contains("camera_settings")) {
        settings=j.at("camera_settings");
        if(!settings.is_null()&&!settings.is_object()) throw std::invalid_argument("Invalid camera settings");
        for(const auto& value:settings.items()) if(!value.value().is_number()||!std::isfinite(value.value().get<float>()))
            throw std::invalid_argument("Invalid camera property");
    }
    if(next.sourceIndex<0||next.mode<0||next.mode>=static_cast<int>(CameraModes().size())||next.fps<0)
        throw std::invalid_argument("Invalid camera source, resolution or FPS");
    if(j.contains("src_name")&&!next.forceIndex) {
        // Keep source lookup by name, with numeric fallback only when explicitly requested.
        const auto name=j.at("src_name").get<std::string>();
        enumerator.RefreshCameraList(); next.cameras.clear();
        for(int i=0;i<enumerator.GetCameraCount();++i) next.cameras.emplace_back(enumerator.GetCameraIndex(i),enumerator.GetCameraName(i));
        MatchName(next,name);
    }
    reopen=reopen||next.sourceIndex!=status.sourceIndex||next.mode!=status.mode||next.fps!=status.fps;
    status=std::move(next); loadedSettings=std::move(settings);
    if(j.contains("set_load_man")) restore=status.restoreSettings;
}
void internal::VideoCaptureSession::Refresh() {
    std::lock_guard lock(mutex); const auto name=CameraName(status); const int previous=status.sourceIndex;
    Enumerate(); MatchName(status,name); reopen=reopen||previous!=status.sourceIndex;
}
void internal::VideoCaptureSession::GetProperties() {
    status.properties.clear();
    for(const auto& [id,name]:Properties()) status.properties[id]={name,static_cast<float>(capture.get(id))};
}
void internal::VideoCaptureSession::ReadProperties() { std::lock_guard lock(mutex); if(capture.isOpened()) GetProperties(); }
void internal::VideoCaptureSession::ConfigureRestore(bool enabled) { std::lock_guard lock(mutex); status.restoreSettings=enabled; }
void internal::VideoCaptureSession::SetProperty(int property,float value) {
    std::lock_guard lock(mutex);
    if(!std::isfinite(value)||!Properties().count(property)) return;
    try { if(capture.isOpened()) { capture.set(property,value); status.properties[property]={Properties().at(property),value}; } }
    catch(const std::exception& e) { status.error=e.what(); }
}
void internal::VideoCaptureSession::Settings() {
    std::lock_guard lock(mutex);
    try { if(capture.isOpened()) capture.set(cv::CAP_PROP_SETTINGS,status.sourceIndex); }
    catch(const std::exception& e) { status.error=e.what(); }
}
void internal::VideoCaptureSession::Open() {
    capture.release(); status.opened=false; status.properties.clear(); status.error.clear(); reopen=false;
    if(status.sourceIndex==0) return;
    int backend=cv::CAP_ANY;
#ifdef _WIN32
    backend=cv::CAP_DSHOW;
#elif defined(__linux__)
    backend=cv::CAP_V4L;
#endif
    if(capture.open(status.sourceIndex-1,backend)) {
        const auto mode=CameraModes().at(status.mode);
        capture.set(cv::CAP_PROP_FRAME_WIDTH,mode.width); capture.set(cv::CAP_PROP_FRAME_HEIGHT,mode.height);
        capture.set(cv::CAP_PROP_FPS,status.fps); status.fps=static_cast<int>(capture.get(cv::CAP_PROP_FPS));
        status.opened=true; GetProperties();
    } else status.error="Cannot open camera index "+std::to_string(status.sourceIndex-1);
}
VideoCapture::VideoCapture() : FlowCV::Node(ProcessOrder::InOrder),session_(std::make_shared<internal::VideoCaptureSession>()) {
    SetComponentName_("Video_Capture"); SetComponentCategory_(Category::Category_Source);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0"); SetInputCount_(0);
    SetOutputCount_(2,{"src","fps"},{IoType::Io_Type_CvMat,IoType::Io_Type_Int}); SetEnabled(true);
}
void VideoCapture::Process_(SignalBus const&,SignalBus& out) {
    if(!IsEnabled()) SetEnabled(true);
    auto& s=*session_; std::lock_guard lock(s.mutex);
    try {
        if(s.reopen||(!s.capture.isOpened()&&s.status.sourceIndex>0)) s.Open();
        if(!s.capture.isOpened()) return;
        if(s.restore) {
            s.restore=false;
            if(s.status.restoreSettings) {
                for(const auto& [id,p]:s.status.properties) if(s.loadedSettings.contains(p.name)) {
                    const auto value=s.loadedSettings.at(p.name).get<float>();
                    if(static_cast<int>(p.value)!=static_cast<int>(value)) s.capture.set(id,value);
                }
                s.GetProperties();
            }
        }
        cv::Mat frame; if(s.capture.read(frame)&&!frame.empty()) { out.SetValue(0,frame); out.SetValue(1,s.status.fps); }
    } catch(const std::exception& e) { s.status.error=e.what(); s.capture.release(); s.status.opened=false; }
}
#ifndef VIDEO_CAPTURE_WITH_QT
EXPORT_PLUGIN(VideoCapture)
#endif
}
