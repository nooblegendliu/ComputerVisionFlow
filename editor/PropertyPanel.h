#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QScrollArea;
class QToolButton;
class QVBoxLayout;

namespace FlowCV
{
    class FlowCV_Manager;

    struct PropertySnapshot;
    struct PropertiesSnapshot;
}

class PropertyPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyPanel(
        FlowCV::FlowCV_Manager& manager,
        QWidget* parent = nullptr);

public Q_SLOTS:
    // 当前选中的 FlowCV 节点 ID。
    // 只更新高亮，不隐藏其他节点的属性。
    void setNode(std::uint64_t nodeId);

    // 与 Manager 同步节点属性分组。
    void refresh();

private:
    struct NodeSection
    {
        QString title;

        QToolButton* header = nullptr;

        QWidget* fields = nullptr;

        QVBoxLayout* layout = nullptr;

        std::uint64_t revision = 0;
    };

private:
    // 创建一个节点的属性内容。
    QWidget* createPropertyFields(
        std::uint64_t nodeId,
        const FlowCV::PropertiesSnapshot& snapshot,
        QWidget* parent);

    // 根据原有属性类型创建 Qt 控件。
    QWidget* createPropertyEditor(
        std::uint64_t nodeId,
        const FlowCV::PropertySnapshot& property,
        QWidget* parent);

    // 只刷新某个节点的属性控件。
    void refreshNodeFields(
        std::uint64_t nodeId,
        const FlowCV::PropertiesSnapshot& snapshot);

    // 延迟刷新，避免在控件信号处理中销毁控件。
    void queueRefresh(std::uint64_t nodeId);

    // 更新选中节点的标题高亮。
    void updateSelectionStyle();

private:
    FlowCV::FlowCV_Manager& manager_;

    std::uint64_t selectedNodeId_ = 0;

    QScrollArea* scrollArea_ = nullptr;

    // 当前显示的节点顺序。
    std::vector<std::uint64_t> nodeOrder_;

    // 每个节点对应的独立属性区域。
    std::unordered_map<std::uint64_t, NodeSection> sections_;
};