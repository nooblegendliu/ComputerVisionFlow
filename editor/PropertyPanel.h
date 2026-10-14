#pragma once

#include <cstdint>

#include <QtWidgets/QWidget>

class QLabel;
class QScrollArea;

namespace FlowCV
{
	class FlowCV_Manager;

	struct PropertySnapshot;

}	// namespace FlowCV

class PropertyPanel final : public QWidget
{
	Q_OBJECT

public:
	explicit PropertyPanel(
		FlowCV::FlowCV_Manager& manager, QWidget* parent = nullptr);

public Q_SLOTS:
	// 传入真实FlowCV节点ID
	// 传入0时清空属性面板
	void setNode(std::uint64_t nodeId);

	// 从Manager重新读取当前节点属性
	void refresh();

private:
	QWidget* createPropertyEditor(
		const FlowCV::PropertySnapshot& property, QWidget* parent);

	// 在当前控件信号处理结束后刷新面板，
    // 避免在控件自身回调中销毁该控件。
	void queueRefresh(std::uint64_t nodeId);

private:
	FlowCV::FlowCV_Manager& manager_;

	std::uint64_t nodeId_ = 0;

	QLabel* title_ = nullptr;

	QScrollArea* scrollArea_ = nullptr;
};