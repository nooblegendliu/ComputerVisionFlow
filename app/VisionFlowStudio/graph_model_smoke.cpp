
#include <stdexcept>

#include <flowcv_manager.h>
#include <VisionFlowGraphModel.hpp>

int main()
{
    FlowCV::FlowCV_Manager manager;
    VisionFlowGraphModel model(manager);

    // 1. 添加节点
    const auto a = model.addNode("Solid");
    const auto b = model.addNode("Solid");
    const auto add = model.addNode("Add");
    const auto viewer = model.addNode("Viewer");

    if (a == QtNodes::InvalidNodeId ||
        b == QtNodes::InvalidNodeId ||
        add == QtNodes::InvalidNodeId ||
        viewer == QtNodes::InvalidNodeId ||
        manager.GetNodeCount() != 4) {
        throw std::runtime_error("Node creation failed");
    }

    // 2. 创建连接
    const QtNodes::ConnectionId c1{ a, 0, add, 0 };
    const QtNodes::ConnectionId c2{ b, 0, add, 1 };
    const QtNodes::ConnectionId c3{ add, 0, viewer, 0 };

    for (const auto& c : { c1, c2, c3 }) {
        if (!model.connectionPossible(c)) {
            throw std::runtime_error("Connection invalid");
        }
        model.addConnection(c);
        if (!model.connectionExists(c)) {
            throw std::runtime_error("Connection failed");
        }
    }

    if (manager.GetWireCount() != 3) {
        throw std::runtime_error("Wire count mismatch");
    }

    // 3. 验证节点位置
    if (!model.setNodeData(
        a,
        QtNodes::NodeRole::Position,
        QPointF(100.0, 200.0))) {
        throw std::runtime_error("Position update failed");
    }

    // 4. 删除连接
    if (!model.deleteConnection(c1) ||
        manager.GetWireCount() != 2) {
        throw std::runtime_error("Connection deletion failed");
    }

    // 5. 删除节点和关联连接
    if (!model.deleteNode(add) ||
        manager.GetNodeCount() != 3 ||
        manager.GetWireCount() != 0) {
        throw std::runtime_error("Node deletion failed");
    }

    return 0;
}
