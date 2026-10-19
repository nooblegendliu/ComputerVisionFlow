// Shared safety and UI lifetime adapter for the original DataOutput plugins.
#pragma once
#include <flowNode.h>
#include <mutex>
#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <cstring>
#include <climits>
#include <filesystem>
#include <stdexcept>
#include <initializer_list>
struct OutputUiBridge {
    std::mutex mutex;
    FlowCV::Node* node = nullptr;
    std::function<std::string()> status;
    std::function<std::vector<std::string>()> ports;
    std::function<void()> refreshPorts;
    std::function<bool()> multicast;
    std::function<void(const nlohmann::json&)> updateControls;
};
inline void InvalidateUi(const std::shared_ptr<OutputUiBridge>& ui) {
    std::lock_guard lock(ui->mutex); ui->node=nullptr; ui->status={}; ui->ports={}; ui->refreshPorts={}; ui->multicast={}; ui->updateControls={};
}
inline bool OutputSettingsChanged(const nlohmann::json& current, const nlohmann::json& patch,
    std::initializer_list<const char*> keys) {
    for (const auto* key : keys)
        if (patch.contains(key) && patch.at(key) != current.at(key)) return true;
    return false;
}
inline std::string OutputPathUtf8(const std::filesystem::path& path) {
    const auto text=path.u8string(); return std::string(reinterpret_cast<const char*>(text.data()),text.size());
}
// Validate a patch before touching settings or live handles. Unknown keys remain ignored, as upstream.
inline void ValidateOutputState(nlohmann::json current, const nlohmann::json& patch) {
    if (!patch.is_object()) throw std::invalid_argument("DataOutput state must be an object");
    for (auto i=patch.begin();i!=patch.end();++i) {
        if (!current.contains(i.key())) continue;
        const auto& old=current.at(i.key()); const auto& value=i.value();
        if (old.is_boolean() && !value.is_boolean()) throw std::invalid_argument("Expected boolean: "+i.key());
        if (old.is_string() && !value.is_string()) throw std::invalid_argument("Expected string: "+i.key());
        if (old.is_number_integer() && (!value.is_number_integer() || value.get<double>()<INT_MIN || value.get<double>()>INT_MAX)) throw std::invalid_argument("Expected integer: "+i.key());
        current[i.key()]=value;
    }
    const auto range=[&](const char* key,int low,int high) {
        if(current.contains(key)) { const auto v=current.at(key).get<int>(); if(v<low || v>high) throw std::invalid_argument(std::string("Invalid ")+key); }
    };
    range("port",0,65535); range("data_mode",0,4); range("data_rate",0,8); range("eol_seq",0,6);
    range("baud_rate_idx",0,19); range("data_bits_idx",0,3); range("parity_idx",0,2); range("stop_bits_idx",0,2); range("flow_ctrl_idx",0,2);
    range("max_file_size_kb",1,INT_MAX); range("num_files",1,INT_MAX);
}
