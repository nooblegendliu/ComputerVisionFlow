#pragma once

#include <iostream>
#include <memory>

#include <DSPatch.h>
#include <flowTypes.h>

namespace FlowCV
{
	class InternalNodeManager
	{
	public:
        InternalNodeManager();
        ~InternalNodeManager() = default;

        uint32_t NodeCount();

        bool GetNodeDescription(uint32_t index, NodeDescription& nodeDesc);

        bool GetNodeDescription(const char* name, NodeDescription& nodeDesc);

        std::shared_ptr<DSPatch::Component> CreateNodeInstance(const char* name);

        bool HasNode(const char* name);

        // 新增加函数
        template <typename T>
        void RegisterNode();

    private:
        using NodeFactory = std::function<std::shared_ptr<DSPatch::Component>()>;

        std::unordered_map<std::string, NodeFactory> factories_;

    private:
        std::vector<NodeDescription> node_list_;
	};  // class InternalNodeManager

    // template functions
    template <typename T>
    void InternalNodeManager::RegisterNode()
    {
        auto prototype = std::make_shared<T>();

        NodeDescription desc;

        desc.name = 
            prototype->GetComponentName();

        desc.category = 
            prototype->GetComponentCategory();

        desc.author =
            prototype->GetComponentAuthor();

        desc.version =
            prototype->GetComponentVersion();

        desc.input_count =
            prototype->GetInputCount();

        desc.output_count =
            prototype->GetOutputCount();

        factories_[desc.name] = []() {
            return std::make_shared<T>();
        };

        node_list_.push_back(std::move(desc));
    }






}	// namespace FlowCV