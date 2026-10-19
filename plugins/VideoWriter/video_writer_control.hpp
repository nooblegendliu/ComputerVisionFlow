#pragma once
#include <cstdint>
#include <string>
namespace DSPatch::DSPatchables {
struct VideoWriterStatus {
    std::string filename, codecString="H264", error;
    int codec=0,fps=30,fpsIndex=7;
    bool autoExtension=true,writing=false,opened=false,hasFpsInput=false;
    std::uint64_t framesWritten=0;
};
class VideoWriterControl {
public:
    virtual ~VideoWriterControl()=default;
    virtual VideoWriterStatus GetWriteStatus() const=0;
    virtual void StartWriting()=0;
    virtual void StopWriting()=0;
};
}
