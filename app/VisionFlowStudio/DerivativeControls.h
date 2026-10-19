#pragma once
#include <memory>

class QWidget;
namespace DSPatch::DSPatchables { class SobelFilter; class LaplacianFilter; class CannyFilter; }

QWidget* CreateSobelControls(std::shared_ptr<DSPatch::DSPatchables::SobelFilter> node, QWidget* parent);
QWidget* CreateLaplacianControls(std::shared_ptr<DSPatch::DSPatchables::LaplacianFilter> node, QWidget* parent);
QWidget* CreateCannyControls(std::shared_ptr<DSPatch::DSPatchables::CannyFilter> node, QWidget* parent);
