#include "ViewerImage.h"
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace ViewerImage
{
bool usesColorMap(const cv::Mat& source)
{
    return source.channels() == 1 && (source.depth() == CV_16U || source.depth() == CV_16S ||
        source.depth() == CV_32F || source.depth() == CV_64F);
}
QImage convert(const cv::Mat& source, const Options& options)
{
    if (source.empty() || source.dims != 2)
        return {};
    cv::Mat bgra;
    if (source.type() == CV_8UC1)
        cv::cvtColor(source, bgra, cv::COLOR_GRAY2BGRA);
    else if (source.type() == CV_8UC3)
        cv::cvtColor(source, bgra, cv::COLOR_BGR2BGRA);
    else if (source.type() == CV_8UC4)
        bgra = source;
    else if (usesColorMap(source)) {
        if (!std::isfinite(options.scale) || options.scale < 0.001 || options.scale > 10.0 ||
            options.colorMap < 0 || options.colorMap > cv::COLORMAP_TURBO)
            return {};
        // FlowCV saturates to eight bits BEFORE min/max normalization.
        cv::Mat scaled, normalized, colored;
        source.convertTo(scaled, CV_8UC1, options.scale);
        cv::normalize(scaled, normalized, 1, 255, cv::NORM_MINMAX, CV_8UC1);
        cv::applyColorMap(normalized, colored, options.colorMap);
        cv::cvtColor(colored, bgra, cv::COLOR_BGR2BGRA);
    }
    else
        return {};
    cv::Mat rgb;
    if (options.channel == Channel::RGB)
        cv::cvtColor(bgra, rgb, cv::COLOR_BGRA2RGB); // Original GL_RGB ignores alpha.
    else {
        cv::Mat plane;
        if (options.channel == Channel::Lum)
            cv::cvtColor(bgra, plane, cv::COLOR_BGRA2GRAY);
        else {
            int index = 0;
            switch (options.channel) {
            case Channel::R: index = 2; break;
            case Channel::G: index = 1; break;
            case Channel::B: index = 0; break;
            case Channel::A: index = 3; break;
            default: return {};
            }
            cv::extractChannel(bgra, plane, index);
        }
        cv::cvtColor(plane, rgb, cv::COLOR_GRAY2RGB);
    }
    return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<qsizetype>(rgb.step),
        QImage::Format_RGB888).copy();
}
QRectF imageRect(QSize image, QSize viewport, Aspect aspect)
{
    if (image.isEmpty() || viewport.isEmpty())
        return {};
    QSizeF size(viewport);
    if (aspect == Aspect::Width)
        size.setHeight(size.width() * image.height() / image.width());
    else if (aspect == Aspect::Height)
        size.setWidth(size.height() * image.width() / image.height());
    return QRectF(QPointF((viewport.width() - size.width()) / 2.0,
        (viewport.height() - size.height()) / 2.0), size);
}
std::optional<PixelPosition> mapPosition(QPointF position, QRectF displayedRect, QSize image)
{
    if (displayedRect.isEmpty() || image.isEmpty())
        return std::nullopt;
    const QPointF uv((position.x() - displayedRect.left()) / displayedRect.width(),
        (position.y() - displayedRect.top()) / displayedRect.height());
    if (uv.x() < 0 || uv.y() < 0 || uv.x() >= 1 || uv.y() >= 1)
        return std::nullopt;
    return PixelPosition{QPoint(static_cast<int>(uv.x() * image.width()),
        static_cast<int>(uv.y() * image.height())), uv};
}
}
