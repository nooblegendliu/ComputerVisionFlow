#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class DrawNumber; }
QWidget* CreateDrawNumberControls(std::shared_ptr<DSPatch::DSPatchables::DrawNumber> node, QWidget* parent);
