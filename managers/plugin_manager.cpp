#include "plugin_manager.h"
#include <filesystem>
#include <algorithm>
#include <memory>

#include <logging/logger.h>

namespace FlowCV
{
	PluginManager::~PluginManager()
	{
		UnLoadPlugins();
	}


	static bool compareName(const PluginInfo& a, const PluginInfo& b)
	{
		return a.plugin_desc.name < b.plugin_desc.name;
	}


	void PluginManager::ScanDirForPlugins(const char* dir_path, bool recursive)
	{
        namespace fs = std::filesystem;

        if (dir_path == nullptr || *dir_path == '\0')
            return;

        if (!fs::is_directory(dir_path))
            return;

        for (const auto& entry : fs::directory_iterator(dir_path))
        {
            if (entry.is_directory())
            {
                if (recursive)
                {
                    const auto subdir = entry.path().string();
                    ScanDirForPlugins(subdir.c_str(), recursive);
                }
                continue;
            }

            if (!entry.is_regular_file())
                continue;

            // 保留FlowCV的 .fp 扩展名
            if (entry.path().extension() != ".fp") continue;

            const std::string filename = entry.path().string();

            LOG_INFO("Loading plugin: {}", filename);

            // 初始化期间使用RAII管理句柄
            auto plugin = std::make_unique<DSPatch::Plugin>(filename);

            if (!plugin->IsLoaded()) continue;

            // 临时实例用于读取节点信息
            auto instance = plugin->Create();

            if (!instance) {
                LOG_WARN("Failed to create plugin: {}", filename);
                continue;
            }

            PluginInfo info{};

            info.plugin_handle = plugin.get();
            info.plugin_desc.name = instance->GetComponentName();
            info.plugin_desc.category = instance->GetComponentCategory();
            info.plugin_desc.author = instance->GetComponentAuthor();
            info.plugin_desc.version = instance->GetComponentVersion();
            info.plugin_desc.input_count = instance->GetInputCount();
            info.plugin_desc.output_count = instance->GetOutputCount();
            
            // 任然使用原版插件容器
            plugins_.push_back(info);

            // 确保临时组件先于动态库释放
            instance.reset();

            // 所有权转移至plugins_
            plugin.release();
        }

        // 保留FlowCV排序行为
        std::sort(plugins_.begin(), plugins_.end(), compareName);
	}

    void PluginManager::LoadPlugins(const char* plugin_path, bool recursive)
    {
        if (plugin_path == nullptr || *plugin_path == '\0')
            return;

        plugin_path_ = plugin_path;

        LOG_INFO("Looking for Plugins in: {}", plugin_path);

        ScanDirForPlugins(plugin_path_.c_str(), recursive);
    }

    void PluginManager::UnLoadPlugins()
    {
        for (auto& plugin : plugins_)
        {
            delete plugin.plugin_handle;

            plugin.plugin_handle = nullptr;
        }

        plugins_.clear();

        plugin_path_.clear();
    }

    uint32_t PluginManager::PluginCount()
    {
        return plugins_.size();
    }

    bool PluginManager::GetPluginDescription(uint32_t index, NodeDescription& nodeDesc)
    {

        if (index >= 0 && index < plugins_.size()) {
            nodeDesc.name = plugins_.at(index).plugin_desc.name;
            nodeDesc.category = plugins_.at(index).plugin_desc.category;
            nodeDesc.author = plugins_.at(index).plugin_desc.author;
            nodeDesc.version = plugins_.at(index).plugin_desc.version;
            nodeDesc.input_count = plugins_.at(index).plugin_desc.input_count;
            nodeDesc.output_count = plugins_.at(index).plugin_desc.output_count;
            return true;
        }

        return false;
    }

    bool PluginManager::GetPluginDescription(const char* name, NodeDescription& nodeDesc)
    {

        for (auto const& p : plugins_) {
            if (p.plugin_desc.name == name) {
                nodeDesc.name = p.plugin_desc.name;
                nodeDesc.category = p.plugin_desc.category;
                nodeDesc.author = p.plugin_desc.author;
                nodeDesc.version = p.plugin_desc.version;
                nodeDesc.input_count = p.plugin_desc.input_count;
                nodeDesc.output_count = p.plugin_desc.output_count;
                return true;
            }
        }

        return false;
    }

    std::shared_ptr<DSPatch::Component> PluginManager::CreatePluginInstance(const char* name)
    {
        for (auto const& p : plugins_) {
            if (p.plugin_desc.name == name) {
                std::shared_ptr<DSPatch::Component> pi = p.plugin_handle->Create();
                return pi;
            }
        }
        return nullptr;
    }

    bool PluginManager::HasPlugin(const char* name)
    {
        for (auto const& p : plugins_) {
            if (p.plugin_desc.name == name) {
                return true;
            }
        }

        return false;
    }

}	// namespace FlowCV