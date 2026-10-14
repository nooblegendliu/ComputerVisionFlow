#include "VisionFlowEditor.h"

#include <VisionFlowGraphModel.hpp>
#include <flowcv_manager.h>
#include <node_manager.h>

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include <QtCore/QTimer>
#include <QtGui/QAction>
#include <QtGui/QCursor>
#include <QtGui/QKeySequence>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QVBoxLayout>

#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/GraphicsView>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

namespace {

    // QtNodes 默认空白处菜单由 BasicGraphicsScene::createSceneMenu 提供。
    // 这里只定制菜单入口，不修改 QtNodes 第三方库。
    class VisionFlowGraphicsScene final : public QtNodes::BasicGraphicsScene
    {
    public:
        using MenuFactory = std::function<QMenu* (QPointF)>;

        VisionFlowGraphicsScene(QtNodes::AbstractGraphModel& model,
            MenuFactory factory,
            QObject* parent)
            : QtNodes::BasicGraphicsScene(model, parent)
            , menuFactory_(std::move(factory))
        {
        }

        QMenu* createSceneMenu(QPointF const scenePosition) override
        {
            return menuFactory_(scenePosition);
        }

    private:
        MenuFactory menuFactory_;
    };

    // 当前 GraphModel 尚未实现 saveNode/loadNode，不能允许删除后的
    // QtNodes 原生 Undo 操作恢复出只有外观、没有 FlowCV 实例的节点。
    void disableUnsupportedGraphActions(QtNodes::GraphicsView* view)
    {
        for (QAction* action : view->actions()) {
            if (action == view->deleteSelectionAction() ||
                action == view->clearSelectionAction()) {
                continue;
            }

            // QtNodes 当前实现中，此时 view 的其余 actions 为
            // Duplicate、Copy、Paste、Undo、Redo。
            action->setEnabled(false);
            action->setShortcuts(QList<QKeySequence>{});
        }
    }

    void deleteMenuLater(QMenu* menu)
    {
        QObject::connect(menu, &QMenu::aboutToHide,
            menu, &QObject::deleteLater);
    }

} // namespace

VisionFlowEditor::VisionFlowEditor(FlowCV::FlowCV_Manager& manager,
    QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    // 生命周期顺序：model -> scene -> view；Qt 父对象销毁时反序析构。
    model_ = new VisionFlowGraphModel(manager_, this);

    scene_ = new VisionFlowGraphicsScene(
        *model_,
        [this](QPointF position) { return createCanvasMenu(position); },
        this);

    view_ = new QtNodes::GraphicsView(scene_, this);
    view_->setObjectName(QStringLiteral("VisionFlowGraphView"));
    view_->setFocusPolicy(Qt::StrongFocus);
    view_->setScaleRange(0.15, 3.0);

    disableUnsupportedGraphActions(view_);

    // 保留 QtNodes 内置的 Delete QAction 快捷键，但更换其执行逻辑：
    // 直接操作现有 GraphModel，而不是通过不完整的 Undo 序列化路径。
    QAction* deleteAction = view_->deleteSelectionAction();
    QObject::disconnect(deleteAction, nullptr, view_, nullptr);
    QObject::connect(deleteAction, &QAction::triggered,
        this, &VisionFlowEditor::deleteSelection);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view_);

    QObject::connect(scene_, &QtNodes::BasicGraphicsScene::nodeSelected,
        this, [this](QtNodes::NodeId nodeId) {
            Q_EMIT selectedFlowNodeChanged(model_->flowNodeId(nodeId));
        });

    QObject::connect(scene_, &QGraphicsScene::selectionChanged,
        this, [this]() {
            for (QGraphicsItem* item : scene_->selectedItems()) {
                if (auto* node = dynamic_cast<QtNodes::NodeGraphicsObject*>(item)) {
                    Q_EMIT selectedFlowNodeChanged(model_->flowNodeId(node->nodeId()));
                    return;
                }
            }
            Q_EMIT selectedFlowNodeChanged(0);
        });

    // NodeGraphicsObject 会发送右键节点信号。推迟弹出菜单，避免在
    // 图形对象本身的 contextMenuEvent 调用栈中直接删除自身。
    QObject::connect(scene_, &QtNodes::BasicGraphicsScene::nodeContextMenu,
        this, [this](QtNodes::NodeId nodeId, QPointF) {
            showNodeMenu(nodeId);
        });

    QObject::connect(model_, &QtNodes::AbstractGraphModel::nodeCreated,
        this, [this](QtNodes::NodeId) { notifyGraphChanged(); });
    QObject::connect(model_, &QtNodes::AbstractGraphModel::nodeDeleted,
        this, [this](QtNodes::NodeId) { notifyGraphChanged(); });
    QObject::connect(model_, &QtNodes::AbstractGraphModel::connectionCreated,
        this, [this](QtNodes::ConnectionId) { notifyGraphChanged(); });
    QObject::connect(model_, &QtNodes::AbstractGraphModel::connectionDeleted,
        this, [this](QtNodes::ConnectionId) { notifyGraphChanged(); });
    QObject::connect(model_, &QtNodes::AbstractGraphModel::modelReset,
        this, [this]() { notifyGraphChanged(); });
}

VisionFlowEditor::~VisionFlowEditor()
{
    // 显式按 View -> Scene -> Model 顺序释放，避免 QObject 的普通
    // 子对象销毁顺序使 Scene 尚在使用时 Model 就被析构。
    delete view_;
    delete scene_;
    delete model_;
}

QAction* VisionFlowEditor::deleteSelectionAction() const
{
    return view_->deleteSelectionAction();
}

QStringList VisionFlowEditor::availableNodeTypes() const
{
    QStringList names;
    if (!manager_.internal_node_manager_) {
        return names;
    }

    const auto& registry = *manager_.internal_node_manager_;
    const std::uint32_t count = registry.NodeCount();
    for (std::uint32_t i = 0; i < count; ++i) {
        FlowCV::NodeDescription description{};
        if (registry.GetNodeDescription(i, description)) {
            names.append(QString::fromStdString(description.name));
        }
    }
    names.removeDuplicates();
    names.sort(Qt::CaseInsensitive);
    return names;
}

QtNodes::NodeId VisionFlowEditor::addNode(QString const& nodeType)
{
    const QPoint center = view_->viewport()->rect().center();
    return addNodeAt(nodeType, view_->mapToScene(center));
}

QtNodes::NodeId VisionFlowEditor::addNodeAt(QString const& nodeType,
    QPointF scenePosition)
{
    const QtNodes::NodeId newId = model_->addNode(nodeType);
    if (newId == QtNodes::InvalidNodeId) {
        Q_EMIT operationFailed(QStringLiteral("Unable to create node: %1").arg(nodeType));
        return newId;
    }

    model_->setNodeData(newId, QtNodes::NodeRole::Position, scenePosition);

    scene_->clearSelection();
    if (auto* graphicsNode = scene_->nodeGraphicsObject(newId)) {
        graphicsNode->setSelected(true);
    }
    view_->setFocus();
    Q_EMIT selectedFlowNodeChanged(model_->flowNodeId(newId));
    return newId;
}

void VisionFlowEditor::deleteSelection()
{
    // 必须先复制 ID，不能在遍历 selectedItems() 时删除图形对象。
    std::vector<QtNodes::NodeId> nodes;
    std::vector<QtNodes::ConnectionId> connections;

    for (QGraphicsItem* item : scene_->selectedItems()) {
        if (auto* node = dynamic_cast<QtNodes::NodeGraphicsObject*>(item)) {
            nodes.push_back(node->nodeId());
        }
        else if (auto* connection =
            dynamic_cast<QtNodes::ConnectionGraphicsObject*>(item)) {
            connections.push_back(connection->connectionId());
        }
    }

    // 优先删除选中的连线；与选中节点关联的连线由 Manager 自己清理。
    for (auto const& connection : connections) {
        if (std::find(nodes.begin(), nodes.end(), connection.inNodeId) != nodes.end() ||
            std::find(nodes.begin(), nodes.end(), connection.outNodeId) != nodes.end()) {
            continue;
        }
        model_->deleteConnection(connection);
    }
    for (auto const nodeId : nodes) {
        model_->deleteNode(nodeId);
    }
    Q_EMIT selectedFlowNodeChanged(0);
}

void VisionFlowEditor::zoomFitAll()
{
    if (model_->allNodeIds().empty()) {
        return;
    }
    QRectF bounds = scene_->itemsBoundingRect();
    bounds.adjust(-80, -60, 80, 60);
    if (!bounds.isEmpty()) {
        view_->fitInView(bounds, Qt::KeepAspectRatio);
    }
}

void VisionFlowEditor::resetZoom()
{
    view_->setupScale(1.0);
}

QMenu* VisionFlowEditor::createCanvasMenu(QPointF scenePosition)
{
    auto* menu = new QMenu;
    deleteMenuLater(menu);

    QMenu* addMenu = menu->addMenu(QStringLiteral("Add Node"));
    const QStringList types = availableNodeTypes();
    for (QString const& type : types) {
        QAction* action = addMenu->addAction(type);
        QObject::connect(action, &QAction::triggered,
            this, [this, type, scenePosition]() {
                addNodeAt(type, scenePosition);
            });
    }
    if (types.isEmpty()) {
        addMenu->setEnabled(false);
    }
    menu->addSeparator();
    QAction* fitAction = menu->addAction(QStringLiteral("Fit All Nodes"));
    QObject::connect(fitAction, &QAction::triggered,
        this, &VisionFlowEditor::zoomFitAll);
    return menu;
}

void VisionFlowEditor::showNodeMenu(QtNodes::NodeId nodeId)
{
    const QPoint screenPosition = QCursor::pos();
    QTimer::singleShot(0, this, [this, nodeId, screenPosition]() {
        if (!model_->nodeExists(nodeId)) {
            return;
        }
        auto* menu = new QMenu;
        deleteMenuLater(menu);
        QAction* remove = menu->addAction(QStringLiteral("Delete Node"));
        QObject::connect(remove, &QAction::triggered,
            this, [this, nodeId]() { model_->deleteNode(nodeId); });
        menu->popup(screenPosition);
        });
}

void VisionFlowEditor::notifyGraphChanged()
{
    Q_EMIT graphChanged(manager_.GetNodeCount(), manager_.GetWireCount());
}
