#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <utility>
namespace DSPatch::DSPatchables {
// Optional host controls use this non-Qt interface; inference owns no GUI objects.
class DnnControlInterface {
public:
    virtual ~DnnControlInterface() = default;
    virtual nlohmann::json GetControlState() = 0;
    virtual void UpdateControlState(const nlohmann::json& patch) = 0;
    virtual bool InitializeNetwork() = 0;
    virtual nlohmann::json GetNetworkStatus() = 0;
    virtual std::vector<std::pair<std::string, int>> GetBackendOptions() = 0;
    virtual std::vector<std::pair<std::string, int>> GetTargetOptions() = 0;
};
struct DnnColor { float x=0,y=0,z=0,w=1; DnnColor()=default; DnnColor(float r,float g,float b,float a):x(r),y(g),z(b),w(a){} };
}
