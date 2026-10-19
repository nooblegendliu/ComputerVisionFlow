#pragma once
#include <flowNode.h>
#include <opencv2/core.hpp>
#include <atomic>
#include <ctime>
#include <mutex>

namespace DSPatch::DSPatchables {
class DrawDateTime final : public FlowCV::Node {
public:
    struct Settings {
        cv::Point position{10, 30};
        float scale = 1.f;
        int font = 0;
        int thickness = 2;
        cv::Vec3f color{1.f, 1.f, 1.f};
        bool show_date = true, show_time = true;
        std::string date_fmt = "%Y-%m-%d", time_fmt = " %X";
        std::string date_draft = date_fmt, time_draft = time_fmt;
        bool valid_date = true, valid_time = true;
    };
    DrawDateTime();
    Settings GetSettings() const;
    std::string GetState() override;
    void SetState(std::string&& state) override;
    void UpdateSettings(const nlohmann::json& changes);
    bool CommitFormat(int mode, std::string format);
    static bool IsFormatValid(int mode, const std::string& format);
    // Uses the same local-time strftime conversion as Process_; no clock ownership.
    static bool FormatTime(const Settings& settings, const std::tm& localTime, std::string& text);
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    mutable std::mutex settings_mutex_;
    Settings settings_;
    std::atomic<bool> processing_error_{false};
};
}
