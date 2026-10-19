#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class PerspectiveWarp; }
QWidget* CreatePerspectiveControls(std::shared_ptr<DSPatch::DSPatchables::PerspectiveWarp> node, QWidget* parent);
