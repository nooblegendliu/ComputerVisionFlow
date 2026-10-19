#include <QtCore/QVariant>
#include "ConvertColorControls.h"
#include <ConvertColor/convert_color.hpp>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QSignalBlocker>

QWidget* CreateConvertColorControls(std::shared_ptr<DSPatch::DSPatchables::ConvertColor> node, QWidget* parent)
{
    auto* panel = new QWidget(parent);
    auto* layout = new QVBoxLayout(panel);
    layout->addWidget(new QLabel("Conversion Type", panel));
    auto* conversion = new QComboBox(panel);
    conversion->setObjectName("convertColorMode");
    for (const auto& name : node->Properties().GetOptions("conv_code"))
        conversion->addItem(QString::fromStdString(name));
    conversion->setCurrentIndex(node->Properties().GetW<int>("conv_code"));
    layout->addWidget(conversion);
    auto* error = new QLabel("Incompatible Conversion Type", panel);
    error->setObjectName("convertColorError");
    error->setProperty("severity", "error");
    error->setVisible(node->HasChannelError());
    layout->addWidget(error);
    const std::weak_ptr<DSPatch::DSPatchables::ConvertColor> weak = node;
    QObject::connect(conversion, &QComboBox::currentIndexChanged, panel, [weak](int index) {
        if (const auto node = weak.lock()) node->Properties().Set("conv_code", index);
    });
    auto* timer = new QTimer(panel);
    QObject::connect(timer, &QTimer::timeout, panel, [weak, conversion, error] {
        if (const auto node = weak.lock()) {
            const QSignalBlocker blocker(conversion);
            conversion->setCurrentIndex(node->Properties().GetW<int>("conv_code"));
            error->setVisible(node->HasChannelError());
        }
    });
    timer->start(100);
    return panel;
}
