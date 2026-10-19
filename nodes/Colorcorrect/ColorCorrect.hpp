#pragma once
#include <flowNode.h>
#include <array>
#include <atomic>
#include <mutex>

namespace DSPatch::DSPatchables {
class Colorcorrect final : public FlowCV::Node {
public:
    Colorcorrect();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
    void Reset();
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    struct Parameters {
        float brightness = 0, contrast = 1, gamma = 1, gain = 1, saturation = 1;
        std::array<float, 3> low{0, 0, 0}, high{1, 1, 1}; // RGB, as in saved flows.
    } parameters_;
    static std::array<unsigned char, 256> MakeLut(float gamma);
    std::array<unsigned char, 256> lut_;
    std::mutex mutex_;
    std::atomic<bool> processing_error_{false};
};
}
