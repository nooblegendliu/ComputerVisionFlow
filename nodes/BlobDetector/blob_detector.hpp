#pragma once
#include <flowNode.h>
#include <atomic>

namespace DSPatch::DSPatchables {
class BlobDetector final : public FlowCV::Node {
public:
    BlobDetector();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
    void RefreshProperties() override;
private:
    std::atomic<bool> processing_error_{false};
};
}
