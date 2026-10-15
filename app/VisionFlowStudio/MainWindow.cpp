
#include "MainWindow.h"

#include <VisionFlowEditor.h>
#include <PropertyPanel.h>
#include <flowcv_manager.h>

#include <cstdint>

#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QCloseEvent>
#include <QtGui/QColor>

#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>

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
MainWindow::MainWindow(
    FlowCV::FlowCV_Manager& manager,
    QWidget* parent)
    : QMainWindow(parent),
    manager_(manager)
{
    setupWindow();

    setupNodeList();
    setupPropertyPanel();
    setupViewerPlaceholder();

    setupDockLayout();
    setupMenusAndToolbar();

    // 必须在全部 Dock 和 Toolbar 创建完成后恢复布局。
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

            // 保留多节点属性面板的刷新方式。
            propertyPanel_->refresh(); 
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

    refreshGraphStatistics();

    statusBar()->showMessage(
        QStringLiteral("Ready"));
}


// =============================================================
// Close Event
// =============================================================
void MainWindow::closeEvent(QCloseEvent* event)
{
    // 保存窗口尺寸和 Dock 布局。
    // 不保存 FlowCV 运行图。
    saveWindowLayout();

    QMainWindow::closeEvent(event);
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

        QLabel {
            color: #d8dfe8;
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

    viewerDock_->setObjectName(
        QStringLiteral("ViewerDock"));

    // 阶段 D 尚未开始。
    // 此处只保留布局占位，不读取图像。
    auto* placeholder = new QLabel(
        QStringLiteral("No Viewer preview"),
        viewerDock_);

    placeholder->setObjectName(
        QStringLiteral("ViewerPlaceholder"));

    placeholder->setAlignment(
        Qt::AlignCenter);

    viewerDock_->setWidget(
        placeholder);
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
    // 只重新排列 Dock，不销毁其中的 Editor、PropertyPanel。
    // 这样不会影响 Manager 中的节点和连线。

    QDockWidget* docks[] = {
        propertyDock_,
        viewerDock_,
        editorDock_,
        nodeLibraryDock_
    };

    for (QDockWidget* dock : docks)
    {
        dock->hide();

        // 浮动窗口重新返回主窗口。
        dock->setFloating(false);

        // 从旧布局中移除，但保留对象。
        removeDockWidget(dock);
    }

    // 重建默认三分区布局。
    setupDockLayout();

    propertyDock_->show();
    viewerDock_->show();
    editorDock_->show();

    nodeLibraryDock_->hide();

    if (graphToolBar_)
    {
        graphToolBar_->show();
    }

    // 恢复默认分割比例。
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
// Menus And Toolbar
// =============================================================
void MainWindow::setupMenusAndToolbar()
{
    // ---------------------------------------------------------
    // File
    // ---------------------------------------------------------
    QMenu* fileMenu =
        menuBar()->addMenu(
            QStringLiteral("&File"));

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


    // 使用 QDockWidget 内置的显示/隐藏 QAction。
    viewMenu->addAction(
        propertyDock_->toggleViewAction());

    viewMenu->addAction(
        viewerDock_->toggleViewAction());

    viewMenu->addAction(
        editorDock_->toggleViewAction());

    viewMenu->addAction(
        nodeLibraryDock_->toggleViewAction());

    viewMenu->addSeparator();


    QAction* fit =
        viewMenu->addAction(
            QStringLiteral("Fit All Nodes"));

    QObject::connect(
        fit,
        &QAction::triggered,
        editor_,
        &VisionFlowEditor::zoomFitAll);


    QAction* resetZoom =
        viewMenu->addAction(
            QStringLiteral("Reset Zoom"));

    QObject::connect(
        resetZoom,
        &QAction::triggered,
        editor_,
        &VisionFlowEditor::resetZoom);


    viewMenu->addSeparator();


    // 恢复默认停靠结构，不清空节点。
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

    // saveState/restoreState 需要唯一 objectName。
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


    graphToolBar_->addAction(
        editor_->deleteSelectionAction());

    graphToolBar_->addSeparator();

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
