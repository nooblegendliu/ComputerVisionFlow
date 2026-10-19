#include <QtCore/QVariant>
#include "LineIntersectControls.h"
#include <LineIntersect/line_intersect.hpp>
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
#include <functional>
#include <vector>

QWidget* CreateLineIntersectControls(std::shared_ptr<DSPatch::DSPatchables::LineIntersect> node, QWidget* parent)
{
    using Node = DSPatch::DSPatchables::LineIntersect;
    using Json = nlohmann::json;
    const std::weak_ptr<Node> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName("lineIntersectControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* status = new QLabel(panel); status->setObjectName("intersectionsStatus"); layout->addWidget(status);
    auto* body = new QWidget(panel); layout->addWidget(body);
    auto* form = new QFormLayout(body);
    std::vector<std::function<void(const Json&)>> refresh;
    for (const auto* key : {"min_angle", "min_dist"}) {
        auto* spin = new QDoubleSpinBox(body); spin->setObjectName(key);
        spin->setRange(0, std::string(key) == "min_angle" ? 360 : 1000);
        spin->setSingleStep(.1); spin->setDecimals(2);
        form->addRow(std::string(key) == "min_angle" ? "Min Angle" : "Min Dist", spin);
        QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel, [weak, key](double value) {
            if (const auto target = weak.lock()) target->SetState(Json{{key, float(value)}}.dump());
        });
        refresh.emplace_back([spin, key](const Json& state) {
            const QSignalBlocker blocker(spin); const double value = state.at(key).get<double>();
            spin->setRange(std::min(spin->minimum(), value), std::max(spin->maximum(), value)); spin->setValue(value);
        });
    }
    auto* draw = new QCheckBox("Draw Intersection Points", body); draw->setObjectName("draw_points"); form->addRow(draw);
    auto* solid = new QCheckBox("Solid Point", body); solid->setObjectName("point_solid"); form->addRow(solid);
    for (auto* check : {draw, solid}) {
        const auto key = check->objectName().toStdString();
        QObject::connect(check, &QCheckBox::toggled, panel, [weak, key](bool value) {
            if (const auto target = weak.lock()) target->SetState(Json{{key, value}}.dump());
        });
        refresh.emplace_back([check, key](const Json& state) { const QSignalBlocker blocker(check); check->setChecked(state.at(key).get<bool>()); });
    }
    auto* picker = new QPushButton("Choose Point Color…", body); picker->setObjectName("pointColorPicker"); form->addRow(picker);
    QObject::connect(picker, &QPushButton::clicked, panel, [weak, panel] {
        if (const auto target = weak.lock()) {
            const auto color = Json::parse(target->GetState()).at("point_color");
            auto* dialog = new QColorDialog(QColor::fromRgbF(std::clamp(color.at("R").get<double>(), 0., 1.),
                std::clamp(color.at("G").get<double>(), 0., 1.), std::clamp(color.at("B").get<double>(), 0., 1.)), panel);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(dialog, &QColorDialog::colorSelected, panel, [weak](const QColor& color) {
                if (const auto target = weak.lock()) target->SetState(Json{{"point_color",
                    {{"R", color.redF()}, {"G", color.greenF()}, {"B", color.blueF()}}}}.dump());
            });
            dialog->open();
        }
    });
    std::vector<QWidget*> drawing{solid, picker};
    for (const auto* channel : {"R", "G", "B"}) {
        auto* spin = new QDoubleSpinBox(body); spin->setObjectName(QString("point_color.") + channel);
        spin->setRange(0, 1); spin->setDecimals(3); spin->setSingleStep(.01); form->addRow(QString("Point ") + channel, spin);
        drawing.push_back(spin);
        QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel, [weak, channel](double value) {
            if (const auto target = weak.lock()) {
                auto color = Json::parse(target->GetState()).at("point_color"); color[channel] = float(value);
                target->SetState(Json{{"point_color", color}}.dump());
            }
        });
        refresh.emplace_back([spin, channel](const Json& state) {
            const QSignalBlocker blocker(spin); const double value = state.at("point_color").at(channel).get<double>();
            spin->setRange(std::min(spin->minimum(), value), std::max(spin->maximum(), value)); spin->setValue(value);
        });
    }
    QSpinBox* thickness = nullptr;
    for (const auto* key : {"point_radius", "point_thickness"}) {
        auto* spin = new QSpinBox(body); spin->setObjectName(key); spin->setRange(1, 25);
        form->addRow(std::string(key) == "point_radius" ? "Point Radius" : "Point Thickness", spin);
        if (std::string(key) == "point_radius") drawing.push_back(spin); else thickness = spin;
        QObject::connect(spin, &QSpinBox::valueChanged, panel, [weak, key](int value) {
            if (const auto target = weak.lock()) target->SetState(Json{{key, value}}.dump());
        });
        refresh.emplace_back([spin, key](const Json& state) {
            const QSignalBlocker blocker(spin); const int value = state.at(key).get<int>();
            spin->setRange(std::min(spin->minimum(), value), std::max(spin->maximum(), value)); spin->setValue(value);
        });
    }
    auto* error = new QLabel("Invalid lines or drawing input.", panel); error->setObjectName("lineIntersectError");
    error->setProperty("severity", "error"); layout->addWidget(error);
    auto* timer = new QTimer(panel);
    const auto update = [weak, panel, body, form, status, error, timer, thickness, drawing, refresh] {
        if (const auto target = weak.lock()) {
            const auto state = Json::parse(target->GetState());
            for (const auto& apply : refresh) apply(state);
            const bool tooMany = target->LineCount() > 200;
            status->setText(tooMany ? "Too Many Input Lines" : QString("Intersections: %1").arg(target->IntersectionCount()));
            status->setStyleSheet(tooMany ? "color: #ef5350;" : ""); body->setVisible(!tooMany);
            for (auto* widget : drawing) form->setRowVisible(widget, state.at("draw_points").get<bool>());
            form->setRowVisible(thickness, state.at("draw_points").get<bool>() && !state.at("point_solid").get<bool>());
            error->setVisible(target->HasProcessingError());
        }
        else { timer->stop(); panel->setEnabled(false); }
    };
    QObject::connect(timer, &QTimer::timeout, panel, update); update(); timer->start(100);
    return panel;
}
