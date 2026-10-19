#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class Min final : public FlowCV::Node
{
public:
    Min();
    std::string GetState() override;
    void SetState(std::string&& state) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
