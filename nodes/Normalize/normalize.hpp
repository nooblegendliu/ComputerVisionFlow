#pragma once
#include <flowNode.h>
#include <atomic>

namespace DSPatch::DSPatchables {
class Normalize final : public FlowCV::Node {
public:
    Normalize();
    std::string GetState() override;
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void RefreshProperties() override;
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    std::atomic_bool processing_error_{false};
};
}
