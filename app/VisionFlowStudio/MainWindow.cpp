#include "MainWindow.h"

#include <VisionFlowEditor.h>
#include <PropertyPanel.h>
#include <flowcv_manager.h>

#include <QtGui/QAction>

#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>


// -------------------------------------------------------------
// Construction
// -------------------------------------------------------------
MainWindow::MainWindow(
    FlowCV::FlowCV_Manager& manager,
    QWidget* parent)
    : QMainWindow(parent),
    manager_(manager)
{
    setupWindow();

    setupMenusAndToolbar();

    setupNodeList();

    // 阶段 C
    setupPropertyPanel();

    // 图结构变化
    QObject::connect(
        editor_,
        &VisionFlowEditor::graphChanged,
        this,
        [this](std::uint64_t, std::uint64_t) {

            refreshGraphStatistics();

            // 节点可能已经删除，或者运行图已经变化。
            // 重新读取当前节点属性。
            propertyPanel_->refresh();
        });

    // ---------------------------------------------------------
    // 阶段 C：节点选择 -> 属性面板
    // ---------------------------------------------------------
    QObject::connect(
        editor_,
        &VisionFlowEditor::selectedFlowNodeChanged,
        propertyPanel_,
        &PropertyPanel::setNode);

    // 保留原有状态栏提示
    QObject::connect(
        editor_,
        &VisionFlowEditor::selectedFlowNodeChanged,
        this,
        [this](std::uint64_t flowId) {

            if (flowId == 0) {

                statusBar()->showMessage(
                    QStringLiteral("Ready"));
            }
            else {

                statusBar()->showMessage(
                    QStringLiteral(
                        "Selected FlowCV node ID: %1")
                    .arg(flowId));
            }
        });

    QObject::connect(
        editor_,
        &VisionFlowEditor::operationFailed,
        this,
        [this](QString const& message) {

            statusBar()->showMessage(
                message,
                5000);
        });

    refreshGraphStatistics();

    statusBar()->showMessage(
        QStringLiteral("Ready"));
}


// -------------------------------------------------------------
// Main Window
// -------------------------------------------------------------
void MainWindow::setupWindow()
{
    setWindowTitle(
        QStringLiteral(
            "VisionFlowStudio - Node Editor"));

    resize(1280, 800);

    setDockNestingEnabled(true);

    editor_ = new VisionFlowEditor(
        manager_,
        this);

    setCentralWidget(editor_);

    statistics_ = new QLabel(this);

    statusBar()->addPermanentWidget(
        statistics_);
}


// -------------------------------------------------------------
// Menus And Toolbar
// -------------------------------------------------------------
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

    for (QString const& type : types) {

        QAction* action =
            addMenu->addAction(type);

        QObject::connect(
            action,
            &QAction::triggered,
            editor_,
            [this, type]() {

                editor_->addNode(type);
            });
    }

    if (types.isEmpty()) {
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

    QAction* fit =
        viewMenu->addAction(
            QStringLiteral("Fit All Nodes"));

    QObject::connect(
        fit,
        &QAction::triggered,
        editor_,
        &VisionFlowEditor::zoomFitAll);

    QAction* reset =
        viewMenu->addAction(
            QStringLiteral("Reset Zoom"));

    QObject::connect(
        reset,
        &QAction::triggered,
        editor_,
        &VisionFlowEditor::resetZoom);


    // ---------------------------------------------------------
    // Toolbar
    // ---------------------------------------------------------
    QToolBar* toolbar =
        addToolBar(
            QStringLiteral("Graph"));

    toolbar->setMovable(false);

    auto* addButton =
        new QToolButton(toolbar);

    addButton->setText(
        QStringLiteral("Add Node"));

    addButton->setMenu(addMenu);

    addButton->setPopupMode(
        QToolButton::InstantPopup);

    addButton->setEnabled(
        !types.isEmpty());

    toolbar->addWidget(addButton);

    toolbar->addAction(
        editor_->deleteSelectionAction());

    toolbar->addSeparator();

    toolbar->addAction(fit);

    toolbar->addAction(reset);
}


// -------------------------------------------------------------
// Node Library
// -------------------------------------------------------------
void MainWindow::setupNodeList()
{
    auto* dock = new QDockWidget(
        QStringLiteral("Node Library"),
        this);

    dock->setObjectName(
        QStringLiteral("NodeLibraryDock"));

    dock->setAllowedAreas(
        Qt::LeftDockWidgetArea |
        Qt::RightDockWidgetArea);

    dock->setMinimumWidth(180);

    nodeList_ = new QListWidget(dock);

    nodeList_->addItems(
        editor_->availableNodeTypes());

    nodeList_->setToolTip(
        QStringLiteral(
            "Double-click a node to add it to the canvas"));

    dock->setWidget(nodeList_);

    addDockWidget(
        Qt::LeftDockWidgetArea,
        dock);

    QObject::connect(
        nodeList_,
        &QListWidget::itemDoubleClicked,
        editor_,
        [this](QListWidgetItem* item) {

            if (item != nullptr) {
                editor_->addNode(
                    item->text());
            }
        });
}


// -------------------------------------------------------------
// Property Panel
// -------------------------------------------------------------
void MainWindow::setupPropertyPanel()
{
    auto* dock = new QDockWidget(
        QStringLiteral("Properties"),
        this);

    dock->setObjectName(
        QStringLiteral("PropertiesDock"));

    dock->setAllowedAreas(
        Qt::LeftDockWidgetArea |
        Qt::RightDockWidgetArea);

    dock->setMinimumWidth(240);

    // 与 Editor 共用同一个 FlowCV Manager。
    // 不新建节点实例，也不复制节点属性数据。
    propertyPanel_ = new PropertyPanel(
        manager_,
        dock);

    dock->setWidget(propertyPanel_);

    addDockWidget(
        Qt::RightDockWidgetArea,
        dock);
}


// -------------------------------------------------------------
// Graph Statistics
// -------------------------------------------------------------
void MainWindow::refreshGraphStatistics()
{
    statistics_->setText(
        QStringLiteral(
            "Nodes: %1   Connections: %2")
        .arg(manager_.GetNodeCount())
        .arg(manager_.GetWireCount()));
}