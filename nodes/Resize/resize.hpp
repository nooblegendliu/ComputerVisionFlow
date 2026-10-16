#pragma once

#include <atomic>
#include <string>

#include <flowNode.h>

namespace DSPatch::DSPatchables
{
    class Resize final : public FlowCV::Node
    {
    public:
        Resize();
        ~Resize() override = default;

        // Keep the original FlowCV state keys: width, height, mode.
        std::string GetState() override;
        void SetState(std::string&& serialized) override;

    protected:
        void Process_(SignalBus const& inputs, SignalBus& outputs) override;
        void RefreshProperties() override;

    private:
        // These flags are updated by the processing thread and read by the UI.
        std::atomic<bool> has_size_input_{ false };
        std::atomic<bool> has_ref_frame_{ false };
    };
}
