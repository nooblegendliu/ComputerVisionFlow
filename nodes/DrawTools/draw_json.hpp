#pragma once
#include <flowNode.h>
#include <opencv2/core.hpp>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace DSPatch::DSPatchables {
class DrawJson final : public FlowCV::Node {
public:
    struct JsonOutItem {
        int out_item_index = 0;
        std::string out_key_path;
        bool is_array = false;
    };
    struct Settings {
        cv::Point position{10, 30};
        float scale = 1.f;
        int font = 0;
        int thickness = 2;
        cv::Vec3f color{1.f, 1.f, 1.f};
        int json_data_index = 0;
        int decimal_places = 3;
        std::vector<JsonOutItem> out_list;
    };
    struct UiSnapshot {
        Settings settings;
        nlohmann::json data;
        uint64_t list_revision = 0;
    };
    DrawJson();
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
    Settings GetSettings() const;
    UiSnapshot GetUiSnapshot() const;
    void UpdateSettings(const nlohmann::json& changes);
    bool AddField(const std::string& path);
    bool RemoveField(size_t index, uint64_t revision);
    bool SetItemIndex(size_t index, int value, uint64_t revision);
    bool HasProcessingError() const { return processing_error_.load(); }
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
private:
    mutable std::mutex mutex_;
    Settings settings_;
    nlohmann::json json_data_;
    uint64_t list_revision_ = 0;
    std::atomic<bool> processing_error_{false};
};
}
