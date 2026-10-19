#include "DerivativeControls.h"
#include <SobelFilter/sobel_filter.hpp>
#include <LaplacianFilter/laplacian_filter.hpp>
#include <Canny/canny.hpp>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <map>
#include <string>
#include <type_traits>
#include <utility>

namespace {
template<class Node>
QWidget* CreateControls(std::shared_ptr<Node> node, QWidget* parent,
    const char* panelName, const char* errorName)
{
    if (!node) return nullptr;
    const std::weak_ptr<Node> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName(QString::fromLatin1(panelName));
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    std::map<std::string, QWidget*> editors;
    const auto initial = node->GetPropertiesSnapshot();
    // Node metadata already follows the upstream Controls order.
    for (const auto& property : initial.properties) {
        QWidget* editor = nullptr;
        if (property.type == FlowCV::PropertyDataTypes::kDataTypeOption) {
            auto* combo = new QComboBox(panel);
            for (const auto& option : property.options)
                combo->addItem(QString::fromStdString(option));
            editor = combo;
        } else if (property.key == "ksize" || property.key == "kernel_size") {
            auto* spin = new QSpinBox(panel);
            // Upstream uses InputInt(..., 2, 2). A step of 1 followed by the
            // odd-kernel correction would prevent decrementing an odd value.
            spin->setSingleStep(2);
            editor = spin;
        } else if (property.type == FlowCV::PropertyDataTypes::kDataTypeFloat) {
            auto* spin = new QDoubleSpinBox(panel);
            spin->setDecimals(6);
            editor = spin;
        }
        if (!editor) continue;
        editor->setObjectName(QString::fromStdString(property.key));
        form->addRow(QString::fromStdString(property.desc), editor);
        editors.emplace(property.key, editor);
    }
    auto* error = new QLabel("Cannot process this image with the selected depth or kernel size.", panel);
    error->setObjectName(QString::fromLatin1(errorName));
    error->setWordWrap(true);
    error->setStyleSheet("color: #ef5350;");
    layout->addWidget(error);
    auto* timer = new QTimer(panel);
    const auto update = [weak, panel, form, error, timer, editors] {
        const auto current = weak.lock();
        if (!current) {
            timer->stop();
            panel->setEnabled(false);
            return;
        }
        const auto snapshot = current->GetPropertiesSnapshot();
        for (const auto& property : snapshot.properties) {
            const auto found = editors.find(property.key);
            if (found == editors.end()) continue;
            QWidget* editor = found->second;
            const QSignalBlocker blocker(editor);
            if (auto* combo = qobject_cast<QComboBox*>(editor)) {
                combo->setCurrentIndex(std::get<int>(property.value));
            } else if (auto* spin = qobject_cast<QSpinBox*>(editor)) {
                const auto& range = std::get<FlowCV::IntRange>(property.range);
                // SetState extends this range for saved kernels such as 9/-1.
                // Interactive edits restore the original 1..7 range in the node.
                spin->setRange(range.min, range.max);
                spin->setValue(std::get<int>(property.value));
            } else if (auto* spin = qobject_cast<QDoubleSpinBox*>(editor)) {
                const auto& range = std::get<FlowCV::FloatRange>(property.range);
                spin->setRange(range.min, range.max);
                spin->setSingleStep(range.step);
                spin->setValue(std::get<float>(property.value));
            }
            editor->setVisible(property.visible);
            if (auto* label = form->labelForField(editor)) label->setVisible(property.visible);
        }
        if constexpr (std::is_same_v<Node, DSPatch::DSPatchables::CannyFilter>)
            error->hide();
        else
            error->setVisible(current->HasProcessingError());
    };
    for (const auto& [key, editor] : editors) {
        if (auto* combo = qobject_cast<QComboBox*>(editor)) {
            QObject::connect(combo, &QComboBox::currentIndexChanged, panel, [weak, key, update](int value) {
                if (value < 0) return;
                if (const auto current = weak.lock()) current->Properties().Set(key, value);
                update();
            });
        } else if (auto* spin = qobject_cast<QSpinBox*>(editor)) {
            QObject::connect(spin, &QSpinBox::valueChanged, panel, [weak, key, update](int value) {
                if (const auto current = weak.lock()) current->Properties().Set(key, value);
                update();
            });
        } else if (auto* spin = qobject_cast<QDoubleSpinBox*>(editor)) {
            QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel, [weak, key, update](double value) {
                if (const auto current = weak.lock()) current->Properties().Set(key, static_cast<float>(value));
                update();
            });
        }
    }
    QObject::connect(timer, &QTimer::timeout, panel, update);
    update();
    timer->start(100);
    return panel;
}
}

QWidget* CreateSobelControls(std::shared_ptr<DSPatch::DSPatchables::SobelFilter> node, QWidget* parent)
{
    return CreateControls(std::move(node), parent, "sobelControls", "sobelError");
}

QWidget* CreateLaplacianControls(std::shared_ptr<DSPatch::DSPatchables::LaplacianFilter> node, QWidget* parent)
{
    return CreateControls(std::move(node), parent, "laplacianControls", "laplacianError");
}

QWidget* CreateCannyControls(std::shared_ptr<DSPatch::DSPatchables::CannyFilter> node, QWidget* parent)
{
    return CreateControls(std::move(node), parent, "cannyControls", "cannyError");
}
