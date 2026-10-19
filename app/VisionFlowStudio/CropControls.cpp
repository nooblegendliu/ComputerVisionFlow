#include "CropControls.h"
#include <Crop/crop.hpp>
#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <limits>

QWidget* CreateCropControls(std::shared_ptr<DSPatch::DSPatchables::Crop> node, QWidget* parent)
{
    using Crop = DSPatch::DSPatchables::Crop;
    auto* panel = new QWidget(parent); panel->setObjectName("cropControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* manual = new QWidget(panel); auto* manualForm = new QFormLayout(manual);
    manualForm->setContentsMargins(0, 0, 0, 0); manualForm->setRowWrapPolicy(QFormLayout::WrapLongRows);
    auto* jsonPanel = new QWidget(panel); auto* jsonForm = new QFormLayout(jsonPanel);
    jsonForm->setContentsMargins(0, 0, 0, 0); jsonForm->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addWidget(manual); layout->addWidget(jsonPanel);
    const std::weak_ptr<Crop> weak = node;
    const auto spin = [panel](QFormLayout* form, const char* key, const char* label, int low, int high) {
        auto* field = new QSpinBox(panel); field->setObjectName(QStringLiteral("Crop_") + key);
        field->setRange(low, high); form->addRow(QString::fromLatin1(label), field); return field;
    };
    auto* x = spin(manualForm, "x", "Start X", 0, 1000);
    auto* y = spin(manualForm, "y", "Start Y", 0, 1000);
    auto* width = spin(manualForm, "width", "Width", 0, 8000);
    auto* height = spin(manualForm, "height", "Height", 0, 8000);
    const auto bindBox = [panel, weak](QSpinBox* field, const char* key) {
        QObject::connect(field, &QSpinBox::valueChanged, panel, [weak, key](int value) {
            if (auto current = weak.lock()) {
                const auto p = current->GetParameters();
                nlohmann::json box{{"X", p.x}, {"Y", p.y}, {"W", p.width}, {"H", p.height}};
                box[key] = value; current->SetState(nlohmann::json{{"bbox", box}}.dump());
            }
        });
    };
    bindBox(x, "X"); bindBox(y, "Y"); bindBox(width, "W"); bindBox(height, "H");
    auto* total = new QCheckBox("Calculate Total BBOX from All", jsonPanel); total->setObjectName("Crop_total");
    jsonForm->addRow(total);
    auto* index = spin(jsonForm, "index", "BBOX Index", 0, 2000);
    auto* paddingX = spin(jsonForm, "paddingX", "Padding X", (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)());
    auto* paddingY = spin(jsonForm, "paddingY", "Padding Y", (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)());
    QObject::connect(total, &QCheckBox::toggled, panel, [weak](bool value) {
        if (auto current = weak.lock()) current->SetState(nlohmann::json{{"calc_total_bbox", value}}.dump());
    });
    const auto bind = [panel, weak](QSpinBox* field, const char* key) {
        QObject::connect(field, &QSpinBox::valueChanged, panel, [weak, key](int value) {
            if (auto current = weak.lock()) current->SetState(nlohmann::json{{key, value}}.dump());
        });
    };
    bind(index, "json_bbox_index"); bind(paddingX, "adjust_size_x"); bind(paddingY, "adjust_size_y");
    auto* error = new QLabel("Invalid crop coordinates or bbox data", panel); error->setObjectName("Crop_error");
    error->setWordWrap(true); layout->addWidget(error);
    const auto update = [weak, manual, jsonPanel, jsonForm, total, index, x, y, width, height, paddingX, paddingY, error] {
        const auto current = weak.lock(); if (!current) return;
        const auto p = current->GetParameters();
        const auto set = [](QSpinBox* field, int value) {
            const QSignalBlocker block(field);
            if (value < field->minimum()) field->setMinimum(value);
            if (value > field->maximum()) field->setMaximum(value);
            field->setValue(value);
        };
        set(x, p.x); set(y, p.y); set(width, p.width); set(height, p.height);
        set(index, p.json_bbox_index); set(paddingX, p.adjust_size_x); set(paddingY, p.adjust_size_y);
        { const QSignalBlocker block(total); total->setChecked(p.calc_total_bbox); }
        manual->setVisible(!current->HasJsonData()); jsonPanel->setVisible(current->HasJsonData());
        jsonForm->setRowVisible(index, !p.calc_total_bbox);
        error->setVisible(current->HasProcessingError());
    };
    auto* timer = new QTimer(panel); timer->setInterval(100);
    QObject::connect(timer, &QTimer::timeout, panel, update); update(); timer->start();
    return panel;
}
