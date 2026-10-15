#pragma once

#include <cstdint>
#include <map>

#include <QtCore/QString>
#include <QtWidgets/QMainWindow>

#include <nlohmann/json.hpp>

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

// 组合 Qt UI、管理 Dock 布局及项目文件。
// DSPatch 运行图仍由 FlowCV_Manager 管理。
class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(
        FlowCV::FlowCV_Manager& manager,
        QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    // ---------------------------------------------------------
    // Main Window
    // ---------------------------------------------------------
    void setupWindow();
    void setupAppearance();
    void setupMenusAndToolbar();

    // ---------------------------------------------------------
    // Dock Panels
    // ---------------------------------------------------------
    void setupNodeList();
    void setupPropertyPanel();
    void setupViewerPlaceholder();
    void syncViewerDocks();

    // ---------------------------------------------------------
    // Dock Layout
    // ---------------------------------------------------------
    void setupDockLayout();
    void resetDockLayout();

    // ---------------------------------------------------------
    // Window Settings
    // ---------------------------------------------------------
    void restoreWindowLayout();
    void saveWindowLayout() const;

    // ---------------------------------------------------------
    // Status Bar
    // ---------------------------------------------------------
    void refreshGraphStatistics();

    // ---------------------------------------------------------
    // Stage E: Project Operations
    // ---------------------------------------------------------
    void newProject();
    void openProject();

    bool saveProject();
    bool saveProjectAs();

    bool saveProjectToFile(const QString& filePath);
    bool loadProjectFromFile(const QString& filePath);

    // 合并 Manager 与 Editor 状态。
    nlohmann::json collectProjectState() const;

    // 记录最近一次成功保存/加载的状态。
    void updateSavedSnapshot();

    // 判断当前项目是否发生变化。
    bool isProjectModified() const;

    // 新建、打开、关闭前检查未保存修改。
    bool confirmDiscardChanges();

    // 更新窗口标题和修改标记。
    void refreshProjectTitle();

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

    // 无 Viewer 时的占位 Dock
    QDockWidget* viewerDock_ = nullptr;

    // 每个 Viewer 节点对应一个独立 Dock。
    // Key: FlowCV Node ID
    std::map<std::uint64_t, QDockWidget*> viewerDocks_;

    // View 菜单中的 Viewer 列表
    QMenu* viewerWindowsMenu_ = nullptr;

    // Toolbar
    QToolBar* graphToolBar_ = nullptr;

    // Status Bar
    QLabel* statistics_ = nullptr;

    // ---------------------------------------------------------
    // Stage E: Project State
    // ---------------------------------------------------------

    // 当前项目的完整文件路径。
    // 空字符串表示尚未保存的新项目。
    QString currentProjectFile_;

    // 上次成功保存或加载的项目快照。
    // 用于判断是否有未保存修改。
    nlohmann::json savedProjectState_;
};