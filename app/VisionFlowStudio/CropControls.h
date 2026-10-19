#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class Crop; }
QWidget* CreateCropControls(std::shared_ptr<DSPatch::DSPatchables::Crop> node, QWidget* parent);
