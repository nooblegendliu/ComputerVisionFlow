#pragma once
#include <flowNode.h>
#include <atomic>

namespace DSPatch::DSPatchables {
class Threshold final : public FlowCV::Node
{
public:
    Threshold();
    std::string GetState() override;
    void SetState(std::string&& state) override;
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    std::atomic<bool> processing_error_{false};
};
}
