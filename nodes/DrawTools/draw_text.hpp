#pragma once
#include <flowNode.h>
#include <opencv2/core.hpp>
#include <atomic>
#include <mutex>

namespace DSPatch::DSPatchables {
class DrawText final : public FlowCV::Node {
public:
    struct Settings {
        std::string text;
        cv::Point position{10, 30};
        float scale = 1.f;
        int font = 0;
        int thickness = 2;
        cv::Vec3f color{1.f, 1.f, 1.f}; // RGB, as stored in FlowCV state.
        bool has_string_input = false;
    };
    DrawText();
    std::string GetState() override;
    void SetState(std::string&& state) override;
    Settings GetSettings() const;
    // UI edits are partial updates; unlike SetState they preserve omitted text.
    void UpdateSettings(const nlohmann::json& changes);
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    static void ApplyChanges(Settings& settings, const nlohmann::json& changes);
    mutable std::mutex settings_mutex_;
    Settings settings_;
    std::atomic<bool> processing_error_{false};
};
}
