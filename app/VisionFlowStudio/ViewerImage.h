#pragma once
#include <opencv2/core/mat.hpp>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtGui/QImage>
#include <optional>

namespace ViewerImage
{
enum class Channel { RGB, R, G, B, A, Lum };
enum class Aspect { Free, Width, Height };
struct Options
{
    Channel channel = Channel::RGB;
    int colorMap = 2; // OpenCV JET, matching FlowCV.
    double scale = 0.15;
};
bool usesColorMap(const cv::Mat& source);
// Returns owned RGB pixels. Never changes source or its underlying storage.
QImage convert(const cv::Mat& source, const Options& options = {});
QRectF imageRect(QSize image, QSize viewport, Aspect aspect);
struct PixelPosition { QPoint pixel; QPointF uv; };
std::optional<PixelPosition> mapPosition(QPointF position, QRectF displayedRect, QSize image);
}
