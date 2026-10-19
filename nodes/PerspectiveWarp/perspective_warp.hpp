#pragma once
#include <flowNode.h>
#include <atomic>
#include <mutex>

namespace DSPatch::DSPatchables {
class PerspectiveWarp final : public FlowCV::Node
{
public:
    struct Parameters {
        float aspect_ratio_adj = 0;
        int interp_mode = 1;
        bool use_fixed_res = false;
        int fixed_width = 640;
        int fixed_height = 480;
    };
    PerspectiveWarp();
    Parameters GetParameters() const;
    bool HasQuadrilateral() const { return is_poly_.load(); }
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    mutable std::mutex parameters_mutex_;
    Parameters parameters_;
    std::atomic<bool> is_poly_{false};
};
}
