#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class AbsDiff final : public FlowCV::Node
{
public:
    AbsDiff();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
