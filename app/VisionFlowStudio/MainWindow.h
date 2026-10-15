
#pragma once

#include <cstdint>
#include <map>

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
class QMenu;
class QToolBar;

class VisionFlowEditor;
class PropertyPanel;


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

    // 无 Viewer 节点时的默认占位区
    void setupViewerPlaceholder();

    // 同步 FlowCV Viewer 节点与独立 Dock
    void syncViewerDocks();

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

    // 原有 Viewer Dock 只作为无节点占位区
    QDockWidget* viewerDock_ = nullptr;

    // 每个 Viewer 节点对应一个独立 QDockWidget
    // Key: FlowCV Node ID
    std::map<std::uint64_t, QDockWidget*> viewerDocks_;

    // View 菜单中的 Viewer 窗口列表
    QMenu* viewerWindowsMenu_ = nullptr;

    // Toolbar
    QToolBar* graphToolBar_ = nullptr;

    // Status Bar
    QLabel* statistics_ = nullptr;
};
