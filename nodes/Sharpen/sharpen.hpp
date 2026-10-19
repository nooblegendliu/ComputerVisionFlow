#pragma once

#include <atomic>
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class Sharpen final : public FlowCV::Node {
public:
    Sharpen();
    bool HasProcessingError() const { return processing_error_.load(); }

protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
    void RefreshProperties() override;

private:
    std::atomic<bool> processing_error_{false};
};
}
