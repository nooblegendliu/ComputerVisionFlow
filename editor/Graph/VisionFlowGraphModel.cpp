#include "VisionFlowGraphModel.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include <QtCore/QDebug>
#include <QtCore/QSize>

#include <QtNodes/NodeData>
#include <QtNodes/StyleCollection>

namespace
{
    // 将 DSPatch::IoType 转换为 QtNodes 显示元数据。
// 此处不进行数据传输，真实数据始终由 DSPatch 传播。
    QtNodes::NodeDataType nodeDataType(DSPatch::IoType type)
    {
        using DSPatch::IoType;

        switch (type) {
        case IoType::Io_Type_CvMat:
            return { "CvMat", "cv::Mat" };

        case IoType::Io_Type_Bool:
            return { "Bool", "bool" };

        case IoType::Io_Type_Int:
            return { "Int", "int" };

        case IoType::Io_Type_Float:
            return { "Float", "float" };

        case IoType::Io_Type_String:
            return { "String", "string" };

        case IoType::Io_Type_JSON:
            return { "JSON", "JSON" };

        case IoType::Io_Type_Bool_Array:
            return { "BoolArray", "bool[]" };

        case IoType::Io_Type_Int_Array:
            return { "IntArray", "int[]" };

        case IoType::Io_Type_Float_Array:
            return { "FloatArray", "float[]" };

        case IoType::Io_Type_String_Array:
            return { "StringArray", "string[]" };

        case IoType::Io_Type_Unspecified:
            return { "Unspecified", "Unspecified" };
        }

        return { "Unknown", "Unknown" };
    }
}	// anonymous namespace

// ---------------------------------------------------------------
//  Constructions
// ---------------------------------------------------------------
VisionFlowGraphModel::VisionFlowGraphModel(
    FlowCV::FlowCV_Manager& manager,
    QObject* parent)
    : manager_(manager)
{
    setParent(parent);

    if (!rebuildFromManager()) {
        throw std::runtime_error("Failed to initialize VisionFlowGraphModel");
    }
}


// ----------------------------------------------------------------------
// ID Mapping
// ----------------------------------------------------------------------
std::uint64_t VisionFlowGraphModel::flowNodeId(
    QtNodes::NodeId nodeId) const noexcept
{
    auto it = qtToFlow_.find(nodeId);

    return it == qtToFlow_.end()
        ? 0
        : it->second;
}

QtNodes::NodeId VisionFlowGraphModel::qtNodeId(
    std::uint64_t flowId) const noexcept
{
    auto it = flowToQt_.find(flowId);

    return it == flowToQt_.end()
        ? QtNodes::InvalidNodeId
        : it->second;
}

QPointF VisionFlowGraphModel::defaultPosition(
    std::uint64_t ordinal)
{
    return QPointF(
        static_cast<qreal>(ordinal % 4) * 240.0,
        static_cast<qreal>(ordinal / 4) * 170.0
    );
}

QtNodes::NodeId VisionFlowGraphModel::newNodeId()
{
    if (nextQtId_ == QtNodes::InvalidNodeId) {
        return QtNodes::InvalidNodeId;
    }

    return nextQtId_++;
}

// -----------------------------------------------------------------------
// Rebuild / Synchronization
// -----------------------------------------------------------------------

bool VisionFlowGraphModel::rebuildFromManager(bool resetPositions)
{
    std::unordered_map<QtNodes::NodeId, std::uint64_t> newQtToFlow;
    std::unordered_map<std::uint64_t, QtNodes::NodeId> newFlowToQt;
    std::unordered_map<QtNodes::NodeId, QPointF> newPositions;

    const std::uint64_t count = manager_.GetNodeCount();

    for (std::uint64_t i = 0; i < count; ++i)
    {
        FlowCV::NodeInfo info{};

        if (!manager_.GetNodeInfoByIndex(i, info) ||
            info.id == 0 ||
            !info.node_ptr)
        {
            return false;
        }

        if (newFlowToQt.find(info.id) != newFlowToQt.end())
        {
            return false;
        }

        QtNodes::NodeId viewId = qtNodeId(info.id);

        if (viewId == QtNodes::InvalidNodeId)
        {
            viewId = newNodeId();

            if (viewId == QtNodes::InvalidNodeId)
            {
                return false;
            }
        }

        newQtToFlow.emplace(viewId, info.id);
        newFlowToQt.emplace(info.id, viewId);

        QPointF position = defaultPosition(i);

        // 只有普通同步时才复用旧的位置。
        // 加载新项目时应从项目文件重新读取。
        if (!resetPositions)
        {
            auto posIt = positions_.find(viewId);

            if (posIt != positions_.end())
            {
                position = posIt->second;
            }
        }

        newPositions.emplace(viewId, position);
    }

    qtToFlow_.swap(newQtToFlow);
    flowToQt_.swap(newFlowToQt);
    positions_.swap(newPositions);

    // QtNodes 重建时需要重新计算图形尺寸。
    sizes_.clear();

    Q_EMIT modelReset();

    return true;
}


// --------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------
bool VisionFlowGraphModel::getNodeInfo(
    QtNodes::NodeId nodeId,
    FlowCV::NodeInfo& info) const
{
    const auto flowId = flowNodeId(nodeId);

    return flowId != 0 && manager_.GetNodeInfoById(flowId, info);
}

bool VisionFlowGraphModel::mapWire(
    FlowCV::Wire const& wire,
    QtNodes::ConnectionId& result) const
{
    const auto outId = qtNodeId(wire.from.id);
    const auto inId = qtNodeId(wire.to.id);

    if (outId == QtNodes::InvalidNodeId || inId == QtNodes::InvalidNodeId) {
        return false;
    }

    result = {
        outId,
        wire.from.index,
        inId,
        wire.to.index
    };

    return true;
}

// -----------------------------------------------------------------------
// Node Queries
// ------------------------------------------------------------------------
std::unordered_set<QtNodes::NodeId> VisionFlowGraphModel::allNodeIds() const
{
    std::unordered_set<QtNodes::NodeId> result;

    for (auto const& [viewId, flowId] : qtToFlow_) {
        (void)flowId;
        result.insert(viewId);
    }

    return result;
}

bool VisionFlowGraphModel::nodeExists(QtNodes::NodeId nodeId) const
{
    FlowCV::NodeInfo info{};

    return getNodeInfo(nodeId, info);
}

// -------------------------------------------------------------------
//  Connection Query
// -------------------------------------------------------------------
std::unordered_set<QtNodes::ConnectionId> VisionFlowGraphModel::allConnectionIds(
    QtNodes::NodeId nodeId) const
{
    std::unordered_set<QtNodes::ConnectionId> result;

    const auto id = flowNodeId(nodeId);

    if (id == 0) {
        return result;
    }

    for (auto const& wire :
        manager_.GetNodeConnectionsFromId(id)) {

        QtNodes::ConnectionId connection{};

        if (mapWire(wire, connection)) {
            result.insert(connection);
        }
    }

    return result;
}

std::unordered_set<QtNodes::ConnectionId>
VisionFlowGraphModel::connections(
    QtNodes::NodeId nodeId,
    QtNodes::PortType portType,
    QtNodes::PortIndex index) const
{
    std::unordered_set<QtNodes::ConnectionId> result;

    if (portType != QtNodes::PortType::In &&
        portType != QtNodes::PortType::Out) {
        return result;
    }

    for (auto const& connection : allConnectionIds(nodeId)) {

        if ((portType == QtNodes::PortType::In &&
            connection.inNodeId == nodeId &&
            connection.inPortIndex == index) ||

            (portType == QtNodes::PortType::Out &&
                connection.outNodeId == nodeId &&
                connection.outPortIndex == index)) {

            result.insert(connection);
        }
    }

    return result;
}

bool VisionFlowGraphModel::connectionExists(
    QtNodes::ConnectionId connection) const
{
    const auto outId = flowNodeId(connection.outNodeId);
    const auto inId = flowNodeId(connection.inNodeId);

    if (outId == 0 || inId == 0) {
        return false;
    }

    for (auto const& wire :
        manager_.GetNodeConnectionsFromId(inId)) {

        if (wire.from.id == outId &&
            wire.from.index == connection.outPortIndex &&
            wire.to.id == inId &&
            wire.to.index == connection.inPortIndex) {

            return true;
        }
    }

    return false;
}

// -------------------------------------------------------------------
// Node Creation
// -------------------------------------------------------------------
QtNodes::NodeId VisionFlowGraphModel::addNode(
    QString const nodeType)
{
    if (nodeType.isEmpty()) {
        return QtNodes::InvalidNodeId;
    }

    // 预留 UI ID。
    // 即使创建失败，也不复用已分配过的 ID。
    const QtNodes::NodeId viewId = newNodeId();

    if (viewId == QtNodes::InvalidNodeId) {
        return QtNodes::InvalidNodeId;
    }

    const QByteArray utf8 = nodeType.toUtf8();

    std::uint64_t flowId = 0;

    try {
        flowId = manager_.CreateNewNodeInstance(
            utf8.constData());
    }
    catch (std::exception const& ex) {
        qWarning()
            << "FlowCV failed to add node:"
            << ex.what();

        return QtNodes::InvalidNodeId;
    }

    if (flowId == 0) {
        return QtNodes::InvalidNodeId;
    }

    qtToFlow_.emplace(viewId, flowId);
    flowToQt_.emplace(flowId, viewId);

    positions_.emplace(
        viewId,
        defaultPosition(manager_.GetNodeCount() - 1)
    );

    Q_EMIT nodeCreated(viewId);

    return viewId;
}


// ---------------------------------------------------------------------
//  Connection Validation
// ---------------------------------------------------------------------
bool VisionFlowGraphModel::connectionPossible(
    QtNodes::ConnectionId connection) const
{
    if (connection.outNodeId == QtNodes::InvalidNodeId ||
        connection.inNodeId == QtNodes::InvalidNodeId ||
        connection.outPortIndex == QtNodes::InvalidPortIndex ||
        connection.inPortIndex == QtNodes::InvalidPortIndex ||
        connectionExists(connection)) {
        return false;
    }

    FlowCV::NodeInfo outInfo{};
    FlowCV::NodeInfo inInfo{};

    if (!getNodeInfo(connection.outNodeId, outInfo) ||
        !getNodeInfo(connection.inNodeId, inInfo) ||
        !outInfo.node_ptr ||
        !inInfo.node_ptr ||
        outInfo.desc.output_count <= 0 ||
        inInfo.desc.input_count <= 0 ||
        connection.outPortIndex >=
        static_cast<unsigned int>(
            outInfo.desc.output_count) ||
        connection.inPortIndex >=
        static_cast<unsigned int>(
            inInfo.desc.input_count)) {

        return false;
    }

    // 与 Manager::ConnectNodes 一致，
    // 仅允许 IoType 相同的端口相连接。
    return outInfo.node_ptr->GetOutputType(
        static_cast<int>(connection.outPortIndex))
        ==
        inInfo.node_ptr->GetInputType(
            static_cast<int>(connection.inPortIndex));
}


// ================================================================
// Connection Creation
// ================================================================
void VisionFlowGraphModel::addConnection(
    QtNodes::ConnectionId connection)
{
    if (!connectionPossible(connection)) {
        return;
    }

    const auto fromId = flowNodeId(connection.outNodeId);
    const auto toId = flowNodeId(connection.inNodeId);

    // 记录目标输入端口的原连接。
    // 不提前删除，Manager 负责执行实际连接替换。
    QtNodes::ConnectionId oldConnection{};

    bool hadOldConnection = false;

    for (auto const& wire :
        manager_.GetNodeConnectionsFromId(toId)) {

        if (wire.to.id == toId &&
            wire.to.index == connection.inPortIndex) {

            hadOldConnection = mapWire(
                wire, oldConnection);

            break;
        }
    }

    const bool connected = manager_.ConnectNodes(
        fromId,
        connection.outPortIndex,
        toId,
        connection.inPortIndex
    );

    if (!connected) {
        return;
    }

    // Manager 已完成旧连接替换，
    // 此时再让 QtNodes 更新图形。
    if (hadOldConnection &&
        oldConnection != connection) {

        Q_EMIT connectionDeleted(oldConnection);
    }

    Q_EMIT connectionCreated(connection);
}

// ---------------------------------------------------------------
// Node Data
// ----------------------------------------------------------------

QVariant VisionFlowGraphModel::nodeData(
    QtNodes::NodeId nodeId,
    QtNodes::NodeRole role) const
{
    FlowCV::NodeInfo info{};

    if (!getNodeInfo(nodeId, info)) {
        return {};
    }

    switch (role) {
    case QtNodes::NodeRole::Type:
        return QString::fromStdString(info.desc.name);

    case QtNodes::NodeRole::Position: {
        auto it = positions_.find(nodeId);

        return it != positions_.end()
            ? QVariant::fromValue(it->second)
            : QVariant::fromValue(QPointF());
    }

    case QtNodes::NodeRole::Size: {
        auto it = sizes_.find(nodeId);

        return it != sizes_.end()
            ? QVariant::fromValue(it->second)
            : QVariant::fromValue(QSize());
    }

    case QtNodes::NodeRole::CaptionVisible:
        return true;

    case QtNodes::NodeRole::Caption:
        return QStringLiteral("%1_%2")
            .arg(QString::fromStdString(info.desc.name))
            .arg(info.node_ptr->GetInstanceCount());

    case QtNodes::NodeRole::Style:
        // 保留 QVariantMap 形式：
        // DefaultNodePainter 使用 fromVariant() 读取。
        return QtNodes::StyleCollection::nodeStyle()
            .toJson()
            .toVariantMap();

    case QtNodes::NodeRole::InPortCount:
        return static_cast<unsigned int>(
            std::max(0, info.desc.input_count));

    case QtNodes::NodeRole::OutPortCount:
        return static_cast<unsigned int>(
            std::max(0, info.desc.output_count));

    case QtNodes::NodeRole::Widget:
    case QtNodes::NodeRole::InternalData:
    case QtNodes::NodeRole::ValidationState:
    case QtNodes::NodeRole::ProcessingStatus:
        return {};
    }

    return {};
}



bool VisionFlowGraphModel::setNodeData(
    QtNodes::NodeId nodeId,
    QtNodes::NodeRole role,
    QVariant value)
{
    if (!nodeExists(nodeId)) {
        return false;
    }

    switch (role) {
    case QtNodes::NodeRole::Position: {
        if (!value.canConvert<QPointF>()) {
            return false;
        }

        const QPointF newPosition =
            value.value<QPointF>();

        auto& oldPosition = positions_[nodeId];

        if (oldPosition != newPosition) {
            oldPosition = newPosition;
            Q_EMIT nodePositionUpdated(nodeId);
        }

        return true;
    }

    case QtNodes::NodeRole::Size: {
        if (!value.canConvert<QSize>()) {
            return false;
        }

        const QSize newSize = value.value<QSize>();

        if (!newSize.isValid() || newSize.isEmpty()) {
            return false;
        }

        // QtNodes::DefaultHorizontalNodeGeometry
        // 调用此接口记录计算出的节点宽高。
        //
        // 不发 nodeUpdated()，避免重复触发
        // Geometry::recomputeSize()。
        sizes_[nodeId] = newSize;

        return true;
    }

    default:
        return false;
    }
}


// ================================================================
// Port Data
// ================================================================

QVariant VisionFlowGraphModel::portData(
    QtNodes::NodeId nodeId,
    QtNodes::PortType portType,
    QtNodes::PortIndex index,
    QtNodes::PortRole role) const
{
    if (portType != QtNodes::PortType::In &&
        portType != QtNodes::PortType::Out) {
        return {};
    }

    FlowCV::NodeInfo info{};

    if (!getNodeInfo(nodeId, info) ||
        !info.node_ptr) {
        return {};
    }

    const int count =
        portType == QtNodes::PortType::In
        ? info.desc.input_count
        : info.desc.output_count;

    if (count <= 0 ||
        index >= static_cast<unsigned int>(count)) {
        return {};
    }

    switch (role) {

    case QtNodes::PortRole::Data:
        // 数据由 DSPatch 传播。
        // QtNodes 不负责计算数据的传递。
        return {};

    case QtNodes::PortRole::DataType: {
        const DSPatch::IoType type =
            portType == QtNodes::PortType::In
            ? info.node_ptr->GetInputType(
                static_cast<int>(index))
            : info.node_ptr->GetOutputType(
                static_cast<int>(index));

        return QVariant::fromValue(
            nodeDataType(type));
    }

    case QtNodes::PortRole::ConnectionPolicyRole:
        return QVariant::fromValue(
            portType == QtNodes::PortType::In
            ? QtNodes::ConnectionPolicy::One
            : QtNodes::ConnectionPolicy::Many
        );

    case QtNodes::PortRole::CaptionVisible:
        return true;

    case QtNodes::PortRole::Caption: {
        const std::string name =
            portType == QtNodes::PortType::In
            ? info.node_ptr->GetInputName(
                static_cast<int>(index))
            : info.node_ptr->GetOutputName(
                static_cast<int>(index));

        return QString::fromStdString(name);
    }
    }

    return {};
}

bool VisionFlowGraphModel::setPortData(
    QtNodes::NodeId nodeId,
    QtNodes::PortType portType,
    QtNodes::PortIndex index,
    QVariant const& value,
    QtNodes::PortRole role)
{
    Q_UNUSED(nodeId);
    Q_UNUSED(portType);
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);

    // 端口名称和类型由节点本身声明。
    // Editor 不应改变这些底层元数据。
    return false;
}

// ================================================================
// Connection Deletion
// ================================================================

bool VisionFlowGraphModel::deleteConnection(
    QtNodes::ConnectionId connection)
{
    if (!connectionExists(connection)) {
        return false;
    }

    const bool disconnected = manager_.DisconnectNodes(
        flowNodeId(connection.outNodeId),
        connection.outPortIndex,
        flowNodeId(connection.inNodeId),
        connection.inPortIndex
    );

    if (!disconnected) {
        return false;
    }

    Q_EMIT connectionDeleted(connection);

    return true;
}

// ================================================================
// Node Deletion
// ================================================================
bool VisionFlowGraphModel::deleteNode(QtNodes::NodeId nodeId)
{
    const auto flowId = flowNodeId(nodeId);

    if (flowId == 0) {
        return false;
    }

    FlowCV::NodeInfo info{};

    if (!manager_.GetNodeInfoById(flowId, info)) {
        return false;
    }

    // 删除前记录关联连线。
    auto attached = allConnectionIds(nodeId);

    // 实际删除仍由原有 Manager 完成。
    if (!manager_.RemoveNodeInstance(flowId)) {
        return false;
    }

    // 先通知 QtNodes 移除连线图形。
    for (auto const& connection : attached) {
        Q_EMIT connectionDeleted(connection);
    }

    qtToFlow_.erase(nodeId);
    flowToQt_.erase(flowId);
    positions_.erase(nodeId);

    // 新增：同步清除画布尺寸缓存。
    sizes_.erase(nodeId);

    Q_EMIT nodeDeleted(nodeId);

    return true;
}
