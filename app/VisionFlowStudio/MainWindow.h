
#pragma once

#include <QtWidgets/QMainWindow>

namespace FlowCV
{
    class FlowCV_Manager;
}

class QAction;
class QCloseEvent;
class QDockWidget;
class QLabel;
class QListWidget;
class QToolBar;

class VisionFlowEditor;
class PropertyPanel;
class ViewerPanel;


// 负责组合 Qt UI、管理 Dock 布局及窗口设置。
// 不管理 DSPatch 运行图和节点计算。
class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(
        FlowCV::FlowCV_Manager& manager,
        QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    // Main Window
    void setupWindow();
    void setupAppearance();
    void setupMenusAndToolbar();

    // Dock Panels
    void setupNodeList();
    void setupPropertyPanel();
    void setupViewerPanel();

    // Dock Layout
    void setupDockLayout();
    void resetDockLayout();

    // Window Settings
    void restoreWindowLayout();
    void saveWindowLayout() const;

    // Status Bar
    void refreshGraphStatistics();

private:
    FlowCV::FlowCV_Manager& manager_;

    // Editor
    VisionFlowEditor* editor_ = nullptr;
    QDockWidget* editorDock_ = nullptr;

    // Node Library
    QListWidget* nodeList_ = nullptr;
    QDockWidget* nodeLibraryDock_ = nullptr;

    // Properties
    PropertyPanel* propertyPanel_ = nullptr;
    QDockWidget* propertyDock_ = nullptr;

    // Viewer Placeholder
    QDockWidget* viewerDock_ = nullptr;
    ViewerPanel* viewerPanel_ = nullptr;        // new add

    // Toolbar
    QToolBar* graphToolBar_ = nullptr;

    // Status Bar
    QLabel* statistics_ = nullptr;
};
