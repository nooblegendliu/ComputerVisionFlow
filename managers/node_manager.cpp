#include "node_manager.h"

using namespace FlowCV;

InternalNodeManager::InternalNodeManager() = default;


// 获取节点数量
uint32_t InternalNodeManager::NodeCount()
{
	return static_cast<uint32_t>(node_list_.size());
}

// 判断是否存在节点
bool InternalNodeManager::HasNode(const char* name)
{
	if (name == nullptr) return false;

	return factories_.find(name) != factories_.end();
}

// 获取节点说明信息
bool InternalNodeManager::GetNodeDescription(uint32_t index, NodeDescription& nodeDesc)
{
	if (index >= node_list_.size()) return false;

	nodeDesc = node_list_[index];

	return true;
}

bool InternalNodeManager::GetNodeDescription(const char* name, NodeDescription& nodeDesc)
{
	if (name == nullptr) return false;

	for (const auto& desc : node_list_) {
		if (desc.name == name) {
			nodeDesc = desc;
			return true;
		}
	}
	return false;
}

// 创建节点实例
std::shared_ptr<DSPatch::Component> InternalNodeManager::CreateNodeInstance(const char* name)
{
	if (name == nullptr) return nullptr;

	const auto it = factories_.find(name);
	if (it == factories_.end()) return nullptr;

	return it->second();
}