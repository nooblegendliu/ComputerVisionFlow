#include <QtCore/QVariant>
#include "BlobControls.h"
#include <BlobDetector/blob_detector.hpp>
#include <QCheckBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <map>

QWidget* CreateBlobControls(std::shared_ptr<DSPatch::DSPatchables::BlobDetector> node, QWidget* parent)
{
    if (!node) return nullptr;
    const std::weak_ptr<DSPatch::DSPatchables::BlobDetector> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName("blobControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    std::map<std::string, QWidget*> editors;
    for (const auto& property : node->GetPropertiesSnapshot().properties) {
        if (property.key.rfind("blob_viz_color.", 0) == 0) continue;
        QWidget* editor = nullptr;
        if (property.type == FlowCV::PropertyDataTypes::kDataTypeBool) editor = new QCheckBox(panel);
        else if (property.type == FlowCV::PropertyDataTypes::kDataTypeInt) editor = new QSpinBox(panel);
        else if (property.type == FlowCV::PropertyDataTypes::kDataTypeFloat) {
            auto* spin = new QDoubleSpinBox(panel);
            spin->setDecimals(3);
            editor = spin;
        }
        if (!editor) continue;
        editor->setObjectName(QString::fromStdString(property.key));
        form->addRow(QString::fromStdString(property.desc), editor);
        editors.emplace(property.key, editor);
    }
    auto* picker = new QPushButton("Blob Color…", panel);
    picker->setObjectName("blob_viz_color");
    layout->addWidget(picker);
    auto* error = new QLabel("Incompatible image format or blob detector settings.", panel);
    error->setObjectName("blobError"); error->setWordWrap(true);
    error->setProperty("severity", "error");
    layout->addWidget(error);
    auto* timer = new QTimer(panel);
    const auto update = [weak, panel, form, editors, error, timer, picker] {
        const auto current = weak.lock();
        if (!current) { timer->stop(); panel->setEnabled(false); return; }
        for (const auto& property : current->GetPropertiesSnapshot().properties) {
            const auto found = editors.find(property.key);
            if (found == editors.end()) continue;
            auto* editor = found->second;
            const QSignalBlocker blocker(editor);
            if (auto* check = qobject_cast<QCheckBox*>(editor)) check->setChecked(std::get<bool>(property.value));
            else if (auto* spin = qobject_cast<QSpinBox*>(editor)) {
                const auto range = std::get<FlowCV::IntRange>(property.range);
                const int value = std::get<int>(property.value);
                spin->setRange((std::min)(range.min, value), (std::max)(range.max, value));
                spin->setValue(value);
            } else if (auto* spin = qobject_cast<QDoubleSpinBox*>(editor)) {
                const auto range = std::get<FlowCV::FloatRange>(property.range);
                const float value = std::get<float>(property.value);
                spin->setRange((std::min)(range.min, value), (std::max)(range.max, value));
                spin->setSingleStep(range.step); spin->setValue(value);
            }
            form->setRowVisible(editor, property.visible);
        }
        const auto color = nlohmann::json::parse(current->GetState()).at("blob_viz_color");
        const auto rgb = QColor::fromRgbF(color.at("R").get<float>(), color.at("G").get<float>(), color.at("B").get<float>());
        picker->setStyleSheet(QString("border: 3px solid %1;").arg(rgb.name()));
        error->setVisible(current->HasProcessingError());
    };
    for (const auto& [key, editor] : editors) {
        if (auto* check = qobject_cast<QCheckBox*>(editor)) {
            QObject::connect(check, &QCheckBox::toggled, panel, [weak, key, update](bool value) {
                if (const auto current = weak.lock()) current->Properties().Set(key, value);
                update();
            });
        } else if (auto* spin = qobject_cast<QSpinBox*>(editor)) {
            QObject::connect(spin, &QSpinBox::valueChanged, panel, [weak, key, update](int value) {
                if (const auto current = weak.lock()) current->Properties().Set(key, key == "min_repeat" ? (std::max)(1, value) : value);
                update();
            });
        } else if (auto* spin = qobject_cast<QDoubleSpinBox*>(editor)) {
            QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel, [weak, key, update](double value) {
                if (const auto current = weak.lock()) current->Properties().Set(key,
                    float(key == "threshold_step" ? (std::max)(1.0, value) : value));
                update();
            });
        }
    }
    QObject::connect(picker, &QPushButton::clicked, panel, [weak, panel, update] {
        if (const auto current = weak.lock()) {
            const auto color = nlohmann::json::parse(current->GetState()).at("blob_viz_color");
            auto* dialog = new QColorDialog(QColor::fromRgbF(color.at("R").get<float>(), color.at("G").get<float>(), color.at("B").get<float>()), panel);
            dialog->setObjectName("blobColorDialog");
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(dialog, &QColorDialog::colorSelected, panel, [weak, update](const QColor& picked) {
                if (const auto target = weak.lock()) target->SetState(nlohmann::json{{"blob_viz_color",
                    {{"R", picked.redF()}, {"G", picked.greenF()}, {"B", picked.blueF()}}}}.dump());
                update();
            });
            dialog->open();
        }
    });
    QObject::connect(timer, &QTimer::timeout, panel, update);
    update(); timer->start(100);
    return panel;
}
