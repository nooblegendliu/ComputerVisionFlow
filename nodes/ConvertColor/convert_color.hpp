#pragma once
#include <flowNode.h>
#include <atomic>

namespace DSPatch::DSPatchables {
class ConvertColor final : public FlowCV::Node
{
public:
    ConvertColor();
    bool HasChannelError() const { return channel_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    std::atomic<bool> channel_error_{false};
};
}
