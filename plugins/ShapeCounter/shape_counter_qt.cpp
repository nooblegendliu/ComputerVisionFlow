// Qt Controls adaptation; the Shape_Counter computation remains Qt-free.
#include "shape_counter.hpp"
#include <NodeUi/NodeUiProvider.h>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <algorithm>
#include <functional>
#include <limits>
#include <vector>
namespace DSPatch::DSPatchables {
class ShapeCounterQt final : public ShapeCounter, public VisionFlow::NodeUiProvider {
public:
    bool hasNodeUi(VisionFlow::NodeUiRole role) const override
    { return role == VisionFlow::NodeUiRole::Controls; }
    QWidget* createNodeUi(VisionFlow::NodeUiRole role, QWidget* parent) override
    {
        if (!hasNodeUi(role)) return nullptr;
        auto* panel = new QWidget(parent);
        panel->setObjectName("ShapeCounterControls");
        auto* form = new QFormLayout(panel);
        const std::weak_ptr<internal::ShapeCounterState> weak = state_;
        using Params = internal::ShapeCounterParams;
        std::vector<std::function<void(const Params&)>> updates;
        const auto number = [&](const char* label, const char* name, int lo, int hi,
            std::function<int(const Params&)> get, std::function<void(Params&, int)> set) {
            auto* spin = new QSpinBox(panel); spin->setObjectName(name); spin->setRange(lo, hi);
            form->addRow(label, spin);
            QObject::connect(spin, &QSpinBox::valueChanged, panel, [weak, set](int v) {
                if (auto s = weak.lock()) { std::lock_guard lock(s->mutex); set(s->params, v); }
            });
            updates.emplace_back([spin, get, lo, hi](const Params& p) {
                QSignalBlocker block(spin);
                const int value = get(p);
                spin->setRange((std::min)(lo, value), (std::max)(hi, value));
                spin->setValue(value);
            });
            return spin;
        };
        const auto combo = [&](const char* label, const char* name, const QStringList& options, int Params::* member) {
            auto* box = new QComboBox(panel); box->setObjectName(name); box->addItems(options);
            form->addRow(label, box);
            QObject::connect(box, &QComboBox::currentIndexChanged, panel, [weak, member](int v) {
                if (auto s = weak.lock()) { std::lock_guard lock(s->mutex); s->params.*member = v; }
            });
            updates.emplace_back([box, member](const Params& p) { QSignalBlocker block(box); box->setCurrentIndex(p.*member); });
            return box;
        };
        combo("Font", "ShapeCounterFont", {"Simplex", "Plain", "Duplex", "Complex", "Triplex", "Complex Small", "Script Simplex", "Script Complex"}, &Params::font);
        number("X", "ShapeCounterX", 0, (std::numeric_limits<int>::max)(), [](const Params& p) { return p.position.x; }, [](Params& p, int v) { p.position.x = v; });
        number("Y", "ShapeCounterY", 0, (std::numeric_limits<int>::max)(), [](const Params& p) { return p.position.y; }, [](Params& p, int v) { p.position.y = v; });
        auto* scale = new QDoubleSpinBox(panel); scale->setObjectName("ShapeCounterScale");
        scale->setRange(.1, 100); scale->setSingleStep(.01); scale->setDecimals(3); form->addRow("Scale", scale);
        QObject::connect(scale, &QDoubleSpinBox::valueChanged, panel, [weak](double v) {
            if (auto s = weak.lock()) { std::lock_guard lock(s->mutex); s->params.scale = static_cast<float>(v); }
        });
        updates.emplace_back([scale](const Params& p) {
            QSignalBlocker block(scale);
            scale->setRange((std::min)(.1, static_cast<double>(p.scale)), (std::max)(100., static_cast<double>(p.scale)));
            scale->setValue(p.scale);
        });
        number("Thickness", "ShapeCounterThickness", 1, 100, [](const Params& p) { return p.thickness; }, [](Params& p, int v) { p.thickness = v; });
        auto* color = new QPushButton("Choose RGBA", panel); color->setObjectName("ShapeCounterColor"); form->addRow("Color", color);
        QObject::connect(color, &QPushButton::clicked, panel, [weak, panel] {
            const auto s = weak.lock(); if (!s) return;
            const auto p = s->Snapshot();
            auto* dialog = new QColorDialog(QColor::fromRgbF(std::clamp(p.color[0], 0.f, 1.f), std::clamp(p.color[1], 0.f, 1.f), std::clamp(p.color[2], 0.f, 1.f), std::clamp(p.color[3], 0.f, 1.f)), panel);
            dialog->setAttribute(Qt::WA_DeleteOnClose); dialog->setOption(QColorDialog::ShowAlphaChannel);
            QObject::connect(dialog, &QColorDialog::colorSelected, panel, [weak](const QColor& c) {
                if (auto current = weak.lock()) { std::lock_guard lock(current->mutex);
                    current->params.color = cv::Vec4f(c.redF(), c.greenF(), c.blueF(), c.alphaF()); }
            });
            dialog->open();
        });
        updates.emplace_back([color](const Params& p) {
            color->setText(QString("RGBA %1, %2, %3, %4").arg(p.color[0], 0, 'f', 2).arg(p.color[1], 0, 'f', 2).arg(p.color[2], 0, 'f', 2).arg(p.color[3], 0, 'f', 2));
            color->setToolTip(QString("RGBA %1, %2, %3, %4").arg(p.color[0], 0, 'g', 9).arg(p.color[1], 0, 'g', 9).arg(p.color[2], 0, 'g', 9).arg(p.color[3], 0, 'g', 9));
        });
        auto* mode = combo("Counting Mode", "ShapeCounterMode", {"By Size", "By ID"}, &Params::count_mode);
        auto* overlay = new QCheckBox(panel); overlay->setObjectName("ShapeCounterOverlay"); form->addRow("Show Overlay", overlay);
        QObject::connect(overlay, &QCheckBox::toggled, panel, [weak](bool v) { if (auto s = weak.lock()) { std::lock_guard lock(s->mutex); s->params.show_overlay = v; } });
        updates.emplace_back([overlay](const Params& p) { QSignalBlocker block(overlay); overlay->setChecked(p.show_overlay); });
        auto* match = number("Size Match", "ShapeCounterSize", 1, 1000, [](const Params& p) { return p.size_match; }, [](Params& p, int v) { p.size_match = v; });
        auto* threshold = number("Size Threshold", "ShapeCounterThreshold", 0, 1000, [](const Params& p) { return p.size_thresh; }, [](Params& p, int v) { p.size_thresh = v; });
        const auto visibility = [form, match, threshold](int index) { form->setRowVisible(match, index == 0); form->setRowVisible(threshold, index == 0); };
        QObject::connect(mode, &QComboBox::currentIndexChanged, panel, visibility);
        const auto refresh = [weak, updates, visibility] {
            if (auto s = weak.lock()) { const auto p = s->Snapshot(); for (const auto& update : updates) update(p); visibility(p.count_mode); }
        };
        auto* timer = new QTimer(panel); QObject::connect(timer, &QTimer::timeout, panel, refresh); timer->start(100); refresh();
        return panel;
    }
};
EXPORT_PLUGIN(ShapeCounterQt)
}
