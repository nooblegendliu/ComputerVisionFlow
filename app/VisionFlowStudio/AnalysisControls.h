#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class Contours; class JsonViewer; }
QWidget* CreateContoursControls(std::shared_ptr<DSPatch::DSPatchables::Contours> node, QWidget* parent);
QWidget* CreateJsonViewerPanel(std::shared_ptr<DSPatch::DSPatchables::JsonViewer> node, QWidget* parent);
