#pragma once
#include <flowNode.h>
#include <atomic>

namespace DSPatch::DSPatchables {
class Split final : public FlowCV::Node {
public:
    Split();
    std::string GetState() override;
    void SetState(std::string&& state) override;
    bool IsColor() const { return is_color_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    std::atomic<bool> is_color_{false};
};
}
