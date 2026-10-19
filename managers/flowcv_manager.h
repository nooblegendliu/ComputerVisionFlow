#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <DSPatch.h>
#include <flowNode.h>
#include <flowProperties.h>
#include <flowTypes.h>

#include "node_manager.h"
#include "plugin_manager.h"

#include <nlohmann/json.hpp>

namespace FlowCV
{
	struct IoInfo
	{
		uint64_t id = 0;
		uint32_t index = 0;
	};


	struct NodeInfo
	{
		uint64_t id;

		std::shared_ptr<DSPatch::Component> node_ptr;

		std::vector<IoInfo> input_conn_map;

		NodeDescription desc;
	};


	struct Wire
	{
        uint64_t id = 0;

		IoInfo from;

		IoInfo to;
	};


	class FlowCV_Manager
	{
	public:
		FlowCV_Manager();

		~FlowCV_Manager();

    public:
        /*
        *   Node Lifecycle
        */
        uint64_t CreateNewNodeInstance(const char* name);

        bool RemoveNodeInstance(uint64_t node_id);      // New Add

        /*
        *   Node Information
        */
        uint64_t GetNodeCount();

        bool GetNodeInfoByIndex(uint64_t index, NodeInfo& nInfo);

        bool GetNodeInfoById(uint64_t id, NodeInfo& nInfo);

        int GetNodeIndexFromId(uint64_t id);

        uint64_t GetNodeIdFromIndex(uint64_t index);

        /*
        *   Properties
        *   
        *   Editor读取属性时使用Snapshot，不暴露ImGui/Qt对象
        */
        bool GetNodePropertiesSnapshot(uint64_t node_id, PropertiesSnapshot& snapshot);

        bool SetNodeProperty(uint64_t node_id, const std::string& key, bool value);

        bool SetNodeProperty(uint64_t node_id, const std::string& key, int value);

        bool SetNodeProperty(uint64_t node_id, const std::string& key, float value);

        bool SetNodeProperty(uint64_t node_id, const std::string& key, const std::string& value);

        bool SetNodeProperty(uint64_t node_id, const std::string& key, const char* value);

        /*
        *   State serialization
        */
        nlohmann::json GetState();

        bool SetState(const nlohmann::json& state);

        void NewState();

        bool LoadState(const char* filepath);

        bool SaveState(const char* filepath);

        /*
        *   connections
        */
        bool ConnectNodes(
            uint64_t from_id, 
            uint32_t from_out_idx, 
            uint64_t to_id, 
            uint32_t to_in_idx);

        bool DisconnectNodes(
            uint64_t from_id, 
            uint32_t from_out_idx, 
            uint64_t to_id, 
            uint32_t to_in_idx);

        bool DisconnectNodeInput(
            uint64_t node_id,
            uint32_t in_index);

        /*
        *   Wire information
        */
        uint64_t GetWireCount();

        Wire GetWireInfoFromIndex(uint64_t index);

        uint64_t GetWireIdFromIndex(uint64_t index);

        void RemoveWireById(uint64_t id);

        void RemoveWireByIndex(uint64_t index);

        std::vector<Wire> GetNodeConnectionsFromId(uint64_t id);

        std::vector<Wire> GetNodeConnectionsFromIndex(uint64_t index);

        /*
        *   Processing
        */
        void SetBufferCount(uint32_t num_buffers);

        int GetBufferCount();
 
        void Tick(DSPatch::Component::TickMode mode = DSPatch::Component::TickMode::Parallel);

        void StartAutoTick(DSPatch::Component::TickMode mode = DSPatch::Component::TickMode::Parallel);

        void StopAutoTick();

        bool IsAutoTickRunning() const noexcept { return auto_tick_running_; }

    protected:

        uint64_t AddNewNodeInstance(const char* name, bool external_plugin = false, uint64_t id = 0);

        uint64_t GetNextId();

    private:
        /*
        *   返回FlowCV::Node
        * 
        *   过渡阶段：老plugin如果直接继承DSPatch::Component，动态转换会失败，属性接口返回false
        */
        std::shared_ptr<FlowCV::Node> GetFlowNodeById(uint64_t id);

        void CheckInstCountValue(NodeInfo& nodeInfo);

    private:
        uint64_t id_counter_;
        bool auto_tick_running_ = false;

        uint64_t wire_id_counter_;

        std::vector<NodeInfo> nodes_;

        std::vector<Wire> wiring_;

        std::shared_ptr<DSPatch::Circuit> circuit_;

    public:
        std::shared_ptr<PluginManager> plugin_manager_;

        std::shared_ptr<InternalNodeManager> internal_node_manager_;
	};

}	// End namespace FlowCV
