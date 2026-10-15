
#include "ViewerPanel.h"

#include <Viewer/Viewer.hpp>
#include <flowcv_manager.h>

#include <memory>
#include <utility>

#include <opencv2/core.hpp>

#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtGui/QPixmap>

#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>


// =============================================================
// Constructor
// =============================================================
ViewerPanel::ViewerPanel(
    FlowCV::FlowCV_Manager& manager,
    std::uint64_t viewerNodeId,
    QWidget* parent)
    : QWidget(parent),
    manager_(manager),
    viewerNodeId_(viewerNodeId)
{
    auto* layout = new QVBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);

    imageLabel_ = new QLabel(this);

    imageLabel_->setAlignment(Qt::AlignCenter);

    imageLabel_->setSizePolicy(
        QSizePolicy::Ignored,
        QSizePolicy::Ignored);

    imageLabel_->setMinimumSize(0, 0);

    imageLabel_->setStyleSheet(
        QStringLiteral(
            "background-color: #101010;"));

    layout->addWidget(imageLabel_);

    // 只刷新 Qt 图像，不驱动 DSPatch 计算。
    refreshTimer_ = new QTimer(this);

    refreshTimer_->setInterval(33);

    connect(
        refreshTimer_,
        &QTimer::timeout,
        this,
        [this]()
        {
            refreshFrame();
        });

    refreshTimer_->start();
}


// =============================================================
// Refresh One Viewer Frame
// =============================================================
void ViewerPanel::refreshFrame()
{
    // Dock 被关闭或隐藏时不需要更新画面。
    // Viewer 节点本身仍由 DSPatch 正常处理。
    if (!isVisible())
        return;

    FlowCV::NodeInfo info{};

    if (!manager_.GetNodeInfoById(
        viewerNodeId_, info))
    {
        imageLabel_->clear();
        return;
    }

    auto viewer =
        std::dynamic_pointer_cast<
        DSPatch::DSPatchables::Viewer>(
            info.node_ptr);

    if (!viewer)
    {
        imageLabel_->clear();
        return;
    }

    // Viewer 自身负责图像缓存和互斥同步。
    const cv::Mat frame = viewer->GetFrameCopy();

    if (frame.empty())
    {
        imageLabel_->clear();
        return;
    }

    // ---------------------------------------------------------
    // OpenCV Mat Type -> Qt Image Format
    // ---------------------------------------------------------
    QImage::Format format;

    switch (frame.type())
    {
    case CV_8UC1:
        format = QImage::Format_Grayscale8;
        break;

    case CV_8UC3:
        format = QImage::Format_BGR888;
        break;

    case CV_8UC4:
        // Windows x64 little-endian:
        // QImage::Format_ARGB32 的内存排列为 BGRA。
        format = QImage::Format_ARGB32;
        break;

    default:
        return;
    }

    // ---------------------------------------------------------
    // Mat -> QImage
    // ---------------------------------------------------------
    QImage image(
        frame.data,
        frame.cols,
        frame.rows,
        static_cast<qsizetype>(frame.step),
        format);

    // 深拷贝，保证图像不依赖局部 cv::Mat 的生命周期。
    image = image.copy();

    // ---------------------------------------------------------
    // Scale To ViewerPanel
    // ---------------------------------------------------------
    const QSize targetSize = imageLabel_->size();

    if (targetSize.isEmpty())
        return;

    QPixmap pixmap =
        QPixmap::fromImage(std::move(image));

    imageLabel_->setPixmap(
        pixmap.scaled(
            targetSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
}
