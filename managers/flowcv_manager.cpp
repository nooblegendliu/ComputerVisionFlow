#include "flowcv_manager.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <utility>

#include <logging/logger.h>

namespace FlowCV
{
	FlowCV_Manager::FlowCV_Manager()
        : id_counter_(1001), 
        wire_id_counter_(500), 
        circuit_(std::make_shared<DSPatch::Circuit>()),
        plugin_manager_(std::make_shared<PluginManager>()),
        internal_node_manager_(std::make_shared<InternalNodeManager>())
	{}


	FlowCV_Manager::~FlowCV_Manager()
	{
		/*nodes_.clear();
		circuit_->RemoveAllComponents();*/
        if (circuit_) {
            circuit_->StopAutoTick();
            circuit_->RemoveAllComponents();
        }
        wiring_.clear();
        nodes_.clear();
	}


	uint64_t FlowCV_Manager::GetNextId()
	{
		uint64_t nextId = id_counter_;

		for (const auto& node : nodes_) {
			if (node.id >= nextId) {
                if (node.id > std::numeric_limits<uint64_t>::max() - 1000)
                {
                    throw std::overflow_error("Node id overflow");
                }
				nextId = node.id + 1000;
			}
		}
		id_counter_ = nextId;
		return nextId;
	}


    uint64_t FlowCV_Manager::AddNewNodeInstance(const char* name, bool external_plugin, uint64_t id)
    {
        if (name == nullptr || *name == '\0') return 0;

        /*
        *   State加载时允许指定ID
        *   但不允许ID冲突
        */
        if (id != 0 && GetNodeIndexFromId(id) >= 0) {
            LOG_WARN("Node id already exists: {}", id);
            return 0;
        }

        NodeInfo nodeInfo;

        if (external_plugin) {
            nodeInfo.node_ptr = plugin_manager_->CreatePluginInstance(name);
        }
        else {
            nodeInfo.node_ptr = internal_node_manager_->CreateNodeInstance(name);
        }

        if (!nodeInfo.node_ptr) {
            LOG_WARN("Unable to create node: {}", name);
            return 0;
        }

        nodeInfo.desc.name = nodeInfo.node_ptr->GetComponentName();
        nodeInfo.desc.category = nodeInfo.node_ptr->GetComponentCategory();
        nodeInfo.desc.author = nodeInfo.node_ptr->GetComponentAuthor();
        nodeInfo.desc.version = nodeInfo.node_ptr->GetComponentVersion();
        nodeInfo.desc.input_count = nodeInfo.node_ptr->GetInputCount();
        nodeInfo.desc.output_count = nodeInfo.node_ptr->GetOutputCount();
        nodeInfo.input_conn_map.resize(static_cast<std::size_t>(nodeInfo.desc.input_count));

        if (id == 0) {
            nodeInfo.id = GetNextId();
        }
        else {
            nodeInfo.id = id;
        }

        /*
        *   保证同一类型Node的Instance count唯一
        */
        CheckInstCountValue(nodeInfo);

        // fix: 只有Circuit成功接收节点才能进入nodes_
        const int componentIndex = circuit_->AddComponent(nodeInfo.node_ptr);
        if (componentIndex < 0) {
            LOG_ERROR("Failed to add node to DSPatch circuit: {}", name);
            return 0;
        }

        const uint64_t newId = nodeInfo.id;
        nodes_.push_back(std::move(nodeInfo));
        return newId;
    }


    uint64_t FlowCV_Manager::CreateNewNodeInstance(const char* name)
    {
        if (name == nullptr || *name == '\0') return 0;

        // 暂时保持FlowCV原行为: plugin优先于Internal Node
        if (plugin_manager_->HasPlugin(name)) {
            return AddNewNodeInstance(name, true);
        }
        if (internal_node_manager_->HasNode(name)) {
            return AddNewNodeInstance(name, false);
        }

        LOG_WARN("Node not found: {}", name);
        return 0;
    }


    // FlowCV节点实例编号管理逻辑，保证同一类型Node拥有图中唯一递增的实例编号
    void FlowCV_Manager::CheckInstCountValue(NodeInfo& nodeInfo)
    {
        int currentNumber =
            nodeInfo.node_ptr
            ->GetInstanceCount();

        if (currentNumber < 0) {
            currentNumber = 0;
        }

        for (const auto& node : nodes_) {
            if (nodeInfo.desc.name
                == node.desc.name &&
                nodeInfo.id != node.id) {

                const int existingNumber =
                    node.node_ptr
                    ->GetInstanceCount();

                if (currentNumber
                    <= existingNumber) {
                    currentNumber =
                        existingNumber + 1;
                }
            }
        }

        nodeInfo.node_ptr
            ->SetInstanceCount(
                currentNumber);
    }


    void FlowCV_Manager::SetBufferCount(uint32_t num_buffers)
    {
        if (num_buffers > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
            throw std::out_of_range("DSPatch buffer count exceeds int range");
        }
        circuit_->SetBufferCount(static_cast<int>(num_buffers));
    }


    int FlowCV_Manager::GetBufferCount()
    {
        return circuit_->GetBufferCount();
    }

    
    // 获取节点数量
    uint64_t FlowCV_Manager::GetNodeCount()
    {
        return static_cast<uint64_t>(nodes_.size());
    }


    // 根据索引获取对应节点信息
    bool FlowCV_Manager::GetNodeInfoByIndex(uint64_t index, NodeInfo& nodeInfo)
    {
        if (index >= nodes_.size()) return false;
        nodeInfo = nodes_[static_cast<std::size_t>(index)];

        return true;
    }


    bool FlowCV_Manager::GetNodeInfoById(uint64_t id, NodeInfo& nInfo)
    {
        const int index = GetNodeIndexFromId(id);
        
        if (index < 0) return false;

        return GetNodeInfoByIndex(static_cast<uint64_t>(index), nInfo);
    }


    int FlowCV_Manager::GetNodeIndexFromId(uint64_t id)
    {

        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            if (nodes_[i].id == id) {
                return static_cast<int>(i);
            }
        }
        /*
        *   原FlowCV返回0, 会将找不到误认为第一个节点
        */
        return -1;
    }


    uint64_t FlowCV_Manager::GetNodeIdFromIndex(uint64_t index)
    {
        if (index >= 0 && index < nodes_.size()) {
            return nodes_.at(index).id;
        }
        return 0;
    }


    std::shared_ptr<FlowCV::Node> FlowCV_Manager::GetFlowNodeById(uint64_t id)
    {
        const int index = GetNodeIndexFromId(id);

        if (index < 0) {
            return nullptr;
        }

        return std::dynamic_pointer_cast<FlowCV::Node>(
            nodes_[static_cast<std::size_t>(index)].node_ptr);
    }


    // ------------------------------------------------------------------------------
    //  Node Property Functions
    // ------------------------------------------------------------------------------

    bool FlowCV_Manager::GetNodePropertiesSnapshot(uint64_t node_id, PropertiesSnapshot& snapshot)
    {
        auto node = GetFlowNodeById(node_id);

        if (!node) {
            return false;
        }

        snapshot = node->GetPropertiesSnapshot();

        return true;
    }


    bool FlowCV_Manager::SetNodeProperty(
        uint64_t node_id,
        const std::string& key,
        bool value)
    {
        auto node = GetFlowNodeById(node_id);

        if (!node)
            return false;


        if (!node->Properties().Exists(key))
            return false;

        try {
            node->Properties().Set(key, value);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR(
                "Failed to set property '{}': {}",
                key,
                e.what());

            return false;
        }
    }


    bool FlowCV_Manager::SetNodeProperty(
            uint64_t node_id,
            const std::string& key,
            int value)
    {
        auto node = GetFlowNodeById(node_id);

        if (!node) 
            return false;

        if (!node->Properties().Exists(key))
            return false;


        try {
            node->Properties().Set(key, value);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR(
                "Failed to set property '{}': {}",
                key,
                e.what());

            return false;
        }
    }


    bool FlowCV_Manager::SetNodeProperty(
            uint64_t node_id,
            const std::string& key,
            float value)
    {
        auto node = GetFlowNodeById(node_id);

        if (!node) 
            return false;

        if (!node->Properties().Exists(key))
            return false;


        try {
            node->Properties().Set(key, value);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR(
                "Failed to set property '{}': {}",
                key,
                e.what());

            return false;
        }
    }


    // -----------------------------------------------------------------
    //  State Functions
    // -----------------------------------------------------------------
    nlohmann::json FlowCV_Manager::GetState()
    {
        using nlohmann::json;

        json state =
            json::object();

        json nodeStates =
            json::array();

        json connectionStates =
            json::array();

        for (const auto& node : nodes_) {
            json serializedNode =
                json::object();

            serializedNode["name"] =
                node.desc.name;

            serializedNode["id"] =
                node.id;

            serializedNode["num"] =
                node.node_ptr
                ->GetInstanceCount();

            serializedNode["enabled"] =
                node.node_ptr
                ->IsEnabled();

            const std::string stateString =
                node.node_ptr
                ->GetState();

            if (!stateString.empty()) {
                serializedNode["params"] =
                    json::parse(
                        stateString);
            }

            nodeStates.push_back(
                std::move(serializedNode));
        }

        state["nodes"] =
            std::move(nodeStates);


        for (const auto& wire : wiring_) {
            json serializedWire =
                json::object();

            serializedWire["from_id"] =
                wire.from.id;

            serializedWire["from_idx"] =
                wire.from.index;

            serializedWire["to_id"] =
                wire.to.id;

            serializedWire["to_idx"] =
                wire.to.index;

            connectionStates.push_back(
                std::move(serializedWire));
        }

        state["connections"] =
            std::move(connectionStates);

        return state;
    }


    bool FlowCV_Manager::SetState(const nlohmann::json& state)
    {
        if (!state.is_object()) {
            return false;
        }

        // 兼容原版FlowCV的空节点列表
        if (state.contains("nodes") && !state["nodes"].is_null() && !state["nodes"].is_array())
            return false;

        // 兼容原版的空连接列表
        if (state.contains("connections") && !state["connections"].is_null() && !state["connections"].is_array())
            return false;

        bool success = true;

        try {
            // 沿用原版FlowCV的清空后加载流程
            NewState();

            // 恢复节点
            if (state.contains("nodes") && state["nodes"].is_array())
            {
                for (const auto& node : state["nodes"])
                {
                    const uint64_t id = node.at("id").get<uint64_t>();

                    const std::string name = node.at("name").get<std::string>();

                    const int instanceNumber = node.value("num", 0);

                    const bool enabled = node.value("enabled", true);

                    // ID=0在当前Manager中保留为无效值
                    if (id == 0) {
                        success = false;
                        continue;
                    }

                    bool externalPlugin = false;

                    // 保留原版优先查找plugin的顺序
                    if (plugin_manager_->HasPlugin(name.c_str())) {
                        externalPlugin = true;
                    }
                    else if (internal_node_manager_->HasNode(name.c_str())) {
                        externalPlugin = false;
                    }
                    else {
                        LOG_WARN("Node not found: {}", name);
                        success = false;
                        continue;
                    }

                    const uint64_t newId = AddNewNodeInstance(name.c_str(), externalPlugin, id);
                    if (newId == 0) {
                        success = false;
                        continue;
                    }

                    const int nodeIndex = GetNodeIndexFromId(newId);
                    if (nodeIndex < 0) {
                        success = false;
                        continue;
                    }

                    auto& nodeInfo = nodes_[static_cast<std::size_t>(nodeIndex)];

                    // 恢复实例编号
                    nodeInfo.node_ptr->SetInstanceCount(instanceNumber);

                    CheckInstCountValue(nodeInfo);

                    // 恢复节点启用状态
                    nodeInfo.node_ptr->SetEnabled(enabled);

                    // 恢复节点属性
                    if (node.contains("params")) {
                        nodeInfo.node_ptr->SetState(node["params"].dump());
                    }
                }
            }

            // 恢复连接
            if (state.contains("connections") && state["connections"].is_array())
            {
                for (const auto& conn : state["connections"]) {
                    const uint64_t fromId = conn.at("from_id").get<uint64_t>();

                    const uint32_t fromIndex = conn.at("from_idx").get<uint32_t>();

                    const uint64_t toId = conn.at("to_id").get<uint64_t>();

                    const uint32_t toIndex = conn.at("to_idx").get<uint32_t>();

                    if (!ConnectNodes(fromId, fromIndex, toId, toIndex))
                    {
                        success = false;
                    }
                }
            }

        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to load state: {}", e.what());

            return false;
        }   // try-catch

        return success;
    }   


    void FlowCV_Manager::NewState()
    {
        circuit_->RemoveAllComponents();

        wiring_.clear();

        nodes_.clear();
    }


    bool FlowCV_Manager::LoadState(const char* filepath)
    {
        if (filepath == nullptr || *filepath == '\0')
            return false;

        try {
            std::ifstream input(filepath);

            if (!input.is_open()) {
                LOG_ERROR("Unable to open state file: {}", filepath);
                return false;
            }
            nlohmann::json state;
            input >> state;
            return SetState(state);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to load state file '{}': {}",
                filepath,
                e.what());
            return false;
        }
    }


    bool FlowCV_Manager::SaveState(const char* filepath)
    {
        if (filepath == nullptr || *filepath == '\0') {
            return false;
        }

        try {
            std::ofstream output(filepath);

            if (!output.is_open())
                return false;

            const nlohmann::json state =
                GetState();

            output << std::setw(4)
                << state
                << '\n';

            output.close();

            return !output.fail();
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to save state: {}", e.what());

            return false;
        }
    }

    // ------------------------------------------------------------------
    //  Connection Functions
    // ------------------------------------------------------------------
    bool FlowCV_Manager::ConnectNodes(
        uint64_t from_id, 
        uint32_t from_out_idx, 
        uint64_t to_id, 
        uint32_t to_in_idx)
    {
        // 检查节点ID
        if (from_id == 0 || to_id == 0) return false;

        const int fromIndex = GetNodeIndexFromId(from_id);
        const int toIndex = GetNodeIndexFromId(to_id);
        if (fromIndex < 0 || toIndex < 0) return false;

        auto& fromNode = nodes_[static_cast<std::size_t>(fromIndex)];
        auto& toNode = nodes_[static_cast<std::size_t>(toIndex)];

        // 检查端口索引
        if (from_out_idx >= static_cast<uint32_t>(fromNode.desc.output_count))
            return false;

        if (to_in_idx >= static_cast<uint32_t>(toNode.desc.input_count))
            return false;

        /*
        *   DSPatch 一个Input只能接收一个Source
        *   - 若已有旧连接, Manager自己的wiring_也必须同步清理
        *   - 保留FlowCV编辑器原有的类型匹配原则
        */
        const auto outputType = fromNode.node_ptr->GetOutputType(static_cast<int>(from_out_idx));
        const auto inputType = toNode.node_ptr->GetInputType(static_cast<int>(to_in_idx));
        if (outputType != inputType) {
            LOG_WARN("Node connection type mismatch");
            return false;
        }

        // 检查是否已经存在相同连接
        const auto existingWire = std::find_if(wiring_.begin(), wiring_.end(),
            [&](const Wire& wire)
            {
                return wire.to.id == to_id && wire.to.index == to_in_idx;
            }
        );

        if (existingWire != wiring_.end() &&
            existingWire->from.id == from_id &&
            existingWire->from.index == from_out_idx)
            return true;

        // DSPatch 内部负责替换该输入端口的旧连接
        const bool connected = circuit_->ConnectOutToIn(
            fromNode.node_ptr,
            static_cast<int>(from_out_idx),
            toNode.node_ptr,
            static_cast<int>(to_in_idx));

        if (!connected) return false;


        // DSPatch已完成连接替换
        // 现在同步Manager自身的记录
        wiring_.erase(std::remove_if(wiring_.begin(), wiring_.end(),
            [&](const Wire& wire)
            {
                return wire.to.id == to_id && wire.to.index == to_in_idx;
            }),
            wiring_.end());

        // 更新目标节点连接信息
        toNode.input_conn_map[to_in_idx].id = from_id;
        toNode.input_conn_map[to_in_idx].index = from_out_idx;

        // 创建新的Wire
        Wire wire;

        wire.id = wire_id_counter_;

        wire.from.id = from_id;
        wire.from.index = from_out_idx;

        wire.to.id = to_id;
        wire.to.index = to_in_idx;

        wire_id_counter_ += 500;

        wiring_.push_back(wire);

        return true;
    }


    bool FlowCV_Manager::DisconnectNodes(
        uint64_t from_id, 
        uint32_t from_out_idx, 
        uint64_t to_id, 
        uint32_t to_in_idx)
    {
        const auto it = std::find_if(wiring_.begin(), wiring_.end(),
            [&](const Wire& wire)
            {
                return
                    wire.from.id == from_id &&
                    wire.from.index == from_out_idx &&
                    wire.to.id == to_id &&
                    wire.to.index == to_in_idx;
            });

        if (it == wiring_.end()) return false;

        const int toNodeIndex = GetNodeIndexFromId(to_id);
        if (toNodeIndex < 0) {
            wiring_.erase(it);
            return false;
        }

        auto& toNode = nodes_[static_cast<std::size_t>(toNodeIndex)];
        circuit_->PauseAutoTick();
        toNode.node_ptr->DisconnectInput(static_cast<int>(to_in_idx));
        circuit_->ResumeAutoTick();

        if (to_in_idx < toNode.input_conn_map.size()) {
            toNode.input_conn_map[to_in_idx] = {};
        }

        wiring_.erase(it);
        return true;
    }


    bool FlowCV_Manager::DisconnectNodeInput(uint64_t node_id, uint32_t in_index)
    {
        const int nodeIndex = GetNodeIndexFromId(node_id);
        if (nodeIndex < 0) {
            return false;
        }

        auto& node = nodes_[static_cast<std::size_t>(nodeIndex)];

        if (in_index >= node.input_conn_map.size()) {
            return false;
        }

        const auto wireIt = std::find_if(wiring_.begin(), wiring_.end(),
            [&](const Wire& wire)
            {
                return
                    wire.to.id
                    == node_id &&
                    wire.to.index
                    == in_index;
            });

        if (wireIt == wiring_.end()) {
            node.input_conn_map[in_index] = {};
            return false;
        }

        circuit_->PauseAutoTick();

        node.node_ptr->DisconnectInput(static_cast<int>(in_index));

        circuit_->ResumeAutoTick();

        wiring_.erase(wireIt);

        node.input_conn_map[in_index] = {};

        return true;
    }


    bool FlowCV_Manager::RemoveNodeInstance(uint64_t node_id)
    {
        const int nodeIndex =
            GetNodeIndexFromId(
                node_id);

        if (nodeIndex < 0) {
            return false;
        }

        /*
         * 如果被删除节点是其它节点的 source，
         * 清空对应 target 的 input_conn_map。
         */
        for (const auto& wire : wiring_) {
            if (wire.from.id == node_id) {
                const int targetIndex = GetNodeIndexFromId(wire.to.id);

                if (targetIndex >= 0) {
                    auto& target = nodes_[static_cast<std::size_t>(targetIndex)];
                    if (wire.to.index < target.input_conn_map.size()) {
                        target.input_conn_map[wire.to.index] = {};
                    }
                }
            }
        }

        wiring_.erase(std::remove_if(wiring_.begin(), wiring_.end(),
            [node_id](
                const Wire& wire)
            {
                return
                    wire.from.id
                    == node_id ||
                    wire.to.id
                    == node_id;
            }), wiring_.end());

        auto nodePtr = nodes_[static_cast<std::size_t>(nodeIndex)].node_ptr;

        /*
         * DSPatch::Circuit::RemoveComponent()
         * 内部已经会 DisconnectComponent()，
         * 不需要再重复调用一次。
         */
        circuit_->RemoveComponent(nodePtr);

        nodes_.erase(nodes_.begin() + nodeIndex);

        return true;
    }


    std::vector<Wire> FlowCV_Manager::GetNodeConnectionsFromIndex(uint64_t index)
    {
        std::vector<Wire> connections;

        if (index >= nodes_.size()) {
            return connections;
        }

        const uint64_t nodeId = nodes_[static_cast<std::size_t>(index)].id;

        for (const auto& wire : wiring_) {
            if (wire.from.id == nodeId || wire.to.id == nodeId) {
                connections.push_back(wire);
            }
        }

        return connections;
    }


    std::vector<Wire> FlowCV_Manager::GetNodeConnectionsFromId(uint64_t id)
    {
        const int nodeIndex = GetNodeIndexFromId(id);

        if (nodeIndex < 0) {
            return {};
        }

        return GetNodeConnectionsFromIndex(static_cast<uint64_t>(nodeIndex));
    }

    // ------------------------------------------------------------------------
    //  Wire Functions
    // ------------------------------------------------------------------------
    uint64_t FlowCV_Manager::GetWireCount()
    {
        return static_cast<uint64_t>(wiring_.size());
    }


    Wire FlowCV_Manager::GetWireInfoFromIndex(uint64_t index)
    {
        if (index >= wiring_.size()) {
            return {};
        }

        return wiring_[static_cast<std::size_t>(index)];
    }


    uint64_t FlowCV_Manager::GetWireIdFromIndex(uint64_t index)
    {
        if (index >= wiring_.size()) {
            return 0;
        }

        return wiring_[static_cast<std::size_t>(index)].id;
    }


    void FlowCV_Manager::RemoveWireById(uint64_t id)
    {
        const auto it = std::find_if(wiring_.begin(), wiring_.end(),
            [id](const Wire& wire)
            {
                return wire.id == id;
            });

        if (it == wiring_.end()) {
            return;
        }

        const Wire wire = *it;

        DisconnectNodes(
            wire.from.id,
            wire.from.index,
            wire.to.id,
            wire.to.index);
    }


    void FlowCV_Manager::RemoveWireByIndex(uint64_t index)
    {
        if (index >= wiring_.size()) {
            return;
        }

        const Wire wire =wiring_[static_cast<std::size_t>(index)];

        DisconnectNodes(
            wire.from.id,
            wire.from.index,
            wire.to.id,
            wire.to.index);
    }

    // ---------------------------------------------------------------------------------
    // Tick Functions
    // ---------------------------------------------------------------------------------
    void FlowCV_Manager::Tick(DSPatch::Component::TickMode mode)
    {
        circuit_->Tick(mode);
    }


    void FlowCV_Manager::StartAutoTick(DSPatch::Component::TickMode mode)
    {
        circuit_->StartAutoTick(mode);
    }


    void FlowCV_Manager::StopAutoTick()
    {
        circuit_->StopAutoTick();
    }

}	// end namespace FlowCV