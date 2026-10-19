#pragma once
#include <flowNode.h>
#include <array>
#include <mutex>

namespace DSPatch::DSPatchables {
class Contours final : public FlowCV::Node
{
public:
    struct Parameters {
        int mode = 2;
        int method = 1;
        bool filter_by_area = true;
        float area_min = 100;
        float area_max = 1000;
        bool draw_contours = true;
        bool fill_contours = false;
        bool draw_bbox = false;
        std::array<float, 3> contour_color{0, 1, 0};
        std::array<float, 3> bbox_color{0, 0, 1};
    };
    Contours();
    Parameters GetParameters() const;
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    mutable std::mutex parameters_mutex_;
    Parameters parameters_;
};
}
