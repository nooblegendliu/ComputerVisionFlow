#include <QtCore/QVariant>
#include "DrawTextControls.h"
#include <DrawTools/draw_text.hpp>
#include <opencv2/imgproc.hpp>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>

QWidget* CreateDrawTextControls(std::shared_ptr<DSPatch::DSPatchables::DrawText> node, QWidget* parent)
{
    if (!node) return nullptr;
    const std::weak_ptr<DSPatch::DSPatchables::DrawText> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName("drawTextControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    auto* text = new QLineEdit(panel); text->setObjectName("text");
    text->setMaxLength(std::numeric_limits<int>::max());
    form->addRow("String", text);
    auto* font = new QComboBox(panel); font->setObjectName("font");
    for (const auto* name : {"Simplex", "Plain", "Duplex", "Complex", "Triplex", "Complex Small", "Script Simplex", "Script Complex"})
        font->addItem(name, font->count());
    form->addRow("Font", font);
    auto* x = new QSpinBox(panel); x->setObjectName("position.x");
    auto* y = new QSpinBox(panel); y->setObjectName("position.y");
    auto* thickness = new QSpinBox(panel); thickness->setObjectName("thickness");
    for (auto* spin : {x, y, thickness}) spin->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    form->addRow("X", x); form->addRow("Y", y);
    auto* scale = new QDoubleSpinBox(panel); scale->setObjectName("scale");
    scale->setDecimals(7); scale->setRange(-std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    scale->setSingleStep(.1);
    form->addRow("Scale", scale); form->addRow("Thickness", thickness);
    auto* color = new QPushButton(panel); color->setObjectName("color"); form->addRow("Color", color);
    auto* error = new QLabel("Invalid image format or text drawing settings.", panel);
    error->setObjectName("drawTextError"); error->setWordWrap(true); error->setProperty("severity", "error");
    layout->addWidget(error);
    auto* timer = new QTimer(panel);
    const auto displayColor = [](const cv::Vec3f& rgb) {
        return QColor::fromRgbF(std::clamp(rgb[0], 0.f, 1.f), std::clamp(rgb[1], 0.f, 1.f), std::clamp(rgb[2], 0.f, 1.f));
    };
    const auto update = [weak, panel, form, text, font, x, y, scale, thickness, color, error, timer, displayColor] {
        const auto current = weak.lock();
        if (!current) { timer->stop(); panel->setEnabled(false); return; }
        const auto settings = current->GetSettings();
        const QSignalBlocker textBlock(text), fontBlock(font), xBlock(x), yBlock(y), scaleBlock(scale), thicknessBlock(thickness);
        if (!text->hasFocus()) text->setText(QString::fromStdString(settings.text));
        form->setRowVisible(text, !settings.has_string_input);
        if (font->findData(settings.font) < 0)
            font->addItem(QString("%1%2 [%3]").arg(font->itemText(settings.font & 15),
                settings.font & cv::FONT_ITALIC ? " (Italic)" : "").arg(settings.font), settings.font);
        font->setCurrentIndex(font->findData(settings.font));
        x->setValue(settings.position.x); y->setValue(settings.position.y);
        scale->setValue(settings.scale); thickness->setValue(settings.thickness);
        const auto rgb = displayColor(settings.color);
        color->setText(rgb.name());
        color->setStyleSheet(QString("background-color: %1; color: %2;").arg(rgb.name(), rgb.lightness() > 127 ? "black" : "white"));
        error->setVisible(current->HasProcessingError());
    };
    QObject::connect(text, &QLineEdit::editingFinished, panel, [weak, text] {
        if (const auto current = weak.lock()) current->UpdateSettings({{"text", text->text().toStdString()}});
    });
    QObject::connect(font, &QComboBox::currentIndexChanged, panel, [weak, font](int index) {
        if (const auto current = weak.lock(); current && index >= 0) current->UpdateSettings({{"font", font->itemData(index).toInt()}});
    });
    const auto updatePosition = [weak, x, y](int) {
        if (const auto current = weak.lock()) current->UpdateSettings({{"position", {{"x", x->value()}, {"y", y->value()}}}});
    };
    QObject::connect(x, &QSpinBox::valueChanged, panel, updatePosition);
    QObject::connect(y, &QSpinBox::valueChanged, panel, updatePosition);
    QObject::connect(scale, &QDoubleSpinBox::valueChanged, panel, [weak](double value) {
        if (const auto current = weak.lock()) current->UpdateSettings({{"scale", static_cast<float>(value)}});
    });
    QObject::connect(thickness, &QSpinBox::valueChanged, panel, [weak](int value) {
        if (const auto current = weak.lock()) current->UpdateSettings({{"thickness", value}});
    });
    QObject::connect(color, &QPushButton::clicked, panel, [weak, panel, displayColor, update] {
        const auto current = weak.lock();
        if (!current) return;
        auto* dialog = new QColorDialog(displayColor(current->GetSettings().color), panel);
        dialog->setObjectName("drawTextColorDialog"); dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setOption(QColorDialog::DontUseNativeDialog);
        QObject::connect(dialog, &QColorDialog::colorSelected, panel, [weak, update](const QColor& selected) {
            if (const auto current = weak.lock()) current->UpdateSettings({{"color", {{"R", selected.redF()}, {"G", selected.greenF()}, {"B", selected.blueF()}}}});
            update();
        });
        dialog->open();
    });
    QObject::connect(timer, &QTimer::timeout, panel, update);
    update(); timer->start(100);
    return panel;
}
