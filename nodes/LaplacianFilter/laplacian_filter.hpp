#pragma once
#include <flowNode.h>
#include <atomic>
#include <mutex>

namespace DSPatch::DSPatchables {
class LaplacianFilter final : public FlowCV::Node {
public:
    LaplacianFilter();
    void SetState(std::string&& state) override;
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
    void RefreshProperties() override;
private:
    std::mutex parameter_mutex_;
    int last_kernel_ = 3;
    std::atomic<bool> processing_error_{false};
};
}
