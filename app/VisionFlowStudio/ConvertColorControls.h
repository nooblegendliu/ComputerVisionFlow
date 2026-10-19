#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class ConvertColor; }
QWidget* CreateConvertColorControls(std::shared_ptr<DSPatch::DSPatchables::ConvertColor> node, QWidget* parent);
