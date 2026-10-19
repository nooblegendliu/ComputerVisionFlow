#include "PerspectiveControls.h"
#include <PerspectiveWarp/perspective_warp.hpp>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <utility>

QWidget* CreatePerspectiveControls(std::shared_ptr<DSPatch::DSPatchables::PerspectiveWarp> node, QWidget* parent)
{
    using Node = DSPatch::DSPatchables::PerspectiveWarp;
    const std::weak_ptr<Node> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName("perspectiveControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* warning = new QLabel("No Quadrilateral Polygon Detected\nNeed Exactly 4 Points", panel);
    warning->setObjectName("polygonWarning");
    warning->setWordWrap(true);
    warning->setStyleSheet("color: #e85d5d;");
    layout->addWidget(warning);
    auto* settings = new QWidget(panel);
    settings->setObjectName("perspectiveSettings");
    auto* form = new QFormLayout(settings);
    form->setContentsMargins(0, 0, 0, 0);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addWidget(settings);
    auto* interpolation = new QComboBox(settings);
    interpolation->setObjectName("interp_mode");
    interpolation->addItems({"Nearest", "Bilinear", "BiCubic", "Area", "Lanczos"});
    form->addRow("Interpolation", interpolation);
    auto* fixed = new QCheckBox("Use Fixed Resolution", settings);
    fixed->setObjectName("use_fixed_res");
    form->addRow(fixed);
    auto* width = new QSpinBox(settings);
    width->setObjectName("fixed_width");
    width->setRange(2, 5000);
    form->addRow("Width", width);
    auto* height = new QSpinBox(settings);
    height->setObjectName("fixed_height");
    height->setRange(2, 5000);
    form->addRow("Height", height);
    auto* ratio = new QDoubleSpinBox(settings);
    ratio->setObjectName("aspect_ratio_adj");
    ratio->setRange(-10, 10);
    ratio->setDecimals(3);
    ratio->setSingleStep(.01);
    form->addRow("Aspect Ratio Adjust", ratio);
    const auto refresh = [weak, panel, warning, settings, form, interpolation, fixed, width, height, ratio] {
        const auto current = weak.lock();
        panel->setEnabled(static_cast<bool>(current));
        if (!current) return;
        const auto p = current->GetParameters();
        warning->setVisible(!current->HasQuadrilateral());
        settings->setVisible(current->HasQuadrilateral());
        const QSignalBlocker b1(interpolation), b2(fixed), b3(width), b4(height), b5(ratio);
        interpolation->setCurrentIndex(p.interp_mode);
        fixed->setChecked(p.use_fixed_res);
        for (const auto& item : {std::pair{width, p.fixed_width}, std::pair{height, p.fixed_height}}) {
            if (item.second < item.first->minimum()) item.first->setMinimum(item.second);
            if (item.second > item.first->maximum()) item.first->setMaximum(item.second);
            item.first->setValue(item.second);
        }
        if (p.aspect_ratio_adj < ratio->minimum()) ratio->setMinimum(p.aspect_ratio_adj);
        if (p.aspect_ratio_adj > ratio->maximum()) ratio->setMaximum(p.aspect_ratio_adj);
        ratio->setValue(p.aspect_ratio_adj);
        form->setRowVisible(width, p.use_fixed_res);
        form->setRowVisible(height, p.use_fixed_res);
        form->setRowVisible(ratio, !p.use_fixed_res);
    };
    QObject::connect(interpolation, &QComboBox::currentIndexChanged, panel, [weak](int value) {
        if (auto current = weak.lock()) current->SetState(nlohmann::json{{"interp_mode", value}}.dump());
    });
    QObject::connect(fixed, &QCheckBox::toggled, panel, [weak, refresh](bool value) {
        if (auto current = weak.lock()) current->SetState(nlohmann::json{{"use_fixed_res", value}}.dump());
        refresh();
    });
    for (const auto& item : {std::pair{width, "fixed_width"}, std::pair{height, "fixed_height"}}) {
        QObject::connect(item.first, &QSpinBox::valueChanged, panel, [weak, key = item.second](int value) {
            if (auto current = weak.lock()) current->SetState(nlohmann::json{{key, value}}.dump());
        });
    }
    QObject::connect(ratio, &QDoubleSpinBox::valueChanged, panel, [weak](double value) {
        if (auto current = weak.lock()) current->SetState(nlohmann::json{{"aspect_ratio_adj", value}}.dump());
    });
    auto* timer = new QTimer(panel);
    timer->setInterval(100);
    QObject::connect(timer, &QTimer::timeout, panel, refresh);
    refresh();
    timer->start();
    return panel;
}
