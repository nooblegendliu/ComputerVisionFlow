#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class GetSize final : public FlowCV::Node
{
public:
    GetSize();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
