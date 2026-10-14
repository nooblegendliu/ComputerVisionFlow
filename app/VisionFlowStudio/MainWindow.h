#pragma once

#include <QtWidgets/QMainWindow> 

namespace FlowCV
{
	class FlowCV_Manager;

}	// namespace FlowCV

class QLabel;
class QListWidget;
class VisionFlowEditor;
class PropertyPanel;

// 仅负责组合现有管理器与编辑器UI，不创建第二套运行图
class MainWindow final : public QMainWindow
{
public:
	explicit MainWindow(FlowCV::FlowCV_Manager& manager, QWidget* parent = nullptr);

private:
	void setupWindow();
	void setupMenusAndToolbar();
	void setupNodeList();

	// 阶段C：属性面板
	void setupPropertyPanel();

	void refreshGraphStatistics();

private:
	FlowCV::FlowCV_Manager& manager_;

	VisionFlowEditor* editor_ = nullptr;

	QListWidget* nodeList_ = nullptr;

	QLabel* statistics_ = nullptr;

	// 阶段C：属性面板
	PropertyPanel* propertyPanel_ = nullptr;
};