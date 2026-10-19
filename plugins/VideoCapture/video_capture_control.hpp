#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
namespace DSPatch::DSPatchables {
struct CameraProperty { std::string name; float value=0; };
struct VideoCaptureStatus {
    int sourceIndex=0,listIndex=0,mode=4,fps=30,fpsIndex=7;
    bool forceIndex=false,restoreSettings=false,opened=false;
    std::string error;
    std::vector<std::pair<int,std::string>> cameras;
    std::map<int,CameraProperty> properties;
};
class VideoCaptureControl {
public:
    virtual ~VideoCaptureControl()=default;
    virtual VideoCaptureStatus GetCaptureStatus() const=0;
    virtual void RefreshDevices()=0;
    virtual void SetCameraProperty(int property,float value)=0;
    virtual void RefreshCameraProperties()=0;
    virtual void OpenExternalSettings()=0;
};
}
