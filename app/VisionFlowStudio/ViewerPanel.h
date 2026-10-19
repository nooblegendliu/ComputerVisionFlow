#pragma once
#include "ViewerImage.h"
#include <QtWidgets/QWidget>
#include <memory>

class QComboBox;
class QDoubleSpinBox;
class QTimer;
class QScrollArea;
class ViewerCanvas;
namespace DSPatch::DSPatchables { class Viewer; }

// One GUI-thread-only panel per node; owns its display options, never the flow.
class ViewerPanel final : public QWidget
{
public:
    explicit ViewerPanel(std::shared_ptr<DSPatch::DSPatchables::Viewer> node,
        QWidget* parent = nullptr);
private:
    void refreshFrame();
    void updateCanvasGeometry();
    bool eventFilter(QObject* watched, QEvent* event) override;
    std::shared_ptr<DSPatch::DSPatchables::Viewer> node_;
    ViewerCanvas* canvas_ = nullptr;
    QScrollArea* scroll_ = nullptr;
    QWidget* colorMapControls_ = nullptr;
    QComboBox* channel_ = nullptr;
    QComboBox* aspect_ = nullptr;
    QComboBox* colorMap_ = nullptr;
    QDoubleSpinBox* scale_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
};
