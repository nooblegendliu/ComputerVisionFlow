#include "VisionFlowEditor.h"

#include <VisionFlowGraphModel.hpp>
#include <flowcv_manager.h>
#include <node_manager.h>
#include "Graph/AutoTickPause.hpp"
#include "Graph/GraphPainters.hpp"

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <set>

#include <QtCore/QTimer>
#include <QtCore/QMap>
#include <QtGui/QAction>
#include <QtGui/QCursor>
#include <QtGui/QKeySequence>
#include <QtGui/QClipboard>
#include <QtGui/QShortcut>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QVBoxLayout>

#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/GraphicsView>
#include <QtNodes/StyleCollection>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>


namespace {

    // QtNodes 默认空白处菜单由 BasicGraphicsScene::createSceneMenu 提供。
    // 这里只定制菜单入口，不修改 QtNodes 第三方库。
    class VisionFlowGraphicsScene final : public QtNodes::BasicGraphicsScene
    {
    public:
        using MenuFactory = std::function<QMenu* (QPointF)>;
        using DraftMenuFactory = std::function<void(QPointF, QtNodes::ConnectionId)>;
        DraftMenuFactory draftMenuFactory;

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

    protected:
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
        {
            // Inspect the draft before QtNodes releases and destroys it.
            auto* draft = dynamic_cast<QtNodes::ConnectionGraphicsObject*>(mouseGrabberItem());
            bool emptyDrop = draft && draft->connectionState().requiresPort()
                && event->button() == Qt::LeftButton;
            QtNodes::ConnectionId pending{};
            if (emptyDrop) {
                pending = draft->connectionId();
                for (auto* item : items(event->scenePos())) {
                    if (dynamic_cast<QtNodes::NodeGraphicsObject*>(item)) { emptyDrop = false; break; }
                }
            }
            const auto position = event->scenePos();
            BasicGraphicsScene::mouseReleaseEvent(event);
            if (emptyDrop && draftMenuFactory) {
                QTimer::singleShot(0, this, [this, position, pending] {
                    draftMenuFactory(position, pending);
                });
            }
        }

    private:
        MenuFactory menuFactory_;
    };

    // 当前 GraphModel 尚未实现 saveNode/loadNode，不能允许删除后的
    // QtNodes 原生 Undo 操作恢复出只有外观、没有 FlowCV 实例的节点。
    void disableUnsupportedGraphActions(QtNodes::GraphicsView* view)
    {
        for (QAction* action : view->actions()) {
            // 保留删除选择和清空选择
            if (action == view->deleteSelectionAction() ||
                action == view->clearSelectionAction()) {
                continue;
            }

#ifndef impl
            // 移除View快捷键注册
            view->removeAction(action);

            // 移除原有快捷键
            action->setShortcuts(QList<QKeySequence>{});

            // 禁用动作
            action->setEnabled(false);
#else
            
            // QtNodes 当前实现中，此时 view 的其余 actions 为
            // Duplicate、Copy、Paste、Undo、Redo。
            action->setEnabled(false);
            action->setShortcuts(QList<QKeySequence>{});
#endif
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
    // 依赖顺序：model -> scene -> view；析构函数显式按相反顺序释放。
    model_ = new VisionFlowGraphModel(manager_, this);

    scene_ = new VisionFlowGraphicsScene(
        *model_,
        [this](QPointF position) { return createCanvasMenu(position); },
        this);

    view_ = new QtNodes::GraphicsView(scene_, this);
    view_->setObjectName(QStringLiteral("VisionFlowGraphView"));
    view_->setFocusPolicy(Qt::StrongFocus);
    view_->setScaleRange(0.15, 3.0);

    scene_->setNodePainter(std::make_unique<FlowNodePainter>(manager_));
    auto connectionPainter = std::make_unique<FlowConnectionPainter>();
    auto* animatedPainter = connectionPainter.get();
    scene_->setConnectionPainter(std::move(connectionPainter));
    auto* animationTimer = new QTimer(this);
    animationTimer->setInterval(33);
    connect(animationTimer, &QTimer::timeout, scene_, [this] { scene_->update(); });
    showFlowAction_ = new QAction(QStringLiteral("Show Flow"), this);
    showFlowAction_->setCheckable(true);
    connect(showFlowAction_, &QAction::toggled, this, [this, animationTimer, animatedPainter](bool enabled) {
        animatedPainter->setAnimated(enabled);
        if (enabled) animationTimer->start(); else animationTimer->stop();
        scene_->update();
    });
    static_cast<VisionFlowGraphicsScene*>(scene_)->draftMenuFactory =
        [this](QPointF position, QtNodes::ConnectionId pending) {
            auto* menu = new QMenu;
            deleteMenuLater(menu);
            populateNodeMenu(menu, position, pending);
            menu->popup(view_->viewport()->mapToGlobal(view_->mapFromScene(position)));
        };

    disableUnsupportedGraphActions(view_);
    auto shortcut = [this](QKeySequence sequence, auto callback) {
        auto* item = new QShortcut(sequence, view_);
        item->setContext(Qt::WidgetWithChildrenShortcut);
        connect(item, &QShortcut::activated, this, callback);
    };
    shortcut(QKeySequence::Copy, [this] { copySelection(); });
    shortcut(QKeySequence::Cut, [this] { cutSelection(); });
    shortcut(QKeySequence::Paste, [this] {
        pasteAt(view_->mapToScene(view_->viewport()->mapFromGlobal(QCursor::pos())));
    });
    shortcut(QKeySequence(Qt::Key_D), [this] { toggleSelectedEnabled(); });
    shortcut(QKeySequence(Qt::Key_F), [this] { zoomFitSelection(); });
    shortcut(QKeySequence(Qt::Key_Tab), [this] { showNodeSearch(); });
    shortcut(QKeySequence(Qt::Key_S), [this] { showFlowAction_->toggle(); });

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
    connect(scene_, &QtNodes::BasicGraphicsScene::nodeDoubleClicked,
        this, [this](QtNodes::NodeId id) { Q_EMIT nodeUiRequested(model_->flowNodeId(id)); });
    connect(model_, &VisionFlowGraphModel::nodeEnabledChanged,
        this, [this](QtNodes::NodeId) { notifyGraphChanged(); });

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


    // QtNodes 当前版本在 NodeGraphicsObject 构造时
    // 使用 toJsonObject() 读取 QVariantMap，导致透明度错误。
    // 在图形节点创建完成后恢复原有 NodeStyle 的透明度。
    auto restoreNodeOpacity = [this](QtNodes::NodeId nodeId) {
        auto* node = scene_->nodeGraphicsObject(nodeId);

        if (node) {
            node->setOpacity(
                QtNodes::StyleCollection::nodeStyle().Opacity);
        }
        };

    // 新建节点
    QObject::connect(
        model_,
        &QtNodes::AbstractGraphModel::nodeCreated,
        this,
        restoreNodeOpacity);

    // 整体重建画布
    QObject::connect(
        model_,
        &QtNodes::AbstractGraphModel::modelReset,
        this,
        [this, restoreNodeOpacity]() {
            for (auto nodeId : model_->allNodeIds()) {
                restoreNodeOpacity(nodeId);
            }
        });

    // 处理构造 Editor 前已经存在的节点
    for (auto nodeId : model_->allNodeIds()) {
        restoreNodeOpacity(nodeId);
    }
}


VisionFlowEditor::~VisionFlowEditor()
{
    // 销毁图形节点会触发 selectionChanged；此时外部窗口可能已在析构。
    // 先断开内部对象到 Editor 的回调，停止选择通知和界面刷新。
    QObject::disconnect(scene_, nullptr, this, nullptr);
    QObject::disconnect(model_, nullptr, this, nullptr);

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


// Stage E: Export Editor State
// 导出节点位置
nlohmann::json VisionFlowEditor::getEditorState() const
{
    using nlohmann::json;

    json editorState = json::array();

    const std::uint64_t count = manager_.GetNodeCount();

    for (std::uint64_t i = 0; i < count; ++i)
    {
        const std::uint64_t flowId =
            manager_.GetNodeIdFromIndex(i);

        // FlowCV ID 转换为 QtNodes ID。
        const QtNodes::NodeId qtId =
            model_->qtNodeId(flowId);

        if (qtId == QtNodes::InvalidNodeId ||
            !model_->nodeExists(qtId))
        {
            throw std::runtime_error(
                "Editor node mapping is inconsistent");
        }

        const QVariant positionData =
            model_->nodeData(
                qtId,
                QtNodes::NodeRole::Position);

        if (!positionData.canConvert<QPointF>())
        {
            throw std::runtime_error(
                "Unable to read editor node position");
        }

        const QPointF position =
            positionData.toPointF();

        // 使用真实 FlowCV ID 保存。
        // 格式与原版 FlowCV 的 editor 字段一致。
        json nodeState = {
            {"id", flowId},
            {"location", {
                {"x", position.x()},
                {"y", position.y()}
            }}
        };

        editorState.push_back(
            std::move(nodeState));
    }

    return editorState;
}


// 恢复节点位置
bool VisionFlowEditor::restoreFromManager(
    const nlohmann::json& state)
{
    using nlohmann::json;

    struct SavedPosition
    {
        std::uint64_t flowId;
        QPointF position;
    };

    std::vector<SavedPosition> savedPositions;

    // ---------------------------------------------------------
    // 1. 读取项目中的 editor 字段。
    //    兼容没有该字段的旧文件。
    // ---------------------------------------------------------
    try
    {
        if (state.contains("editor") &&
            !state["editor"].is_null())
        {
            if (!state["editor"].is_array())
            {
                return false;
            }

            for (const auto& item : state["editor"])
            {
                const std::uint64_t flowId =
                    item.at("id").get<std::uint64_t>();

                const auto& location =
                    item.at("location");

                const double x =
                    location.at("x").get<double>();

                const double y =
                    location.at("y").get<double>();

                if (!std::isfinite(x) ||
                    !std::isfinite(y))
                {
                    return false;
                }

                savedPositions.push_back({
                    flowId,
                    QPointF(x, y)
                    });
            }
        }
    }
    catch (const std::exception&)
    {
        return false;
    }

    // ---------------------------------------------------------
    // 2. 清理当前选择。
    // ---------------------------------------------------------
    scene_->clearSelection();

    // ---------------------------------------------------------
    // 3. 从 Manager 重建 QtNodes 图模型。
    //
    // true 表示不沿用旧项目的位置。
    // modelReset 会通知 BasicGraphicsScene 重建画布。
    // ---------------------------------------------------------
    if (!model_->rebuildFromManager(true))
    {
        return false;
    }

    // ---------------------------------------------------------
    // 4. 恢复保存的节点位置。
    // ---------------------------------------------------------
    for (const SavedPosition& item : savedPositions)
    {
        const QtNodes::NodeId qtId =
            model_->qtNodeId(item.flowId);

        // 文件中可能存在已经不存在的节点位置记录。
        // 保留 Manager 当前实际加载的节点为准。
        if (qtId == QtNodes::InvalidNodeId)
        {
            continue;
        }

        if (!model_->setNodeData(
            qtId,
            QtNodes::NodeRole::Position,
            item.position))
        {
            return false;
        }
    }

    // ---------------------------------------------------------
    // 5. 清理旧节点选择状态。
    // ---------------------------------------------------------
    Q_EMIT selectedFlowNodeChanged(0);

    return true;
}



QStringList VisionFlowEditor::availableNodeTypes() const
{
    QStringList names;
    if (manager_.internal_node_manager_) {
        const auto& registry = *manager_.internal_node_manager_;
        for (std::uint32_t i = 0; i < registry.NodeCount(); ++i) {
            FlowCV::NodeDescription description{};
            if (registry.GetNodeDescription(i, description)) {
                names.append(QString::fromStdString(description.name));
            }
        }
    }
    if (manager_.plugin_manager_) {
        auto& registry = *manager_.plugin_manager_;
        for (std::uint32_t i = 0; i < registry.PluginCount(); ++i) {
            FlowCV::NodeDescription description{};
            if (registry.GetPluginDescription(i, description)) {
                names.append(QString::fromStdString(description.name));
            }
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
    AutoTickPause pause(manager_);
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
    populateNodeMenu(addMenu, scenePosition);
    menu->addAction(QStringLiteral("Paste"), this, &VisionFlowEditor::pasteSelection);
    menu->addAction(showFlowAction_);
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
        menu->addAction(QStringLiteral("Open Controls"), this, [this, nodeId] {
            Q_EMIT nodeUiRequested(model_->flowNodeId(nodeId));
        });
        menu->addAction(QStringLiteral("Node Information"), this, [this, nodeId] {
            QMessageBox::information(this, QStringLiteral("Node Information"), model_->nodeInformation(nodeId));
        });
        auto* enabled = menu->addAction(QStringLiteral("Enabled"));
        enabled->setCheckable(true);
        enabled->setChecked(model_->nodeEnabled(nodeId));
        connect(enabled, &QAction::toggled, this, [this, nodeId](bool value) {
            model_->setNodeEnabled(nodeId, value);
        });
        menu->addSeparator();
        menu->addAction(QStringLiteral("Copy Selection"), this, [this] { copySelection(); });
        menu->addAction(QStringLiteral("Cut Selection"), this, &VisionFlowEditor::cutSelection);
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

QString VisionFlowEditor::nodeCategory(QString const& nodeType) const
{
    FlowCV::NodeDescription description{};
    const auto name = nodeType.toStdString();
    bool found = false;
    if (manager_.plugin_manager_) {
        for (std::uint32_t i=0; i<manager_.plugin_manager_->PluginCount(); ++i) {
            if (manager_.plugin_manager_->GetPluginDescription(i, description) && description.name == name) {
                found = true; break;
            }
        }
    }
    if (!found && manager_.internal_node_manager_) {
        for (std::uint32_t i=0; i<manager_.internal_node_manager_->NodeCount(); ++i) {
            if (manager_.internal_node_manager_->GetNodeDescription(i, description) && description.name == name) {
                found = true; break;
            }
        }
    }
    if (found) {
        if (description.category == DSPatch::Category::Category_OpenVino) return QStringLiteral("OpenVino");
        const auto& categories = DSPatch::getCategories();
        auto category = categories.find(description.category);
        if (category != categories.end()) return QString::fromUtf8(category->second);
    }
    return QStringLiteral("Other");
}

void VisionFlowEditor::populateNodeMenu(QMenu* menu, QPointF position, QtNodes::ConnectionId pending)
{
    QMap<QString, QMenu*> categories;
    for (const auto& type : availableNodeTypes()) {
        const auto category = nodeCategory(type);
        if (!categories.contains(category)) categories[category] = menu->addMenu(category);
        categories[category]->addAction(type, this, [this, type, position, pending] {
            createNodeFromPort(type, position, pending);
        });
    }
    menu->setEnabled(!categories.isEmpty());
}

void VisionFlowEditor::createNodeFromPort(QString const& type, QPointF position, QtNodes::ConnectionId pending)
{
    AutoTickPause pause(manager_);
    const auto id = addNodeAt(type, position);
    if (id == QtNodes::InvalidNodeId) return;
    if (pending.outNodeId != QtNodes::InvalidNodeId) {
        pending.inNodeId = id;
        pending.inPortIndex = 0;
    } else if (pending.inNodeId != QtNodes::InvalidNodeId) {
        pending.outNodeId = id;
        pending.outPortIndex = 0;
    } else {
        return;
    }
    if (model_->connectionPossible(pending)) model_->addConnection(pending);
}

void VisionFlowEditor::showNodeSearch()
{
    const auto position = view_->mapToScene(view_->viewport()->mapFromGlobal(QCursor::pos()));
    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(QStringLiteral("Add Node"));
    dialog->setWindowModality(Qt::WindowModal);
    auto* layout = new QVBoxLayout(dialog);
    auto* search = new QLineEdit(dialog);
    search->setPlaceholderText(QStringLiteral("Search nodes..."));
    auto* list = new QListWidget(dialog);
    layout->addWidget(search);
    layout->addWidget(list);
    for (const auto& type : availableNodeTypes()) {
        auto* item = new QListWidgetItem(QStringLiteral("%1  ·  %2").arg(type, nodeCategory(type)), list);
        item->setData(Qt::UserRole, type);
    }
    auto selectFirst = [list] {
        for (int i=0; i<list->count(); ++i) {
            if (!list->item(i)->isHidden()) { list->setCurrentRow(i); return; }
        }
        list->setCurrentRow(-1);
    };
    connect(search, &QLineEdit::textChanged, dialog, [list, selectFirst](const QString& text) {
        for (int i=0; i<list->count(); ++i) {
            auto* item = list->item(i);
            item->setHidden(!item->data(Qt::UserRole).toString().contains(text, Qt::CaseInsensitive));
        }
        selectFirst();
    });
    auto activate = [this, dialog, list, position] {
        auto* item = list->currentItem();
        if (!item || item->isHidden()) return;
        addNodeAt(item->data(Qt::UserRole).toString(), position);
        dialog->accept();
    };
    connect(search, &QLineEdit::returnPressed, dialog, activate);
    connect(list, &QListWidget::itemActivated, dialog, [activate](QListWidgetItem*) { activate(); });
    selectFirst();
    dialog->resize(400, 420);
    dialog->show();
    search->setFocus();
}

void VisionFlowEditor::zoomFitSelection()
{
    QRectF bounds;
    bool hasSelection = false;
    for (auto* item : scene_->selectedItems()) {
        if (dynamic_cast<QtNodes::NodeGraphicsObject*>(item)) {
            bounds = hasSelection ? bounds.united(item->sceneBoundingRect()) : item->sceneBoundingRect();
            hasSelection = true;
        }
    }
    if (!hasSelection) { zoomFitAll(); return; }
    bounds.adjust(-80,-60,80,60);
    view_->fitInView(bounds, Qt::KeepAspectRatio);
}

void VisionFlowEditor::toggleSelectedEnabled()
{
    AutoTickPause pause(manager_);
    for (auto* item : scene_->selectedItems()) {
        if (auto* node = dynamic_cast<QtNodes::NodeGraphicsObject*>(item))
            model_->setNodeEnabled(node->nodeId(), !model_->nodeEnabled(node->nodeId()));
    }
}

nlohmann::json VisionFlowEditor::selectedSubgraph() const
{
    AutoTickPause pause(manager_);
    using nlohmann::json;
    json result = {{"nodes", json::array()}, {"connections", json::array()}, {"editor", json::array()}};
    std::vector<QtNodes::NodeId> selected;
    std::unordered_set<std::uint64_t> ids;
    for (auto* item : scene_->selectedItems()) {
        if (auto* node = dynamic_cast<QtNodes::NodeGraphicsObject*>(item)) selected.push_back(node->nodeId());
    }
    std::sort(selected.begin(), selected.end());
    for (auto id : selected) {
        const auto flowId = model_->flowNodeId(id);
        FlowCV::NodeInfo info{};
        if (!manager_.GetNodeInfoById(flowId, info)) throw std::runtime_error("Selected node no longer exists");
        ids.insert(flowId);
        json node = {{"id", flowId}, {"name", info.desc.name},
            {"enabled", info.node_ptr->IsEnabled()}, {"num", info.node_ptr->GetInstanceCount()}};
        const auto params = info.node_ptr->GetState();
        if (!params.empty()) node["params"] = json::parse(params);
        result["nodes"].push_back(std::move(node));
        const auto p = model_->nodeData(id, QtNodes::NodeRole::Position).toPointF();
        result["editor"].push_back({{"id", flowId}, {"location", {{"x", p.x()}, {"y", p.y()}}}});
    }
    for (std::uint64_t i=0; i<manager_.GetWireCount(); ++i) {
        const auto wire = manager_.GetWireInfoFromIndex(i);
        if (ids.count(wire.from.id) && ids.count(wire.to.id))
            result["connections"].push_back({{"from_id", wire.from.id}, {"from_idx", wire.from.index},
                {"to_id", wire.to.id}, {"to_idx", wire.to.index}});
    }
    return result;
}

bool VisionFlowEditor::copySelection()
{
    try {
        const auto state = selectedSubgraph();
        if (state["nodes"].empty()) return false;
        QApplication::clipboard()->setText(QString::fromStdString(state.dump(2)));
        return true;
    } catch (const std::exception& error) {
        Q_EMIT operationFailed(QStringLiteral("Copy failed: %1").arg(QString::fromUtf8(error.what())));
        return false;
    }
}

void VisionFlowEditor::cutSelection()
{
    if (copySelection()) deleteSelection();
}

void VisionFlowEditor::pasteSelection()
{
    pasteAt(view_->mapToScene(view_->viewport()->rect().center()));
}

bool VisionFlowEditor::pasteAt(QPointF position)
{
    try {
        return pasteSubgraph(nlohmann::json::parse(QApplication::clipboard()->text().toStdString()), position);
    } catch (const std::exception& error) {
        Q_EMIT operationFailed(QStringLiteral("Paste failed: %1").arg(QString::fromUtf8(error.what())));
        return false;
    }
}

bool VisionFlowEditor::pasteSubgraph(const nlohmann::json& state, QPointF anchor)
{
    AutoTickPause pause(manager_);
    std::vector<QtNodes::NodeId> created;
    try {
        auto integer = [](const nlohmann::json& value, bool nonzero) -> std::uint64_t {
            if (!value.is_number_integer() ||
                (value.is_number_integer() && !value.is_number_unsigned() && value.get<std::int64_t>() < 0))
                throw std::runtime_error("Clipboard IDs and port indexes must be nonnegative integers");
            const auto result = value.get<std::uint64_t>();
            if (nonzero && result == 0) throw std::runtime_error("Clipboard node ID is zero");
            return result;
        };
        if (!std::isfinite(anchor.x()) || !std::isfinite(anchor.y())) throw std::runtime_error("Invalid paste position");
        if (!state.is_object() || !state.contains("nodes") || !state["nodes"].is_array() || state["nodes"].empty())
            throw std::runtime_error("Clipboard does not contain a FlowCV subgraph");
        const auto available = availableNodeTypes();
        std::unordered_set<std::uint64_t> ids;
        for (const auto& node : state["nodes"]) {
            const auto id = integer(node.at("id"), true);
            if (!ids.insert(id).second) throw std::runtime_error("Duplicate clipboard node ID");
            const auto name = node.at("name").get<std::string>();
            if (!available.contains(QString::fromStdString(name)))
                throw std::runtime_error("Node/plugin unavailable: " + name);
            if (node.contains("enabled") && !node["enabled"].is_boolean())
                throw std::runtime_error("Invalid enabled state");
        }
        std::unordered_map<std::uint64_t, QPointF> positions;
        QPointF origin;
        bool hasOrigin = false;
        if (state.contains("editor") && !state["editor"].is_null()) {
            if (!state["editor"].is_array()) throw std::runtime_error("Invalid clipboard editor state");
            for (const auto& item : state["editor"]) {
                const auto id = integer(item.at("id"), true);
                const auto& location = item.at("location");
                QPointF position(location.at("x").get<double>(), location.at("y").get<double>());
                if (!std::isfinite(position.x()) || !std::isfinite(position.y()))
                    throw std::runtime_error("Invalid clipboard node position");
                // Original FlowCV can include positions for nodes omitted from its node list.
                if (!ids.count(id)) continue;
                if (!positions.emplace(id, position).second) throw std::runtime_error("Duplicate clipboard node position");
                if (!hasOrigin) { origin = position; hasOrigin = true; }
            }
        }
        struct Wire { std::uint64_t from, to; QtNodes::PortIndex out, in; };
        std::vector<Wire> wires;
        std::set<std::pair<std::uint64_t, QtNodes::PortIndex>> inputs;
        if (state.contains("connections") && !state["connections"].is_null()) {
            if (!state["connections"].is_array()) throw std::runtime_error("Invalid clipboard connections");
            for (const auto& wire : state["connections"]) {
                const auto from = integer(wire.at("from_id"), true), to = integer(wire.at("to_id"), true);
                const auto out = integer(wire.at("from_idx"), false), in = integer(wire.at("to_idx"), false);
                if (!ids.count(from) || !ids.count(to) || out >= QtNodes::InvalidPortIndex || in >= QtNodes::InvalidPortIndex)
                    throw std::runtime_error("Clipboard connection references an invalid node or port");
                if (!inputs.emplace(to, static_cast<QtNodes::PortIndex>(in)).second)
                    throw std::runtime_error("Clipboard has multiple connections to one input");
                wires.push_back({from, to, static_cast<QtNodes::PortIndex>(out), static_cast<QtNodes::PortIndex>(in)});
            }
        }
        std::unordered_map<std::uint64_t, QtNodes::NodeId> remap;
        for (const auto& node : state["nodes"]) {
            const auto oldId = integer(node.at("id"), true);
            const auto id = model_->addNode(QString::fromStdString(node.at("name").get<std::string>()));
            if (id == QtNodes::InvalidNodeId) throw std::runtime_error("Could not instantiate pasted node");
            created.push_back(id);
            remap.emplace(oldId, id);
            FlowCV::NodeInfo info{};
            if (!manager_.GetNodeInfoById(model_->flowNodeId(id), info)) throw std::runtime_error("Pasted node missing");
            if (node.contains("params")) info.node_ptr->SetState(node["params"].dump());
            // Keep the freshly assigned instance number; copied numbers belong to the source graph.
            info.node_ptr->SetEnabled(node.value("enabled", true));
            const auto position = positions.find(oldId);
            model_->setNodeData(id, QtNodes::NodeRole::Position,
                position == positions.end() ? anchor : position->second + anchor - origin);
            Q_EMIT model_->nodeUpdated(id);
        }
        for (const auto& wire : wires) {
            QtNodes::ConnectionId connection{remap.at(wire.from), wire.out, remap.at(wire.to), wire.in};
            if (!model_->connectionPossible(connection)) throw std::runtime_error("Pasted connection is incompatible");
            model_->addConnection(connection);
            if (!model_->connectionExists(connection)) throw std::runtime_error("Could not connect pasted nodes");
        }
        scene_->clearSelection();
        for (auto id : created) if (auto* item = scene_->nodeGraphicsObject(id)) item->setSelected(true);
        view_->setFocus();
        return true;
    } catch (const std::exception& error) {
        for (auto i=created.rbegin(); i!=created.rend(); ++i) model_->deleteNode(*i);
        Q_EMIT operationFailed(QStringLiteral("Paste failed: %1").arg(QString::fromUtf8(error.what())));
        return false;
    }
}
