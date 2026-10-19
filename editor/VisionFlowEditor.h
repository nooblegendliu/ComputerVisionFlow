#pragma once

#include <cstdint>

#include <QtCore/QPointF>
#include <QtCore/QStringList>
#include <QtNodes/Definitions>
#include <QtWidgets/QWidget>

#include <nlohmann/json.hpp>

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
// 真实节点与连线仍由 FlowCV_Manager 管理。
class VisionFlowEditor final : public QWidget
{
    Q_OBJECT

public:
    explicit VisionFlowEditor(
        FlowCV::FlowCV_Manager& manager,
        QWidget* parent = nullptr);

    ~VisionFlowEditor() override;

    VisionFlowGraphModel* graphModel() const noexcept
    {
        return model_;
    }

    QtNodes::BasicGraphicsScene* graphicsScene() const noexcept
    {
        return scene_;
    }

    QtNodes::GraphicsView* graphicsView() const noexcept
    {
        return view_;
    }

    QAction* deleteSelectionAction() const;
    QAction* showFlowAction() const { return showFlowAction_; }

    // Uses FlowCV's nodes/connections/editor clipboard schema.
    nlohmann::json selectedSubgraph() const;
    bool pasteSubgraph(const nlohmann::json& state, QPointF position);
    bool copySelection();
    bool pasteAt(QPointF position);
    QString nodeCategory(QString const& nodeType) const;

    // Node Library
    QStringList availableNodeTypes() const;

    // Node Creation
    QtNodes::NodeId addNode(QString const& nodeType);

    QtNodes::NodeId addNodeAt(
        QString const& nodeType,
        QPointF scenePosition);

    // ---------------------------------------------------------
    // Stage E: Editor State
    // ---------------------------------------------------------

    // 保存画布节点的位置。
    // 返回原版 FlowCV 的 editor 数组。
    nlohmann::json getEditorState() const;

    // Manager 完成 SetState / NewState 后调用。
    // 同步 QtNodes，并恢复 editor 节点位置。
    bool restoreFromManager(
        const nlohmann::json& state);

public Q_SLOTS:
    void deleteSelection();
    void zoomFitAll();
    void resetZoom();
    void cutSelection();
    void pasteSelection();
    void toggleSelectedEnabled();
    void zoomFitSelection();
    void showNodeSearch();

Q_SIGNALS:
    void selectedFlowNodeChanged(std::uint64_t flowId);
    void graphChanged(std::uint64_t nodeCount, std::uint64_t wireCount);
    void operationFailed(QString const& message);
    void nodeUiRequested(std::uint64_t flowId);

private:
    QMenu* createCanvasMenu(QPointF scenePosition);

    void showNodeMenu(QtNodes::NodeId nodeId);

    void notifyGraphChanged();
    void populateNodeMenu(QMenu* menu, QPointF position,
        QtNodes::ConnectionId pending = {QtNodes::InvalidNodeId, QtNodes::InvalidPortIndex,
            QtNodes::InvalidNodeId, QtNodes::InvalidPortIndex});
    void createNodeFromPort(QString const& type, QPointF position, QtNodes::ConnectionId pending);

private:
    FlowCV::FlowCV_Manager& manager_;

    VisionFlowGraphModel* model_ = nullptr;

    QtNodes::BasicGraphicsScene* scene_ = nullptr;

    QtNodes::GraphicsView* view_ = nullptr;
    QAction* showFlowAction_ = nullptr;
};
