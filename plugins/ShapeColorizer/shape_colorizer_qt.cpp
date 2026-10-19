// Qt Controls adaptation of FlowCV ShapeColorizer; computation is Qt-free.
#include "shape_colorizer.hpp"
#include <algorithm>
#include <NodeUi/NodeUiProvider.h>
#include <QTimer>
#include <QSignalBlocker>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace DSPatch::DSPatchables {
class ShapeColorizerQt final : public ShapeColorizer, public VisionFlow::NodeUiProvider {
public:
    bool hasNodeUi(VisionFlow::NodeUiRole role) const override
    { return role == VisionFlow::NodeUiRole::Controls; }
    QWidget* createNodeUi(VisionFlow::NodeUiRole role, QWidget* parent) override
    {
        if (!hasNodeUi(role)) return nullptr;
        auto* panel = new QWidget(parent);
        panel->setObjectName("ShapeColorizerControls");
        auto* layout = new QVBoxLayout(panel);
        auto* form = new QFormLayout;
        layout->addLayout(form);
        auto* method = new QComboBox(panel);
        method->setObjectName("ShapeColorizerMethod");
        method->addItems({"Mean", "Max", "Closest"});
        form->addRow("Color Method", method);
        auto* overlay = new QCheckBox("Overlay", panel);
        auto* labels = new QCheckBox("Show Labels", panel);
        auto* ignore = new QCheckBox("Ignore None", panel);
        overlay->setObjectName("ShapeColorizerOverlay");
        labels->setObjectName("ShapeColorizerShowLabels");
        ignore->setObjectName("ShapeColorizerIgnoreNone");
        form->addRow(overlay); form->addRow(labels); form->addRow(ignore);
        auto* closest = new QWidget(panel);
        closest->setObjectName("ShapeColorizerClosest");
        layout->addWidget(closest);
        auto* closestLayout = new QVBoxLayout(closest);
        auto* matchForm = new QFormLayout;
        closestLayout->addLayout(matchForm);
        auto* distance = new QComboBox(closest);
        distance->setObjectName("ShapeColorizerDistance");
        distance->addItems({"RGB", "CIE76"});
        auto* threshold = new QSpinBox(closest);
        threshold->setObjectName("ShapeColorizerThreshold");
        threshold->setRange(1, 255);
        matchForm->addRow("Distance Method", distance);
        matchForm->addRow("Match Threshold", threshold);
        closestLayout->addWidget(new QLabel("Color Match List:", closest));
        auto* buttons = new QHBoxLayout;
        auto* add = new QPushButton("Add", closest);
        auto* clear = new QPushButton("Clear All", closest);
        add->setObjectName("ShapeColorizerAdd"); clear->setObjectName("ShapeColorizerClear");
        buttons->addWidget(add); buttons->addWidget(clear);
        closestLayout->addLayout(buttons);
        auto* rows = new QVBoxLayout;
        closestLayout->addLayout(rows);
        auto* error = new QLabel(panel);
        error->setObjectName("ShapeColorizerError");
        error->setProperty("severity", "error");
        error->setWordWrap(true); error->setTextFormat(Qt::PlainText);
        layout->addWidget(error);
        const std::weak_ptr<internal::ShapeColorizerState> weak = state_;
        const auto patch = [weak](const char* key, auto value) {
            if (const auto state = weak.lock()) state->Apply({{key, value}});
        };
        QObject::connect(method, &QComboBox::currentIndexChanged, panel,
            [patch](int value) { patch("method", value); });
        QObject::connect(distance, &QComboBox::currentIndexChanged, panel,
            [patch](int value) { patch("dist_method", value); });
        QObject::connect(threshold, &QSpinBox::valueChanged, panel,
            [patch](int value) { patch("threshold", value); });
        QObject::connect(overlay, &QCheckBox::toggled, panel,
            [patch](bool value) { patch("overlay", value); });
        QObject::connect(labels, &QCheckBox::toggled, panel,
            [patch](bool value) { patch("show_labels", value); });
        QObject::connect(ignore, &QCheckBox::toggled, panel,
            [patch](bool value) { patch("ignore_none", value); });
        QObject::connect(add, &QPushButton::clicked, panel, [weak] {
            if (const auto state = weak.lock()) {
                std::lock_guard lock(state->mutex);
                state->parameters.colors.emplace_back();
                ++state->parameters.listRevision;
            }
        });
        QObject::connect(clear, &QPushButton::clicked, panel,
            [patch] { patch("color_match", nlohmann::json::array()); });
        auto lastList = std::make_shared<std::string>();
        auto lastRevision = std::make_shared<uint64_t>(0);
        auto refresh = [=] {
            const auto state = weak.lock();
            if (!state) { panel->setEnabled(false); return; }
            const auto p = state->Snapshot();
            { const QSignalBlocker a(method), b(distance), c(threshold), d(overlay), e(labels), f(ignore);
                method->setCurrentIndex(p.method); distance->setCurrentIndex(p.dist_method);
                threshold->setRange(std::min(1, p.threshold), std::max(255, p.threshold));
                threshold->setValue(p.threshold); overlay->setChecked(p.overlay);
                labels->setChecked(p.show_labels); ignore->setChecked(p.ignore_none);
            }
            closest->setVisible(p.method == 2);
            { std::lock_guard lock(state->mutex); error->setText(QString::fromUtf8(state->error.c_str())); }
            const auto serialized = state->Serialize().value("color_match", nlohmann::json::array()).dump();
            if (*lastList == serialized && *lastRevision == p.listRevision) return;
            *lastList = serialized;
            *lastRevision = p.listRevision;
            while (auto* item = rows->takeAt(0)) {
                auto* oldRow = item->widget();
                // Destruction can emit editingFinished on a focused editor.
                // Retired row callbacks must not write into the refreshed list.
                for (auto* sender : oldRow->findChildren<QObject*>())
                    QObject::disconnect(sender, nullptr, oldRow, nullptr);
                delete oldRow; delete item;
            }
            for (size_t i = 0; i < p.colors.size(); ++i) {
                auto* row = new QWidget(closest);
                auto* rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                auto* remove = new QPushButton("X", row);
                remove->setObjectName(QString("ShapeColorizerRemove%1").arg(i));
                auto* color = new QPushButton("Color", row);
                color->setObjectName(QString("ShapeColorizerColor%1").arg(i));
                const auto rgb = p.colors[i].color;
                const auto current = QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);
                color->setStyleSheet(QString("background-color: %1; color: %2;")
                    .arg(current.name(), current.lightnessF() > .5 ? "black" : "white"));
                auto* label = new QLineEdit(QString::fromUtf8(p.colors[i].label.c_str()), row);
                label->setObjectName(QString("ShapeColorizerLabel%1").arg(i));
                label->setPlaceholderText("Label");
                rowLayout->addWidget(remove); rowLayout->addWidget(color); rowLayout->addWidget(label);
                rows->addWidget(row);
                const auto revision = p.listRevision;
                QObject::connect(remove, &QPushButton::clicked, row, [weak, i, revision] {
                    if (const auto state = weak.lock()) {
                        std::lock_guard lock(state->mutex);
                        if (revision == state->parameters.listRevision && i < state->parameters.colors.size()) {
                            state->parameters.colors.erase(state->parameters.colors.begin() + i);
                            ++state->parameters.listRevision;
                        }
                    }
                });
                QObject::connect(label, &QLineEdit::editingFinished, row, [weak, i, label, revision] {
                    if (const auto state = weak.lock()) {
                        std::lock_guard lock(state->mutex);
                        if (revision == state->parameters.listRevision && i < state->parameters.colors.size())
                            state->parameters.colors[i].label = label->text().toUtf8().toStdString();
                    }
                });
                QObject::connect(color, &QPushButton::clicked, row, [weak, i, current, row, revision] {
                    auto* dialog = new QColorDialog(current, row);
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    QObject::connect(dialog, &QColorDialog::colorSelected, row, [weak, i, revision](const QColor& value) {
                        if (const auto state = weak.lock()) {
                            std::lock_guard lock(state->mutex);
                            if (revision == state->parameters.listRevision && i < state->parameters.colors.size())
                                state->parameters.colors[i].color = {float(value.redF()), float(value.greenF()), float(value.blueF())};
                        }
                    });
                    dialog->open();
                });
            }
        };
        auto* timer = new QTimer(panel);
        QObject::connect(timer, &QTimer::timeout, panel, refresh);
        timer->start(100); refresh();
        return panel;
    }
};
EXPORT_PLUGIN(ShapeColorizerQt)
}
