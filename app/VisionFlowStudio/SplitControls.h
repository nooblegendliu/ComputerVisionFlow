#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class Split; }
QWidget* CreateSplitControls(std::shared_ptr<DSPatch::DSPatchables::Split> node, QWidget* parent);
