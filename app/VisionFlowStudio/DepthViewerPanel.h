#pragma once
#include <QWidget>
#include <memory>
namespace DSPatch::DSPatchables {class DepthViewer3D;}
QWidget* CreateDepthViewerPanel(std::shared_ptr<DSPatch::DSPatchables::DepthViewer3D>,QWidget*);
QWidget* CreateDepthViewerControls(std::shared_ptr<DSPatch::DSPatchables::DepthViewer3D>,QWidget*);
