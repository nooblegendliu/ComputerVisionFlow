#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class Multiply final : public FlowCV::Node
{
public:
    Multiply();
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
