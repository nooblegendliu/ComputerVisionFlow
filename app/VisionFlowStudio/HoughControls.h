#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class HCircles; class HoughLines; }
QWidget* CreateHoughCirclesControls(std::shared_ptr<DSPatch::DSPatchables::HCircles> node, QWidget* parent);
QWidget* CreateHoughLinesControls(std::shared_ptr<DSPatch::DSPatchables::HoughLines> node, QWidget* parent);
