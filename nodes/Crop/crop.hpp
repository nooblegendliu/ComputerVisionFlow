#pragma once
#include <flowNode.h>
#include <mutex>
#include <atomic>

namespace DSPatch::DSPatchables {
class Crop final : public FlowCV::Node {
public:
    struct Parameters {
        int x = 0, y = 0, width = 100, height = 100;
        bool calc_total_bbox = false;
        int json_bbox_index = 0, adjust_size_x = 0, adjust_size_y = 0;
    };
    Crop();
    Parameters GetParameters() const;
    bool HasJsonData() const { return has_json_data_.load(); }
    bool HasProcessingError() const { return processing_error_.load(); }
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    mutable std::mutex mutex_;
    Parameters parameters_;
    std::atomic_bool has_json_data_{false}, processing_error_{false};
};
}
