#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class AddWeighted final : public FlowCV::Node
{
public:
    AddWeighted();
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
