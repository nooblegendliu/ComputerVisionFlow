#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class Threshold; class Morphology; }
QWidget* CreateThresholdControls(std::shared_ptr<DSPatch::DSPatchables::Threshold> node, QWidget* parent);
QWidget* CreateMorphologyControls(std::shared_ptr<DSPatch::DSPatchables::Morphology> node, QWidget* parent);
