#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class DrawText; }
QWidget* CreateDrawTextControls(std::shared_ptr<DSPatch::DSPatchables::DrawText> node, QWidget* parent);
