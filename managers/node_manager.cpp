#include "node_manager.h"

#include <algorithm>
#include <internal_nodes.h>

namespace FlowCV
{
	InternalNodeManager::InternalNodeManager()
	{
		/*
		*  所有已完成的新接口的InternalNodes在此注册
		*/
		RegisterNode<DSPatch::DSPatchables::Add>();
		RegisterNode<DSPatch::DSPatchables::Solid>();
		RegisterNode<DSPatch::DSPatchables::Viewer>();
		//...

		std::sort(node_list_.begin(), node_list_.end(),
			[](const NodeDescription& lhs, const NodeDescription& rhs)
			{
				return lhs.name < rhs.name;
			});
	}


	// 获取节点数量
	uint32_t InternalNodeManager::NodeCount() const
	{
		return static_cast<uint32_t>(node_list_.size());
	}

	// 判断是否存在节点
	bool InternalNodeManager::HasNode(const char* name) const
	{
		if (name == nullptr || *name == '\0') return false;

		return factories_.find(name) != factories_.end();
	}

	// 获取节点说明信息
	bool InternalNodeManager::GetNodeDescription(uint32_t index, NodeDescription& nodeDesc) const
	{
		if (index >= node_list_.size()) return false;

		nodeDesc = node_list_[index];

		return true;
	}

	bool InternalNodeManager::GetNodeDescription(const char* name, NodeDescription& nodeDesc) const
	{
		if (name == nullptr || *name == '\0') return false;

		const auto it = std::find_if(node_list_.begin(), node_list_.end(),
			[name](const NodeDescription& desc)
			{
				return desc.name == name;
			});

		if (it == node_list_.end()) return false;

		nodeDesc = *it;
		return true;
	}

	// 创建节点实例
	std::shared_ptr<FlowCV::Node> InternalNodeManager::CreateNodeInstance(const char* name) const
	{
		if (name == nullptr || *name == '\0') return nullptr;

		const auto it = factories_.find(name);

		if (it == factories_.end()) return nullptr;

		return it->second();
	}

}	// namespace FlowCV

