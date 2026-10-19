#pragma once
#include <flowNode.h>
#include <atomic>

namespace DSPatch::DSPatchables {
class CopyMakeBorder final : public FlowCV::Node {
public:
    CopyMakeBorder();
    std::string GetState() override;
    void SetState(std::string&& state) override;
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    std::atomic<bool> processing_error_{false};
};
}
