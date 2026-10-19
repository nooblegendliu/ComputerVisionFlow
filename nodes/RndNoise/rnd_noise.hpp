#pragma once
#include <flowNode.h>
#include <atomic>
#include <chrono>
#include <mutex>

namespace DSPatch::DSPatchables {
class RndNoise final : public FlowCV::Node {
public:
    RndNoise();
    std::string GetState() override;
    void SetState(std::string&& state) override;
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
    void RefreshProperties() override;
private:
    struct Parameters {
        int width = 1280, height = 720, mode = 0, fps = 30, fps_index = 7;
    };
    Parameters SnapshotLocked();
    static void Validate(const Parameters& parameters);
    std::mutex parameter_mutex_;
    std::mutex io_mutex_;
    int last_fps_index_ = 7;
    std::chrono::steady_clock::time_point last_time_;
    std::atomic<bool> processing_error_{false};
};
}
