#pragma once

#include <flowNode.h>

namespace DSPatch::DSPatchables
{
    // Retain the original FlowCV class name and component name ("Canny").
    class CannyFilter final : public FlowCV::Node
    {
    public:
        CannyFilter();
        ~CannyFilter() override = default;

    protected:
        void Process_(SignalBus const& inputs, SignalBus& outputs) override;
        void RefreshProperties() override;
    };
}
