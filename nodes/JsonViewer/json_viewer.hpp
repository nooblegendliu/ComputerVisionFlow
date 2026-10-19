#pragma once
#include <flowNode.h>
#include <mutex>

namespace DSPatch::DSPatchables {
class JsonViewer final : public FlowCV::Node
{
public:
    JsonViewer();
    nlohmann::json GetJsonCopy() const;
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    mutable std::mutex io_mutex_;
    nlohmann::json json_data_;
};
}
