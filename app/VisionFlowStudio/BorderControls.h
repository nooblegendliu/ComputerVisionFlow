#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class CopyMakeBorder; }
QWidget* CreateCopyMakeBorderControls(std::shared_ptr<DSPatch::DSPatchables::CopyMakeBorder> node, QWidget* parent);
