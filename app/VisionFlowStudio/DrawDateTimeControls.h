#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class DrawDateTime; }
QWidget* CreateDrawDateTimeControls(std::shared_ptr<DSPatch::DSPatchables::DrawDateTime> node, QWidget* parent);
