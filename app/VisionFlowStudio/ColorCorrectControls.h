#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class Colorcorrect; }
QWidget* CreateColorCorrectControls(std::shared_ptr<DSPatch::DSPatchables::Colorcorrect> node, QWidget* parent);
