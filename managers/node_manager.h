#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <type_traits>
#include <stdexcept>
#include <memory>

#include <DSPatch.h>
#include <flowTypes.h>
#include <flowNode.h>

namespace FlowCV
{
	class InternalNodeManager
	{
	public:
        InternalNodeManager();

        ~InternalNodeManager() = default;

        uint32_t NodeCount() const;

        bool GetNodeDescription(uint32_t index, NodeDescription& nodeDesc) const;

        bool GetNodeDescription(const char* name, NodeDescription& nodeDesc) const;

        //std::shared_ptr<DSPatch::Component> CreateNodeInstance(const char* name);
        std::shared_ptr<FlowCV::Node> CreateNodeInstance(const char* name) const;

        bool HasNode(const char* name) const;

        /*
        *   注册一个内置节点类型
        *   
        *   T必须：
        *   1.继承FlowCV::Node
        *   2.可默认构造
        */
        template <typename T>
        void RegisterNode();

    private:
        //using NodeFactory = std::function<std::shared_ptr<DSPatch::Component>()>;
        using NodeFactory = std::function<std::shared_ptr<FlowCV::Node>()>;

        std::unordered_map<std::string, NodeFactory> factories_;

    private:
        std::vector<NodeDescription> node_list_;
	};  // class InternalNodeManager

    // template functions
    template <typename T>
    void InternalNodeManager::RegisterNode()
    {
        // 添加继承正确性检查
        static_assert(std::is_base_of_v<FlowCV::Node, T>, "Internal Node must be derive from FlowCV::Node");
        
        static_assert(std::is_default_constructible_v<T>, "Internal Node must be default constructible");

        // 构造prototype目的: 读取节点元数据
        auto prototype = std::make_shared<T>();

        NodeDescription desc;
        desc.name = prototype->GetComponentName();
        desc.category = prototype->GetComponentCategory();
        desc.author = prototype->GetComponentAuthor();
        desc.version = prototype->GetComponentVersion();
        desc.input_count = prototype->GetInputCount();
        desc.output_count = prototype->GetOutputCount();

        if (desc.name.empty()) throw std::logic_error("Can't register an unnamed node");

        /*
        * 单节点对应一个Factory
        */
        auto [factoryIt, inserted] = factories_.emplace(desc.name,
            []() -> std::shared_ptr<FlowCV::Node>
            {
                return std::make_shared<T>();
            });
        if (!inserted) {
            throw std::logic_error("Node already register: " + desc.name);
        }

        /*
        * 保证factoroes_与node_list_尽量保持一致
        */
        try {
            node_list_.push_back(std::move(desc));
        }
        catch (...) {
            factories_.erase(factoryIt);
            throw;
        }
    }






}	// namespace FlowCV