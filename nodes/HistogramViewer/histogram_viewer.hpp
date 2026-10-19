#pragma once
#include <flowNode.h>
#include <opencv2/core.hpp>
#include <mutex>
#include <vector>

namespace DSPatch::DSPatchables {
class HistogramViewer final : public FlowCV::Node {
public:
    struct Snapshot {
        bool has_input = false, is_color = true, error = false;
        std::vector<float> x, red, green, blue;
    };
    HistogramViewer();
    Snapshot GetSnapshot() const;
    std::string GetState() override { return "null"; }
    void SetState(std::string&& state) override { (void)nlohmann::json::parse(state); }
protected:
    void Process_(SignalBus const&, SignalBus&) override;
private:
    mutable std::mutex mutex_;
    Snapshot snapshot_;
};
}
