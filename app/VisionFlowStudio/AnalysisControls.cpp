#include "AnalysisControls.h"
#include <Contours/contours.hpp>
#include <JsonViewer/json_viewer.hpp>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>

using DSPatch::DSPatchables::Contours;
using DSPatch::DSPatchables::JsonViewer;

namespace {
QColor toColor(const std::array<float, 3>& value)
{
    return QColor::fromRgbF(std::clamp(value[0], 0.0f, 1.0f),
        std::clamp(value[1], 0.0f, 1.0f), std::clamp(value[2], 0.0f, 1.0f));
}
QString cellText(const nlohmann::json& value)
{
    if (value.is_string()) return QString::fromStdString(value.get<std::string>());
    if (value.is_number_float()) return QString::fromStdString(std::to_string(value.get<float>()));
    if (value.is_array()) return QStringLiteral("<Array Data>");
    if (value.is_object()) return QStringLiteral("<JSON Object>");
    if (value.is_null()) return {};
    return QString::fromStdString(value.dump());
}
}

QWidget* CreateContoursControls(std::shared_ptr<Contours> node, QWidget* parent)
{
    auto* panel = new QWidget(parent);
    panel->setObjectName("contoursControls");
    auto* form = new QFormLayout(panel);
    const std::weak_ptr<Contours> weak = node;
    const auto patch = [weak](const char* key, nlohmann::json value) {
        if (auto target = weak.lock()) target->SetState(nlohmann::json{{key, std::move(value)}}.dump());
    };
    const auto makeCombo = [&](const char* key, const QString& label, const QStringList& options) {
        auto* combo = new QComboBox(panel);
        combo->setObjectName(QStringLiteral("contours_") + key);
        combo->addItems(options);
        form->addRow(label, combo);
        QObject::connect(combo, &QComboBox::currentIndexChanged, panel, [patch, key](int index) { patch(key, index); });
        return combo;
    };
    auto* mode = makeCombo("mode", "Contour Mode", {"External", "List", "COMP", "Tree"});
    auto* method = makeCombo("method", "Contour Method", {"None", "Simple", "TC89_L1", "TC89_KCOS"});
    const auto makeCheck = [&](const char* key, const QString& label) {
        auto* check = new QCheckBox(label, panel);
        check->setObjectName(QStringLiteral("contours_") + key);
        form->addRow(check);
        QObject::connect(check, &QCheckBox::toggled, panel, [patch, key](bool value) { patch(key, value); });
        return check;
    };
    auto* filter = makeCheck("filter_by_area", "Filter by Area");
    const auto makeArea = [&](const char* key, const QString& label, double maximum) {
        auto* spin = new QDoubleSpinBox(panel);
        spin->setObjectName(QStringLiteral("contours_") + key);
        spin->setRange(1, maximum);
        spin->setDecimals(2);
        spin->setSingleStep(0.1);
        form->addRow(label, spin);
        QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel,
            [patch, key](double value) { patch(key, static_cast<float>(value)); });
        return spin;
    };
    auto* minimum = makeArea("area_min", "Min Area", 1000);
    auto* maximum = makeArea("area_max", "Max Area", 5000);
    auto* draw = makeCheck("draw_contours", "Draw Contours");
    auto* fill = makeCheck("fill_contours", "Fill Contours");
    const auto makeColor = [&](const char* key, const QString& label, bool boundingBox) {
        auto* button = new QPushButton(panel);
        button->setObjectName(QStringLiteral("contours_") + key);
        form->addRow(label, button);
        QObject::connect(button, &QPushButton::clicked, panel, [weak, patch, key, button, boundingBox] {
            const auto target = weak.lock();
            if (!target) return;
            const auto parameters = target->GetParameters();
            auto* dialog = new QColorDialog(toColor(boundingBox ? parameters.bbox_color : parameters.contour_color), button);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setOption(QColorDialog::DontUseNativeDialog);
            QObject::connect(dialog, &QColorDialog::colorSelected, button, [patch, key](const QColor& color) {
                patch(key, {{"R", color.redF()}, {"G", color.greenF()}, {"B", color.blueF()}});
            });
            dialog->open();
        });
        return button;
    };
    auto* contourColor = makeColor("contour_color", "Contour Color", false);
    auto* box = makeCheck("draw_bbox", "Draw BBox");
    auto* boxColor = makeColor("bbox_color", "BBox Color", true);
    const auto refresh = [=] {
        const auto target = weak.lock();
        if (!target) { panel->setEnabled(false); return; }
        const auto p = target->GetParameters();
        const QSignalBlocker b1(mode), b2(method), b3(filter), b4(minimum), b5(maximum), b6(draw), b7(fill), b8(box);
        mode->setCurrentIndex(p.mode);
        method->setCurrentIndex(p.method);
        filter->setChecked(p.filter_by_area);
        if (!minimum->hasFocus()) minimum->setValue(p.area_min);
        if (!maximum->hasFocus()) maximum->setValue(p.area_max);
        draw->setChecked(p.draw_contours);
        fill->setChecked(p.fill_contours);
        box->setChecked(p.draw_bbox);
        for (const auto& pair : {std::make_pair(contourColor, toColor(p.contour_color)), std::make_pair(boxColor, toColor(p.bbox_color))}) {
            pair.first->setText(pair.second.name());
            pair.first->setStyleSheet(QStringLiteral("background-color: %1; color: %2;")
                .arg(pair.second.name(), pair.second.lightnessF() > .5 ? "black" : "white"));
        }
        form->setRowVisible(minimum, p.filter_by_area);
        form->setRowVisible(maximum, p.filter_by_area);
        form->setRowVisible(fill, p.draw_contours);
        form->setRowVisible(contourColor, p.draw_contours);
        form->setRowVisible(boxColor, p.draw_bbox);
    };
    refresh();
    auto* timer = new QTimer(panel);
    QObject::connect(timer, &QTimer::timeout, panel, refresh);
    QObject::connect(filter, &QCheckBox::toggled, panel, refresh);
    QObject::connect(draw, &QCheckBox::toggled, panel, refresh);
    QObject::connect(box, &QCheckBox::toggled, panel, refresh);
    timer->start(100);
    return panel;
}

QWidget* CreateJsonViewerPanel(std::shared_ptr<JsonViewer> node, QWidget* parent)
{
    auto* panel = new QWidget(parent);
    panel->setObjectName("jsonViewerPanel");
    auto* layout = new QVBoxLayout(panel);
    auto* toolbar = new QHBoxLayout;
    auto* rawMode = new QCheckBox("Show Raw Data", panel);
    rawMode->setObjectName("jsonViewerShowRaw");
    auto* copy = new QPushButton("Copy JSON", panel);
    copy->setObjectName("jsonViewerCopy");
    toolbar->addWidget(rawMode);
    toolbar->addStretch();
    toolbar->addWidget(copy);
    layout->addLayout(toolbar);
    auto* stack = new QStackedWidget(panel);
    auto* table = new QTableWidget(stack);
    table->setObjectName("jsonViewerTable");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    auto* raw = new QPlainTextEdit(stack);
    raw->setObjectName("jsonViewerRaw");
    raw->setReadOnly(true);
    raw->setLineWrapMode(QPlainTextEdit::NoWrap);
    stack->addWidget(table);
    stack->addWidget(raw);
    layout->addWidget(stack);
    QObject::connect(rawMode, &QCheckBox::toggled, panel, [stack](bool show) { stack->setCurrentIndex(show ? 1 : 0); });
    const std::weak_ptr<JsonViewer> weak = node;
    const auto copyJson = [weak] {
        if (const auto target = weak.lock()) {
            const auto snapshot = target->GetJsonCopy();
            if (!snapshot.empty()) QApplication::clipboard()->setText(QString::fromStdString(snapshot.dump(4)));
        }
    };
    QObject::connect(copy, &QPushButton::clicked, panel, copyJson);
    auto* copyAction = new QAction("Copy JSON to Clipboard", raw);
    QObject::connect(copyAction, &QAction::triggered, panel, copyJson);
    raw->addAction(copyAction);
    raw->setContextMenuPolicy(Qt::ActionsContextMenu);
    const auto previous = std::make_shared<nlohmann::json>();
    const auto refresh = [weak, panel, previous, table, raw, copy] {
        const auto target = weak.lock();
        if (!target) { panel->setEnabled(false); return; }
        const auto snapshot = target->GetJsonCopy();
        copy->setEnabled(!snapshot.empty());
        if (snapshot == *previous) return;
        *previous = snapshot;
        raw->setPlainText(snapshot.empty() ? QString{} : QString::fromStdString(snapshot.dump(4)));
        table->clear();
        table->setRowCount(0);
        table->setColumnCount(0);
        if (!snapshot.is_object() || !snapshot.contains("data")) return;
        const auto& data = snapshot.at("data");
        // Upstream's table expects an array of objects. Other JSON remains
        // available in Raw mode without throwing from the GUI timer.
        if (!data.is_array() || data.empty() || !data.front().is_object()) return;
        QStringList headers;
        for (auto it = data.front().begin(); it != data.front().end(); ++it)
            headers.append(QString::fromStdString(it.key()));
        table->setColumnCount(static_cast<int>(headers.size()));
        table->setHorizontalHeaderLabels(headers);
        table->setRowCount(static_cast<int>(data.size()));
        for (std::size_t row = 0; row < data.size(); ++row) {
            if (!data[row].is_object()) continue;
            for (int column = 0; column < headers.size(); ++column) {
                const auto key = headers[column].toStdString();
                if (data[row].contains(key))
                    table->setItem(static_cast<int>(row), column, new QTableWidgetItem(cellText(data[row].at(key))));
            }
        }
    };
    refresh();
    auto* timer = new QTimer(panel);
    QObject::connect(timer, &QTimer::timeout, panel, refresh);
    timer->start(100);
    return panel;
}
