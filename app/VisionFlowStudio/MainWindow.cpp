#include "MainWindow.h"

#include <VisionFlowEditor.h>
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

MainWindow::MainWindow(FlowCV::FlowCV_Manager& manager, QWidget* parent)
    : QMainWindow(parent), manager_(manager)
{
    setupWindow();
    setupMenusAndToolbar();
    setupNodeList();

    QObject::connect(editor_, &VisionFlowEditor::graphChanged,
        this, [this](std::uint64_t, std::uint64_t) {
            refreshGraphStatistics();
        });
    QObject::connect(editor_, &VisionFlowEditor::selectedFlowNodeChanged,
        this, [this](std::uint64_t flowId) {
            if (flowId == 0) {
                statusBar()->showMessage(QStringLiteral("Ready"));
            }
            else {
                statusBar()->showMessage(
                    QStringLiteral("Selected FlowCV node ID: %1").arg(flowId));
            }
        });
    QObject::connect(editor_, &VisionFlowEditor::operationFailed,
        this, [this](QString const& message) {
            statusBar()->showMessage(message, 5000);
        });

    refreshGraphStatistics();
    statusBar()->showMessage(QStringLiteral("Ready"));
}

void MainWindow::setupWindow()
{
    setWindowTitle(QStringLiteral("VisionFlowStudio - Node Editor"));
    resize(1280, 800);
    setDockNestingEnabled(true);

    editor_ = new VisionFlowEditor(manager_, this);
    setCentralWidget(editor_);

    statistics_ = new QLabel(this);
    statusBar()->addPermanentWidget(statistics_);
}


void MainWindow::setupMenusAndToolbar()
{
    QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    QAction* quit = fileMenu->addAction(QStringLiteral("E&xit"));
    QObject::connect(quit, &QAction::triggered, this, &QWidget::close);

    QMenu* nodeMenu = menuBar()->addMenu(QStringLiteral("&Node"));
    QMenu* addMenu = nodeMenu->addMenu(QStringLiteral("Add Node"));
    const QStringList types = editor_->availableNodeTypes();
    for (QString const& type : types) {
        QAction* action = addMenu->addAction(type);
        QObject::connect(action, &QAction::triggered,
            editor_, [this, type]() { editor_->addNode(type); });
    }
    if (types.isEmpty()) {
        addMenu->setEnabled(false);
    }

    QMenu* editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(editor_->deleteSelectionAction());

    QMenu* viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
    QAction* fit = viewMenu->addAction(QStringLiteral("Fit All Nodes"));
    QObject::connect(fit, &QAction::triggered,
        editor_, &VisionFlowEditor::zoomFitAll);
    QAction* reset = viewMenu->addAction(QStringLiteral("Reset Zoom"));
    QObject::connect(reset, &QAction::triggered,
        editor_, &VisionFlowEditor::resetZoom);

    QToolBar* toolbar = addToolBar(QStringLiteral("Graph"));
    toolbar->setMovable(false);
    auto* addButton = new QToolButton(toolbar);
    addButton->setText(QStringLiteral("Add Node"));
    addButton->setMenu(addMenu);
    addButton->setPopupMode(QToolButton::InstantPopup);
    addButton->setEnabled(!types.isEmpty());
    toolbar->addWidget(addButton);
    toolbar->addAction(editor_->deleteSelectionAction());
    toolbar->addSeparator();
    toolbar->addAction(fit);
    toolbar->addAction(reset);
}

void MainWindow::setupNodeList()
{
    auto* dock = new QDockWidget(QStringLiteral("Node Library"), this);
    dock->setObjectName(QStringLiteral("NodeLibraryDock"));
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setMinimumWidth(180);

    nodeList_ = new QListWidget(dock);
    nodeList_->addItems(editor_->availableNodeTypes());
    nodeList_->setToolTip(QStringLiteral("Double-click a node to add it to the canvas"));
    dock->setWidget(nodeList_);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    QObject::connect(nodeList_, &QListWidget::itemDoubleClicked,
        editor_, [this](QListWidgetItem* item) {
            if (item != nullptr) {
                editor_->addNode(item->text());
            }
        });
}

void MainWindow::refreshGraphStatistics()
{
    statistics_->setText(
        QStringLiteral("Nodes: %1   Connections: %2")
        .arg(manager_.GetNodeCount())
        .arg(manager_.GetWireCount()));
}