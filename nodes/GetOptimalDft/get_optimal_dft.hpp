#pragma once
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class GetOptimalDft final : public FlowCV::Node {
public:
    GetOptimalDft();
    std::string GetState() override;
    void SetState(std::string&& state) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
};
}
