#include <QtCore/QVariant>
#include "HoughControls.h"
#include <HoughCircles/hough_circles.hpp>
#include <HoughLines/hough_lines.hpp>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
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
#include <string>

namespace {
using Json = nlohmann::json;
template<class Node>
QWidget* Create(std::shared_ptr<Node> node, QWidget* parent, bool lines)
{
    if (!node) return nullptr;
    const std::weak_ptr<Node> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName(lines ? "houghLinesControls" : "houghCirclesControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* error = new QLabel("Invalid input image format or detection settings.", panel);
    error->setObjectName("houghError");
    error->setWordWrap(true);
    error->setProperty("severity", "error");
    layout->addWidget(error);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    std::map<std::string, QWidget*> editors;
    for (const auto& property : node->GetPropertiesSnapshot().properties) {
        if (property.key.find('.') != std::string::npos) continue;
        QWidget* editor = nullptr;
        switch (property.type) {
        case FlowCV::PropertyDataTypes::kDataTypeBool: editor = new QCheckBox(panel); break;
        case FlowCV::PropertyDataTypes::kDataTypeInt: editor = new QSpinBox(panel); break;
        case FlowCV::PropertyDataTypes::kDataTypeFloat: {
            auto* spin = new QDoubleSpinBox(panel); spin->setDecimals(6); editor = spin; break;
        }
        case FlowCV::PropertyDataTypes::kDataTypeOption: {
            auto* combo = new QComboBox(panel);
            for (const auto& option : property.options) combo->addItem(QString::fromStdString(option));
            editor = combo; break;
        }
        default: break;
        }
        if (!editor) continue;
        editor->setObjectName(QString::fromStdString(property.key));
        form->addRow(QString::fromStdString(property.desc), editor);
        editors.emplace(property.key, editor);
    }
    std::map<std::string, QPushButton*> colors;
    for (const auto* key : {"circle_color", "center_color", "text_color", "line_color"}) {
        if (lines != (std::string(key) == "line_color")) continue;
        auto* button = new QPushButton("Choose…", panel);
        button->setObjectName(QString::fromLatin1(key));
        QString label = QString::fromLatin1(key); label.replace('_', ' ');
        form->addRow(label, button);
        colors.emplace(key, button);
        QObject::connect(button, &QPushButton::clicked, panel, [weak, panel, key] {
            const auto current = weak.lock();
            if (!current) return;
            const auto color = Json::parse(current->GetState()).at(key);
            const auto initial = QColor::fromRgbF(color.at("R").get<float>(), color.at("G").get<float>(), color.at("B").get<float>());
            auto* dialog = new QColorDialog(initial, panel);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setOption(QColorDialog::DontUseNativeDialog);
            QObject::connect(dialog, &QColorDialog::colorSelected, panel, [weak, key](const QColor& selected) {
                if (const auto locked = weak.lock()) locked->SetState(Json{{key, {
                    {"R", selected.redF()}, {"G", selected.greenF()}, {"B", selected.blueF()}}}}.dump());
            });
            dialog->open();
        });
    }
    auto* timer = new QTimer(panel);
    const auto update = [weak, panel, form, editors, colors, error, timer, lines] {
        const auto current = weak.lock();
        if (!current) { timer->stop(); panel->setEnabled(false); return; }
        const auto snapshot = current->GetPropertiesSnapshot();
        for (const auto& property : snapshot.properties) {
            const auto found = editors.find(property.key);
            if (found == editors.end()) continue;
            QWidget* editor = found->second;
            const QSignalBlocker blocker(editor);
            if (auto* check = qobject_cast<QCheckBox*>(editor)) check->setChecked(std::get<bool>(property.value));
            else if (auto* combo = qobject_cast<QComboBox*>(editor)) combo->setCurrentIndex(std::get<int>(property.value));
            else if (auto* spin = qobject_cast<QSpinBox*>(editor)) {
                const auto range = std::get<FlowCV::IntRange>(property.range);
                const auto value = std::get<int>(property.value);
                spin->setRange(std::min(range.min, value), std::max(range.max, value)); spin->setValue(value);
            } else if (auto* spin = qobject_cast<QDoubleSpinBox*>(editor)) {
                const auto range = std::get<FlowCV::FloatRange>(property.range);
                const auto value = std::get<float>(property.value);
                spin->setRange(std::min(range.min, value), std::max(range.max, value));
                spin->setSingleStep(range.step); spin->setValue(value);
            }
            form->setRowVisible(editor, property.visible);
        }
        const auto state = Json::parse(current->GetState());
        for (const auto& [key, button] : colors) {
            const auto color = state.at(key);
            const auto display = QColor::fromRgbF(color.at("R").get<float>(), color.at("G").get<float>(), color.at("B").get<float>());
            button->setText(display.name());
            button->setStyleSheet(QString("background-color: %1; color: %2;").arg(display.name(), display.lightness() > 127 ? "black" : "white"));
            form->setRowVisible(button, !lines || state.at("draw_lines").get<bool>());
        }
        error->setVisible(current->HasProcessingError());
    };
    const auto set = [weak, lines, update](const std::string& key, auto value) {
        if (const auto current = weak.lock()) {
            current->Properties().Set(key, value);
            if (lines && (key == "min_theta" || key == "max_theta")) {
                const auto state = Json::parse(current->GetState());
                if (state.at("min_theta").get<float>() > state.at("max_theta").get<float>())
                    current->Properties().Set("min_theta", state.at("max_theta").get<float>() - 1.f);
            }
        }
        update();
    };
    for (const auto& [key, editor] : editors) {
        if (auto* check = qobject_cast<QCheckBox*>(editor))
            QObject::connect(check, &QCheckBox::toggled, panel, [set, key](bool value) { set(key, value); });
        else if (auto* combo = qobject_cast<QComboBox*>(editor))
            QObject::connect(combo, &QComboBox::currentIndexChanged, panel, [set, key](int value) { if (value >= 0) set(key, value); });
        else if (auto* spin = qobject_cast<QSpinBox*>(editor))
            QObject::connect(spin, &QSpinBox::valueChanged, panel, [set, key](int value) { set(key, value); });
        else if (auto* spin = qobject_cast<QDoubleSpinBox*>(editor))
            QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel, [set, key](double value) { set(key, static_cast<float>(value)); });
    }
    QObject::connect(timer, &QTimer::timeout, panel, update);
    update(); timer->start(100);
    return panel;
}
}
QWidget* CreateHoughCirclesControls(std::shared_ptr<DSPatch::DSPatchables::HCircles> node, QWidget* parent)
{ return Create(std::move(node), parent, false); }
QWidget* CreateHoughLinesControls(std::shared_ptr<DSPatch::DSPatchables::HoughLines> node, QWidget* parent)
{ return Create(std::move(node), parent, true); }
