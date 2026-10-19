#pragma once
#include <QWidget>
#include <memory>
namespace DSPatch::DSPatchables { class HistogramViewer; }
QWidget* CreateHistogramPanel(std::shared_ptr<DSPatch::DSPatchables::HistogramViewer>, QWidget*);
