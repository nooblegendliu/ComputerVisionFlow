// Adapted from FlowCV Plugins/ShapeColorizer (GPL-3.0).
#pragma once
#include <flowNode.h>
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace DSPatch::DSPatchables {
namespace internal {
struct ColorMatch { std::array<float, 3> color{0.5f, 0.5f, 0.5f}; std::string label; };
struct ShapeColorizerParameters {
    int method = 0, threshold = 75, dist_method = 0;
    bool overlay = true, show_labels = false, ignore_none = true;
    std::vector<ColorMatch> colors;
    // Runtime generation invalidates callbacks from rows removed or restored.
    uint64_t listRevision = 0;
};
struct ShapeColorizerState {
    mutable std::mutex mutex;
    ShapeColorizerParameters parameters;
    std::string error;
    ShapeColorizerParameters Snapshot() const;
    void Apply(const nlohmann::json& patch);
    nlohmann::json Serialize() const;
};
}
class ShapeColorizer : public FlowCV::Node {
public:
    ShapeColorizer();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
    std::shared_ptr<internal::ShapeColorizerState> state_;
};
}
