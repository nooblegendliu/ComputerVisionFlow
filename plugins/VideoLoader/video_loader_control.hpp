// Qt-free runtime controls; movie state retains FlowCV's original keys.
#pragma once
#include <string>
namespace DSPatch::DSPatchables {
struct VideoLoaderStatus {
    std::string path, error;
    bool looping = true, useFps = true, playing = true, opened = false;
    int frame = 0, frameCount = 0, fps = 0;
};
class VideoLoaderControl {
public:
    virtual ~VideoLoaderControl() = default;
    virtual VideoLoaderStatus GetPlaybackStatus() const = 0;
    virtual void SetPlaying(bool playing) = 0;
    // Frames are one-based, matching the upstream frame output.
    virtual void SeekFrame(int frame) = 0;
};
}
