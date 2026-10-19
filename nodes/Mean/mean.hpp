#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class Mean final : public FlowCV::Node {
public:
    Mean();
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
