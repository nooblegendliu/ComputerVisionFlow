#include "HistogramPanel.h"
#include "DepthViewerPanel.h"
#include "DnnControls.h"
#include <internal_nodes.h>
#include "DrawNumberControls.h"
#include <DrawTools/draw_number.hpp>
#include "DrawDateTimeControls.h"
#include <DrawTools/draw_datetime.hpp>
#include "DrawJsonControls.h"
#include <DrawTools/draw_json.hpp>
#include "DrawTextControls.h"
#include <DrawTools/draw_text.hpp>
#include "BlobControls.h"
#include "HoughControls.h"
#include "LineIntersectControls.h"
#include <BlobDetector/blob_detector.hpp>
#include <HoughCircles/hough_circles.hpp>
#include <HoughLines/hough_lines.hpp>
#include <LineIntersect/line_intersect.hpp>
#include "StudioNodeUis.h"
#include "DerivativeControls.h"
#include <Canny/canny.hpp>
#include <SobelFilter/sobel_filter.hpp>
#include <LaplacianFilter/laplacian_filter.hpp>
#include "ColorCorrectControls.h"
#include "SplitControls.h"
#include <Split/split.hpp>
#include <Colorcorrect/ColorCorrect.hpp>
#include "CropControls.h"
#include <Crop/crop.hpp>
#include "PerspectiveControls.h"
#include "BorderControls.h"
#include <PerspectiveWarp/perspective_warp.hpp>
#include <CopyMakeBorder/copy_make_border.hpp>
#include "ViewerPanel.h"
#include "ConvertColorControls.h"
#include "FilterControls.h"
#include "AnalysisControls.h"
#include <Threshold/threshold.hpp>
#include <Morphology/Morphology.hpp>
#include <Contours/contours.hpp>
#include <JsonViewer/json_viewer.hpp>
#include <ConvertColor/convert_color.hpp>
#include <NodeUi/NodeUiRegistry.h>
#include <Viewer/viewer.hpp>

void RegisterStudioNodeUis(VisionFlow::NodeUiRegistry& registry)
{
    registry.registerFactory("Histogram", VisionFlow::NodeUiRole::Main,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::HistogramViewer>(node);
            return value ? CreateHistogramPanel(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Depth_Viewer_3D", VisionFlow::NodeUiRole::Main,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::DepthViewer3D>(node);
            return value ? CreateDepthViewerPanel(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Depth_Viewer_3D", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::DepthViewer3D>(node);
            return value ? CreateDepthViewerControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Classification", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::Classification>(node);
            return value ? CreateDnnControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Object_Detection", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::ObjectDetection>(node);
            return value ? CreateDnnControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Segmentation", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::Segmentation>(node);
            return value ? CreateDnnControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Human_Pose", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::HumanPose>(node);
            return value ? CreateDnnControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Text_Detection", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::TextDetection>(node);
            return value ? CreateDnnControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Text_Recognition", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::TextRecognition>(node);
            return value ? CreateDnnControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Image_Processing", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::ImageProcessing>(node);
            return value ? CreateDnnControls(std::move(value), parent) : nullptr;
        });

    registry.registerFactory("Draw_Number", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::DrawNumber>(node);
            return value ? CreateDrawNumberControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Draw_Date_Time", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::DrawDateTime>(node);
            return value ? CreateDrawDateTimeControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Draw_JSON", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::DrawJson>(node);
            return value ? CreateDrawJsonControls(std::move(value), parent) : nullptr;
        });

    registry.registerFactory("Draw_Text", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::DrawText>(node);
            return value ? CreateDrawTextControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Blob_Detector", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::BlobDetector>(node);
            return value ? CreateBlobControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Hough_Circles", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::HCircles>(node);
            return value ? CreateHoughCirclesControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Hough_Lines", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::HoughLines>(node);
            return value ? CreateHoughLinesControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Line_Intersections", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::LineIntersect>(node);
            return value ? CreateLineIntersectControls(std::move(value), parent) : nullptr;
        });

    registry.registerFactory("Canny", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::CannyFilter>(node);
            return value ? CreateCannyControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Sobel", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::SobelFilter>(node);
            return value ? CreateSobelControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Laplacian", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::LaplacianFilter>(node);
            return value ? CreateLaplacianControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Split", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::Split>(node);
            return value ? CreateSplitControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Color_Correct", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::Colorcorrect>(node);
            return value ? CreateColorCorrectControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Perspective_Warp", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::PerspectiveWarp>(node);
            return value ? CreatePerspectiveControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Copy_Make_Border", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::CopyMakeBorder>(node);
            return value ? CreateCopyMakeBorderControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Crop", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::Crop>(node);
            return value ? CreateCropControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Threshold", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::Threshold>(node);
            return value ? CreateThresholdControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Morphology", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::Morphology>(node);
            return value ? CreateMorphologyControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Contours", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::Contours>(node);
            return value ? CreateContoursControls(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Json_Viewer", VisionFlow::NodeUiRole::Main,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto value = std::dynamic_pointer_cast<DSPatch::DSPatchables::JsonViewer>(node);
            return value ? CreateJsonViewerPanel(std::move(value), parent) : nullptr;
        });
    registry.registerFactory("Convert_Color", VisionFlow::NodeUiRole::Controls,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto conversion = std::dynamic_pointer_cast<DSPatch::DSPatchables::ConvertColor>(node);
            return conversion ? CreateConvertColorControls(std::move(conversion), parent) : nullptr;
        });
    registry.registerFactory("Viewer", VisionFlow::NodeUiRole::Main,
        [](const DSPatch::Component::SPtr& node, QWidget* parent) -> QWidget* {
            auto viewer = std::dynamic_pointer_cast<DSPatch::DSPatchables::Viewer>(node);
            return viewer ? new ViewerPanel(std::move(viewer), parent) : nullptr;
        });
}
