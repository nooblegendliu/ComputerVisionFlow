#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class Divide final : public FlowCV::Node
{
public:
    Divide();
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
