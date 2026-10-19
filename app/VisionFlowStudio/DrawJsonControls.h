#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class DrawJson; }
QWidget* CreateDrawJsonControls(std::shared_ptr<DSPatch::DSPatchables::DrawJson> node, QWidget* parent);
