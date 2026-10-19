#include <QtCore/QVariant>
#include "BorderControls.h"
#include <CopyMakeBorder/copy_make_border.hpp>
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
#include <functional>
#include <vector>

QWidget* CreateCopyMakeBorderControls(std::shared_ptr<DSPatch::DSPatchables::CopyMakeBorder> node, QWidget* parent)
{
    using Node = DSPatch::DSPatchables::CopyMakeBorder;
    const std::weak_ptr<Node> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName("copyMakeBorderControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    std::vector<std::function<void()>> refresh;
    auto* center = new QCheckBox(panel);
    center->setObjectName("center");
    form->addRow("Center", center);
    QObject::connect(center, &QCheckBox::toggled, panel, [weak](bool checked) {
        if (const auto current = weak.lock()) current->Properties().Set("center", checked);
    });
    refresh.emplace_back([weak, center] {
        if (const auto current = weak.lock()) {
            const QSignalBlocker blocker(center);
            center->setChecked(current->Properties().GetW<bool>("center"));
        }
    });
    for (const auto* key : {"add_width", "add_height"}) {
        auto* spin = new QSpinBox(panel);
        spin->setObjectName(QString::fromLatin1(key));
        spin->setRange(0, 500);
        form->addRow(key == std::string("add_width") ? "Border Width" : "Border Height", spin);
        QObject::connect(spin, &QSpinBox::valueChanged, panel, [weak, key](int value) {
            if (const auto current = weak.lock()) current->Properties().Set(key, value);
        });
        refresh.emplace_back([weak, key, spin] {
            if (const auto current = weak.lock()) {
                const QSignalBlocker blocker(spin);
                const int value = current->Properties().GetW<int>(key);
                if (value < spin->minimum()) spin->setMinimum(value);
                if (value > spin->maximum()) spin->setMaximum(value);
                spin->setValue(value);
            }
        });
    }
    auto* type = new QComboBox(panel);
    type->setObjectName("border_type");
    for (const auto& name : node->Properties().GetOptions("border_type")) type->addItem(QString::fromStdString(name));
    form->addRow("Border Type", type);
    auto* color = new QWidget(panel);
    color->setObjectName("border_color");
    auto* colors = new QVBoxLayout(color);
    colors->setContentsMargins(0, 0, 0, 0);
    auto* picker = new QPushButton("Border Color…", color);
    picker->setObjectName("border_colorPicker");
    colors->addWidget(picker);
    layout->addWidget(color);
    QObject::connect(type, &QComboBox::currentIndexChanged, panel, [weak, color](int index) {
        if (const auto current = weak.lock()) current->Properties().Set("border_type", index);
        color->setVisible(index == 0);
    });
    refresh.emplace_back([weak, type, color] {
        if (const auto current = weak.lock()) {
            const QSignalBlocker blocker(type);
            const int value = current->Properties().GetW<int>("border_type");
            type->setCurrentIndex(value);
            color->setVisible(value == 0);
        }
    });
    QObject::connect(picker, &QPushButton::clicked, panel, [weak, panel] {
        if (const auto current = weak.lock()) {
            const auto state = nlohmann::json::parse(current->GetState()).at("border_color");
            auto* dialog = new QColorDialog(QColor::fromRgbF(state.at("R").get<float>(), state.at("G").get<float>(), state.at("B").get<float>()), panel);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(dialog, &QColorDialog::colorSelected, panel, [weak](const QColor& picked) {
                if (const auto target = weak.lock()) target->SetState(nlohmann::json{{"border_color",
                    {{"R", picked.redF()}, {"G", picked.greenF()}, {"B", picked.blueF()}}}}.dump());
            });
            dialog->open();
        }
    });
    for (const auto* channel : {"R", "G", "B"}) {
        const auto key = std::string("border_color.") + channel;
        auto* spin = new QDoubleSpinBox(color);
        spin->setObjectName(QString::fromStdString(key));
        spin->setPrefix(QString::fromLatin1(channel) + " ");
        spin->setRange(0, 1); spin->setDecimals(3); spin->setSingleStep(.01);
        colors->addWidget(spin);
        QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel, [weak, key](double value) {
            if (const auto current = weak.lock()) current->Properties().Set(key, float(value));
        });
        refresh.emplace_back([weak, key, spin] {
            if (const auto current = weak.lock()) {
                const QSignalBlocker blocker(spin);
                spin->setValue(current->Properties().GetW<float>(key));
            }
        });
    }
    auto* error = new QLabel("Incompatible input or border dimensions", panel);
    error->setObjectName("borderError"); error->setWordWrap(true);
    error->setProperty("severity", "error");
    layout->addWidget(error);
    refresh.emplace_back([weak, error] {
        if (const auto current = weak.lock()) error->setVisible(current->HasProcessingError());
    });
    for (const auto& update : refresh) update();
    auto* timer = new QTimer(panel);
    QObject::connect(timer, &QTimer::timeout, panel, [refresh = std::move(refresh)] {
        for (const auto& update : refresh) update();
    });
    timer->start(100);
    return panel;
}
