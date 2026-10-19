#pragma once
#include <memory>
class QWidget;
namespace DSPatch::DSPatchables { class BlobDetector; }
QWidget* CreateBlobControls(std::shared_ptr<DSPatch::DSPatchables::BlobDetector> node, QWidget* parent);
