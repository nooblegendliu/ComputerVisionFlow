#include "SplitControls.h"
#include <Split/split.hpp>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

QWidget* CreateSplitControls(std::shared_ptr<DSPatch::DSPatchables::Split> node, QWidget* parent)
{
    const std::weak_ptr<DSPatch::DSPatchables::Split> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName("splitControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* warning = new QLabel("No Color Image Input Detected", panel);
    warning->setObjectName("splitWarning");
    warning->setWordWrap(true);
    warning->setStyleSheet("color: #ef5350;");
    layout->addWidget(warning);
    auto* timer = new QTimer(panel);
    const auto update = [weak, panel, warning, timer] {
        if (const auto current = weak.lock()) warning->setVisible(!current->IsColor());
        else {
            timer->stop();
            panel->setEnabled(false);
        }
    };
    QObject::connect(timer, &QTimer::timeout, panel, update);
    update();
    timer->start(100);
    return panel;
}
