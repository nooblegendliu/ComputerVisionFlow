#pragma once
#include <flowNode.h>
#include <array>
#include <atomic>
#include <mutex>

namespace DSPatch::DSPatchables {
class LineIntersect final : public FlowCV::Node {
public:
    LineIntersect();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
    int LineCount() const { return line_count_.load(); }
    int IntersectionCount() const { return intersect_count_.load(); }
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    struct Parameters {
        float min_angle = 60, min_dist = 10;
        bool draw_points = true, point_solid = false;
        int point_radius = 4, point_thickness = 1;
        std::array<float, 3> point_color{0, 1, 1};
    } parameters_;
    std::mutex mutex_;
    std::atomic<int> line_count_{0}, intersect_count_{0};
    std::atomic<bool> processing_error_{false};
};
}
