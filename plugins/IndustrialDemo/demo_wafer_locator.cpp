// Temporary industrial demo node. Uses the existing FlowCV/DSPatch contracts.
#include "demo_wafer_locator.hpp"

#include <opencv2/imgproc.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace DSPatch::DSPatchables {
    namespace {

        struct Line {
            // ax + by + c = 0, with sqrt(a*a + b*b) == 1.
            double a = 0.0;
            double b = 0.0;
            double c = 0.0;
        };

        struct Samples {
            std::vector<cv::Point2f> top, bottom, left, right;
        };

        bool Finite(const cv::Point2f& p) {
            return std::isfinite(p.x) && std::isfinite(p.y);
        }

        Line Through(const cv::Point2f& p, const cv::Point2f& q) {
            const double a = static_cast<double>(q.y) - p.y;
            const double b = static_cast<double>(p.x) - q.x;
            const double length = std::hypot(a, b);
            if (length < 1e-6)
                throw std::runtime_error("Degenerate line endpoints");
            const double na = a / length, nb = b / length;
            return { na, nb, -(na * p.x + nb * p.y) };
        }

        cv::Point2f Intersection(const Line& lhs, const Line& rhs) {
            const double determinant = lhs.a * rhs.b - rhs.a * lhs.b;
            if (std::abs(determinant) < 1e-4)
                throw std::runtime_error("Two fitted edges are nearly parallel");
            const double x = (lhs.b * rhs.c - rhs.b * lhs.c) / determinant;
            const double y = (rhs.a * lhs.c - lhs.a * rhs.c) / determinant;
            const cv::Point2f result(static_cast<float>(x), static_cast<float>(y));
            if (!Finite(result))
                throw std::runtime_error("Non-finite intersection");
            return result;
        }

        std::optional<Line> RobustFit(const std::vector<cv::Point2f>& points,
            double distanceThreshold,
            int requiredInliers = 50) {
            if (points.size() < static_cast<std::size_t>(requiredInliers))
                return std::nullopt;

            cv::RNG rng(0x5EED);
            int bestCount = 0;
            double bestError = std::numeric_limits<double>::infinity();
            Line bestLine{};

            // Deterministic two-point RANSAC; no mechanical-arm pixels need be erased.
            for (int trial = 0; trial < 700; ++trial) {
                const int first = rng.uniform(0, static_cast<int>(points.size()));
                const int second = rng.uniform(0, static_cast<int>(points.size()));
                if (first == second || cv::norm(points[first] - points[second]) < 20.0)
                    continue;
                const Line candidate = Through(points[first], points[second]);
                int count = 0;
                double error = 0.0;
                for (const auto& p : points) {
                    const double distance = std::abs(candidate.a * p.x + candidate.b * p.y + candidate.c);
                    if (distance < distanceThreshold) {
                        ++count;
                        error += distance;
                    }
                }
                if (count > bestCount || (count == bestCount && error < bestError)) {
                    bestCount = count;
                    bestError = error;
                    bestLine = candidate;
                }
            }
            if (bestCount < requiredInliers)
                return std::nullopt;

            std::vector<cv::Point2f> inliers;
            inliers.reserve(points.size());
            for (const auto& p : points)
                if (std::abs(bestLine.a * p.x + bestLine.b * p.y + bestLine.c) < distanceThreshold)
                    inliers.push_back(p);

            cv::Vec4f fitted;
            cv::fitLine(inliers, fitted, cv::DIST_L2, 0.0, 0.01, 0.01);
            const cv::Point2f origin(fitted[2], fitted[3]);
            const cv::Point2f direction(fitted[0], fitted[1]);
            return Through(origin, origin + direction * 100.0f);
        }

        Samples ExtractSideSamples(const cv::Mat& mask, const cv::Rect& bounds,
            double rightLimitFraction, double bottomLimitFraction) {
            Samples result;
            const int xFirst = bounds.x + cvRound(bounds.width * 0.04);
            const int xLast = bounds.x + cvRound(bounds.width * 0.96);
            const int yFirst = bounds.y + cvRound(bounds.height * 0.03);
            const int yLast = bounds.y + cvRound(bounds.height * 0.82);
            const double rightLimit = bounds.x + bounds.width * rightLimitFraction;
            const double bottomLimit = bounds.y + bounds.height * bottomLimitFraction;

            for (int x = xFirst; x < xLast; ++x) {
                int top = -1, bottom = -1;
                for (int y = bounds.y; y < bounds.y + bounds.height; ++y) {
                    if (mask.at<unsigned char>(y, x) == 0)
                        continue;
                    if (top < 0) top = y;
                    bottom = y;
                }
                if (top < 0) continue;
                result.top.emplace_back(static_cast<float>(x), static_cast<float>(top));
                // The downward robot projection lies below this silhouette limit.
                if (bottom < bottomLimit)
                    result.bottom.emplace_back(static_cast<float>(x), static_cast<float>(bottom));
            }
            for (int y = yFirst; y < yLast; ++y) {
                int left = -1, right = -1;
                const auto* row = mask.ptr<unsigned char>(y);
                for (int x = bounds.x; x < bounds.x + bounds.width; ++x) {
                    if (row[x] == 0) continue;
                    if (left < 0) left = x;
                    right = x;
                }
                if (left < 0) continue;
                result.left.emplace_back(static_cast<float>(left), static_cast<float>(y));
                // Ignore scanlines whose extreme-right pixel belongs to robot shadow.
                if (right < rightLimit)
                    result.right.emplace_back(static_cast<float>(right), static_cast<float>(y));
            }
            return result;
        }

        std::string DisplayValue(double v, int precision = 2) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(precision) << v;
            return ss.str();
        }

        void DrawOverlay(cv::Mat& image, const std::array<cv::Point2f, 4>& corners,
            const cv::Point2f center, double angleDegrees) {
            for (int i = 0; i < 4; ++i) {
                cv::line(image, cv::Point(cvRound(corners[i].x), cvRound(corners[i].y)),
                    cv::Point(cvRound(corners[(i + 1) % 4].x), cvRound(corners[(i + 1) % 4].y)),
                    cv::Scalar(50, 235, 70), 2, cv::LINE_AA);
                cv::circle(image, cv::Point(cvRound(corners[i].x), cvRound(corners[i].y)), 5, cv::Scalar(0, 220, 255), -1, cv::LINE_AA);
            }
            cv::drawMarker(image, cv::Point(cvRound(center.x), cvRound(center.y)), cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 30, 2, cv::LINE_AA);
            const cv::Point2f topMiddle = (corners[0] + corners[1]) * 0.5f;
            cv::line(image, cv::Point(cvRound(center.x), cvRound(center.y)),
                cv::Point(cvRound(topMiddle.x), cvRound(topMiddle.y)), cv::Scalar(0, 200, 255), 2, cv::LINE_AA);

            const cv::Rect panel(8, 8, std::min(image.cols - 8, 335), std::min(image.rows - 8, 90));
            if (panel.width > 0 && panel.height > 0)
                cv::rectangle(image, panel, cv::Scalar(35, 35, 35), cv::FILLED);
            cv::putText(image, "WAFER LOCATE OK", { 16, 32 }, cv::FONT_HERSHEY_SIMPLEX, 0.60,
                cv::Scalar(60, 240, 70), 2, cv::LINE_AA);
            cv::putText(image, "X=" + DisplayValue(center.x) + "  Y=" + DisplayValue(center.y),
                { 16, 57 }, cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(240, 240, 240), 1, cv::LINE_AA);
            cv::putText(image, "R=" + DisplayValue(angleDegrees) + " deg",
                { 16, 81 }, cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(240, 240, 240), 1, cv::LINE_AA);
        }

    } // anonymous namespace

    DemoWaferLocator::DemoWaferLocator() : FlowCV::Node(ProcessOrder::OutOfOrder) {
        SetComponentName_("Demo_Wafer_Locator");
        SetComponentCategory_(Category::Category_Feature_Detection);
        SetComponentAuthor_("VisionFlow");
        SetComponentVersion_("0.1.0");
        SetInputCount_(1, { "in" }, { IoType::Io_Type_CvMat });
        SetOutputCount_(2, { "out", "pose" }, { IoType::Io_Type_CvMat, IoType::Io_Type_JSON });

        properties_.AddBool("dark_object", "Dark wafer on light background", true);
        properties_.AddFloat("min_area_ratio", "Minimum component / image area", 0.10f, 0.01f, 0.95f, 0.01f);
        properties_.AddFloat("right_limit_ratio", "Exclude far-right projection", 0.94f, 0.60f, 1.00f, 0.01f);
        properties_.AddFloat("bottom_limit_ratio", "Exclude bottom projection", 0.90f, 0.60f, 1.00f, 0.01f);
        properties_.AddFloat("fit_tolerance_px", "RANSAC line tolerance (px)", 2.5f, 0.5f, 12.0f, 0.1f);
        properties_.AddBool("draw_result", "Draw quadrilateral and center", true);
        SetEnabled(true);
    }

    void DemoWaferLocator::Process_(SignalBus const& inputs, SignalBus& outputs) {
        const auto* source = inputs.GetValue<cv::Mat>(0);
        if (!source || source->empty()) return;
        properties_.Sync();

        nlohmann::json pose = {
            {"data_type", "wafer_pose"},
            {"success", false},
            {"unit", "pixel"},
            {"ref_frame", {{"w", source->cols}, {"h", source->rows}}},
            {"status", "not_found"}
        };
        cv::Mat rendered;
        try {
            if (source->depth() != CV_8U || (source->channels() != 1 && source->channels() != 3 && source->channels() != 4))
                throw std::runtime_error("Input must be 8-bit grayscale, BGR, or BGRA");
            cv::Mat gray;
            if (source->channels() == 1) gray = *source;
            else cv::cvtColor(*source, gray, source->channels() == 3 ? cv::COLOR_BGR2GRAY : cv::COLOR_BGRA2GRAY);
            cv::cvtColor(gray, rendered, cv::COLOR_GRAY2BGR);
            if (!IsEnabled()) {
                pose["status"] = "disabled";
                outputs.SetValue(0, rendered);
                outputs.SetValue(1, pose);
                return;
            }

            const float minAreaRatio = properties_.Get<float>("min_area_ratio");
            const float rightLimit = properties_.Get<float>("right_limit_ratio");
            const float bottomLimit = properties_.Get<float>("bottom_limit_ratio");
            const float fitTolerance = properties_.Get<float>("fit_tolerance_px");
            if (!(minAreaRatio > 0 && minAreaRatio < 1 && rightLimit > 0 && rightLimit <= 1 &&
                bottomLimit > 0 && bottomLimit <= 1 && fitTolerance > 0 && std::isfinite(fitTolerance)))
                throw std::runtime_error("Invalid localization properties");

            cv::Mat softened, foreground, labels, stats, centroids;
            cv::GaussianBlur(gray, softened, cv::Size(5, 5), 0);
            const int mode = properties_.Get<bool>("dark_object") ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
            cv::threshold(softened, foreground, 0, 255, mode | cv::THRESH_OTSU);
            const int count = cv::connectedComponentsWithStats(foreground, labels, stats, centroids, 8, CV_32S);
            int bestLabel = -1, bestArea = 0;
            for (int label = 1; label < count; ++label) {
                const int area = stats.at<int>(label, cv::CC_STAT_AREA);
                if (area > bestArea) { bestArea = area; bestLabel = label; }
            }
            const double fullImageArea = static_cast<double>(source->rows) * source->cols;
            if (bestLabel < 0 || bestArea < minAreaRatio * fullImageArea)
                throw std::runtime_error("No sufficiently large wafer component");

            const cv::Rect bounds(stats.at<int>(bestLabel, cv::CC_STAT_LEFT),
                stats.at<int>(bestLabel, cv::CC_STAT_TOP),
                stats.at<int>(bestLabel, cv::CC_STAT_WIDTH),
                stats.at<int>(bestLabel, cv::CC_STAT_HEIGHT));
            const cv::Mat waferMask = labels == bestLabel;
            const Samples points = ExtractSideSamples(waferMask, bounds, rightLimit, bottomLimit);
            const auto top = RobustFit(points.top, fitTolerance);
            const auto bottom = RobustFit(points.bottom, fitTolerance);
            const auto left = RobustFit(points.left, fitTolerance);
            const auto right = RobustFit(points.right, fitTolerance);
            if (!top || !bottom || !left || !right)
                throw std::runtime_error("Insufficient visible wafer edges for four-line fitting");

            const std::array<cv::Point2f, 4> corners = {
                Intersection(*top, *left), Intersection(*top, *right),
                Intersection(*bottom, *right), Intersection(*bottom, *left)
            };
            const cv::Point2f center = Intersection(Through(corners[0], corners[2]),
                Through(corners[1], corners[3]));
            const cv::Point2f topDirection = corners[1] - corners[0];
            const double angle = std::atan2(topDirection.y, topDirection.x) * 180.0 / CV_PI;

            nlohmann::json cornersJson = nlohmann::json::array();
            for (const auto& point : corners)
                cornersJson.push_back({ {"x", point.x}, {"y", point.y} });
            pose["success"] = true;
            pose["status"] = "ok";
            pose["center"] = { {"x", center.x}, {"y", center.y} };
            pose["angle_deg"] = angle;
            pose["corners"] = std::move(cornersJson);
            pose["area_px2"] = bestArea;

            if (properties_.Get<bool>("draw_result"))
                DrawOverlay(rendered, corners, center, angle);
        }
        catch (const std::exception& error) {
            pose["success"] = false;
            pose["status"] = error.what();
        }
        if (rendered.empty()) rendered = source->clone();
        outputs.SetValue(0, rendered);
        outputs.SetValue(1, pose);
    }

    EXPORT_PLUGIN(DemoWaferLocator)

} // namespace DSPatch::DSPatchables
