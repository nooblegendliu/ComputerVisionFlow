#pragma once

#include <atomic>
#include <mutex>
#include <flowNode.h>

namespace DSPatch::DSPatchables {
class Transform final : public FlowCV::Node {
public:
    Transform();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
    bool HasProcessingError() const { return processing_error_.load(); }

protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
    void RefreshProperties() override;

private:
    void UpdateMetadataLocked();
    std::mutex parameter_mutex_;
    int ui_scale_mode_ = 0;
    bool has_frame_metadata_ = false;
    std::atomic<bool> processing_error_{false};
};
}
