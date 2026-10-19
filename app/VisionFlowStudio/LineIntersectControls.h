#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class LineIntersect; }
QWidget* CreateLineIntersectControls(std::shared_ptr<DSPatch::DSPatchables::LineIntersect> node, QWidget* parent);
