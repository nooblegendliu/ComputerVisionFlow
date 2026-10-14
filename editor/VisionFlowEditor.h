#pragma once

#include <cstdint>

#include <QtCore/QPointF>
#include <QtCore/QStringList>
#include <QtNodes/Definitions>
#include <QtWidgets/QWidget>

class QAction;
class QMenu;

namespace FlowCV {
    class FlowCV_Manager;
}

namespace QtNodes {
    class BasicGraphicsScene;
    class GraphicsView;
}

class VisionFlowGraphModel;

// QtNodes 画布组件。
// 不管理 DSPatch 节点生命周期；真实节点/连线始终由 GraphModel 转交 Manager 管理。
class VisionFlowEditor final : public QWidget
{
    Q_OBJECT

public:
    explicit VisionFlowEditor(FlowCV::FlowCV_Manager& manager,
        QWidget* parent = nullptr);
    ~VisionFlowEditor() override;

    VisionFlowGraphModel* graphModel() const noexcept { return model_; }
    QtNodes::BasicGraphicsScene* graphicsScene() const noexcept { return scene_; }
    QtNodes::GraphicsView* graphicsView() const noexcept { return view_; }
    QAction* deleteSelectionAction() const;

    // 从已注册的内置节点读取菜单条目；不硬编码 Solid / Add / Viewer。
    QStringList availableNodeTypes() const;

    // 在当前视口中心 / 给定 scene 坐标创建节点。
    QtNodes::NodeId addNode(QString const& nodeType);
    QtNodes::NodeId addNodeAt(QString const& nodeType, QPointF scenePosition);

public Q_SLOTS:
    void deleteSelection();
    void zoomFitAll();
    void resetZoom();

Q_SIGNALS:
    void selectedFlowNodeChanged(std::uint64_t flowId);
    void graphChanged(std::uint64_t nodeCount, std::uint64_t wireCount);
    void operationFailed(QString const& message);

private:
    QMenu* createCanvasMenu(QPointF scenePosition);
    void showNodeMenu(QtNodes::NodeId nodeId);
    void notifyGraphChanged();

    FlowCV::FlowCV_Manager& manager_;
    VisionFlowGraphModel* model_ = nullptr;
    QtNodes::BasicGraphicsScene* scene_ = nullptr;
    QtNodes::GraphicsView* view_ = nullptr;
};
