// Optional Qt Controls; NodeUiRegistry keeps the node/module alive until teardown.
#include "simple_blob_tracker.hpp"
#include <NodeUi/NodeUiProvider.h>
#include <QtCore/QSignalBlocker>
#include <QtCore/QTimer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <algorithm>

namespace DSPatch::DSPatchables {
class SimpleBlobTrackerQt final : public SimpleBlobTracker, public VisionFlow::NodeUiProvider
{
public:
    bool hasNodeUi(VisionFlow::NodeUiRole role) const override
    { return role == VisionFlow::NodeUiRole::Controls; }
    QWidget* createNodeUi(VisionFlow::NodeUiRole role, QWidget* parent) override
    {
        if (!hasNodeUi(role)) return nullptr;
        auto* panel = new QWidget(parent);
        panel->setObjectName("SimpleBlobTrackerControls");
        auto* layout = new QFormLayout(panel);
        auto* reset = new QPushButton("Reset Tracking", panel);
        reset->setObjectName("SimpleBlobTrackerReset");
        layout->addRow(reset);
        QObject::connect(reset, &QPushButton::clicked, panel, [this] { RequestReset(); });
        const auto snapshot = GetPropertiesSnapshot();
        for (const auto& property : snapshot.properties) {
            const auto key = property.key;
            const auto label = QString::fromStdString(property.desc);
            if (property.type == FlowCV::PropertyDataTypes::kDataTypeBool) {
                auto* box = new QCheckBox(panel);
                box->setObjectName(QString::fromStdString(key));
                box->setChecked(std::get<bool>(property.value));
                QObject::connect(box, &QCheckBox::toggled, panel,
                    [this, key](bool value) { Properties().Set(key, value); });
                layout->addRow(label, box);
            } else {
                auto* box = new QSpinBox(panel);
                box->setObjectName(QString::fromStdString(key));
                const auto range = std::get<FlowCV::IntRange>(property.range);
                const auto value = std::get<int>(property.value);
                box->setRange(std::min(range.min, value), std::max(range.max, value));
                box->setValue(value);
                QObject::connect(box, &QSpinBox::valueChanged, panel,
                    [this, key](int value) { Properties().Set(key, value); });
                layout->addRow(label, box);
            }
        }
        auto* timer = new QTimer(panel);
        QObject::connect(timer, &QTimer::timeout, panel, [this, panel] {
            for (const auto& property : GetPropertiesSnapshot().properties) {
                const auto name = QString::fromStdString(property.key);
                if (auto* box = panel->findChild<QCheckBox*>(name)) {
                    const QSignalBlocker blocker(box);
                    box->setChecked(std::get<bool>(property.value));
                } else if (auto* box = panel->findChild<QSpinBox*>(name)) {
                    const QSignalBlocker blocker(box);
                    const auto range = std::get<FlowCV::IntRange>(property.range);
                    const auto value = std::get<int>(property.value);
                    box->setRange(std::min(range.min, value), std::max(range.max, value));
                    box->setValue(value);
                }
            }
        });
        timer->start(100);
        return panel;
    }
};
EXPORT_PLUGIN(SimpleBlobTrackerQt)
}
