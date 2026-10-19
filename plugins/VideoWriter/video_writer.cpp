// Adapted from FlowCV Plugins/VideoWriter (GPL-3.0).
#include "video_writer.hpp"
#include <stdexcept>
namespace DSPatch::DSPatchables {
const std::vector<std::pair<std::string,std::string>>& VideoCodecs() {
    static const std::vector<std::pair<std::string,std::string>> codecs={
        {"H264",".mp4"},{"I420",".mp4"},{"IYUV",".mp4"},{"M4S2",".mp4"},{"MP4S",".mp4"},{"MP4V",".mp4"},
        {"MJPG",".avi"},{"UYVY",".avi"},{"WMV1",".wmv"},{"WMV2",".wmv"},{"WMV3",".wmv"},{"WMVA",".wmv"},
        {"WMVP",".wmv"},{"WVC1",".wmv"},{"WVP2",".wmv"},{"YUY2",".mpg"},{"YV12",".mpg"},{"YVU9",".mpg"},{"YVYU",".mpg"}};
    return codecs;
}
VideoWriterStatus internal::VideoWriterSession::Snapshot() const { std::lock_guard lock(mutex); return status; }
std::string internal::VideoWriterSession::State() const {
    std::lock_guard lock(mutex);
    return nlohmann::json{{"filename",status.filename},{"codec",status.codec},{"codec_str",status.codecString},
        {"fps",status.fps},{"fps_index",status.fpsIndex},{"auto_ext",status.autoExtension}}.dump(4);
}
void internal::VideoWriterSession::Restore(const std::string& state) {
    const auto j=nlohmann::json::parse(state); std::lock_guard lock(mutex); auto next=status;
    if(j.contains("filename")) next.filename=j.at("filename").get<std::string>();
    if(j.contains("codec")) next.codec=j.at("codec").get<int>();
    if(j.contains("codec_str")) next.codecString=j.at("codec_str").get<std::string>();
    if(j.contains("fps")) next.fps=j.at("fps").get<int>();
    if(j.contains("fps_index")) next.fpsIndex=j.at("fps_index").get<int>();
    if(j.contains("auto_ext")) next.autoExtension=j.at("auto_ext").get<bool>();
    if(next.codec<0||next.codec>=static_cast<int>(VideoCodecs().size())||next.codecString.size()!=4)
        throw std::invalid_argument("Invalid video codec");
    // Like the original checkbox, auto_ext only affects the next normal open.
    // Reopening an active writer here would truncate its existing recording.
    if(next.filename!=status.filename||next.codecString!=status.codecString) reopen=true;
    status=std::move(next);
}
void internal::VideoWriterSession::Start() { std::lock_guard lock(mutex); reopen=true; status.writing=true; }
void internal::VideoWriterSession::Stop() { std::lock_guard lock(mutex); status.writing=false; writer.release(); status.opened=false; }
void internal::VideoWriterSession::Open(const cv::Mat& frame) {
    writer.release(); status.opened=false; status.error.clear(); reopen=false;
    if(status.filename.empty()) { status.error="Choose an output movie file"; return; }
    if(status.fps<=0) { status.error="Video FPS must be positive"; return; }
    if(status.autoExtension) {
        const auto& ext=VideoCodecs().at(status.codec).second;
        if(status.filename.find(ext)==std::string::npos) {
            const auto pos=status.filename.find_last_of('.');
            const auto slash=status.filename.find_last_of("/\\");
            if(pos!=std::string::npos&&(slash==std::string::npos||pos>slash)) status.filename.resize(pos);
            status.filename+=ext;
        }
    }
    const auto& codec=status.codecString;
    status.opened=writer.open(status.filename,cv::VideoWriter::fourcc(codec[0],codec[1],codec[2],codec[3]),status.fps,frame.size(),true);
    lastFps=status.fps; lastCodec=status.codec; frameSize=frame.size();
    if(!status.opened) status.error="Cannot open video writer: "+status.filename;
}
VideoWriter::VideoWriter() : FlowCV::Node(ProcessOrder::OutOfOrder),session_(std::make_shared<internal::VideoWriterSession>()) {
    SetComponentName_("Video_Writer"); SetComponentCategory_(Category::Category_Output);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(3,{"in","fps","start"},{IoType::Io_Type_CvMat,IoType::Io_Type_Int,IoType::Io_Type_Bool}); SetOutputCount_(0); SetEnabled(true);
}
void VideoWriter::Process_(SignalBus const& in,SignalBus&) {
    const auto* frame=in.GetValue<cv::Mat>(0); if(!frame) return;
    auto& s=*session_; std::lock_guard lock(s.mutex);
    try {
        const auto* fps=in.GetValue<int>(1); s.status.hasFpsInput=fps!=nullptr; if(fps) s.status.fps=*fps;
        if(const auto* start=in.GetValue<bool>(2);start&&*start) {
            s.status.writing=!s.status.writing;
            if(s.status.writing) s.reopen=true;
            else { s.writer.release(); s.status.opened=false; }
        }
        // Upstream toggles for every true input and does not gate writing on IsEnabled().
        if(frame->empty()||!s.status.writing) return;
        if(frame->type()!=CV_8UC3) { s.status.error="Video_Writer requires an 8-bit BGR image"; return; }
        if(s.status.fps<=0) { s.status.error="Video FPS must be positive"; return; }
        if(s.status.fps!=s.lastFps||s.status.codec!=s.lastCodec||s.reopen) s.Open(*frame);
        if(!s.writer.isOpened()) s.Open(*frame);
        else {
            if(frame->size()!=s.frameSize) { s.status.error="Frame dimensions changed while writing"; return; }
            s.writer.write(*frame); ++s.status.framesWritten; s.status.error.clear();
        }
    } catch(const std::exception& e) { s.status.error=e.what(); s.writer.release(); s.status.opened=false; }
}
#ifndef VIDEO_WRITER_WITH_QT
EXPORT_PLUGIN(VideoWriter)
#endif
}
