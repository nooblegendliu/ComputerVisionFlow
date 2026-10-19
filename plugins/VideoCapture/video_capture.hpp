// Adapted from FlowCV Plugins/VideoCapture (GPL-3.0).
#pragma once
#include "video_capture_control.hpp"
#include "camera_enumerator.hpp"
#include <flowNode.h>
#include <opencv2/videoio.hpp>
#include <memory>
#include <mutex>
namespace DSPatch::DSPatchables {
const std::vector<cv::Size>& CameraModes();
namespace internal {
struct VideoCaptureSession {
    mutable std::mutex mutex;
    VideoCaptureStatus status;
    Camera_Enumerator enumerator;
    cv::VideoCapture capture;
    nlohmann::json loadedSettings;
    bool reopen=false,restore=false;
    VideoCaptureSession();
    VideoCaptureStatus Snapshot() const;
    std::string State() const;
    void Restore(const std::string& state);
    void Refresh(); void ReadProperties(); void SetProperty(int property,float value); void Settings();
    void ConfigureRestore(bool enabled);
    void Enumerate(); void GetProperties(); void Open();
};
}
class VideoCapture : public FlowCV::Node, public VideoCaptureControl {
public:
    VideoCapture();
    VideoCaptureStatus GetCaptureStatus() const override { return session_->Snapshot(); }
    void RefreshDevices() override { session_->Refresh(); }
    void SetCameraProperty(int property,float value) override { session_->SetProperty(property,value); }
    void RefreshCameraProperties() override { session_->ReadProperties(); }
    void OpenExternalSettings() override { session_->Settings(); }
    std::string GetState() override { return session_->State(); }
    void SetState(std::string&& state) override { session_->Restore(state); }
protected:
    void Process_(SignalBus const&,SignalBus& out) override;
    std::shared_ptr<internal::VideoCaptureSession> session_;
};
}
