
#pragma once

#include <cstdint>

#include <QtWidgets/QWidget>

class QLabel;
class QTimer;

namespace FlowCV
{
    class FlowCV_Manager;
}

// 一个 ViewerPanel 只负责一个 Viewer 节点。
// Dock 的创建与销毁由 MainWindow 负责。
class ViewerPanel final : public QWidget
{
public:
    explicit ViewerPanel(
        FlowCV::FlowCV_Manager& manager,
        std::uint64_t viewerNodeId,
        QWidget* parent = nullptr);

private:
    void refreshFrame();

private:
    FlowCV::FlowCV_Manager& manager_;

    // 真实 FlowCV Node ID，而不是 QtNodes NodeId。
    std::uint64_t viewerNodeId_ = 0;

    QLabel* imageLabel_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
};
