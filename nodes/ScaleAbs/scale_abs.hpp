#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class ScaleAbs final : public FlowCV::Node
{
public:
    ScaleAbs();
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
