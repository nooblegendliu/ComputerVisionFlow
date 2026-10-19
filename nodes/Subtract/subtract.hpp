#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class Subtract final : public FlowCV::Node {
public:
    Subtract();
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
