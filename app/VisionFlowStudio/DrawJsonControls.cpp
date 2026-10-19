#include "DrawJsonControls.h"
#include <DrawTools/draw_json.hpp>
#include <opencv2/imgproc.hpp>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>

namespace {
std::string EscapeToken(const std::string& key)
{
    std::string result;
    for (const char c : key) {
        if (c == '~') result += "~0";
        else if (c == '/') result += "~1";
        else result += c;
    }
    return result;
}
void AppendFields(const nlohmann::json& value, const std::string& path, QStringList& result)
{
    if (value.is_object()) {
        for (auto it = value.begin(); it != value.end(); ++it)
            AppendFields(it.value(), path + "/" + EscapeToken(it.key()), result);
    } else result.push_back(QString::fromStdString(path)); // Arrays stay leaves, as in FlowCV.
}
}

QWidget* CreateDrawJsonControls(std::shared_ptr<DSPatch::DSPatchables::DrawJson> node, QWidget* parent)
{
    if (!node) return nullptr;
    const std::weak_ptr<DSPatch::DSPatchables::DrawJson> weak = node;
    auto* panel = new QWidget(parent);
    panel->setObjectName("drawJsonControls");
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->addLayout(form);
    auto* font = new QComboBox(panel); font->setObjectName("font");
    for (const auto* name : {"Simplex", "Plain", "Duplex", "Complex", "Triplex", "Complex Small", "Script Simplex", "Script Complex"})
        font->addItem(name, font->count());
    form->addRow("Font", font);
    auto* x = new QSpinBox(panel); x->setObjectName("position.x");
    auto* y = new QSpinBox(panel); y->setObjectName("position.y");
    auto* thickness = new QSpinBox(panel); thickness->setObjectName("thickness");
    for (auto* spin : {x, y}) spin->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    thickness->setRange(1, 100);
    form->addRow("X", x); form->addRow("Y", y);
    auto* scale = new QDoubleSpinBox(panel); scale->setObjectName("scale");
    scale->setDecimals(7); scale->setRange(-std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    scale->setSingleStep(.1);
    form->addRow("Scale", scale); form->addRow("Thickness", thickness);
    auto* color = new QPushButton(panel); color->setObjectName("color"); form->addRow("Color", color);
    auto* precision = new QSpinBox(panel); precision->setObjectName("decimal_places"); precision->setRange(1, 10);
    form->addRow("Decimal Places", precision);
    auto* dataIndex = new QSpinBox(panel); dataIndex->setObjectName("json_data_index");
    dataIndex->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    form->addRow("Data Index", dataIndex);
    auto* availableLabel = new QLabel("JSON Available Fields (double click to add)", panel);
    layout->addWidget(availableLabel);
    auto* available = new QListWidget(panel); available->setObjectName("availableFields");
    available->setMinimumHeight(66); available->setMaximumHeight(200); layout->addWidget(available);
    auto* selectedLabel = new QLabel("Draw List (double click to remove)", panel);
    layout->addWidget(selectedLabel);
    auto* selected = new QTreeWidget(panel); selected->setObjectName("drawList");
    selected->setRootIsDecorated(false); selected->setColumnCount(2);
    selected->setHeaderLabels({"Field", "Array Index"}); selected->setMinimumHeight(66); selected->setMaximumHeight(200);
    selected->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    layout->addWidget(selected);
    auto* noData = new QLabel("JSON Contains No Data", panel); noData->setObjectName("noData"); layout->addWidget(noData);
    auto* error = new QLabel("Invalid JSON field, image format or drawing settings.", panel);
    error->setObjectName("drawJsonError"); error->setWordWrap(true); error->setStyleSheet("color: #ef5350;");
    layout->addWidget(error);
    auto* timer = new QTimer(panel);
    const auto displayColor = [](const cv::Vec3f& rgb) {
        return QColor::fromRgbF(std::clamp(rgb[0], 0.f, 1.f), std::clamp(rgb[1], 0.f, 1.f), std::clamp(rgb[2], 0.f, 1.f));
    };
    const auto revision = std::make_shared<uint64_t>(std::numeric_limits<uint64_t>::max());
    const auto update = [weak, panel, form, font, x, y, scale, thickness, color, precision, dataIndex,
                         availableLabel, available, selectedLabel, selected, noData, error, timer, displayColor, revision] {
        const auto current = weak.lock();
        if (!current) { timer->stop(); panel->setEnabled(false); return; }
        const auto snapshot = current->GetUiSnapshot();
        const auto& settings = snapshot.settings;
        const QSignalBlocker fontBlock(font), xBlock(x), yBlock(y), scaleBlock(scale), thicknessBlock(thickness),
            precisionBlock(precision), indexBlock(dataIndex);
        if (font->findData(settings.font) < 0)
            font->addItem(QString("%1%2 [%3]").arg(font->itemText(settings.font & 15),
                settings.font & cv::FONT_ITALIC ? " (Italic)" : "").arg(settings.font), settings.font);
        font->setCurrentIndex(font->findData(settings.font));
        x->setValue(settings.position.x); y->setValue(settings.position.y);
        scale->setValue(settings.scale);
        // Imported values outside the original editing ranges remain visible and unchanged.
        thickness->setRange(std::min(1, settings.thickness), std::max(100, settings.thickness));
        thickness->setValue(settings.thickness);
        precision->setRange(std::min(1, settings.decimal_places), std::max(10, settings.decimal_places));
        precision->setValue(settings.decimal_places);
        dataIndex->setValue(settings.json_data_index);
        const auto data = snapshot.data.find("data");
        const bool hasData = data != snapshot.data.end() && data->is_array() && !data->empty();
        form->setRowVisible(dataIndex, hasData);
        availableLabel->setVisible(hasData); available->setVisible(hasData);
        selectedLabel->setVisible(hasData); selected->setVisible(hasData);
        noData->setVisible(!snapshot.data.empty() && !hasData);
        QStringList paths;
        if (hasData && settings.json_data_index >= 0 && size_t(settings.json_data_index) < data->size())
            AppendFields(data->at(size_t(settings.json_data_index)), "", paths);
        QStringList previous;
        for (int i = 0; i < available->count(); ++i) previous.push_back(available->item(i)->text());
        if (previous != paths) { available->clear(); available->addItems(paths); }
        if (*revision != snapshot.list_revision) {
            *revision = snapshot.list_revision;
            selected->clear();
            for (size_t i = 0; i < settings.out_list.size(); ++i) {
                const auto& entry = settings.out_list[i];
                auto* item = new QTreeWidgetItem(selected, {QString::fromStdString(entry.out_key_path)});
                item->setData(0, Qt::UserRole, QVariant::fromValue<qulonglong>(snapshot.list_revision));
                if (entry.is_array) {
                    auto* spin = new QSpinBox(selected); spin->setObjectName(QString("item_index.%1").arg(i));
                    spin->setRange(std::min(0, entry.out_item_index), std::numeric_limits<int>::max());
                    spin->setValue(entry.out_item_index); selected->setItemWidget(item, 1, spin);
                    QObject::connect(spin, &QSpinBox::valueChanged, panel, [weak, spin, i, generation = snapshot.list_revision](int value) {
                        if (const auto node = weak.lock()) {
                            if (node->SetItemIndex(i, value, generation) && value < 0) {
                                const QSignalBlocker block(spin); spin->setValue(0);
                            }
                        }
                    });
                }
            }
        } else {
            for (size_t i = 0; i < settings.out_list.size(); ++i) {
                const auto& entry = settings.out_list[i];
                if (auto* spin = qobject_cast<QSpinBox*>(selected->itemWidget(selected->topLevelItem(int(i)), 1))) {
                    const QSignalBlocker block(spin);
                    spin->setRange(std::min(0, entry.out_item_index), std::numeric_limits<int>::max());
                    spin->setValue(entry.out_item_index);
                }
            }
        }
        const auto rgb = displayColor(settings.color);
        color->setText(rgb.name());
        color->setStyleSheet(QString("background-color: %1; color: %2;").arg(rgb.name(), rgb.lightness() > 127 ? "black" : "white"));
        error->setVisible(current->HasProcessingError());
    };
    QObject::connect(precision, &QSpinBox::valueChanged, panel, [weak](int value) {
        if (const auto current = weak.lock()) current->UpdateSettings({{"decimal_places", std::clamp(value, 1, 10)}});
    });
    QObject::connect(dataIndex, &QSpinBox::valueChanged, panel, [weak, dataIndex](int value) {
        const auto current = weak.lock();
        if (!current) return;
        const auto snapshot = current->GetUiSnapshot();
        const auto data = snapshot.data.find("data");
        if (data == snapshot.data.end() || !data->is_array() || data->empty()) return;
        const int clamped = std::clamp(value, 0, int(std::min<size_t>(data->size() - 1, std::numeric_limits<int>::max())));
        current->UpdateSettings({{"json_data_index", clamped}});
        const QSignalBlocker block(dataIndex); dataIndex->setValue(clamped);
    });
    QObject::connect(available, &QListWidget::itemDoubleClicked, panel, [weak, update](QListWidgetItem* item) {
        if (const auto current = weak.lock()) current->AddField(item->text().toStdString());
        update();
    });
    QObject::connect(selected, &QTreeWidget::itemDoubleClicked, panel, [weak, selected, update](QTreeWidgetItem* item, int column) {
        if (column != 0) return;
        if (const auto current = weak.lock())
            current->RemoveField(size_t(selected->indexOfTopLevelItem(item)), item->data(0, Qt::UserRole).toULongLong());
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
        if (const auto current = weak.lock()) current->UpdateSettings({{"thickness", std::clamp(value, 1, 100)}});
    });
    QObject::connect(color, &QPushButton::clicked, panel, [weak, panel, displayColor, update] {
        const auto current = weak.lock();
        if (!current) return;
        auto* dialog = new QColorDialog(displayColor(current->GetSettings().color), panel);
        dialog->setObjectName("drawJsonColorDialog"); dialog->setAttribute(Qt::WA_DeleteOnClose);
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
