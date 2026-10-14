#include "PropertyPanel.h"

#include <flowcv_manager.h>
#include <flowProperties.h>

#include <variant>
#include <QtCore/QTimer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

// ---------------------------------------
// Constructor
// ---------------------------------------
PropertyPanel::PropertyPanel(
	FlowCV::FlowCV_Manager& manager, QWidget* parent)
	:QWidget(parent), manager_(manager)
{
    auto* layout = new QVBoxLayout(this);

    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // 当前选中节点名称
    title_ = new QLabel(QStringLiteral("Properties"), this);

    title_->setObjectName(QStringLiteral("PropertyPanelTitle"));

    layout->addWidget(title_);

    // 属性较多时允许滚动
    scrollArea_ = new QScrollArea(this);

    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);

    layout->addWidget(scrollArea_);

    refresh();
}



// --------------------------------------------------------
// Selected Node
// --------------------------------------------------------
void PropertyPanel::setNode(std::uint64_t nodeId)
{
    if (nodeId_ == nodeId)
        return;

    nodeId_ = nodeId;

    refresh();
}


// --------------------------------------------------------
// Refresh Property UI
// --------------------------------------------------------
void PropertyPanel::refresh()
{
    // 每次从 Manager 取得最新 Snapshot，
    // 不在面板中缓存另一份属性值。
    auto* content = new QWidget;

    auto* formLayout = new QFormLayout(content);

    formLayout->setContentsMargins(4, 4, 4, 4);
    formLayout->setSpacing(8);

    formLayout->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);

    // 没有选中节点
    if (nodeId_ == 0) {

        title_->setText(
            QStringLiteral("Properties"));

        formLayout->addRow(
            new QLabel(
                QStringLiteral("Select a node"),
                content));

        scrollArea_->setWidget(content);

        return;
    }

    // 获取节点基本信息
    FlowCV::NodeInfo nodeInfo{};

    if (!manager_.GetNodeInfoById(nodeId_, nodeInfo)) {

        nodeId_ = 0;

        title_->setText(
            QStringLiteral("Properties"));

        formLayout->addRow(
            new QLabel(
                QStringLiteral("Select a node"),
                content));

        scrollArea_->setWidget(content);

        return;
    }

    // 使用原 FlowCV 的节点名称
    title_->setText(
        QString::fromStdString(nodeInfo.desc.name));

    // 读取节点属性快照
    FlowCV::PropertiesSnapshot snapshot{};

    if (!manager_.GetNodePropertiesSnapshot(
        nodeId_, snapshot)) {

        formLayout->addRow(
            new QLabel(
                QStringLiteral("No editable properties"),
                content));

        scrollArea_->setWidget(content);

        return;
    }

    int visibleCount = 0;

    for (const auto& property : snapshot.properties) {

        // 与原版 DrawUi() 一致：
        // 不可见属性不创建 UI。
        if (!property.visible) {
            continue;
        }

        QWidget* editor = createPropertyEditor(
            property, content);

        if (!editor) {
            continue;
        }

        // Bool 直接使用带标签的复选框
        if (property.type ==
            FlowCV::PropertyDataTypes::kDataTypeBool) {

            formLayout->addRow(editor);
        }
        else {

            formLayout->addRow(
                QString::fromStdString(property.desc),
                editor);
        }

        ++visibleCount;
    }

    // Viewer 等节点可能没有可编辑属性
    if (visibleCount == 0) {

        formLayout->addRow(
            new QLabel(
                QStringLiteral("No editable properties"),
                content));
    }

    // QScrollArea 接管 content 的所有权，
    // 替换时删除之前的属性控件。
    scrollArea_->setWidget(content);
}


// ---------------------------------------------------------
// Create Property Editor
// ---------------------------------------------------------
QWidget* PropertyPanel::createPropertyEditor(
    const FlowCV::PropertySnapshot& property,
    QWidget* parent)
{
    using FlowCV::PropertyDataTypes;

    // 属性 key 与 FlowCV 节点 ID 保持原样。
    const std::string key = property.key;
    const std::uint64_t nodeId = nodeId_;

    switch (property.type)
    {
        // ---------------------------------------------------------
        // Bool -> QCheckBox
        // ---------------------------------------------------------
    case PropertyDataTypes::kDataTypeBool:
    {
        auto* checkBox = new QCheckBox(
            QString::fromStdString(property.desc),
            parent);

        checkBox->setChecked(
            std::get<bool>(property.value));

        QObject::connect(
            checkBox,
            &QCheckBox::toggled,
            this,
            [this, nodeId, key](bool value) {

                manager_.SetNodeProperty(
                    nodeId,
                    key,
                    value);

                // 更新属性可见性，例如 Solid 的 Color / Has Alpha
                queueRefresh(nodeId);
            });

        return checkBox;
    }

    // ---------------------------------------------------------
    // Int -> QSpinBox
    // ---------------------------------------------------------
    case PropertyDataTypes::kDataTypeInt:
    {
        auto* spinBox = new QSpinBox(parent);

        if (const auto* range =
            std::get_if<FlowCV::IntRange>(&property.range)) {

            spinBox->setRange(
                range->min,
                range->max);
        }

        // IntRange.step 是原版 DragInt 的编辑速度，
        // 并非整数步长或取整规则。
        spinBox->setSingleStep(1);

        spinBox->setValue(
            std::get<int>(property.value));

        QObject::connect(
            spinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            [this, nodeId, key](int value) {

                if (!manager_.SetNodeProperty(
                    nodeId,
                    key,
                    value)) {

                    queueRefresh(nodeId);
                }
            });

        return spinBox;
    }

    // ---------------------------------------------------------
    // Float -> QDoubleSpinBox
    // ---------------------------------------------------------
    case PropertyDataTypes::kDataTypeFloat:
    {
        auto* spinBox = new QDoubleSpinBox(parent);

        // 保留常用浮点属性的显示精度。
        // step 只控制 UI 调整幅度，不对值做量化。
        spinBox->setDecimals(6);

        if (const auto* range =
            std::get_if<FlowCV::FloatRange>(&property.range)) {

            spinBox->setRange(
                static_cast<double>(range->min),
                static_cast<double>(range->max));

            spinBox->setSingleStep(
                static_cast<double>(range->step));
        }

        spinBox->setValue(
            static_cast<double>(
                std::get<float>(property.value)));

        QObject::connect(
            spinBox,
            QOverload<double>::of(
                &QDoubleSpinBox::valueChanged),
            this,
            [this, nodeId, key](double value) {

                if (!manager_.SetNodeProperty(
                    nodeId,
                    key,
                    static_cast<float>(value))) {

                    queueRefresh(nodeId);
                }
            });

        return spinBox;
    }

    // ---------------------------------------------------------
    // Option -> QComboBox
    // ---------------------------------------------------------
    case PropertyDataTypes::kDataTypeOption:
    {
        auto* comboBox = new QComboBox(parent);

        for (const auto& option : property.options) {

            comboBox->addItem(
                QString::fromStdString(option));
        }

        // FlowCV 的 Option 值使用从 0 开始的索引。
        comboBox->setCurrentIndex(
            std::get<int>(property.value));

        QObject::connect(
            comboBox,
            QOverload<int>::of(
                &QComboBox::currentIndexChanged),
            this,
            [this, nodeId, key](int index) {

                if (index < 0) {
                    return;
                }

                manager_.SetNodeProperty(
                    nodeId,
                    key,
                    index);

                // Option 也可能控制其他属性的显隐。
                queueRefresh(nodeId);
            });

        return comboBox;
    }

    // ---------------------------------------------------------
    // Other types
    // ---------------------------------------------------------
    case PropertyDataTypes::kDataTypeSeparator:
    case PropertyDataTypes::kDataTypeUndefined:
    default:
        return nullptr;
    }
}



// ----------------------------------------------------------
// Deferred Refresh
// ----------------------------------------------------------
void PropertyPanel::queueRefresh(std::uint64_t nodeId)
{
    QTimer::singleShot(
        0,
        this,
        [this, nodeId]() {

            // 节点可能已被删除或选中了其他节点。
            // 只刷新当前选中节点。
            if (nodeId_ == nodeId) {
                refresh();
            }
        });
}