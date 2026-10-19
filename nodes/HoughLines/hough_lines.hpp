#pragma once
#include <flowNode.h>
#include <atomic>

namespace DSPatch::DSPatchables {
class HoughLines final : public FlowCV::Node {
public:
    HoughLines();
    std::string GetState() override;
    void SetState(std::string&& state) override;
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
    void RefreshProperties() override;
private:
    std::atomic<bool> processing_error_{false};
};
}
