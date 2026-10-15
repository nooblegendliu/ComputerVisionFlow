#include "PropertyPanel.h"

#include <flowcv_manager.h>
#include <flowProperties.h>

#include <utility>
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
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>


// =============================================================
// Constructor
// =============================================================
PropertyPanel::PropertyPanel(
    FlowCV::FlowCV_Manager& manager,
    QWidget* parent)
    : QWidget(parent),
    manager_(manager)
{
    auto* layout = new QVBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    scrollArea_ = new QScrollArea(this);

    scrollArea_->setWidgetResizable(true);

    scrollArea_->setFrameShape(
        QFrame::NoFrame);

    layout->addWidget(scrollArea_);

    refresh();
}


// =============================================================
// Selected Node
// =============================================================
void PropertyPanel::setNode(std::uint64_t nodeId)
{
    if (selectedNodeId_ == nodeId) {
        return;
    }

    selectedNodeId_ = nodeId;

    // 原版选择节点只改变对应标题的高亮。
    // 不因此重建全部属性控件。
    updateSelectionStyle();
}


// =============================================================
// Refresh All Node Sections
// =============================================================
void PropertyPanel::refresh()
{
    struct NodeData
    {
        std::uint64_t id = 0;

        QString title;

        FlowCV::PropertiesSnapshot snapshot;
    };

    std::vector<NodeData> nodes;

    std::vector<std::uint64_t> newOrder;

    const std::uint64_t nodeCount =
        manager_.GetNodeCount();

    // ---------------------------------------------------------
    // 遍历 Manager 中的节点。
    //
    // 原版通过 NodeHasUI(Controls) 识别属性窗口；
    // 当前去除 ImGui 后，通过属性快照判断。
    // ---------------------------------------------------------
    for (std::uint64_t i = 0; i < nodeCount; ++i)
    {
        FlowCV::NodeInfo info{};

        if (!manager_.GetNodeInfoByIndex(i, info)) {
            continue;
        }

        if (!info.node_ptr) {
            continue;
        }

        FlowCV::PropertiesSnapshot snapshot{};

        if (!manager_.GetNodePropertiesSnapshot(
            info.id,
            snapshot)) {
            continue;
        }

        // Viewer 等没有可编辑属性的节点，
        // 不显示空的 Controls 分组。
        if (snapshot.properties.empty()) {
            continue;
        }

        // 与原版一致：NodeName_InstanceNumber
        QString title =
            QStringLiteral("%1_%2")
            .arg(QString::fromStdString(info.desc.name))
            .arg(info.node_ptr->GetInstanceCount());

        newOrder.push_back(info.id);

        nodes.push_back({
            info.id,
            std::move(title),
            std::move(snapshot)
            });
    }


    // ---------------------------------------------------------
    // 节点列表未改变：
    // 不重建所有分组。
    //
    // 连接创建/删除也会触发 graphChanged，
    // 不应因此让正在编辑的控件丢失焦点。
    // ---------------------------------------------------------
    if (newOrder == nodeOrder_)
    {
        for (const auto& node : nodes)
        {
            auto it = sections_.find(node.id);

            if (it == sections_.end()) {
                continue;
            }

            NodeSection& section = it->second;

            if (section.title != node.title)
            {
                section.title = node.title;

                section.header->setText(node.title);
            }

            // 仅当属性确实更新后重新绘制对应分组。
            if (section.revision != node.snapshot.revision)
            {
                refreshNodeFields(
                    node.id,
                    node.snapshot);
            }
        }

        updateSelectionStyle();

        return;
    }


    // ---------------------------------------------------------
    // 节点数量或顺序改变：
    // 保存各分组原有的折叠状态。
    // ---------------------------------------------------------
    std::unordered_map<std::uint64_t, bool> expanded;

    for (const auto& [nodeId, section] : sections_)
    {
        if (section.header)
        {
            expanded.emplace(
                nodeId,
                section.header->isChecked());
        }
    }


    // ---------------------------------------------------------
    // 构建新的滚动内容。
    // ---------------------------------------------------------
    auto* content = new QWidget;

    auto* contentLayout = new QVBoxLayout(content);

    contentLayout->setContentsMargins(6, 6, 6, 6);

    contentLayout->setSpacing(6);


    std::unordered_map<std::uint64_t, NodeSection>
        newSections;


    // ---------------------------------------------------------
    // 创建每个节点的独立属性分组。
    // ---------------------------------------------------------
    for (const auto& node : nodes)
    {
        auto* sectionWidget = new QWidget(content);

        auto* sectionLayout =
            new QVBoxLayout(sectionWidget);

        sectionLayout->setContentsMargins(0, 0, 0, 0);
        sectionLayout->setSpacing(0);


        // -----------------------------------------------------
        // Collapsing Header
        // -----------------------------------------------------
        auto* header = new QToolButton(sectionWidget);

        header->setText(node.title);

        header->setCheckable(true);

        header->setToolButtonStyle(
            Qt::ToolButtonTextBesideIcon);

        header->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Preferred);


        // 新节点默认展开，与原版 DefaultOpen 对应。
        bool isExpanded = true;

        auto expandedIt = expanded.find(node.id);

        if (expandedIt != expanded.end())
        {
            isExpanded = expandedIt->second;
        }

        header->setChecked(isExpanded);

        header->setArrowType(
            isExpanded
            ? Qt::DownArrow
            : Qt::RightArrow);


        // -----------------------------------------------------
        // Property Fields
        // -----------------------------------------------------
        QWidget* fields = createPropertyFields(
            node.id,
            node.snapshot,
            sectionWidget);

        fields->setVisible(isExpanded);


        sectionLayout->addWidget(header);
        sectionLayout->addWidget(fields);

        contentLayout->addWidget(sectionWidget);


        newSections.emplace(
            node.id,
            NodeSection{
                node.title,
                header,
                fields,
                sectionLayout,
                node.snapshot.revision
            });


        // -----------------------------------------------------
        // 独立展开/折叠
        // -----------------------------------------------------
        QObject::connect(
            header,
            &QToolButton::toggled,
            this,
            [this, nodeId = node.id](bool checked)
            {
                auto it = sections_.find(nodeId);

                if (it == sections_.end()) {
                    return;
                }

                NodeSection& section = it->second;

                section.header->setArrowType(
                    checked
                    ? Qt::DownArrow
                    : Qt::RightArrow);

                section.fields->setVisible(checked);
            });
    }


    // ---------------------------------------------------------
    // 没有可编辑节点时的提示。
    // ---------------------------------------------------------
    if (nodes.empty())
    {
        auto* label = new QLabel(
            QStringLiteral("No editable nodes"),
            content);

        label->setAlignment(Qt::AlignCenter);

        contentLayout->addWidget(label);
    }

    contentLayout->addStretch();


    // ---------------------------------------------------------
    // 替换整个内容。
    //
    // QScrollArea 负责销毁旧 content。
    // Qt 父子关系负责清理旧控件。
    // ---------------------------------------------------------
    nodeOrder_ = std::move(newOrder);

    sections_ = std::move(newSections);

    scrollArea_->setWidget(content);

    updateSelectionStyle();
}


// =============================================================
// Create Property Fields For One Node
// =============================================================
QWidget* PropertyPanel::createPropertyFields(
    std::uint64_t nodeId,
    const FlowCV::PropertiesSnapshot& snapshot,
    QWidget* parent)
{
    auto* fields = new QWidget(parent);

    auto* formLayout = new QFormLayout(fields);

    formLayout->setContentsMargins(8, 6, 8, 10);
    formLayout->setSpacing(6);

    formLayout->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);

    formLayout->setRowWrapPolicy(
        QFormLayout::WrapLongRows);


    int visibleCount = 0;

    for (const auto& property : snapshot.properties)
    {
        // 使用原有的属性可见性规则。
        // 由节点 RefreshProperties() 决定。
        if (!property.visible) {
            continue;
        }

        QWidget* editor = createPropertyEditor(
            nodeId,
            property,
            fields);

        if (!editor) {
            continue;
        }

        if (property.type ==
            FlowCV::PropertyDataTypes::kDataTypeBool)
        {
            formLayout->addRow(editor);
        }
        else
        {
            formLayout->addRow(
                QString::fromStdString(property.desc),
                editor);
        }

        ++visibleCount;
    }


    if (visibleCount == 0)
    {
        formLayout->addRow(
            new QLabel(
                QStringLiteral("No editable properties"),
                fields));
    }

    return fields;
}


// =============================================================
// Create Qt Editor For Property
// =============================================================
QWidget* PropertyPanel::createPropertyEditor(
    std::uint64_t nodeId,
    const FlowCV::PropertySnapshot& property,
    QWidget* parent)
{
    using FlowCV::PropertyDataTypes;

    const std::string key = property.key;

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
            [this, nodeId, key](bool value)
            {
                manager_.SetNodeProperty(
                    nodeId,
                    key,
                    value);

                // Color / Has Alpha 等属性可能影响
                // 其他属性的可见性。
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
            std::get_if<FlowCV::IntRange>(
                &property.range))
        {
            spinBox->setRange(
                range->min,
                range->max);
        }

        // 原版 IntRange.step 是 DragInt 编辑速度，
        // 不是数值的量化步长。
        spinBox->setSingleStep(1);

        spinBox->setValue(
            std::get<int>(property.value));

        QObject::connect(
            spinBox,
            QOverload<int>::of(
                &QSpinBox::valueChanged),
            this,
            [this, nodeId, key](int value)
            {
                if (!manager_.SetNodeProperty(
                    nodeId,
                    key,
                    value))
                {
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

        spinBox->setDecimals(6);

        if (const auto* range =
            std::get_if<FlowCV::FloatRange>(
                &property.range))
        {
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
            [this, nodeId, key](double value)
            {
                if (!manager_.SetNodeProperty(
                    nodeId,
                    key,
                    static_cast<float>(value)))
                {
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

        for (const auto& option : property.options)
        {
            comboBox->addItem(
                QString::fromStdString(option));
        }

        comboBox->setCurrentIndex(
            std::get<int>(property.value));

        QObject::connect(
            comboBox,
            QOverload<int>::of(
                &QComboBox::currentIndexChanged),
            this,
            [this, nodeId, key](int index)
            {
                if (index < 0) {
                    return;
                }

                manager_.SetNodeProperty(
                    nodeId,
                    key,
                    index);

                // Option 属性也可能影响其他参数的显隐。
                queueRefresh(nodeId);
            });

        return comboBox;
    }


    // ---------------------------------------------------------
    // Other Types
    // ---------------------------------------------------------
    case PropertyDataTypes::kDataTypeSeparator:
    case PropertyDataTypes::kDataTypeUndefined:
    default:
        return nullptr;
    }
}


// =============================================================
// Refresh One Node Fields
// =============================================================
void PropertyPanel::refreshNodeFields(
    std::uint64_t nodeId,
    const FlowCV::PropertiesSnapshot& snapshot)
{
    auto it = sections_.find(nodeId);

    if (it == sections_.end()) {
        return;
    }

    NodeSection& section = it->second;

    QWidget* oldFields = section.fields;

    // 只重新生成指定节点的属性控件。
    QWidget* newFields = createPropertyFields(
        nodeId,
        snapshot,
        section.header->parentWidget());


    // ---------------------------------------------------------
    // 替换当前节点的属性区域。
    // 不影响其他节点的展开状态与编辑控件。
    // ---------------------------------------------------------
    section.layout->removeWidget(oldFields);

    oldFields->hide();

    section.layout->insertWidget(
        1,
        newFields);

    newFields->setVisible(
        section.header->isChecked());

    section.fields = newFields;

    section.revision = snapshot.revision;


    // 延迟销毁旧控件，避免与当前 Qt 信号处理冲突。
    oldFields->deleteLater();
}


// =============================================================
// Deferred Refresh
// =============================================================
void PropertyPanel::queueRefresh(std::uint64_t nodeId)
{
    QTimer::singleShot(
        0,
        this,
        [this, nodeId]()
        {
            // 节点删除后，不再操作旧属性区域。
            if (sections_.find(nodeId) ==
                sections_.end()) {
                return;
            }

            FlowCV::PropertiesSnapshot snapshot{};

            if (!manager_.GetNodePropertiesSnapshot(
                nodeId,
                snapshot)) {
                return;
            }

            refreshNodeFields(
                nodeId,
                snapshot);
        });
}


// =============================================================
// Selected Node Highlight
// =============================================================
void PropertyPanel::updateSelectionStyle()
{
    for (const auto& [nodeId, section] : sections_)
    {
        if (!section.header) {
            continue;
        }

        const bool selected =
            nodeId == selectedNodeId_;


        // 对应原版：
        // Selected: IM_COL32(39, 154, 14, 255)
        // Normal:   IM_COL32(45, 52, 65, 255)
        if (selected)
        {
            section.header->setStyleSheet(
                QStringLiteral(
                    "QToolButton {"
                    " background-color: rgb(39, 154, 14);"
                    " color: white;"
                    " border: none;"
                    " border-radius: 2px;"
                    " padding: 6px;"
                    " text-align: left;"
                    " font-weight: bold;"
                    "}"));
        }
        else
        {
            section.header->setStyleSheet(
                QStringLiteral(
                    "QToolButton {"
                    " background-color: rgb(45, 52, 65);"
                    " color: white;"
                    " border: none;"
                    " border-radius: 2px;"
                    " padding: 6px;"
                    " text-align: left;"
                    "}"));
        }
    }
}