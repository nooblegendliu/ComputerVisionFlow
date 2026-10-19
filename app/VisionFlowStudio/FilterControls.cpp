#include "FilterControls.h"
#include <Threshold/threshold.hpp>
#include <Morphology/Morphology.hpp>
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
#include <limits>
#include <vector>

namespace {
using Refresh = std::vector<std::function<void()>>;

// A normal box layout excludes hidden pages from every size calculation,
// including minimumHeightForWidth used by the property scroll area's layout.
class MethodPages final : public QWidget
{
public:
    explicit MethodPages(QWidget* parent) : QWidget(parent)
    {
        auto* pagesLayout = new QVBoxLayout(this);
        pagesLayout->setContentsMargins(0, 0, 0, 0);
        pagesLayout->setSpacing(0);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
    void addWidget(QWidget* page)
    {
        pages_.push_back(page);
        layout()->addWidget(page);
        page->setVisible(pages_.size() == 1);
    }
    void setCurrentIndex(int index)
    {
        if (index < 0 || index >= static_cast<int>(pages_.size()) || index == currentIndex_) return;
        currentIndex_ = index;
        for (int i = 0; i < static_cast<int>(pages_.size()); ++i) pages_[i]->setVisible(i == index);
        updateGeometry();
    }
private:
    std::vector<QWidget*> pages_;
    int currentIndex_ = 0;
};

template<class Node>
QComboBox* AddOption(QFormLayout* layout, QWidget* panel, const std::shared_ptr<Node>& node,
    const char* key, const char* label, Refresh& refresh)
{
    auto* combo = new QComboBox(panel);
    combo->setObjectName(QString::fromLatin1(key));
    for (const auto& name : node->Properties().GetOptions(key)) combo->addItem(QString::fromStdString(name));
    const std::weak_ptr<Node> weak = node;
    const std::string property = key;
    QObject::connect(combo, &QComboBox::currentIndexChanged, panel, [weak, property](int index) {
        if (const auto current = weak.lock()) current->Properties().Set(property, index);
    });
    refresh.emplace_back([weak, property, combo] {
        if (const auto current = weak.lock()) {
            const QSignalBlocker blocker(combo);
            combo->setCurrentIndex(current->Properties().template GetW<int>(property));
        }
    });
    layout->addRow(QString::fromLatin1(label), combo);
    return combo;
}

template<class Node>
void AddInteger(QFormLayout* layout, QWidget* panel, const std::shared_ptr<Node>& node,
    const char* key, const char* label, int minimum, int maximum, Refresh& refresh, bool odd = false)
{
    auto* spin = new QSpinBox(panel);
    spin->setObjectName(QString::fromLatin1(key));
    spin->setRange(minimum, maximum);
    spin->setSingleStep(odd ? 2 : 1);
    const std::weak_ptr<Node> weak = node;
    const std::string property = key;
    QObject::connect(spin, &QSpinBox::valueChanged, panel, [weak, property, spin, odd](int value) {
        if (odd && value % 2 == 0) {
            ++value;
            const QSignalBlocker blocker(spin);
            spin->setValue(value);
        }
        if (const auto current = weak.lock()) current->Properties().Set(property, value);
    });
    refresh.emplace_back([weak, property, spin] {
        if (const auto current = weak.lock()) {
            const QSignalBlocker blocker(spin);
            const int value = current->Properties().template GetW<int>(property);
            // Saved valid values beyond the original drag range are displayed faithfully.
            if (value < spin->minimum()) spin->setMinimum(value);
            if (value > spin->maximum()) spin->setMaximum(value);
            spin->setValue(value);
        }
    });
    layout->addRow(QString::fromLatin1(label), spin);
}

template<class Node>
void AddColor(QFormLayout* layout, QWidget* panel, const std::shared_ptr<Node>& node,
    const char* group, const char* label, Refresh& refresh)
{
    auto* row = new QWidget(panel);
    auto* colorLayout = new QVBoxLayout(row);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    const std::weak_ptr<Node> weak = node;
    const std::string property = group;
    auto* picker = new QPushButton("Color…", row);
    picker->setObjectName(QString::fromLatin1(group) + "Picker");
    colorLayout->addWidget(picker);
    QObject::connect(picker, &QPushButton::clicked, panel, [weak, property, panel] {
        const auto current = weak.lock();
        if (!current) return;
        const auto state = nlohmann::json::parse(current->GetState()).at(property);
        const auto color = QColor::fromRgbF(state.at("R").get<float>(), state.at("G").get<float>(), state.at("B").get<float>());
        auto* dialog = new QColorDialog(color, panel);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(dialog, &QColorDialog::colorSelected, panel, [weak, property](const QColor& picked) {
            if (const auto target = weak.lock())
                target->SetState(nlohmann::json{{property, {{"R", picked.redF()}, {"G", picked.greenF()}, {"B", picked.blueF()}}}}.dump());
        });
        dialog->open();
    });
    for (const auto* channel : {"R", "G", "B"}) {
        const auto key = property + "." + channel;
        auto* spin = new QDoubleSpinBox(row);
        spin->setObjectName(QString::fromStdString(key));
        spin->setPrefix(QString::fromLatin1(channel) + " ");
        spin->setRange(0, 1);
        spin->setDecimals(3);
        spin->setSingleStep(.01);
        colorLayout->addWidget(spin);
        QObject::connect(spin, &QDoubleSpinBox::valueChanged, panel, [weak, key](double value) {
            if (const auto current = weak.lock()) current->Properties().Set(key, static_cast<float>(value));
        });
        refresh.emplace_back([weak, key, spin] {
            if (const auto current = weak.lock()) {
                const QSignalBlocker blocker(spin);
                spin->setValue(current->Properties().template GetW<float>(key));
            }
        });
    }
    layout->addRow(QString::fromLatin1(label), row);
}

template<class Node>
void Finish(QWidget* panel, QVBoxLayout* layout, const std::shared_ptr<Node>& node, Refresh refresh)
{
    auto* error = new QLabel("Incompatible input or parameters", panel);
    error->setWordWrap(true);
    error->setObjectName("filterError");
    error->setStyleSheet("color: #ef5350;");
    layout->addWidget(error);
    const std::weak_ptr<Node> weak = node;
    refresh.emplace_back([weak, error] {
        if (const auto current = weak.lock()) error->setVisible(current->HasProcessingError());
    });
    for (const auto& update : refresh) update();
    auto* timer = new QTimer(panel);
    QObject::connect(timer, &QTimer::timeout, panel, [refresh = std::move(refresh)] {
        for (const auto& update : refresh) update();
    });
    timer->start(100);
}
}

QWidget* CreateThresholdControls(std::shared_ptr<DSPatch::DSPatchables::Threshold> node, QWidget* parent)
{
    auto* panel = new QWidget(parent);
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    Refresh refresh;
    auto* method = AddOption(form, panel, node, "thresh_method", "Method", refresh);
    AddOption(form, panel, node, "thresh_type", "Type", refresh);
    auto* pages = new MethodPages(panel);
    pages->setObjectName("thresholdMethodPages");
    for (int i = 0; i < 4; ++i) {
        auto* page = new QWidget(pages);
        auto* content = new QFormLayout(page);
        content->setRowWrapPolicy(QFormLayout::WrapLongRows);
        if (i == 0) AddInteger(content, page, node, "thresh_amt", "Thresh", 1, 255, refresh);
        if (i == 1) {
            AddColor(content, page, node, "low_range", "Low", refresh);
            AddColor(content, page, node, "high_range", "High", refresh);
        }
        if (i == 2) {
            AddOption(content, page, node, "adapt_type", "Adapt Type", refresh);
            AddInteger(content, page, node, "adapt_thresh", "Adapt Thresh", 0, 255, refresh);
            AddInteger(content, page, node, "adapt_block", "Block Size", 3, (std::numeric_limits<int>::max)(), refresh, true);
        }
        pages->addWidget(page);
    }
    QObject::connect(method, &QComboBox::currentIndexChanged, pages, &MethodPages::setCurrentIndex);
    const std::weak_ptr<DSPatch::DSPatchables::Threshold> weak = node;
    refresh.emplace_back([weak, pages] {
        if (const auto current = weak.lock()) pages->setCurrentIndex(current->Properties().GetW<int>("thresh_method"));
    });
    layout->addWidget(pages);
    Finish(panel, layout, node, std::move(refresh));
    return panel;
}

QWidget* CreateMorphologyControls(std::shared_ptr<DSPatch::DSPatchables::Morphology> node, QWidget* parent)
{
    auto* panel = new QWidget(parent);
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    Refresh refresh;
    AddOption(form, panel, node, "operation", "Operation", refresh);
    AddOption(form, panel, node, "shape", "Morph Shape", refresh);
    AddInteger(form, panel, node, "amt", "Morph Amt", 1, 200, refresh);
    AddInteger(form, panel, node, "iter", "Iter", 1, 50, refresh);
    AddOption(form, panel, node, "border_type", "Border Type", refresh);
    AddColor(form, panel, node, "border_color", "Border Color", refresh);
    Finish(panel, layout, node, std::move(refresh));
    return panel;
}
