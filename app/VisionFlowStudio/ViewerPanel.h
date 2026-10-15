#pragma once

#include <cstdint>
#include <vector>

#include <QtWidgets/QWidget>

class QLabel;
class QStackedWidget;
class QTabWidget;
class QTimer;

namespace FlowCV {
    class FlowCV_Manager;
}

class ViewerPanel final : public QWidget
{
public:
    explicit ViewerPanel(
        FlowCV::FlowCV_Manager& manager, QWidget* parent = nullptr);

    // 节点图变化后同步 Viewer 实例
    void syncViewers();

private:
    void refreshFrames();

    struct Preview
    {
        std::uint64_t nodeId = 0;
        QLabel* imageLabel = nullptr;
    };

    FlowCV::FlowCV_Manager& manager_;

    QStackedWidget* stack_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    QLabel* placeholder_ = nullptr;
    QTimer* refreshTimer_ = nullptr;

    std::vector<Preview> previews_;
};