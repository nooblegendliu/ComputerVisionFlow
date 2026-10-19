#include <QtCore/QVariant>
#include "DrawDateTimeControls.h"
#include <DrawTools/draw_datetime.hpp>
#include <opencv2/imgproc.hpp>
#include <QCheckBox>
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

QWidget* CreateDrawDateTimeControls(std::shared_ptr<DSPatch::DSPatchables::DrawDateTime> node, QWidget* parent)
{
    if (!node) return nullptr;
    const std::weak_ptr<DSPatch::DSPatchables::DrawDateTime> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName("drawDateTimeControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    auto* showDate = new QCheckBox("Draw Date", panel); showDate->setObjectName("show_date");
    auto* showTime = new QCheckBox("Draw Time", panel); showTime->setObjectName("show_time");
    auto* date = new QLineEdit(panel); date->setObjectName("date_fmt");
    auto* time = new QLineEdit(panel); time->setObjectName("time_fmt");
    date->setMaxLength(std::numeric_limits<int>::max()); time->setMaxLength(std::numeric_limits<int>::max());
    date->setToolTip("%Y/%y: year; %C: century; %G/%g: week year; %B/%b/%h: month name; %m: month;\n"
        "%U/%W/%V: week; %j: year day; %d/%e: month day; %A/%a: weekday; %w/%u: weekday number; %x/%D/%F: date");
    time->setToolTip("%H/%I: hour; %M: minute; %S: second; %c: date and time; %X/%r/%R/%T: time; %p: AM/PM; %z/%Z: timezone");
    auto* dateError = new QLabel("Invalid Date Format", panel); dateError->setObjectName("dateFormatError");
    auto* timeError = new QLabel("Invalid Time Format", panel); timeError->setObjectName("timeFormatError");
    for (auto* label : {dateError, timeError}) label->setProperty("severity", "error");
    form->addRow(showDate); form->addRow("Date Format", date); form->addRow(dateError);
    form->addRow(showTime); form->addRow("Time Format", time); form->addRow(timeError);
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
    auto* error = new QLabel("Invalid image, date/time format or drawing settings.", panel);
    error->setObjectName("drawDateTimeError"); error->setWordWrap(true); error->setProperty("severity", "error");
    layout->addWidget(error);
    auto* timer = new QTimer(panel);
    const auto displayColor = [](const cv::Vec3f& rgb) {
        return QColor::fromRgbF(std::clamp(rgb[0], 0.f, 1.f), std::clamp(rgb[1], 0.f, 1.f), std::clamp(rgb[2], 0.f, 1.f));
    };
    const auto update = [weak, panel, form, date, time, showDate, showTime, dateError, timeError, font, x, y, scale, thickness, color, error, timer, displayColor] {
        const auto current = weak.lock();
        if (!current) { timer->stop(); panel->setEnabled(false); return; }
        const auto settings = current->GetSettings();
        const QSignalBlocker dateBlock(date), timeBlock(time), showDateBlock(showDate), showTimeBlock(showTime), fontBlock(font), xBlock(x), yBlock(y), scaleBlock(scale), thicknessBlock(thickness);
        if (!date->hasFocus()) date->setText(QString::fromStdString(settings.date_draft));
        if (!time->hasFocus()) time->setText(QString::fromStdString(settings.time_draft));
        showDate->setChecked(settings.show_date); showTime->setChecked(settings.show_time);
        form->setRowVisible(date, settings.show_date); form->setRowVisible(time, settings.show_time);
        form->setRowVisible(dateError, settings.show_date && !settings.valid_date);
        form->setRowVisible(timeError, settings.show_time && !settings.valid_time);
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
    QObject::connect(date, &QLineEdit::returnPressed, panel, [weak, date, update] {
        if (const auto current = weak.lock()) current->CommitFormat(0, date->text().toStdString());
        update();
    });
    QObject::connect(time, &QLineEdit::returnPressed, panel, [weak, time, update] {
        if (const auto current = weak.lock()) current->CommitFormat(1, time->text().toStdString());
        update();
    });
    QObject::connect(showDate, &QCheckBox::toggled, panel, [weak, update](bool value) {
        if (const auto current = weak.lock()) current->UpdateSettings({{"show_date", value}});
        update();
    });
    QObject::connect(showTime, &QCheckBox::toggled, panel, [weak, update](bool value) {
        if (const auto current = weak.lock()) current->UpdateSettings({{"show_time", value}});
        update();
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
        dialog->setObjectName("drawDateTimeColorDialog"); dialog->setAttribute(Qt::WA_DeleteOnClose);
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
