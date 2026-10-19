#pragma once
#include <flowNode.h>
#include <atomic>

namespace DSPatch::DSPatchables {
class Bitwise final : public FlowCV::Node {
public:
    Bitwise();
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    std::atomic_bool processing_error_{false};
};
}
