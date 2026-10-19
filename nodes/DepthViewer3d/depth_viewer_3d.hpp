#pragma once
#include <flowNode.h>
#include <opencv2/core.hpp>
#include <mutex>
#include <vector>

namespace DSPatch::DSPatchables {
class DepthViewer3D final : public FlowCV::Node {
public:
    struct Vertex { float x=0,y=0,z=0,r=0,g=0,b=0,a=0,nx=0,ny=0,nz=0; };
    struct Snapshot {
        std::vector<Vertex> points;
        bool has_intrinsic=false;
        std::string error;
        uint64_t revision=0;
    };
    DepthViewer3D();
    std::string GetState() override;
    void SetState(std::string&&) override;
    nlohmann::json GetSettings() const;
    void UpdateSettings(const nlohmann::json&);
    void InitializeView();
    Snapshot GetSnapshot() const;
    static nlohmann::json DefaultCamera();
protected:
    void Process_(SignalBus const&, SignalBus&) override;
private:
    mutable std::mutex mutex_;
    nlohmann::json settings_;
    Snapshot snapshot_;
    bool view_initialized_=false;
};
}
