#include "ColorCorrectControls.h"
#include <Colorcorrect/ColorCorrect.hpp>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>
#include <vector>

QWidget* CreateColorCorrectControls(std::shared_ptr<DSPatch::DSPatchables::Colorcorrect> node, QWidget* parent)
{
    using Node = DSPatch::DSPatchables::Colorcorrect;
    using Json = nlohmann::json;
    const std::weak_ptr<Node> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName("colorCorrectControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* reset = new QPushButton("Reset", panel);
    reset->setObjectName("reset");
    layout->addWidget(reset);
    std::vector<std::function<void(const Json&)>> refresh;
    const auto updateSpin = [](QDoubleSpinBox* spin, double value) {
        const QSignalBlocker blocker(spin);
        if (value < spin->minimum()) spin->setMinimum(value);
        if (value > spin->maximum()) spin->setMaximum(value);
        spin->setValue(value);
    };
    for (const auto* name : {"low_color", "high_color"}) {
        const std::string key(name);
        auto* group = new QGroupBox(key == "low_color" ? "Low Range" : "High Range", panel);
        group->setObjectName(QString::fromStdString(key));
        auto* colors = new QVBoxLayout(group);
        auto* picker = new QPushButton("Choose Color…", group);
        picker->setObjectName(QString::fromStdString(key + "Picker"));
        colors->addWidget(picker);
        QObject::connect(picker, &QPushButton::clicked, panel, [weak, panel, key] {
            if (const auto current = weak.lock()) {
                const auto color = Json::parse(current->GetState()).at(key);
                auto* dialog = new QColorDialog(QColor::fromRgbF(
                    std::clamp(color.at("R").get<double>(), 0.0, 1.0),
                    std::clamp(color.at("G").get<double>(), 0.0, 1.0),
                    std::clamp(color.at("B").get<double>(), 0.0, 1.0)), panel);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &QColorDialog::colorSelected, panel, [weak, key](const QColor& picked) {
                    if (const auto target = weak.lock()) target->SetState(Json{{key,
                        {{"R", picked.redF()}, {"G", picked.greenF()}, {"B", picked.blueF()}}}}.dump());
                });
                dialog->open();
            }
        });
        for (const auto* channel : {"R", "G", "B"}) {
            auto* spin = new QDoubleSpinBox(group);
            spin->setObjectName(QString::fromStdString(key + "." + channel));
            spin->setPrefix(QString::fromLatin1(channel) + " ");
            spin->setRange(0, 1);
            spin->setDecimals(3);
            spin->setSingleStep(.01);
            colors->addWidget(spin);
            QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel, [weak, key, channel](double value) {
                if (const auto current = weak.lock()) {
                    auto color = Json::parse(current->GetState()).at(key);
                    color[channel] = float(value);
                    current->SetState(Json{{key, std::move(color)}}.dump());
                }
            });
            refresh.emplace_back([key, channel, spin, updateSpin](const Json& state) {
                updateSpin(spin, state.at(key).at(channel).get<double>());
            });
        }
        layout->addWidget(group);
    }
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    struct Field { const char* key; const char* label; double minimum; double maximum; };
    for (const auto& field : {Field{"saturation", "Saturation", 0, 4},
             Field{"brightness", "Brightness", -3, 3}, Field{"contrast", "Contrast", 0, 5},
             Field{"gamma", "Gamma", 0, 5}, Field{"gain", "Gain", 0, 5}}) {
        auto* spin = new QDoubleSpinBox(panel);
        spin->setObjectName(QString::fromLatin1(field.key));
        spin->setRange(field.minimum, field.maximum);
        spin->setDecimals(3);
        spin->setSingleStep(.01);
        form->addRow(QString::fromLatin1(field.label), spin);
        QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel, [weak, key = field.key](double value) {
            if (const auto current = weak.lock()) current->SetState(Json{{key, float(value)}}.dump());
        });
        refresh.emplace_back([key = field.key, spin, updateSpin](const Json& state) {
            updateSpin(spin, state.at(key).get<double>());
        });
    }
    auto* error = new QLabel("Unsupported color image or mask. Use 8-bit BGR/BGRA and a single-channel 8-bit mask.", panel);
    error->setObjectName("colorCorrectError");
    error->setWordWrap(true);
    error->setStyleSheet("color: #ef5350;");
    layout->addWidget(error);
    auto* timer = new QTimer(panel);
    const auto update = [weak, panel, error, timer, refresh = std::move(refresh)] {
        if (const auto current = weak.lock()) {
            const auto state = Json::parse(current->GetState());
            for (const auto& apply : refresh) apply(state);
            error->setVisible(current->HasProcessingError());
        }
        else {
            timer->stop();
            panel->setEnabled(false);
        }
    };
    QObject::connect(reset, &QPushButton::clicked, panel, [weak, update] {
        if (const auto current = weak.lock()) current->Reset();
        update();
    });
    QObject::connect(timer, &QTimer::timeout, panel, update);
    update();
    timer->start(100);
    return panel;
}
