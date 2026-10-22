#pragma once

#include <flowNode.h>

namespace DSPatch::DSPatchables
{
    // VisionMaster-like condition evaluation, not conditional execution.
    // Receives one JSON object and emits a B1 inspection_result plus a 1/0/-1 flag.
    class ConditionCheck final : public FlowCV::Node
    {
    public:
        ConditionCheck();
        ~ConditionCheck() override = default;

    protected:
        void Process_(SignalBus const& inputs, SignalBus& outputs) override;
    };

}	// namespace DSPatch::DSPatchables