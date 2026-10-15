
#include "MainWindow.h"
#include "ViewerPanel.h"

#include <VisionFlowEditor.h>
#include <PropertyPanel.h>

#include <flowcv_manager.h>
#include <Viewer/Viewer.hpp>

#include <cstdint>
#include <memory>
#include <exception>

#include <QtCore/QSettings>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QCloseEvent>
#include <QtGui/QColor>
#include <QtGui/QKeySequence>

#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <QtNodes/StyleCollection>
#include <QtNodes/NodeStyle>
#include <QtNodes/GraphicsViewStyle>


namespace
{
    // Qt 主窗口布局版本。
    // 后续 Dock 结构发生不兼容变化时可以递增。
    constexpr int kLayoutVersion = 1;
}


// =============================================================
// Construction
// =============================================================
MainWindow::MainWindow(FlowCV::FlowCV_Manager& manager, QWidget* parent)
    : QMainWindow(parent), manager_(manager)
{
    setupWindow();

    setupNodeList();
    setupPropertyPanel();
    setupViewerPlaceholder();

    setupDockLayout();
    setupMenusAndToolbar();

    // 先恢复主窗口布局，
    // 再创建动态 Viewer Dock。
    restoreWindowLayout();

    // ---------------------------------------------------------
    // Graph Changed
    // ---------------------------------------------------------
    QObject::connect(
        editor_,
        &VisionFlowEditor::graphChanged,
        this,
        [this](std::uint64_t, std::uint64_t)
        {
            refreshGraphStatistics();

            // 保留现有属性面板刷新。
            propertyPanel_->refresh();

            // 根据 Manager 中的 Viewer 实例，
            // 创建或销毁独立预览 Dock。
            syncViewerDocks();
        });


    // ---------------------------------------------------------
    // Selection -> PropertyPanel
    // ---------------------------------------------------------
    QObject::connect(
        editor_,
        &VisionFlowEditor::selectedFlowNodeChanged,
        propertyPanel_,
        &PropertyPanel::setNode);


    // ---------------------------------------------------------
    // Selection -> StatusBar
    // ---------------------------------------------------------
    QObject::connect(
        editor_,
        &VisionFlowEditor::selectedFlowNodeChanged,
        this,
        [this](std::uint64_t flowId)
        {
            if (flowId == 0)
            {
                statusBar()->showMessage(
                    QStringLiteral("Ready"));
            }
            else
            {
                statusBar()->showMessage(
                    QStringLiteral(
                        "Selected FlowCV node ID: %1")
                    .arg(flowId));
            }
        });


    // ---------------------------------------------------------
    // Editor Errors
    // ---------------------------------------------------------
    QObject::connect(
        editor_,
        &VisionFlowEditor::operationFailed,
        this,
        [this](QString const& message)
        {
            statusBar()->showMessage(
                message,
                5000);
        });


    // 处理创建 MainWindow 之前已经存在的 Viewer。
    syncViewerDocks();

#ifdef USE_OLD
    refreshGraphStatistics();
    statusBar()->showMessage(QStringLiteral("Ready"));
#else 
    refreshGraphStatistics();
    statusBar()->showMessage(QStringLiteral("Ready"));

    // Stage E: 初始化当前项目状态。
    savedProjectState_ = collectProjectState();
    refreshProjectTitle();

    // ---------------------------------------------------------
    // 定期比较项目状态。
    // 与原版 FlowCV 的状态快照比较思路一致。
    //
    // 用于检测节点位置、属性以及连接等变化。
    // 该定时器不参与 DSPatch 计算。
    // ---------------------------------------------------------
    auto* projectStateTimer = new QTimer(this);

    projectStateTimer->setInterval(1000);

    QObject::connect(
        projectStateTimer,
        &QTimer::timeout,
        this,
        [this]()
        {
            try
            {
                refreshProjectTitle();
            }
            catch (const std::exception&)
            {
                // 读取失败不应在 Qt 定时器回调中
                // 直接导致程序异常退出。
                // 保存与关闭时仍会明确提示错误。
            }
        });

    projectStateTimer->start();

#endif
}



// =============================================================
// Close Event
// =============================================================
void MainWindow::closeEvent(QCloseEvent* event)
{
#ifdef USE_OLD
    // 保存窗口尺寸和 Dock 布局。
    // 不保存 FlowCV 运行图。
    saveWindowLayout();

    QMainWindow::closeEvent(event);
#else 
    // ------------------------ 提醒未保存修改 ---------------------------
    if (!confirmDiscardChanges())
    {
        event->ignore();
        return;
    }

    // ---------------- 沿用原有QSettings保存Dock布局 ---------------------
    saveWindowLayout();

    // ------------------------- 正常关闭窗口 ------------------------------
    QMainWindow::closeEvent(event);

#endif
}


// =============================================================
// Main Window
// =============================================================
void MainWindow::setupWindow()
{
    setWindowTitle(
        QStringLiteral("VisionFlowStudio"));

    resize(1280, 800);

    setDockNestingEnabled(true);

    setDockOptions(
        QMainWindow::AllowNestedDocks |
        QMainWindow::AllowTabbedDocks
    );

    // 在 VisionFlowEditor 创建之前设置 QtNodes 样式。
    setupAppearance();


    // ---------------------------------------------------------
    // Editor Dock
    // ---------------------------------------------------------
    editorDock_ = new QDockWidget(
        QStringLiteral("Editor"),
        this);

    editorDock_->setObjectName(
        QStringLiteral("EditorDock"));

    editor_ = new VisionFlowEditor(
        manager_,
        editorDock_);

    editorDock_->setWidget(editor_);


    // ---------------------------------------------------------
    // Status Bar
    // ---------------------------------------------------------
    statistics_ = new QLabel(this);

    statusBar()->addPermanentWidget(
        statistics_);
}


// =============================================================
// Appearance
// =============================================================
void MainWindow::setupAppearance()
{
    // ---------------------------------------------------------
    // Qt Widgets Dark Appearance
    //
    // 仅在 Studio 主窗口及其子控件中生效。
    // ---------------------------------------------------------
    setStyleSheet(QString::fromUtf8(R"QSS(

        QMainWindow {
            background-color: #20242b;
            color: #dfe5ed;
        }

        QDockWidget {
            color: #dfe5ed;
            font-weight: normal;
        }

        QDockWidget::title {
            background-color: #303946;
            border: 1px solid #424c5a;
            padding: 5px 8px;
            text-align: left;
        }

        QMenuBar {
            background-color: #252b34;
            color: #e1e6ed;
            border-bottom: 1px solid #3b4552;
            spacing: 4px;
        }

        QMenuBar::item {
            padding: 5px 9px;
            background: transparent;
        }

        QMenuBar::item:selected {
            background-color: #3d5265;
        }

        QMenu {
            background-color: #28313b;
            color: #e1e6ed;
            border: 1px solid #495564;
            padding: 4px;
        }

        QMenu::item {
            padding: 5px 24px 5px 12px;
        }

        QMenu::item:selected {
            background-color: #39576b;
        }

        QMenu::separator {
            height: 1px;
            background-color: #46515e;
            margin: 4px 6px;
        }

        QToolBar {
            background-color: #252b34;
            border-bottom: 1px solid #3b4552;
            spacing: 5px;
            padding: 3px;
        }

        QToolBar::separator {
            background-color: #46515e;
            width: 1px;
            margin: 3px 5px;
        }

        QToolButton {
            color: #e1e6ed;
            background-color: #303946;
            border: 1px solid #45505e;
            border-radius: 3px;
            padding: 4px 7px;
        }

        QToolButton:hover {
            background-color: #405366;
        }

        QToolButton:pressed {
            background-color: #2c647a;
        }

        QStatusBar {
            background-color: #252b34;
            color: #c7d0da;
            border-top: 1px solid #3b4552;
        }

        QDockWidget QLabel, QStatusBar QLabel {
            color: #d8dfe8;
        }

        QMessageBox {
            background-color: #28313b;
            color: #dfe5ed;
        }

        QMessageBox QLabel {
            background-color: transparent;
            color: #dfe5ed;
        }

        QMessageBox QPushButton {
            background-color: #303946;
            color: #e1e6ed;
            border: 1px solid #536070;
            border-radius: 3px;
            padding: 5px 14px;
            min-width: 56px;
        }

        QMessageBox QPushButton:default,
        QMessageBox QPushButton:focus {
            border-color: #44a6c3;
        }

        QMessageBox QPushButton:hover:enabled {
            background-color: #405366;
        }

        QMessageBox QPushButton:pressed:enabled {
            background-color: #2c647a;
        }

        QMessageBox QPushButton:disabled {
            background-color: #29313b;
            color: #687584;
            border-color: #424c5a;
        }

        QLabel#ViewerPlaceholder {
            background-color: #222830;
            color: #8995a5;
            padding: 12px;
        }

        QListWidget {
            background-color: #252c35;
            color: #dfe5ed;
            border: none;
            outline: none;
        }

        QListWidget::item {
            padding: 5px 7px;
        }

        QListWidget::item:selected {
            background-color: #35566a;
            color: white;
        }

        QScrollArea {
            background-color: #252c35;
            border: none;
        }

        QScrollArea#PropertyScrollArea > QWidget,
        QWidget#PropertyContent {
            background-color: #252c35;
            color: #dfe5ed;
        }

        QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #333c48;
            color: #e1e6ed;
            border: 1px solid #536070;
            border-radius: 3px;
            padding: 3px 5px;
            min-height: 20px;
        }

        QSpinBox:focus,
        QDoubleSpinBox:focus,
        QComboBox:focus {
            border: 1px solid #44a6c3;
        }

        QSpinBox, QDoubleSpinBox {
            padding-right: 28px;
        }

        QSpinBox::up-button, QDoubleSpinBox::up-button,
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            subcontrol-origin: border;
            width: 24px;
            background-color: #303946;
            border-left: 1px solid #536070;
        }

        QSpinBox::up-button, QDoubleSpinBox::up-button {
            subcontrol-position: top right;
            border-bottom: 1px solid #536070;
            border-top-right-radius: 3px;
        }

        QSpinBox::down-button, QDoubleSpinBox::down-button {
            subcontrol-position: bottom right;
            border-bottom-right-radius: 3px;
        }

        QSpinBox::up-button:hover:enabled:!off,
        QDoubleSpinBox::up-button:hover:enabled:!off,
        QSpinBox::down-button:hover:enabled:!off,
        QDoubleSpinBox::down-button:hover:enabled:!off {
            background-color: #405366;
        }

        QSpinBox::up-button:pressed:enabled:!off,
        QDoubleSpinBox::up-button:pressed:enabled:!off,
        QSpinBox::down-button:pressed:enabled:!off,
        QDoubleSpinBox::down-button:pressed:enabled:!off {
            background-color: #2c647a;
        }

        /* :off marks a direction blocked by the minimum or maximum value. */
        QSpinBox::up-button:disabled, QSpinBox::up-button:off,
        QDoubleSpinBox::up-button:disabled, QDoubleSpinBox::up-button:off,
        QSpinBox::down-button:disabled, QSpinBox::down-button:off,
        QDoubleSpinBox::down-button:disabled, QDoubleSpinBox::down-button:off {
            background-color: #29313b;
            border-color: #424c5a;
        }

        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
            image: url(:/visionflowstudio/icons/spin-up.svg);
            width: 10px;
            height: 6px;
        }

        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
            image: url(:/visionflowstudio/icons/spin-down.svg);
            width: 10px;
            height: 6px;
        }

        QSpinBox::up-arrow:disabled, QSpinBox::up-arrow:off,
        QDoubleSpinBox::up-arrow:disabled, QDoubleSpinBox::up-arrow:off {
            image: url(:/visionflowstudio/icons/spin-up-disabled.svg);
        }

        QSpinBox::down-arrow:disabled, QSpinBox::down-arrow:off,
        QDoubleSpinBox::down-arrow:disabled, QDoubleSpinBox::down-arrow:off {
            image: url(:/visionflowstudio/icons/spin-down-disabled.svg);
        }

        QComboBox QAbstractItemView {
            background-color: #303946;
            color: #e1e6ed;
            selection-background-color: #39576b;
            selection-color: white;
        }

        QCheckBox {
            color: #dfe5ed;
            spacing: 6px;
        }

        QTabWidget::pane {
            border: 1px solid #46515e;
        }

        QTabBar::tab {
            background-color: #303946;
            color: #c9d2dc;
            border: 1px solid #46515e;
            padding: 5px 13px;
        }

        QTabBar::tab:selected {
            background-color: #40556a;
            color: white;
        }

        QTabBar::tab:hover {
            background-color: #3b4c5e;
        }

    )QSS"));


    // ---------------------------------------------------------
    // QtNodes Node Style
    //
    // 通过 QtNodes 公开样式接口设置。
    // 不修改 third_party/nodeeditor。
    // ---------------------------------------------------------
    QtNodes::NodeStyle nodeStyle =
        QtNodes::StyleCollection::nodeStyle();

    nodeStyle.setBackgroundColor(
        QColor(39, 46, 56));

    nodeStyle.NormalBoundaryColor =
        QColor(100, 116, 133);

    nodeStyle.SelectedBoundaryColor =
        QColor(69, 175, 209);

    nodeStyle.FontColor =
        QColor(232, 237, 243);

    nodeStyle.FontColorFaded =
        QColor(164, 179, 193);

    nodeStyle.ConnectionPointColor =
        QColor(80, 176, 210);

    nodeStyle.FilledConnectionPointColor =
        QColor(80, 176, 210);

    nodeStyle.PenWidth = 1.0f;

    nodeStyle.HoveredPenWidth = 1.7f;

    nodeStyle.Opacity = 1.0f;

    nodeStyle.ShadowEnabled = false;

    QtNodes::StyleCollection::setNodeStyle(
        nodeStyle);


    // ---------------------------------------------------------
    // QtNodes Canvas Style
    // ---------------------------------------------------------
    QtNodes::GraphicsViewStyle viewStyle =
        QtNodes::StyleCollection::flowViewStyle();

    viewStyle.BackgroundColor =
        QColor(32, 38, 47);

    viewStyle.FineGridColor =
        QColor(43, 51, 61);

    viewStyle.CoarseGridColor =
        QColor(57, 68, 82);

    QtNodes::StyleCollection::setGraphicsViewStyle(
        viewStyle);
}


// =============================================================
// Node Library
// =============================================================
void MainWindow::setupNodeList()
{
    nodeLibraryDock_ = new QDockWidget(
        QStringLiteral("Node Library"),
        this);

    nodeLibraryDock_->setObjectName(
        QStringLiteral("NodeLibraryDock"));

    nodeList_ = new QListWidget(
        nodeLibraryDock_);

    nodeList_->setMinimumWidth(180);

    nodeList_->addItems(
        editor_->availableNodeTypes());

    nodeList_->setToolTip(
        QStringLiteral(
            "Double-click a node to add it to the canvas"));

    nodeLibraryDock_->setWidget(
        nodeList_);

    // 保留原有的节点创建方式。
    QObject::connect(
        nodeList_,
        &QListWidget::itemDoubleClicked,
        editor_,
        [this](QListWidgetItem* item)
        {
            if (item != nullptr)
            {
                editor_->addNode(
                    item->text());
            }
        });
}


// =============================================================
// Properties
// =============================================================
void MainWindow::setupPropertyPanel()
{
    propertyDock_ = new QDockWidget(
        QStringLiteral("Properties"),
        this);

    propertyDock_->setObjectName(
        QStringLiteral("PropertiesDock"));

    propertyPanel_ = new PropertyPanel(
        manager_,
        propertyDock_);

    propertyPanel_->setMinimumWidth(220);

    propertyDock_->setWidget(
        propertyPanel_);
}


// =============================================================
// Viewer Placeholder
// =============================================================
void MainWindow::setupViewerPlaceholder()
{
    viewerDock_ = new QDockWidget(
        QStringLiteral("Viewer"),
        this);

    // 保留旧 objectName，兼容原有基础 Dock 布局。
    viewerDock_->setObjectName(
        QStringLiteral("ViewerDock"));

    auto* placeholder = new QLabel(
        QStringLiteral("No Viewer preview"),
        viewerDock_);

    placeholder->setObjectName(
        QStringLiteral("ViewerPlaceholder"));

    placeholder->setAlignment(
        Qt::AlignCenter);

    viewerDock_->setWidget(placeholder);
}



void MainWindow::syncViewerDocks()
{
    // ---------------------------------------------------------
    // 1. 查询 Manager 中当前存在的 Viewer
    // ---------------------------------------------------------
    std::map<std::uint64_t, QString> currentViewers;

    const std::uint64_t count =
        manager_.GetNodeCount();

    for (std::uint64_t i = 0; i < count; ++i)
    {
        FlowCV::NodeInfo info{};

        if (!manager_.GetNodeInfoByIndex(i, info))
            continue;

        auto viewer =
            std::dynamic_pointer_cast<
            DSPatch::DSPatchables::Viewer>(
                info.node_ptr);

        if (!viewer)
            continue;

        const QString title =
            QStringLiteral("Viewer_%1")
            .arg(viewer->GetInstanceCount());

        currentViewers.emplace(info.id, title);
    }


    // ---------------------------------------------------------
    // 2. 删除 Manager 中已经不存在的 Viewer Dock
    // ---------------------------------------------------------
    for (auto it = viewerDocks_.begin();
        it != viewerDocks_.end();)
    {
        const std::uint64_t nodeId = it->first;

        if (currentViewers.find(nodeId) !=
            currentViewers.end())
        {
            ++it;
            continue;
        }

        QDockWidget* dock = it->second;

        // 从 View 菜单中移除该 Viewer。
        viewerWindowsMenu_->removeAction(
            dock->toggleViewAction());

        // 从主窗口布局移除。
        removeDockWidget(dock);

        dock->hide();

        // 避免在 GraphModel 信号调用过程中
        // 立即销毁仍可能被 Qt 使用的控件。
        dock->deleteLater();

        it = viewerDocks_.erase(it);
    }


    // ---------------------------------------------------------
    // 3. 为新增 Viewer 创建独立 Dock
    // ---------------------------------------------------------
    for (const auto& [nodeId, title] : currentViewers)
    {
        auto existing = viewerDocks_.find(nodeId);

        if (existing != viewerDocks_.end())
        {
            // 已有 Dock 保持原位置，仅更新标题。
            existing->second->setWindowTitle(title);
            continue;
        }

        auto* dock = new QDockWidget(title, this);

        // 使用 FlowCV Node ID 生成唯一 objectName。
        // saveState / restoreDockWidget 依赖该名称。
        dock->setObjectName(
            QStringLiteral("ViewerDock_%1")
            .arg(
                static_cast<qulonglong>(nodeId)));

        dock->setAllowedAreas(
            Qt::AllDockWidgetAreas);

        dock->setFeatures(
            QDockWidget::DockWidgetClosable |
            QDockWidget::DockWidgetMovable |
            QDockWidget::DockWidgetFloatable);

        // 一个 Dock 只管理一个 ViewerPanel。
        auto* panel = new ViewerPanel(
            manager_,
            nodeId,
            dock);

        dock->setWidget(panel);


        // -----------------------------------------------------
        // 4. 尝试恢复此前保存的 Dock 位置。
        //
        // 仅当 restoreWindowLayout() 已调用后，
        // restoreDockWidget() 才适用于延迟创建的 Dock。
        // -----------------------------------------------------
        const bool restored =
            restoreDockWidget(dock);

        if (!restored)
        {
            // 默认在右侧 Viewer 区域水平排列。
            QDockWidget* anchor = viewerDock_;

            // 优先以已经存在且正常停靠的 Viewer
            // 作为相邻分割的参照。
            for (const auto& [otherId, otherDock]
                : viewerDocks_)
            {
                (void)otherId;

                if (otherDock->isFloating())
                    continue;

                if (otherDock->isHidden())
                    continue;

                // 已被手动标签化的 Dock 不适合作为
                // 默认分割锚点。
                if (!tabifiedDockWidgets(
                    otherDock).isEmpty())
                {
                    continue;
                }

                anchor = otherDock;
                break;
            }

            // 空占位区在需要分割时可临时显示。
            if (anchor == viewerDock_)
                viewerDock_->show();

            splitDockWidget(
                anchor,
                dock,
                Qt::Horizontal);

            dock->show();
        }

        // 保存 UI 映射。
        viewerDocks_.emplace(nodeId, dock);

        // 加入 View -> Viewer Windows 菜单。
        viewerWindowsMenu_->addAction(
            dock->toggleViewAction());
    }


    // ---------------------------------------------------------
    // 5. 更新占位区域与菜单
    // ---------------------------------------------------------
    viewerWindowsMenu_->setEnabled(
        !viewerDocks_.empty());

    // 存在实际 Viewer 时隐藏空占位窗口。
    // 没有 Viewer 时恢复占位区。
    viewerDock_->setVisible(
        viewerDocks_.empty());
}


// =============================================================
// Default Dock Layout
// =============================================================
void MainWindow::setupDockLayout()
{
    // ---------------------------------------------------------
    // Properties：左侧
    // ---------------------------------------------------------
    addDockWidget(
        Qt::LeftDockWidgetArea,
        propertyDock_);


    // ---------------------------------------------------------
    // Viewer：右上
    // ---------------------------------------------------------
    splitDockWidget(
        propertyDock_,
        viewerDock_,
        Qt::Horizontal);


    // ---------------------------------------------------------
    // Editor：右下
    // ---------------------------------------------------------
    splitDockWidget(
        viewerDock_,
        editorDock_,
        Qt::Vertical);


    // ---------------------------------------------------------
    // Node Library 与 Properties 组合成标签。
    // 默认显示 Properties。
    // ---------------------------------------------------------
    addDockWidget(
        Qt::LeftDockWidgetArea,
        nodeLibraryDock_);

    tabifyDockWidget(
        propertyDock_,
        nodeLibraryDock_);

    propertyDock_->raise();

    nodeLibraryDock_->hide();


    // ---------------------------------------------------------
    // 初始分割比例。
    // ---------------------------------------------------------
    resizeDocks(
        { propertyDock_, viewerDock_ },
        { width() / 5, width() * 4 / 5 },
        Qt::Horizontal);

    resizeDocks(
        { viewerDock_, editorDock_ },
        { height() * 55 / 100,
          height() * 45 / 100 },
        Qt::Vertical);
}


// =============================================================
// Reset Dock Layout
// =============================================================
void MainWindow::resetDockLayout()
{
    // ---------------------------------------------------------
    // 1. 从现有布局中移除动态 Viewer Dock
    //    不删除 Viewer 节点，也不销毁预览控件。
    // ---------------------------------------------------------
    for (const auto& [nodeId, dock] : viewerDocks_)
    {
        (void)nodeId;

        dock->hide();

        if (dock->isFloating())
            dock->setFloating(false);

        removeDockWidget(dock);
    }


    // ---------------------------------------------------------
    // 2. 恢复原有基础 Dock 布局
    // ---------------------------------------------------------
    QDockWidget* docks[] = {
        propertyDock_,
        viewerDock_,
        editorDock_,
        nodeLibraryDock_
    };

    for (QDockWidget* dock : docks)
    {
        dock->hide();

        dock->setFloating(false);

        removeDockWidget(dock);
    }

    setupDockLayout();

    propertyDock_->show();
    viewerDock_->show();
    editorDock_->show();

    nodeLibraryDock_->hide();

    if (graphToolBar_)
        graphToolBar_->show();


    // ---------------------------------------------------------
    // 3. 将所有独立 Viewer 重新放回默认区域
    // ---------------------------------------------------------
    QDockWidget* anchor = viewerDock_;

    for (const auto& [nodeId, dock] : viewerDocks_)
    {
        (void)nodeId;

        splitDockWidget(
            anchor,
            dock,
            Qt::Horizontal);

        dock->show();

        anchor = dock;
    }


    // ---------------------------------------------------------
    // 4. 根据 Viewer 数量显示或隐藏占位区
    // ---------------------------------------------------------
    viewerDock_->setVisible(
        viewerDocks_.empty());


    // ---------------------------------------------------------
    // 5. 恢复默认大小比例
    // ---------------------------------------------------------
    resizeDocks(
        { propertyDock_, viewerDock_ },
        {
            width() / 5,
            width() * 4 / 5
        },
        Qt::Horizontal);

    resizeDocks(
        { viewerDock_, editorDock_ },
        {
            height() * 55 / 100,
            height() * 45 / 100
        },
        Qt::Vertical);
}



// =============================================================
// Restore Window Layout
// =============================================================
void MainWindow::restoreWindowLayout()
{
    QSettings settings;

    settings.beginGroup(
        QStringLiteral("MainWindow"));

    const QByteArray geometry =
        settings.value(
            QStringLiteral("Geometry"))
        .toByteArray();

    const QByteArray state =
        settings.value(
            QStringLiteral("DockState"))
        .toByteArray();

    settings.endGroup();

    if (!geometry.isEmpty())
    {
        restoreGeometry(geometry);
    }

    if (!state.isEmpty())
    {
        restoreState(
            state,
            kLayoutVersion);
    }
}


// =============================================================
// Save Window Layout
// =============================================================
void MainWindow::saveWindowLayout() const
{
    QSettings settings;

    settings.beginGroup(
        QStringLiteral("MainWindow"));

    settings.setValue(
        QStringLiteral("Geometry"),
        saveGeometry());

    settings.setValue(
        QStringLiteral("DockState"),
        saveState(kLayoutVersion));

    settings.endGroup();
}


// =============================================================
// Collect Project State: 收集完整项目状态
// =============================================================
nlohmann::json MainWindow::collectProjectState() const
{
    // 运行图：节点、参数和连接。
    nlohmann::json state = manager_.GetState();

    // 编辑器：节点位置。
    state["editor"] = editor_->getEditorState();

    return state;
}


// =============================================================
// Saved Snapshot: 保存当前项目快照
// =============================================================
void MainWindow::updateSavedSnapshot()
{
    savedProjectState_ = collectProjectState();

    refreshProjectTitle();
}


// =============================================================
// Project Modified: 检查未保存修改
// =============================================================
bool MainWindow::isProjectModified() const
{
    return collectProjectState() != savedProjectState_;
}


// =============================================================
// Project Window Title: 更新窗口标题
// =============================================================
void MainWindow::refreshProjectTitle()
{
    const QString fileName =
        currentProjectFile_.isEmpty()
        ? QStringLiteral("Untitled")
        : QFileInfo(currentProjectFile_).fileName();

    const bool modified = isProjectModified();

    setWindowTitle(
        QStringLiteral("VisionFlowStudio - %1%2")
        .arg(fileName)
        .arg(modified
            ? QStringLiteral(" *")
            : QString()));
}


// =============================================================
// Confirm Unsaved Changes: 未保存修改提示
// =============================================================
bool MainWindow::confirmDiscardChanges()
{
    try
    {
        if (!isProjectModified())
        {
            return true;
        }
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(
            this,
            QStringLiteral("Project Error"),
            QStringLiteral(
                "Unable to inspect project changes:\n%1")
            .arg(QString::fromUtf8(e.what())));

        return false;
    }

    const QMessageBox::StandardButton answer =
        QMessageBox::question(
            this,
            QStringLiteral("Unsaved Changes"),
            QStringLiteral(
                "The current project has unsaved changes.\n"
                "Do you want to save them?"),
            QMessageBox::Save |
            QMessageBox::Discard |
            QMessageBox::Cancel,
            QMessageBox::Save);

    switch (answer)
    {
    case QMessageBox::Save:
        // 保存失败或取消 Save As 时，不继续操作。
        return saveProject();

    case QMessageBox::Discard:
        return true;

    case QMessageBox::Cancel:
    default:
        return false;
    }
}


// =============================================================
// Write Project File: 保存到指定文件
// =============================================================
bool MainWindow::saveProjectToFile(
    const QString& filePath)
{
    try
    {
        // -----------------------------------------------------
        // 1. 合并 Manager 和 Editor 状态。
        // -----------------------------------------------------
        const nlohmann::json state =
            collectProjectState();

        // -----------------------------------------------------
        // 2. 转换为 JSON 文本。
        // -----------------------------------------------------
        const std::string jsonText =
            state.dump(4) + "\n";

        const QByteArray data =
            QByteArray::fromStdString(jsonText);

        // -----------------------------------------------------
        // 3. 写入 .flow 文件。
        // -----------------------------------------------------
        QSaveFile file(filePath);

        if (!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::warning(
                this,
                QStringLiteral("Save Failed"),
                QStringLiteral(
                    "Unable to open project file:\n%1\n\n%2")
                .arg(filePath, file.errorString()));

            return false;
        }

        if (file.write(data) != data.size())
        {
            QMessageBox::warning(
                this,
                QStringLiteral("Save Failed"),
                QStringLiteral(
                    "Unable to write project file:\n%1\n\n%2")
                .arg(filePath, file.errorString()));

            file.cancelWriting();

            return false;
        }

        if (!file.commit())
        {
            QMessageBox::warning(
                this,
                QStringLiteral("Save Failed"),
                QStringLiteral(
                    "Unable to complete project save:\n%1\n\n%2")
                .arg(filePath, file.errorString()));

            return false;
        }

        // -----------------------------------------------------
        // 4. 保存成功后更新文件路径和基准状态。
        // -----------------------------------------------------
        currentProjectFile_ = filePath;

        savedProjectState_ = state;

        refreshProjectTitle();

        statusBar()->showMessage(
            QStringLiteral("Project saved: %1")
            .arg(filePath),
            4000);

        return true;
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(
            this,
            QStringLiteral("Save Failed"),
            QStringLiteral(
                "Unable to save project:\n%1")
            .arg(QString::fromUtf8(e.what())));

        return false;
    }
}



// =============================================================
// Save: Save操作
// =============================================================
bool MainWindow::saveProject()
{
    // 尚未选择项目路径时转为 Save As。
    if (currentProjectFile_.isEmpty())
    {
        return saveProjectAs();
    }

    return saveProjectToFile(currentProjectFile_);
}



// =============================================================
// Save As: Save as操作
// =============================================================
bool MainWindow::saveProjectAs()
{
    const QString suggestedFile =
        currentProjectFile_.isEmpty()
        ? QStringLiteral("Untitled.flow")
        : currentProjectFile_;

    QString filePath =
        QFileDialog::getSaveFileName(
            this,
            QStringLiteral("Save Flow Project"),
            suggestedFile,
            QStringLiteral(
                "FlowCV Project (*.flow)"));

    if (filePath.isEmpty())
    {
        return false;
    }

    // 保持原版 FlowCV 的 .flow 后缀。
    if (!filePath.endsWith(
        QStringLiteral(".flow"),
        Qt::CaseInsensitive))
    {
        filePath += QStringLiteral(".flow");
    }

    return saveProjectToFile(filePath);
}


// =============================================================
// load Project File: 实现项目加载，先停止自动执行，再恢复节点、连接和画布
// =============================================================
bool MainWindow::loadProjectFromFile(
    const QString& filePath)
{
    using nlohmann::json;

    json state;

    // ---------------------------------------------------------
    // 1. 读取 JSON 文件。
    // ---------------------------------------------------------
    try
    {
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(
                this,
                QStringLiteral("Open Failed"),
                QStringLiteral(
                    "Unable to open project file:\n%1\n\n%2")
                .arg(filePath, file.errorString()));

            return false;
        }

        const QByteArray data = file.readAll();

        state = json::parse(
            data.constBegin(),
            data.constEnd());

        // 与 Manager 的状态格式保持一致。
        if (!state.is_object())
        {
            QMessageBox::warning(
                this,
                QStringLiteral("Open Failed"),
                QStringLiteral(
                    "Invalid FlowCV project format."));

            return false;
        }

        // 基本结构检查应在清空现有图之前完成。
        if ((state.contains("nodes") &&
            !state["nodes"].is_null() &&
            !state["nodes"].is_array()) ||
            (state.contains("connections") &&
                !state["connections"].is_null() &&
                !state["connections"].is_array()) ||
            (state.contains("editor") &&
                !state["editor"].is_null() &&
                !state["editor"].is_array()))
        {
            QMessageBox::warning(
                this,
                QStringLiteral("Open Failed"),
                QStringLiteral(
                    "Invalid project state structure."));

            return false;
        }
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(
            this,
            QStringLiteral("Open Failed"),
            QStringLiteral(
                "Unable to read project:\n%1")
            .arg(QString::fromUtf8(e.what())));

        return false;
    }

    // ---------------------------------------------------------
    // 2. 停止 DSPatch 自动执行。
    // ---------------------------------------------------------
    manager_.StopAutoTick();

    bool managerLoaded = false;
    bool editorLoaded = false;

    QString restoreError;

    // ---------------------------------------------------------
    // 3. 恢复 Manager 和 Editor。
    // ---------------------------------------------------------
    try
    {
        // 恢复节点、参数和连接。
        managerLoaded = manager_.SetState(state);

        // 根据 Manager 中实际存在的节点重建画布，
        // 随后恢复 editor 节点位置。
        editorLoaded =
            editor_->restoreFromManager(state);
    }
    catch (const std::exception& e)
    {
        restoreError = QString::fromUtf8(e.what());

        // 尝试让画布重新对应 Manager 当前实际状态。
        try
        {
            editor_->restoreFromManager(json::object());
        }
        catch (...)
        {
        }
    }

    // ---------------------------------------------------------
    // 4. 清理旧选中状态，并同步属性与 Viewer Dock。
    // ---------------------------------------------------------
    propertyPanel_->setNode(0);
    propertyPanel_->refresh();

    syncViewerDocks();

    refreshGraphStatistics();

    // ---------------------------------------------------------
    // 5. 恢复原有自动执行方式。
    // ---------------------------------------------------------
    manager_.StartAutoTick();

    // ---------------------------------------------------------
    // 6. 检查加载结果。
    // ---------------------------------------------------------
    if (!managerLoaded || !editorLoaded)
    {
        QString message =
            QStringLiteral(
                "The project could not be fully restored.\n"
                "The current graph reflects the nodes that "
                "FlowCV Manager successfully loaded.");

        if (!restoreError.isEmpty())
        {
            message +=
                QStringLiteral("\n\n%1")
                .arg(restoreError);
        }

        QMessageBox::warning(
            this,
            QStringLiteral("Open Failed"),
            message);

        return false;
    }

    // ---------------------------------------------------------
    // 7. 加载成功后更新当前文件路径和状态。
    // ---------------------------------------------------------
    currentProjectFile_ = filePath;

    updateSavedSnapshot();

    statusBar()->showMessage(
        QStringLiteral("Project loaded: %1")
        .arg(filePath),
        4000);

    return true;
}


// =============================================================
// New Project: 实现New 和 Open
// =============================================================
void MainWindow::newProject()
{
    if (!confirmDiscardChanges())
    {
        return;
    }

    manager_.StopAutoTick();

    bool restored = false;
    QString error;

    try
    {
        // 沿用 FlowCV Manager 的 NewState。
        manager_.NewState();

        // 重建空白 QtNodes 画布。
        restored = editor_->restoreFromManager(
            nlohmann::json::object());
    }
    catch (const std::exception& e)
    {
        error = QString::fromUtf8(e.what());
    }

    propertyPanel_->setNode(0);
    propertyPanel_->refresh();

    syncViewerDocks();
    refreshGraphStatistics();

    manager_.StartAutoTick();

    if (!restored)
    {
        QMessageBox::warning(
            this,
            QStringLiteral("New Project Failed"),
            error.isEmpty()
            ? QStringLiteral(
                "Unable to reset the editor.")
            : error);

        return;
    }

    currentProjectFile_.clear();

    updateSavedSnapshot();

    statusBar()->showMessage(
        QStringLiteral("New project created"),
        4000);
}

void MainWindow::openProject()
{
    const QString filePath =
        QFileDialog::getOpenFileName(
            this,
            QStringLiteral("Open Flow Project"),
            QString(),
            QStringLiteral(
                "FlowCV Project (*.flow)"));

    if (filePath.isEmpty())
    {
        return;
    }

    // 用户选择文件后，再确认是否丢弃当前修改。
    if (!confirmDiscardChanges())
    {
        return;
    }

    loadProjectFromFile(filePath);
}



// =============================================================
// Menus And Toolbar
// =============================================================
void MainWindow::setupMenusAndToolbar()
{
    // ---------------------------------------------------------
    // File
    // ---------------------------------------------------------
    QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

    // New
    QAction* newAction = fileMenu->addAction(QStringLiteral("&New"));

    newAction->setShortcut(QKeySequence::New);

    QObject::connect(newAction, &QAction::triggered, this, &MainWindow::newProject);

    // Open
    QAction* openAction = fileMenu->addAction(QStringLiteral("&Open..."));

    // 与原版 FlowCV 的 Ctrl+L 保持一致。
    // 同时支持常见的 Ctrl+O。
    openAction->setShortcuts({
        QKeySequence::Open,
        QKeySequence(QStringLiteral("Ctrl+L"))
        });

    QObject::connect(
        openAction,
        &QAction::triggered,
        this,
        &MainWindow::openProject);

    fileMenu->addSeparator();

    // Save
    QAction* saveAction =
        fileMenu->addAction(
            QStringLiteral("&Save"));

    saveAction->setShortcut(QKeySequence::Save);

    QObject::connect(
        saveAction,
        &QAction::triggered,
        this,
        [this]()
        {
            saveProject();
        });

    // Save As
    QAction* saveAsAction =
        fileMenu->addAction(
            QStringLiteral("Save &As..."));

    saveAsAction->setShortcut(
        QKeySequence::SaveAs);

    QObject::connect(
        saveAsAction,
        &QAction::triggered,
        this,
        [this]()
        {
            saveProjectAs();
        });

    fileMenu->addSeparator();

    // Exit
    QAction* quit =
        fileMenu->addAction(
            QStringLiteral("E&xit"));

    QObject::connect(
        quit,
        &QAction::triggered,
        this,
        &QWidget::close);


    // ---------------------------------------------------------
    // Node
    // ---------------------------------------------------------
    QMenu* nodeMenu =
        menuBar()->addMenu(
            QStringLiteral("&Node"));

    QMenu* addMenu =
        nodeMenu->addMenu(
            QStringLiteral("Add Node"));

    const QStringList types =
        editor_->availableNodeTypes();

    for (QString const& type : types)
    {
        QAction* action =
            addMenu->addAction(type);

        QObject::connect(
            action,
            &QAction::triggered,
            editor_,
            [this, type]()
            {
                editor_->addNode(type);
            });
    }

    if (types.isEmpty())
    {
        addMenu->setEnabled(false);
    }


    // ---------------------------------------------------------
    // Edit
    // ---------------------------------------------------------
    QMenu* editMenu =
        menuBar()->addMenu(
            QStringLiteral("&Edit"));

    editMenu->addAction(
        editor_->deleteSelectionAction());


    // ---------------------------------------------------------
    // View
    // ---------------------------------------------------------
    QMenu* viewMenu =
        menuBar()->addMenu(
            QStringLiteral("&View"));

    viewMenu->addAction(
        propertyDock_->toggleViewAction());

    // 每个 Viewer 都通过独立的 toggleViewAction
    // 控制显示和隐藏。
    viewerWindowsMenu_ =
        viewMenu->addMenu(
            QStringLiteral("Viewer Windows"));

    viewerWindowsMenu_->setEnabled(false);

    viewMenu->addAction(
        editorDock_->toggleViewAction());

    viewMenu->addAction(
        nodeLibraryDock_->toggleViewAction());

    viewMenu->addSeparator();


    // Fit All
    QAction* fit =
        viewMenu->addAction(
            QStringLiteral("Fit All Nodes"));

    QObject::connect(
        fit,
        &QAction::triggered,
        editor_,
        &VisionFlowEditor::zoomFitAll);


    // Reset Zoom
    QAction* resetZoom =
        viewMenu->addAction(
            QStringLiteral("Reset Zoom"));

    QObject::connect(
        resetZoom,
        &QAction::triggered,
        editor_,
        &VisionFlowEditor::resetZoom);

    viewMenu->addSeparator();


    // Reset Layout
    QAction* resetLayout =
        viewMenu->addAction(
            QStringLiteral("Reset Layout"));

    QObject::connect(
        resetLayout,
        &QAction::triggered,
        this,
        &MainWindow::resetDockLayout);


    // ---------------------------------------------------------
    // Toolbar
    // ---------------------------------------------------------
    graphToolBar_ =
        addToolBar(
            QStringLiteral("Graph"));

    graphToolBar_->setObjectName(
        QStringLiteral("GraphToolBar"));

    graphToolBar_->setMovable(false);


    auto* addButton =
        new QToolButton(graphToolBar_);

    addButton->setText(
        QStringLiteral("Add Node"));

    addButton->setMenu(addMenu);

    addButton->setPopupMode(
        QToolButton::InstantPopup);

    addButton->setEnabled(
        !types.isEmpty());

    graphToolBar_->addWidget(
        addButton);


    // Delete
    graphToolBar_->addAction(
        editor_->deleteSelectionAction());

    graphToolBar_->addSeparator();

    // Zoom
    graphToolBar_->addAction(fit);
    graphToolBar_->addAction(resetZoom);
}



// =============================================================
// Graph Statistics
// =============================================================
void MainWindow::refreshGraphStatistics()
{
    statistics_->setText(
        QStringLiteral(
            "Nodes: %1   Connections: %2")
        .arg(manager_.GetNodeCount())
        .arg(manager_.GetWireCount()));
}
