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
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>


ViewerPanel::ViewerPanel(
    FlowCV::FlowCV_Manager& manager,
    QWidget* parent)
    : QWidget(parent),
    manager_(manager)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    stack_ = new QStackedWidget(this);

    placeholder_ = new QLabel(
        QStringLiteral("No Viewer preview"),
        stack_);

    placeholder_->setAlignment(Qt::AlignCenter);

    tabs_ = new QTabWidget(stack_);

    stack_->addWidget(placeholder_);
    stack_->addWidget(tabs_);

    layout->addWidget(stack_);

    // 定时器只负责 UI 刷新，不参与 DSPatch 调度。
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(33);

    connect(
        refreshTimer_,
        &QTimer::timeout,
        this,
        [this]() { refreshFrames(); });

    syncViewers();
    refreshTimer_->start();
}

void ViewerPanel::syncViewers()
{
    struct ViewerInfo
    {
        std::uint64_t id;
        QString title;
    };

    std::vector<ViewerInfo> viewers;

    const auto count = manager_.GetNodeCount();

    for (std::uint64_t i = 0; i < count; ++i)
    {
        FlowCV::NodeInfo info{};

        if (!manager_.GetNodeInfoByIndex(i, info))
            continue;

        auto viewer =
            std::dynamic_pointer_cast<
            DSPatch::DSPatchables::Viewer>(
                info.node_ptr);

        if (!viewer)
            continue;

        viewers.push_back({
            info.id,
            QStringLiteral("Viewer_%1")
                .arg(viewer->GetInstanceCount())
            });
    }

    // Viewer 列表未改变时，不重建图像控件。
    bool same = viewers.size() == previews_.size();

    if (same)
    {
        for (std::size_t i = 0; i < viewers.size(); ++i)
        {
            if (viewers[i].id != previews_[i].nodeId)
            {
                same = false;
                break;
            }
        }
    }

    if (same)
    {
        for (int i = 0; i < tabs_->count(); ++i)
            tabs_->setTabText(i, viewers[i].title);
        return;
    }

    // 保存当前正在预览的 Viewer
    std::uint64_t selectedId = 0;

    const int current = tabs_->currentIndex();

    if (current >= 0 &&
        static_cast<std::size_t>(current) < previews_.size())
    {
        selectedId = previews_[current].nodeId;
    }

    // 删除旧 UI，不影响 Manager 内部节点。
    while (tabs_->count() > 0)
    {
        QWidget* page = tabs_->widget(0);
        tabs_->removeTab(0);
        delete page;
    }

    previews_.clear();

    for (const auto& viewer : viewers)
    {
        auto* label = new QLabel(tabs_);

        label->setAlignment(Qt::AlignCenter);

        label->setSizePolicy(
            QSizePolicy::Ignored,
            QSizePolicy::Ignored);

        label->setMinimumSize(0, 0);

        label->setStyleSheet(
            QStringLiteral(
                "background-color: #101010;"));

        tabs_->addTab(label, viewer.title);

        previews_.push_back({
            viewer.id,
            label
            });
    }

    // 尽量保持当前 Viewer 选项
    for (std::size_t i = 0; i < previews_.size(); ++i)
    {
        if (previews_[i].nodeId == selectedId)
        {
            tabs_->setCurrentIndex(static_cast<int>(i));
            break;
        }
    }

    stack_->setCurrentWidget(
        previews_.empty()
        ? static_cast<QWidget*>(placeholder_)
        : static_cast<QWidget*>(tabs_));
}

void ViewerPanel::refreshFrames()
{
    for (auto& preview : previews_)
    {
        FlowCV::NodeInfo info{};

        if (!manager_.GetNodeInfoById(
            preview.nodeId, info))
        {
            continue;
        }

        auto viewer =
            std::dynamic_pointer_cast<
            DSPatch::DSPatchables::Viewer>(
                info.node_ptr);

        if (!viewer)
            continue;

        const cv::Mat frame = viewer->GetFrameCopy();

        if (frame.empty())
        {
            preview.imageLabel->clear();
            continue;
        }

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
            // Windows x64 little-endian：
            // Format_ARGB32 的内存字节顺序是 BGRA。
            format = QImage::Format_ARGB32;
            break;

        default:
            continue;
        }

        // QImage 先包装 cv::Mat 内存，
        // 再进行深拷贝，避免帧析构后悬空引用。
        QImage image(
            frame.data,
            frame.cols,
            frame.rows,
            static_cast<qsizetype>(frame.step),
            format);

        image = image.copy();

        const QSize targetSize =
            preview.imageLabel->size();

        if (targetSize.isEmpty())
            continue;

        QPixmap pixmap =
            QPixmap::fromImage(std::move(image));

        preview.imageLabel->setPixmap(
            pixmap.scaled(
                targetSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    }
}