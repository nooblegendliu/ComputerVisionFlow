#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include <QtCore/QPointF>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <QtNodes/AbstractGraphModel>

#include <flowcv_manager.h>

// --------------------------------------------------------
// QtNodes只保存画布状态，节点实例和连线都以FlowCV_Manager为准

class VisionFlowGraphModel final : public QtNodes::AbstractGraphModel
{
	Q_OBJECT

public:
	explicit VisionFlowGraphModel(FlowCV::FlowCV_Manager& manager, QObject* parent = nullptr);

	// QtNodes NodeId是unsigned int
	// FlowCV NodeId是uint64_t
	// 映射只在编辑器内部使用，不修改FlowCV的真实ID
	std::uint64_t flowNodeId(QtNodes::NodeId nodeId) const noexcept;

	QtNodes::NodeId qtNodeId(std::uint64_t flowId) const noexcept;

    // 仅返回自定义实例显示名；没有重命名时返回空字符串。
    QString customNodeDisplayName(QtNodes::NodeId nodeId) const;
    // 空白名称恢复默认标题；不修改 FlowCV 的节点类型或真实 ID。
    bool setNodeDisplayName(QtNodes::NodeId nodeId, const QString& name);

	// 用于SetState/NewState或外部修改运行图之后，重新读取Manager数据，并通知QtNodes整体刷新
	// 参数为true时不复用原画布的位置
	bool rebuildFromManager(bool resetPositions = false);

	// ------------------------------------------------
	// Node ID/ Query
	// ------------------------------------------------
	QtNodes::NodeId newNodeId() override;

	std::unordered_set<QtNodes::NodeId> allNodeIds() const override;

	std::unordered_set<QtNodes::ConnectionId> 
		allConnectionIds(
			QtNodes::NodeId nodeId) const override;

	std::unordered_set<QtNodes::ConnectionId>
		connections(
			QtNodes::NodeId nodeId,
			QtNodes::PortType portType,
			QtNodes::PortIndex index) const override;


	bool connectionExists(QtNodes::ConnectionId connectionId) const override;

	// -----------------------------------------------------
	// Node / Connection creation
	// -----------------------------------------------------
	QtNodes::NodeId addNode(QString const nodeType = QString()) override;

	bool connectionPossible(
		QtNodes::ConnectionId connectionId) const override;

	void addConnection(QtNodes::ConnectionId connectionId) override;

	// ---------------------------------------------------------
	// Node data
	// ---------------------------------------------------------
	bool nodeExists(QtNodes::NodeId nodeId) const override;

	QVariant nodeData(QtNodes::NodeId nodeId, QtNodes::NodeRole role) const override;

	bool setNodeData(
		QtNodes::NodeId nodeId, QtNodes::NodeRole role, QVariant value) override;

	// -------------------------------------------------------------
	// Port data
	// -------------------------------------------------------------
	QVariant portData(
		QtNodes::NodeId nodeId,
		QtNodes::PortType portType,
		QtNodes::PortIndex index,
		QtNodes::PortRole role) const override;

	bool setPortData(
		QtNodes::NodeId nodeId,
		QtNodes::PortType portType,
		QtNodes::PortIndex index,
		QVariant const& value,
		QtNodes::PortRole role = QtNodes::PortRole::Data
	) override;

	// -----------------------------------------------------------------------
	// Removal
	// -----------------------------------------------------------------------
	bool deleteConnection(
		QtNodes::ConnectionId connectionId) override;

	bool deleteNode(
		QtNodes::NodeId nodeId) override;

	// 10.14 new add
	bool detachPossible(QtNodes::ConnectionId const) const override
	{
		return false;
	}

    bool setNodeEnabled(QtNodes::NodeId nodeId, bool enabled);
    bool nodeEnabled(QtNodes::NodeId nodeId) const;
    QString nodeInformation(QtNodes::NodeId nodeId) const;

Q_SIGNALS:
    void nodeAboutToBeRemoved(std::uint64_t flowId);
    void nodeEnabledChanged(QtNodes::NodeId nodeId);
    void nodeDisplayNameChanged(QtNodes::NodeId nodeId);

private:
	bool getNodeInfo(
		QtNodes::NodeId nodeId,
		FlowCV::NodeInfo& info) const;

	bool mapWire(
		FlowCV::Wire const& wire,
		QtNodes::ConnectionId& result) const;

	static QPointF defaultPosition(
		std::uint64_t ordinal);

private:
	FlowCV::FlowCV_Manager& manager_;

	QtNodes::NodeId nextQtId_ = 1;

	std::unordered_map<QtNodes::NodeId,std::uint64_t> qtToFlow_;

	std::unordered_map<std::uint64_t, QtNodes::NodeId> flowToQt_;

	// QtNodes画布专用状态
	std::unordered_map<QtNodes::NodeId, QPointF> positions_;
	// 新增：保存QtNodes自动计算的节点尺寸
	std::unordered_map<QtNodes::NodeId, QSize> sizes_;

    // 编辑器专有状态：以真实 FlowCV 节点 ID 绑定实例显示名称。
    std::unordered_map<std::uint64_t, QString> customDisplayNames_;
};
