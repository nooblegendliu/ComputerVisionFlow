#include "ViewerPanel.h"
#include <Viewer/viewer.hpp>
#include <opencv2/core.hpp>
#include <QtCore/QTimer>
#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QCursor>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QVBoxLayout>

class ViewerCanvas final : public QWidget
{
public:
    explicit ViewerCanvas(QWidget* parent) : QWidget(parent)
    {
        setObjectName(QStringLiteral("viewerCanvas"));
        setMouseTracking(true);
        setMinimumSize(0, 0);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        qApp->installEventFilter(this);
    }
    QImage image;
    ViewerImage::Aspect aspect = ViewerImage::Aspect::Width;
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(16, 16, 16));
        if (image.isNull()) {
            painter.setPen(Qt::lightGray);
            painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("No supported image"));
            return;
        }
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(ViewerImage::imageRect(image.size(), size(), aspect), image);
    }
    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (!(event->modifiers() & Qt::ControlModifier)) {
            QToolTip::hideText();
            return;
        }
        inspectPixel(event->position(), event->globalPosition().toPoint());
    }
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (underMouse() && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)) {
            const auto* key = static_cast<QKeyEvent*>(event);
            if (key->key() == Qt::Key_Control) {
                if (event->type() == QEvent::KeyPress)
                    inspectPixel(mapFromGlobal(QCursor::pos()), QCursor::pos());
                else
                    QToolTip::hideText();
            }
        }
        return QWidget::eventFilter(watched, event);
    }
    void leaveEvent(QEvent*) override { QToolTip::hideText(); }
private:
    void inspectPixel(QPointF local, QPoint global)
    {
        auto position = ViewerImage::mapPosition(local,
            ViewerImage::imageRect(image.size(), size(), aspect), image.size());
        if (!position) {
            QToolTip::hideText();
            return;
        }
        const QColor color = image.pixelColor(position->pixel);
        const auto text = QStringLiteral("Res: %1 x %2\nU %3  V %4\nCoord: %5, %6\n"
            "R %7  G %8  B %9\nH %10  S %11  V %12")
            .arg(image.width()).arg(image.height())
            .arg(position->uv.x(), 0, 'f', 3).arg(position->uv.y(), 0, 'f', 3)
            .arg(position->pixel.x()).arg(position->pixel.y())
            .arg(color.red()).arg(color.green()).arg(color.blue())
            .arg(qMax(0.0f, color.hsvHueF()), 0, 'f', 3)
            .arg(color.hsvSaturationF(), 0, 'f', 3).arg(color.valueF(), 0, 'f', 3);
        QToolTip::showText(global, text, this);
    }
};

ViewerPanel::ViewerPanel(std::shared_ptr<DSPatch::DSPatchables::Viewer> node, QWidget* parent)
    : QWidget(parent), node_(std::move(node))
{
    setObjectName(QStringLiteral("viewerPanel"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* controls = new QHBoxLayout;
    aspect_ = new QComboBox(this);
    aspect_->setObjectName(QStringLiteral("viewerAspect"));
    aspect_->addItems({QStringLiteral("Free"), QStringLiteral("Width"), QStringLiteral("Height")});
    aspect_->setCurrentIndex(1);
    aspect_->setToolTip(QStringLiteral("Free stretches to the view; Width / Height keep the image ratio, anchored to that dimension."));
    channel_ = new QComboBox(this);
    channel_->setObjectName(QStringLiteral("viewerChannel"));
    channel_->addItems({QStringLiteral("RGB"), QStringLiteral("R"), QStringLiteral("G"),
        QStringLiteral("B"), QStringLiteral("A"), QStringLiteral("Lum")});
    controls->addWidget(new QLabel(QStringLiteral("Aspect:"), this));
    controls->addWidget(aspect_);
    controls->addWidget(new QLabel(QStringLiteral("Channel:"), this));
    controls->addWidget(channel_);
    controls->addStretch();
    layout->addLayout(controls);

    colorMapControls_ = new QWidget(this);
    colorMapControls_->setObjectName(QStringLiteral("viewerColorMapControls"));
    auto* colorLayout = new QHBoxLayout(colorMapControls_);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorMap_ = new QComboBox(colorMapControls_);
    colorMap_->setObjectName(QStringLiteral("viewerColorMap"));
    colorMap_->addItems({"AUTUMN", "BONE", "JET", "WINTER", "RAINBOW", "OCEAN", "SUMMER",
        "SPRING", "COOL", "HSV", "PINK", "HOT", "PARULA", "MAGMA", "INFERNO", "PLASMA",
        "VIRIDIS", "CIVIDIS", "TWILIGHT", "TWILIGHT_SHIFTED", "TURBO"});
    colorMap_->setCurrentIndex(2);
    scale_ = new QDoubleSpinBox(colorMapControls_);
    scale_->setObjectName(QStringLiteral("viewerScale"));
    scale_->setRange(0.001, 10.0);
    scale_->setDecimals(3);
    scale_->setSingleStep(0.001);
    scale_->setValue(0.15);
    colorLayout->addWidget(new QLabel(QStringLiteral("Color map:"), colorMapControls_));
    colorLayout->addWidget(colorMap_);
    colorLayout->addWidget(new QLabel(QStringLiteral("Scale:"), colorMapControls_));
    colorLayout->addWidget(scale_);
    colorLayout->addStretch();
    colorMapControls_->hide();
    layout->addWidget(colorMapControls_);
    scroll_ = new QScrollArea(this);
    scroll_->setObjectName(QStringLiteral("viewerScrollArea"));
    scroll_->setAlignment(Qt::AlignCenter);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setStyleSheet(QStringLiteral("QScrollArea { background: #101010; }"));
    scroll_->viewport()->installEventFilter(this);
    canvas_ = new ViewerCanvas(scroll_);
    scroll_->setWidget(canvas_);
    layout->addWidget(scroll_, 1);
    connect(aspect_, &QComboBox::currentIndexChanged, this, [this](int index) {
        canvas_->aspect = static_cast<ViewerImage::Aspect>(index);
        updateCanvasGeometry();
        canvas_->update();
    });
    connect(channel_, &QComboBox::currentIndexChanged, this, [this] { refreshFrame(); });
    connect(colorMap_, &QComboBox::currentIndexChanged, this, [this] { refreshFrame(); });
    connect(scale_, &QDoubleSpinBox::valueChanged, this, [this] { refreshFrame(); });
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setObjectName(QStringLiteral("viewerRefreshTimer"));
    refreshTimer_->setInterval(33);
    connect(refreshTimer_, &QTimer::timeout, this, [this] { refreshFrame(); });
    refreshTimer_->start();
}

void ViewerPanel::refreshFrame()
{
    if (!isVisible())
        return;
    const cv::Mat frame = node_ ? node_->GetFrameCopy() : cv::Mat{};
    colorMapControls_->setVisible(ViewerImage::usesColorMap(frame));
    ViewerImage::Options options;
    options.channel = static_cast<ViewerImage::Channel>(channel_->currentIndex());
    options.colorMap = colorMap_->currentIndex();
    options.scale = scale_->value();
    try {
        canvas_->image = ViewerImage::convert(frame, options);
    }
    catch (const cv::Exception&) {
        canvas_->image = {};
    }
    updateCanvasGeometry();
    canvas_->update();
}

void ViewerPanel::updateCanvasGeometry()
{
    if (!canvas_ || !scroll_)
        return;
    const QSize available = scroll_->viewport()->size();
    const auto desired = ViewerImage::imageRect(canvas_->image.size(), available, canvas_->aspect);
    canvas_->resize(desired.isEmpty() ? available : desired.size().toSize());
}

bool ViewerPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (scroll_ && watched == scroll_->viewport() && event->type() == QEvent::Resize)
        updateCanvasGeometry();
    return QWidget::eventFilter(watched, event);
}
