# VisionFlowStudio 节点编辑器阶段开发任务书（Codex 执行版）

> 文档用途：交付 Codex，指导其在 **ComputerVisionFlow** 当前代码基础上逐步实现 Qt 图形节点编辑器。
>
> 编写日期：2026-10-13（Asia/Taipei）  
> 对照仓库：[`nooblegendliu/ComputerVisionFlow`](https://github.com/nooblegendliu/ComputerVisionFlow)，`master`，核对提交 [`c6d0e08154e013e011911a42e33a67c6c08aeb0b`](https://github.com/nooblegendliu/ComputerVisionFlow/commit/c6d0e08154e013e011911a42e33a67c6c08aeb0b)  
> 原版参照：[`FlowCV-org/FlowCV`](https://github.com/FlowCV-org/FlowCV)，`main`，核对提交 [`b09c48edcc9437253da6049365d319fe036eff61`](https://github.com/FlowCV-org/FlowCV/commit/b09c48edcc9437253da6049365d319fe036eff61)  
> **重要**：以上是编写时的基线。Codex 执行前必须核对实际工作区 HEAD 和文件内容；若代码发生变化，以工作区最新代码为准，先说明差异再执行，不应直接按旧片段覆盖。

---

## 0. 给 Codex 的总任务与最重要原则

### 0.1 总任务

在已经通过的无 UI 运行闭环（`Solid A + Solid B → Add → Viewer`）基础上，完成首版可操作的 **VisionFlowStudio Qt 节点编辑器**。用户应能够在界面内创建与删除节点、编辑连线、修改参数、执行现有 DSPatch 流程、查看 Viewer 图像，并在后续阶段保存/加载包含节点位置的项目文件。

**核心原则：实现 UI 到现有运行时的适配，不重新设计计算引擎。**

系统职责必须保持：

- **Qt 6 / QtNodes**：图形展示、交互、节点位置、选择状态、参数控件、Viewer 窗口。
- **FlowCV::FlowCV_Manager**：真实运行图的节点、连线、参数访问、状态序列化、执行入口。
- **DSPatch**：原有计算与调度。
- **FlowCV::Node / Nodes**：原有图像处理和参数计算，继续与 Qt / ImGui 解耦。

**不得让 QtNodes 自己运行一套替代 DSPatch 的数据流计算。**

### 0.2 强制约束（优先级高于方便性）

1. **尊重原版 FlowCV**。除替换 ImGui 相关 UI、完成已确认的 UI/属性解耦、解决阻止集成的必要缺陷外，不擅自重写原库的运行逻辑。
2. **不修改 DSPatch 的执行规则**，不增加独立的 `Stopped/Running/Paused` 状态机，不设计新的任务调度器或事件总线。
3. **不修改 Solid、Add、Viewer 的计算算法**。UI 不调用或修改其 `Process_()`。
4. **不让 `sdk/`、`managers/`、`nodes/` 依赖 Qt**，也不把 `QWidget`、`QObject`、`QImage`、`QPointF` 引入公共计算核心。
5. **不为迎合 QtNodes 改写 Manager 的节点/连线所有权**：由 Manager / DSPatch 维护真实节点和连接，编辑器只是适配者。
6. **优先使用现有公开接口**。确有缺失时先列证据和影响面；只有必要才做最小、向后兼容的 Manager API 补充，不做大规模封装重构。
7. **不修改 `third_party/nodeeditor` 供应商源码作为首选方案**。优先用其公开的 `AbstractGraphModel`、`BasicGraphicsScene`、`GraphicsView` 扩展点。
8. **不新增多余模块**（如第二套 Graph Engine、第二套 Runner、独立 Controller Framework），不改变现有 `sdk/`、`managers/`、`nodes/` 总体目录结构。
9. **保留 Release 构建路线**：继续使用现有 `local-x64-release` 和 Release OpenCV；不为实现 UI 强加 Debug OpenCV，不添加全局 Debug 禁令。
10. **遵循文件级、小步交付**：每阶段先使现有目标编译通过，再进入下一阶段；任何新增文件必须显式加入相应 CMake 源码清单。
11. **不将未实现功能写成已完成**。文档、验收报告必须如实标注通过、未验证或受本机环境阻塞。
12. **只修改本任务范围文件**；如果发现无关缺陷，记录而不是顺带重构。

### 0.3 本阶段工作范围

| 优先级 | 范围 | 目标 |
|---|---|---|
| P0 | QtNodes 图模型适配 | UI 中节点/连线与 Manager 始终一致 |
| P0 | Qt 可用画布及主窗口 | 可通过 UI 创建、移动、连接、删除 Solid/Add/Viewer |
| P1 | 通用属性面板 | 通过已存在的 Snapshot / SetNodeProperty 编辑参数 |
| P1 | 单次执行与 Viewer 图像显示 | UI 重现原 640×480、像素值 70 的闭环 |
| P2 | 布局及项目保存恢复 | 同时保存运行图和 `editor` 节点位置，重启可恢复 |
| P3 / 后续 | 连续执行、完整撤销重做、复制粘贴、批量布局、复杂插件体验 | 在基本闭环稳定后单独实施 |

P0/P1 优先于 P2；不因设计未来的插件、撤销框架而延迟第一版 GUI 可运行闭环。

---

## 1. 代码基线与原版行为核对

### 1.1 当前实际已存在的实现

| 位置 | 核对结果 | 本阶段决策 |
|---|---|---|
| `app/VisionFlowStudio/main.cpp` | 当前为控制台最小闭环：创建 2 个 Solid、1 个 Add、1 个 Viewer，配置、连接、调用同步 `Tick(Series)` 并检查图像 | GUI 入口开发以这套已验证流程为验收基准；尽量保留原有闭环验证能力 |
| `editor/VisionFlowEditor.h/.cpp` | 仍为占位文件 | 从这里开始实现 Editor，不另起 UI 工程 |
| `editor/CMakeLists.txt` | 已建 `VisionFlowEditor` 静态库；链接 Sdk、Managers、QtNodes | 补充 Qt 依赖、AUTOMOC 与编辑器源码 |
| `managers/flowcv_manager.h/.cpp` | 节点创建/删除、连线/断线、Wire 查询、属性读取/写入、GetState/SetState、Tick/AutoTick 等已提供 | 直接调用；避免重复管理运行图 |
| `managers/node_manager.h/.cpp` | `InternalNodeManager` 工厂和节点元数据列表已存在，注册 Add、Solid、Viewer | 用于节点创建菜单和类别列表 |
| `sdk/flowNode.h` | `FlowCV::Node` 继承 DSPatch Component，具有属性快照和 State API | 保留，无 Qt 化 |
| `sdk/flowProperties.h/.cpp` | Bool、Int、Float、Option 等，支持 `Snapshot()`、`Sync()`、可见性及范围提示 | 直接驱动 Qt 控件，不另设计属性模型 |
| `nodes/Viewer/Viewer.cpp` | 已有加锁深拷贝 `GetFrameCopy()` | 图像显示侧仅调用此接口 |
| `third_party/nodeeditor` | 集成 QtNodes，含 `AbstractGraphModel`、`BasicGraphicsScene`、`GraphicsView` 和自带示例 | 使用其模型 API，而非旧版 QtNodes 2.x 教程 |
| `docs/architecture.md` | 已声明 UI 尚未实现、QtNodes 与 Manager 的边界 | 实现完成后同步更新实际状态 |

### 1.2 必须参照的当前仓库文件

- [`docs/architecture.md`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/docs/architecture.md)
- [`managers/flowcv_manager.h`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/managers/flowcv_manager.h)
- [`managers/flowcv_manager.cpp`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/managers/flowcv_manager.cpp)
- [`managers/node_manager.h`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/managers/node_manager.h)
- [`sdk/flowProperties.h`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/sdk/flowProperties.h)
- [`sdk/flowNode.h`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/sdk/flowNode.h)
- [`nodes/Solid/Solid.cpp`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/nodes/Solid/Solid.cpp)
- [`nodes/Add/add.cpp`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/nodes/Add/add.cpp)
- [`nodes/Viewer/Viewer.cpp`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/nodes/Viewer/Viewer.cpp)
- [`third_party/nodeeditor/include/QtNodes/internal/AbstractGraphModel.hpp`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/third_party/nodeeditor/include/QtNodes/internal/AbstractGraphModel.hpp)
- [`third_party/nodeeditor/examples/simple_graph_model/SimpleGraphModel.cpp`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/third_party/nodeeditor/examples/simple_graph_model/SimpleGraphModel.cpp)
- [`third_party/nodeeditor/docs/guide/graph-models.rst`](https://github.com/nooblegendliu/ComputerVisionFlow/blob/master/third_party/nodeeditor/docs/guide/graph-models.rst)

### 1.3 原版 FlowCV 的参照点

- [`Editor_UI/Node_Editor/node_editor.cpp`](https://github.com/FlowCV-org/FlowCV/blob/main/Editor_UI/Node_Editor/node_editor.cpp)：创建、连线、断线、删除、选中、参数显示入口、编辑器节点位置、复制粘贴处理。
- [`Managers/FlowCV_Manager.cpp`](https://github.com/FlowCV-org/FlowCV/blob/main/Managers/FlowCV_Manager.cpp)：真实节点和 DSPatch Circuit 的关系。

**只借鉴原版规则，不复制 ImGui 实现**：

1. 节点由 FlowCV Manager 创建/删除。
2. 端口数量、方向、名称、`IoType` 来自实际 DSPatch Component。
3. 图形连接通过 Manager 创建/删除，不只在 UI 上连一条曲线。
4. 输入端口只能有一个有效上游；连接须校验类型。
5. 节点位置、编辑器显示状态属于 UI，独立于运行图。
6. 底层运行图沿用 `GetState()` / `SetState()`。

---

## 2. 推荐架构与对象生命周期

### 2.1 依赖关系

```text
app/VisionFlowStudio
  ├── QApplication + MainWindow                (Qt 应用装配)
  └── editor/VisionFlowEditor                   (画布、菜单、选择交互)
         ├── VisionFlowGraphModel               (QtNodes/Manager 适配)
         ├── PropertyPanel                      (属性编辑)
         └── ViewerPanel                        (cv::Mat → QImage)

VisionFlowGraphModel / PropertyPanel / ViewerPanel
              │ UI 无关 Manager API / NodeInfo / Viewer::GetFrameCopy()
              ▼
        FlowCV_Manager
          ├── InternalNodeManager
          ├── PluginManager（本阶段不主攻）
          └── DSPatch::Circuit → Solid/Add/Viewer
```

- `FlowCV_Manager` 不应依赖或持有 Qt 画布。
- `VisionFlowGraphModel` 是 **适配模型**，不是第二个运行图。
- 编辑器可以缓存节点的 `QPointF` 等 **界面状态**；不得缓存另一份被当作运行权威的节点和连接集合。
- 主窗口持有 Manager 的生命周期必须长于 GraphModel / Scene / Editor 及相关 ViewerPanel 引用。关闭窗口时停止必要的自动执行、销毁依赖者，再析构 Manager。
- 首版所有 **修改运行图** 的 UI 操作安排在 Qt 主线程；不得在后台处理线程同时直接增删图结构而无同步。

### 2.2 QtNodes 选型

**使用**：`QtNodes::AbstractGraphModel` + `QtNodes::BasicGraphicsScene` + `QtNodes::GraphicsView`。

**不使用**：`QtNodes::DataFlowGraphModel` 管理真实计算。它自带 Qt 节点数据传播行为，与现有 DSPatch Circuit 重复。

首版不需自定义节点绘制器、连接 Painter、Scene 布局引擎。优先使用 QtNodes 的默认外观、缩放、拖动、连接交互。

---

## 3. P0-1：实现 VisionFlowGraphModel（先于 UI 主窗口）

### 3.1 建议文件

```text
editor/VisionFlowGraphModel.h        # 新增
editor/VisionFlowGraphModel.cpp      # 新增
editor/CMakeLists.txt                # 修改：添加文件、Qt、AUTOMOC
```

上述文件也可以放入 `editor/graph/`，但不是强制的；不要为了目录分组而创建大量空 CMake 子项目。

### 3.2 核心类设计

```cpp
class VisionFlowGraphModel : public QtNodes::AbstractGraphModel
{
    Q_OBJECT
public:
    explicit VisionFlowGraphModel(FlowCV::FlowCV_Manager& manager,
                                  QObject* parent = nullptr);

    // 实现当前仓库 AbstractGraphModel.hpp 的全部纯虚接口。
    // 下方只是设计轮廓，不代表可直接编译的完整头文件。

private:
    FlowCV::FlowCV_Manager& manager_;  // 非拥有引用

    // 仅存 GUI ID 与 Manager 实际 uint64_t ID 的映射，以及节点位置。
};
```

**约束**：不得在这个类里 `std::make_shared<Solid>()` 或重新创建 DSPatch Circuit。

### 3.3 关键接口与 Manager 映射

| QtNodes API | 正确行为 | Manager / 数据来源 |
|---|---|---|
| `newNodeId()` | 申请未使用的 QtNodes ID；**不得仅因调用此函数就在 Manager 创建节点** | Editor 自有 ID 分配 |
| `addNode(QString)` | 使用真实类型名调用 Manager；成功后建立 Qt ID 映射并通知画布 | `CreateNewNodeInstance` |
| `allNodeIds()` | 返回现存 Manager 节点对应的 Qt ID | `GetNodeCount` / `GetNodeInfoByIndex` |
| `nodeExists()` | 通过映射校验真实节点存在 | `GetNodeInfoById` |
| `deleteNode()` | 执行 Manager 删除；成功后发出关联连接删除及节点删除通知 | `RemoveNodeInstance` |
| `nodeData()` | 根据 `NodeRole` 返回名称、位置、入/出端口数、样式等 | `NodeInfo` + GUI 自身位置表 |
| `setNodeData()` | 处理 `Position`、必要的 `Size` 等界面数据 | 编辑器本地状态 |
| `portData()` | 根据 `PortRole` 返回类型、名称、连接策略；不是实际图像数据 | `GetInputType/Name`、`GetOutputType/Name` |
| `setPortData()` | 未使用 QtNodes 数据传输时原则上不支持数据写入，返回 false | 无需调用处理器 |
| `allConnectionIds()` | 返回某节点关联真实 Wire 对应的四元组 | `GetNodeConnectionsFromId` |
| `connections()` | 返回指定端口的连接集合 | Manager Wire 查询后过滤 |
| `connectionExists()` | 查询运行图中是否存在相同四元组 | Manager Wire 查询 |
| `connectionPossible()` | 验证真实节点、端口、类型、相同连接等 | 见 3.6 |
| `addConnection()` | 先调用 `ConnectNodes`；成功再通知 QtNodes | `ConnectNodes` |
| `deleteConnection()` | 先调用 Manager 断线，成功再通知 | `DisconnectNodes` |
| `saveNode()` / `loadNode()` | 关系到 QtNodes 原生 Undo/Redo/复制粘贴；首版务必明确限制，不能假实现 | 见 3.9 |

注意：`BasicGraphicsScene` 通过 `nodeCreated`、`nodeDeleted`、`connectionCreated`、`connectionDeleted`、`nodePositionUpdated`、`modelReset` 等 Qt 信号更新画布。**Manager 修改成功 ≠ Qt 图形会自动刷新**，必须按模型规则发正确的信号。

### 3.4 QtNodes NodeId 与 Manager uint64_t ID

当前源码：

- Manager：`NodeInfo::id` 是 `uint64_t`，当前分配从 1001 开始，按 1000 间隔。
- QtNodes：`QtNodes::NodeId` 是 `unsigned int`（通常 32 位）；`InvalidNodeId` 是最大值。

**建议：Editor 层使用双向映射，不修改 Manager 的 ID 规则**。

```text
QtNodes::NodeId  ↔  uint64_t FlowCV NodeId
```

至少需要：

- Qt ID → Manager ID 的转换与存在性检查。
- Manager ID → Qt ID 的转换，避免重复分配。
- Qt ID 单调生成、跳过无效哨兵值和冲突。
- 创建成功后再入映射；删除成功后删映射及位置。
- 载入运行图时重新建立映射；**位置保存使用 Manager 64 位 ID**，不依赖 Qt 临时 ID。

原因：简单直接 `static_cast<QtNodes::NodeId>(uint64_t)` 在大 ID 项目文件中可能截断；QtNodes 自带的 JSON Undo 还会记录 Qt NodeId，二者生命周期不能混淆。

### 3.5 端口信息与类型

- `NodeInfo.desc.input_count` / `output_count` 用作 `InPortCount` / `OutPortCount`。
- `node_ptr->GetInputName(index)` / `GetOutputName(index)` 用作端口 Caption。
- `node_ptr->GetInputType(index)` / `GetOutputType(index)` 用作端口类型。
- 在 QtNodes 的 `PortRole::DataType` 上返回 **`QtNodes::NodeDataType`**，类型 ID 必须稳定且忠实映射 DSPatch 的 `IoType`（而非返回随意的 QString 或 `cv::Mat`）。
- `PortRole::Data` 返回空数据即可：实际帧在 DSPatch 内传输，QtNodes 不承担 `NodeData` 传播。
- `PortRole::ConnectionPolicyRole`：输出端口可连接多个下游；一个输入端口只允许一个最终连接。不要误设为输出也只能连一次。
- 仅支持现有节点静态端口；若未来有动态端口，再按 QtNodes `portsAboutToBeDeleted`/`portsInserted` 接口适配，不提前改核心。

### 3.6 连接合法性与替换（高风险）

当前 Manager 的 `ConnectNodes()` 已负责：

1. 节点 ID 检查。
2. 输入/输出端口索引检查。
3. DSPatch `IoType` 精确匹配。
4. 判断相同连接已经存在。
5. 对同一目标输入端口的新连接，成功时替换旧连接并更新 `wiring_`。

**禁止在 Editor 再实现一套 DSPatch 连接存储。**

`connectionPossible()` 至少校验：

- 两端节点在映射和 Manager 中都真实存在。
- 输出/输入方向及端口索引正确。
- `GetOutputType(...) == GetInputType(...)`。
- 避免相同四元组重复添加。
- 自连接按照原版编辑器行为拒绝。
- 关于环路：原版 UI 不等于 QtNodes DataFlowGraphModel 的默认无环算法；不得未经验证就把额外拓扑限制写入底层 Manager。首版明确规则及其影响，必要时只在 UI 侧限定。

输入连接替换必须按以下顺序：

```text
用户拖线到已有输入端口
  → UI 预检查合法
  → 记录旧连接四元组（如有）
  → Manager.ConnectNodes(新连接)
      ├── false：不改变最终 UI 模型，旧连接继续存在
      └── true ：核查 Manager 当前实际 Wire 集合
                   → 通知 QtNodes 删除已不存在的旧连接
                   → 通知 QtNodes 新连接已创建
```

不得先调用 `DisconnectNodeInput()` 再尝试 `ConnectNodes()`：若新连接失败，旧连接不应被 UI 误删。

避免对已存在的相同连接重复发 `connectionCreated` 信号；信号必须与实际集合变化一致。QtNodes 的 `ConnectionId` 是 `(outNodeId,outPortIndex,inNodeId,inPortIndex)`，**不是** Manager 的单独 `Wire::id`。

还需核对 QtNodes `ConnectionPolicy` / 鼠标拖线交互；不能只靠 UI 的 `ConnectionPolicy::One` 自动执行 Manager 连接替换，最终应以 Manager 结果为准。

### 3.7 删除连线、删除节点

**删除连线**：

- 转换 `QtNodes::ConnectionId` 到两个 Manager ID / 端口。
- `DisconnectNodes()` 返回成功后，再 `emit connectionDeleted()`。
- 若目标不存在或操作失败，不能假装成功删除。

**删除节点**：

- 删除前收集其实际关联连线。
- 调用 `RemoveNodeInstance()`；该函数已经处理 DSPatch 组件及关联 Manager Wire。
- 成功后先向 QtNodes 通知旧连接删除，再通知节点删除。
- 清除 Qt ID 双向映射及位置缓存。
- 不再手动调用 `Circuit::RemoveComponent()` 或重复卸载相同节点。

### 3.8 节点位置及 QtModels 同步

- 节点移动后通过 `setNodeData(NodeRole::Position, QPointF)` 保存位置。
- `nodeData(NodeRole::Position)` 直接返回位置缓存，不随意生成新的随机位置。
- 设置位置要按 QtNodes 文档发 `nodePositionUpdated`；不同方向的回调必须避免递归更新。
- 在第一次创建节点时给出合理位置：右键菜单或 View 的坐标通过 `mapToScene` 得到。
- `modelReset` 只在整体加载 / 重建时发，避免频繁全画布重建。

### 3.9 QtNodes 自带 Undo/Redo/复制粘贴：首版特殊处理

**这是已核对源码确认的真实集成风险，不可忽略。**

仓库中的 `QtNodes::GraphicsView` 默认注册 Undo、Redo、Copy、Paste、Delete 等快捷操作；其 `UndoCommands.cpp` 中：

- `CreateCommand` / `DeleteCommand` 依赖 `saveNode()` / `loadNode()`。
- `CopyCommand` / `PasteCommand` 依赖节点完整序列化和 ID 改写。
- `ConnectCommand` / `DisconnectCommand` 通过模型增删实际连接。

而 Manager 当前只有“创建新 ID 节点”的公开 API，不能简单通过 `addNode()` 保证撤销恢复后原 Manager ID 不变；属性、连接也不能仅靠 Qt 图形序列化自动恢复。

首版**二选一，优先方案 A**：

- **方案 A（推荐）**：P0/P1 先明确禁止未实现的 Undo/Redo、复制粘贴、重复节点等 UI 快捷操作及菜单入口。必须检查 `GraphicsView` 已安装的 QAction 和继承可覆盖的操作，确保实际不可触发；删除、创建、连线仍通过模型正常完成。禁止只隐藏菜单但仍保留 Ctrl+Z/Ctrl+V 有效。
- **方案 B**：若选择首版同时支持这些功能，需完整定义 `saveNode/loadNode` 的节点类型、参数、GUI ID/Manager ID、关联连接、恢复次序、失败处理及命令入栈语义，先验证再宣称支持。不得用空 `{}` 假实现。

**不得修改 QtNodes vendor 源码绕开此问题**。如要禁止部分快捷操作，使用可控的 View 派生类、QAction 禁用和 Qt 公开扩展点，不要用脆弱的文本匹配硬编码隐藏动作而不检查行为。

### 3.10 P0-1 验收

- [ ] 在不改 DSPatch 的前提下可以从 Model 创建 Solid、Add、Viewer。
- [ ] Manager 的 `GetNodeCount()` 与画布实际节点数一致。
- [ ] 管理器创建成功才触发 `nodeCreated`。
- [ ] 端口名称/数量/类型与节点实际定义一致。
- [ ] 相同连接不会产生重复图形线。
- [ ] 不兼容端口连线失败后无幽灵连接。
- [ ] 输入端口替换连接后，旧线消失且 Manager 仅保留新线。
- [ ] 一个输出可以连接多个输入。
- [ ] 删除节点及线条后，Manager/画布同步。
- [ ] GUI ID 映射及大 ID 处理不存在静默截断。
- [ ] 未完成的 QtNodes 撤销/复制/粘贴操作无法意外触发。

---

## 4. P0-2：VisionFlowEditor 和 Qt 主窗口

### 4.1 文件建议

```text
editor/VisionFlowEditor.h/.cpp                  # 完成现有占位文件
app/VisionFlowStudio/MainWindow.h/.cpp          # 新增，承载 Qt 主窗口
app/VisionFlowStudio/main.cpp                    # 从控制台入口逐步转为 Qt 程序
editor/CMakeLists.txt                            # 新增 GraphModel / Qt 源码
app/VisionFlowStudio/CMakeLists.txt              # 新增 MainWindow / Qt 依赖
```

第一版推荐布局：

```text
┌──────────────── VisionFlowStudio ─────────────────┐
│ 文件/操作工具栏：新建、打开、保存、单次执行       │
├──────────────┬────────────────────┬──────────────┤
│ 节点列表     │  QtNodes 画布      │ 属性面板     │
│ Solid        │                    │              │
│ Add          │  节点 + 端口 + 线  │ 当前选中节点 │
│ Viewer       │                    │              │
├──────────────┴────────────────────┴──────────────┤
│ Viewer 图像显示（P1）                            │
└───────────────────────────────────────────────────┘
```

先用 `QMainWindow` + 合理的 `QDockWidget` 或 `QSplitter` 组合即可，不要求自定义主题系统或复杂工具栏。

### 4.2 核心动作

1. `QApplication` 正常启动，创建唯一 Manager 实例。
2. 创建 `VisionFlowGraphModel` → `BasicGraphicsScene` → `GraphicsView`，确保生命周期顺序正确。
3. 在节点列表或右键菜单中列出 `InternalNodeManager` 可创建的节点类型（首版至少 Solid、Add、Viewer）。
4. 选择节点类型后，调用 GraphModel 的创建路径，确认 Manager 成功后才显示图形。
5. 可移动、选中、连线、断线、删除节点。
6. 选择节点时通知 PropertyPanel（P1）；选择 Viewer 时通知 ViewerPanel（P1）。
7. 菜单“运行一次”先使用现有 `Tick(Series)` 完成同一工作流，不自行发明调度机制。

**避免**把 `main.cpp` 变成大型 GUI 控制逻辑集合。应用装配在 `main.cpp`，窗口和组件交互在 Editor/MainWindow 内。

### 4.3 不遗失已有最小闭环

原控制台 `main.cpp` 已验证：

- `Solid A = 30`、`Solid B = 40`；
- 两个 Solid 输出灰度 `640 × 480`；
- 连接到 Add 的 `[0]` 和 `[1]`，Add 输出到 Viewer `[0]`；
- `GetNodeCount() == 4`、`GetWireCount() == 3`；
- 同步 `Tick(Series)` 后 Viewer 帧为 `CV_8UC1`，最小/最大值都为 `70`。

将原入口替换为 GUI 时应保留这组验收值和可复现方法。若有必要保留 CLI smoke 模式，应选择对现有结构影响最小的方式（例如可选命令行入口），**不要提前实现独立 ProcessEngine 或新 Runner**。

### 4.4 P0-2 验收

- [ ] `VisionFlowStudio` 显示 Qt 主窗口并进入事件循环。
- [ ] 内置节点菜单可列出 Solid、Add、Viewer。
- [ ] 通过 UI 创建 4 个节点与 3 条连接，图形与 Manager 一致。
- [ ] 鼠标拖动节点、缩放和平移画布正常。
- [ ] 删除节点/线条后 UI 和 Manager 一致。
- [ ] UI 层调用失败时给出可理解提示，而非留下半完成图形。

---

## 5. P1-1：PropertyPanel 属性面板

### 5.1 文件建议

```text
editor/PropertyPanel.h
editor/PropertyPanel.cpp
```

使用 Manager 现有的：

```cpp
bool GetNodePropertiesSnapshot(uint64_t node_id,
                               PropertiesSnapshot& snapshot);
bool SetNodeProperty(uint64_t node_id,
                     const std::string& key, bool value);
bool SetNodeProperty(uint64_t node_id,
                     const std::string& key, int value);
bool SetNodeProperty(uint64_t node_id,
                     const std::string& key, float value);
```

UI 只拿 `PropertiesSnapshot`，不接管 `FlowCV_Properties` 实例生命周期。

### 5.2 属性映射

| `PropertyDataTypes` | Qt 控件 | 行为 |
|---|---|---|
| `kDataTypeBool` | `QCheckBox` | 读写 `bool` |
| `kDataTypeInt` | `QSpinBox` | 整数、最小/最大值，具体步进策略不得改变底层值语义 |
| `kDataTypeFloat` | `QDoubleSpinBox` | 浮点数、范围及步长提示，转换时校验 |
| `kDataTypeOption` | `QComboBox` | `options` 作为显示项，写回选项的 `int` 索引 |
| `kDataTypeSeparator` | `QFrame` 等 | 若 Snapshot 中出现则绘制分隔符 |
| `kDataTypeUndefined` | 不可编辑占位/跳过 | 不臆造类型 |

所有控件显示遵从 `PropertySnapshot.visible`；标签优先使用 `desc`，数据绑定使用稳定 `key`。

### 5.3 修改流程

```text
选中 Qt 节点
    → 映射为 FlowCV uint64_t ID
    → Manager.GetNodePropertiesSnapshot
    → 构造/更新 Qt 控件
    → 用户修改
    → Manager.SetNodeProperty
    → 读取新快照并按 visible/options/range 刷新
```

- 不直接调用节点的 `Process_()`。
- `SetNodeProperty()` 改写的是编辑值；真实运算路径原有 `properties_.Sync()` 仍负责生效。
- `Revision` 只在**当前属性实例内部**有意义，切换节点必须获取新的快照，不能把不同节点的 revision 直接比较。
- 对于 Solid，`is_color` 和 `has_alpha` 影响其他属性可见性，应在成功写入后刷新快照，**不要把 Solid 特例规则写进 PropertyPanel**。
- 整数/浮点属性范围是 UI 编辑提示，底层 `Set()` 并不统一强制裁剪；UI 应避免产生非法值，不得借此更改 SDK 运行逻辑。
- 如果节点属性为空（如 Viewer），显示合适空状态，而非空指针异常。
- 避免控件程序性设置值时再次触发写入循环（例如 `QSignalBlocker`）。

### 5.4 验收

- [ ] 选择 Solid 后能修改 `width`、`height`、`is_color`、`value`、颜色、帧率选项。
- [ ] Solid 的动态可见性正确。
- [ ] 选择 Add 后能修改 `mask_mode`。
- [ ] 选择 Viewer 后不报错，能显示无通用属性提示。
- [ ] 参数变更通过 Manager，新的单次 Tick 能反映修改。
- [ ] 选中节点切换不会把参数写入之前的节点。

---

## 6. P1-2：ViewerPanel 与第一次 GUI 运行闭环

### 6.1 文件建议

```text
editor/ViewerPanel.h
editor/ViewerPanel.cpp
```

查看帧的唯一路径优先是：

```text
Manager.GetNodeInfoById(viewerId)
  → dynamic_pointer_cast<DSPatch::DSPatchables::Viewer>
  → Viewer::GetFrameCopy()  // 加锁、深拷贝
  → 转换 QImage
  → QPixmap / QLabel 或合适的 Qt 图像控件
```

不修改 `Viewer::Process_()` 以直接显示 Qt 界面；不让 Viewer 节点继承 QWidget。

### 6.2 图像转换

必须至少支持本阶段闭环的 `CV_8UC1`；建议顺便规范支持：

- `CV_8UC1` → `QImage::Format_Grayscale8`。
- `CV_8UC3`（OpenCV BGR）→ `QImage::Format_BGR888` 或可靠的颜色转换。
- `CV_8UC4`（OpenCV BGRA）→ 合适的 Qt BGRA 兼容格式或转换。

注意：

1. `QImage` 不能长期借用已释放的 `cv::Mat` 数据。必要时 `copy()` 使图像独立拥有缓存。
2. 使用 Mat 的 `step` / bytesPerLine，不能假定所有矩阵内存都无行填充。
3. 为空图像或不支持类型时给出清晰提示；不要崩溃。
4. 图像显示缩放保持长宽比，不影响真实像素值。
5. 即便 Viewer 节点构造时含默认黑图，也不能以“非空帧”认定流程已经执行成功。

### 6.3 执行入口：先同步，后连续

**第一版**：

- GUI 提供“运行一次”按钮。
- 使用与原控制台闭环一致的 `SetBufferCount(0)` 和 `Tick(DSPatch::Component::TickMode::Series)` 组合（以实际已验证配置为准）。
- Tick 结束后取 Viewer 深拷贝并刷新 UI。
- 不将 `QTimer` 当作替代 DSPatch 的计算调度器。

**注意 UI 阻塞边界**：Solid 当前保留原版 FPS 等待逻辑；GUI 线程同步 Tick 可能短暂停顿。首个功能闭环允许单次执行，但在需要连续运行时必须评估响应性，不能直接在 GUI 线程进行无限循环。

**后续连续运行**：可按已有 `StartAutoTick()` / `StopAutoTick()` 执行、通过 Qt 定时器仅刷新预览；需验证与动态增删图结构的同步，不新增第三套执行线程系统。未验证前不标注“连续运行已支持”。

### 6.4 P1 总验收：GUI 精确复现最小闭环

通过图形编辑完成：

```text
Solid A (is_color=false, width=640, height=480, value=30)
         ──[out 0]──────┐
                        ├──> Add (input 0 / input 1)
Solid B (is_color=false, width=640, height=480, value=40)
         ──[out 0]──────┘          │
                                   └──[out 0]──> Viewer (input 0)
```

严格断言/手工核对：

- 4 个 Manager 节点，3 条 Manager Wire。
- `Viewer::GetFrameCopy()` 得到 `640 × 480`、`CV_8UC1`。
- `cv::minMaxLoc` 的 `min == max == 70`（可在日志或现有诊断工具中验证，不一定把数值嵌进正式 UI）。
- ViewerPanel 实际显示对应图像。
- 从任一 Solid 改值后再次 Tick，输出会对应改变。
- 删除 Add→Viewer 连接后，管理器连接集合立即正确更新，画布不残留旧线；观察 Viewer 可能有原库缓存行为，不因此擅自修改节点逻辑。

---

## 7. P2：项目状态与 UI 布局保存/恢复

### 7.1 原则：运行图由 Manager 保存，位置由 Editor 保存

Manager 当前 `GetState()` / `SetState()` 负责 `nodes` 与 `connections`，其中节点含 `name`、`id`、`num`、`enabled`、`params` 等；**不应为了位置保存而往 Manager 强塞 Qt 坐标**。

参照原版 FlowCV，完整项目 JSON 可在上层附加 `editor` 字段：

```json
{
  "nodes": [
    {
      "name": "Solid",
      "id": 1001,
      "num": 0,
      "enabled": true,
      "params": { "width": 640, "height": 480 }
    }
  ],
  "connections": [],
  "editor": [
    { "id": 1001, "location": { "x": 100.0, "y": 200.0 } }
  ]
}
```

这是字段示例，实际节点参数请沿用 `GetState()` 返回值，不手工拼写 Solid 的完整参数表。`editor` 应用 **Manager uint64_t ID** 关联节点；不要使用会因画布重建改变的 Qt ID。

### 7.2 保存顺序

1. 读取 `manager.GetState()`。
2. 从 GraphModel 获取 Manager ID → `QPointF` 的位置。
3. 组合 `editor: [{id, location:{x,y}}]`。
4. 在应用层写入完整 JSON。
5. 对空路径、IO 失败、JSON 序列化失败给出错误反馈。

注意：当前 `manager.SaveState(path)` 只写 Manager 自己的状态，**不会自动含 UI 位置**。因此完整 GUI 项目保存不可只调用它然后假定成功。

### 7.3 加载顺序

1. 解析 JSON，并确认顶层对象、核心字段形态正确。
2. 视需要在执行编辑器层协调时停止自动 Tick；不要重构内部 DSPatch 调度。
3. `manager.SetState(fullJson)`：其原有逻辑负责恢复节点和连线，通常会忽略额外 `editor` 字段。
4. 检查恢复结果；即使返回 false，也不能假定旧运行图还存在（目前非事务式加载）。
5. **以 Manager 当前实际节点/连接重新建立 Qt ID 双向映射并重建 Scene**。
6. 根据每条 `editor.id` 对应的 Manager ID 恢复位置；找不到位置时给默认布局，不创建不存在的节点。
7. 刷新属性选中状态和 ViewerPanel；清理旧的临时 UI 选择。

建议直接使用现有 `nlohmann::json` 进行整个项目文件处理，以保留 64 位整型 ID；避免通过可能丢失整数精度的 Qt JSON 浮点转换中转 Manager ID。

### 7.4 失败与兼容性

- 当前 `SetState()` 是 **清空后恢复**，不提供原子回滚；UI 必须按实际结果重建，不伪称“失败后原项目完全保留”。
- 没有 `editor` 字段的旧 FlowCV 工作流仍应正常载入，节点使用默认布局。
- `editor` 含未知节点 ID 时忽略并告警，不修改 Manager 数据。
- 本阶段不强制保存复杂 Dock 布局、缩放比例、个人设置、窗口几何；节点位置优先。
- 加载后如启动连续处理，必须待图与界面恢复一致再启动。

### 7.5 P2 验收

- [ ] 保存的项目至少包含 `nodes`、`connections`、`editor`。
- [ ] 重启应用后恢复 4 节点、3 条连线、参数及其位置。
- [ ] 再次 Tick 仍输出正确像素值。
- [ ] 缺少 `editor` 字段的文件能打开。
- [ ] 部分损坏的 JSON / 不存在节点能明确报告失败，不发生崩溃或虚假的完整成功提示。

---

## 8. 明确不做的内容与后续迭代入口

本轮 **不要**将以下工作提前写进 P0/P1：

1. 重构 DSPatch 调度、Circuit 管理方式、TickMode、线程池。
2. 扩展新运行状态机（`Stopped/Running/Paused` 等）。
3. 修改 Solid/Add/Viewer 原有算法、fps 控制、缓存时效语义。
4. 将各 Node 变成 QWidget 或为每类节点手写独立参数对话框。
5. 移植 ImGui/ax::NodeEditor 的端口 ID 偏移策略（QtNodes 已有 ConnectionId）。
6. 为 QtNodes 添加另一套数据传播、缓存与 Tick 机制。
7. 改写 Manager GetState/SetState 来承载所有 UI 布局。
8. 在没有实际插件端到端运行前声明“插件可视化支持已完善”。
9. 完整 Undo/Redo、Clipboard、Copy/Paste、节点分组、自动布局、搜索快捷键、精细主题、Dock 工作区恢复。
10. CI / 单元测试框架 / 安装打包 / SDK 公开 ABI 重整（本轮聚焦 GUI 功能闭环；允许必要的轻量验收）。

对未来可能需要的 API，只记录设计接口和 TODO；无调用场景时不新增空模块。

---

## 9. CMake / Qt / Release 配置规范

### 9.1 当前构建事实

- C++20 / CMake ≥3.25；Windows x64 / MSVC / Ninja。
- 项目根 CMake 已在 `VISIONFLOW_BUILD_STUDIO=ON` 时查找 Qt 6 Core、Gui、Widgets、OpenGL。
- QtNodes 为项目内固定源码并构建为静态库，不构建其 examples/tests/docs。
- `editor/CMakeLists.txt` 已创建 `VisionFlowEditor`，链接 `VisionFlow::Sdk`、`VisionFlow::Managers`、`QtNodes::QtNodes`。
- `app/VisionFlowStudio/CMakeLists.txt` 已创建 `VisionFlowStudio`，尚未纳入 Qt MainWindow。
- 机器本地的 `local-x64-release` 预设来自 **`CMakeUserPresets.json`**，该文件不入库；不能在共享 CMake 文件里硬编码本机 SDK 路径。
- Release OpenCV 没有 Debug 对应库，本任务不要求 Debug 构建。

### 9.2 修改动作

- 在 `VisionFlowEditor` 目标上显式开启 `AUTOMOC`，例如：

  ```cmake
  set_target_properties(VisionFlowEditor PROPERTIES AUTOMOC ON)
  ```

- 添加新 Editor 头/源文件到明确的目标源文件清单；`Q_OBJECT` 类需确保 AUTOMOC 能扫描。
- 哪个目标直接使用 Qt Widgets/Gui/Core，就明确通过 `target_link_libraries` 链接对应 `Qt6::Widgets`、`Qt6::Gui`、`Qt6::Core` 等实际依赖。
- App 中新增 `MainWindow.cpp/.h` 并加入 CMake。
- 不使用 `file(GLOB_RECURSE)` 自动扫描替代现有显式源码清单。
- 不改变 `VISIONFLOW_BUILD_STUDIO=OFF` 时的无 Qt 构建边界。
- Windows 的运行时 DLL 复制机制沿用现有实现；如遇 Qt 平台插件 `qwindows` 缺失，应解决部署/运行路径问题，不修改计算核心。

### 9.3 构建与启动命令（具备本机预设时）

```powershell
cmake --preset local-x64-release
cmake --build --preset local-x64-release
& ./out/build/local-x64-release/app/VisionFlowStudio/VisionFlowStudio.exe
```

如果 Codex 运行环境缺少 Windows/MSVC/Qt/OpenCV 或本地 `CMakeUserPresets.json`，应如实说明未能实际构建，提供需要用户在 Windows 端执行的命令；不得声称编译和 GUI 验收已经通过。

---

## 10. 建议文件变更清单

| 阶段 | 文件 | 类型 | 内容 | 对计算核心影响 |
|---|---|---|---|---|
| P0-1 | `editor/VisionFlowGraphModel.h/.cpp` | 新增 | QtNodes 适配、ID、连接、位置信号 | 无 |
| P0-1 | `editor/CMakeLists.txt` | 修改 | 新文件、AUTOMOC、Qt 目标依赖 | 无 |
| P0-2 | `editor/VisionFlowEditor.h/.cpp` | 完善 | Scene/View、节点列表或右键菜单、选中事件 | 无 |
| P0-2 | `app/VisionFlowStudio/MainWindow.h/.cpp` | 新增 | 主窗口和菜单/布局 | 无 |
| P0-2 | `app/VisionFlowStudio/main.cpp` | 修改 | Qt 入口，尽量保留原闭环验收方式 | 不应影响运行引擎 |
| P0-2 | `app/VisionFlowStudio/CMakeLists.txt` | 修改 | 新文件/Qt 依赖 | 无 |
| P1-1 | `editor/PropertyPanel.h/.cpp` | 新增 | 快照驱动的通用属性编辑器 | 无 |
| P1-2 | `editor/ViewerPanel.h/.cpp` | 新增 | Viewer 帧显示和转换 | 无 |
| P2 | Editor/App 对应现有文件 | 扩展 | 项目完整 JSON 和 GUI 布局存取 | 无 |
| 每阶段 | `README.md`、`docs/architecture.md` | 按实际更新 | 真实的完成状态与编译运行说明 | 无 |

只有在明确证明上述接口无法满足需求时，才考虑 **最小必要** 修改 `managers/`；必须在变更记录中写出“为什么非改不可、是否与原版冲突、有哪些调用点受影响”。

---

## 11. 执行顺序（Codex 请严格按顺序）

### Step 0：阅读和核对

- [ ] `git status`、`git log -1`、当前分支及未提交改动。
- [ ] 对照本文第 1 节文件核对所有接口和现存实现。
- [ ] 先记录 QtNodes 具体版本/API：`AbstractGraphModel`、`BasicGraphicsScene`、`GraphicsView`、`UndoCommands`。
- [ ] 对需要修改的 Manager/SDK 逻辑先给出必要性证据，默认不修改。
- [ ] 建立阶段性变更清单，不覆盖用户现有未提交代码。

### Step 1：完成 GraphModel（P0-1）

- [ ] 实现全部必须的 QtNodes 抽象接口。
- [ ] 构建 GUI ID ↔ Manager ID 映射、节点位置存储。
- [ ] 从 Manager 查询真实节点和 Wire；端口返回稳定 `NodeDataType`。
- [ ] 完成节点创建、删除、连接校验、成功信号和失败保护。
- [ ] 明确关闭暂未可用的 Undo/Redo/复制粘贴，避免错误恢复真实节点。
- [ ] 编译通过、验收 3.10 中各项。

### Step 2：完成画布和主窗口（P0-2）

- [ ] 创建 Qt application 和窗口。
- [ ] GraphModel → Scene → View 装配；正确管理对象销毁顺序。
- [ ] 添加 Solid/Add/Viewer 菜单，支持鼠标新建及移动。
- [ ] 支持连接、删除和节点选中。
- [ ] UI 上可构建 4 节点 3 连线，与 Manager 一致。

### Step 3：完成属性面板（P1-1）

- [ ] 通用控件映射 Bool/Int/Float/Option。
- [ ] 支持 visible/range/options 和不同节点选择切换。
- [ ] 通过 Manager 写回属性并验证 Sync 生效。

### Step 4：完成单次执行及 Viewer（P1-2）

- [ ] 由 GUI 按钮触发现有 `Tick(Series)`。
- [ ] Viewer 深拷贝 → QImage 展示，至少支持 Grayscale8。
- [ ] 验证输出图像 `640×480 CV_8UC1`、像素值全为 `70`。
- [ ] 修改参数后再次运行，预览更新。

### Step 5：完整项目保存和恢复（P2）

- [ ] Manager JSON + `editor` 位置，使用 Manager 64 位 ID。
- [ ] 保存/打开对话框，错误检查，支持无 `editor` 字段的旧工作流。
- [ ] 加载后重新构建 ID 映射/画布、恢复位置、清理旧 UI 状态。
- [ ] 重开程序复验 4 节点 3 连接及相同输出。

### Step 6：收尾交付

- [ ] 更新 README 和 `docs/architecture.md`，只陈述实际已实现的功能。
- [ ] 提供变更文件列表和核心实现说明。
- [ ] 提供真实构建命令、运行结果、失败项和未验证项。
- [ ] 给出下一轮优先问题，但不顺带实施 P3 高级功能。

---

## 12. 关键验收矩阵

| 编号 | 场景 | 期望结果 | 最迟阶段 |
|---|---|---|---|
| A01 | 在空画布创建 2 Solid、1 Add、1 Viewer | 画布 4 节点，Manager 4 节点 | P0-2 |
| A02 | 连 Solid A/B 到 Add，再 Add 到 Viewer | 画布 3 线，Manager 3 Wire | P0-2 |
| A03 | 输出连不匹配端口或无效节点 | 拒绝连接，不出现幽灵 Wire | P0-2 |
| A04 | 将某输入改连另一上游 | Manager 只有一条最终输入线；画布旧线删除 | P0-2 |
| A05 | 将同一输出连多个下游 | 合法场景可以同时存在 | P0-2 |
| A06 | 删除一条线 | UI 和 Manager 同时删除 | P0-2 |
| A07 | 删除有连接的节点 | 关联 Wire/画布线全部同步移除 | P0-2 |
| A08 | 修改 Solid 的颜色/灰度切换 | 属性可见性正确更新 | P1-1 |
| A09 | 修改 Solid 值为 30、40 并执行 | Viewer 帧全像素 70 | P1-2 |
| A10 | Viewer 无输入或显示前为空 | 不崩溃；尊重 Viewer 原有缓存行为 | P1-2 |
| A11 | 保存、关闭、重开并加载 | 节点/线/属性/位置恢复 | P2 |
| A12 | 打开不带 `editor` 字段的状态文件 | 图可恢复，位置给默认值 | P2 |
| A13 | 加载 JSON 错误/未知节点 | 明确报错，画布与实际 Manager 一致 | P2 |
| A14 | Ctrl+Z/Ctrl+V 在尚未完成支持时 | 不触发破坏性错误恢复 | P0-2 |
| A15 | `VISIONFLOW_BUILD_STUDIO=OFF` | 不因此引入 Qt 强依赖 | 每阶段 |
| A16 | Release 构建 | 不新增 Debug OpenCV 依赖 | 每阶段 |

**每阶段验证报告必须区分**：静态代码审查通过、编译通过、真实 GUI 交互通过、受环境限制未验证。不得用源码分析代替运行验收。

---

## 13. 已知集成陷阱与禁止做法

1. **QtNodes `NodeId` 是较窄整数**：不能无检查地把 Manager 的 `uint64_t` 强转。建议 GUI ID 映射，保存以 Manager ID 为准。
2. **QtNodes 自身有连接与 Undo 命令**：不代表其真实节点恢复自动适用于 FlowCV；严禁空 `saveNode/loadNode` 配上可点击 Undo/Paste。
3. **`ConnectionId` 不是 `Wire::id`**：图连接按四元组辨认；应使用 Manager 的实际连接列表转换。
4. **Manager 的输入连接替换是现有行为**：不要提前手动断旧连接造成失败时掉线。
5. **Manager 的 `SetState()` 非事务式**：载入失败后需按实际图重建，不可继续显示旧画布。
6. **Viewer 默认有一张黑色缓存图像**：非空帧不证明 Add 输出已生效。
7. **`cv::Mat` 和 `QImage` 内存引用问题**：图像跨函数/帧保留时需要确保独立拥有底层缓冲区。
8. **BGR/RGB 混淆**：彩色预览必须正确处理 OpenCV 通道顺序。
9. **AUTOMOC 目标范围**：QtNodes 自己设置 AUTOMOC 不代表 Editor 自己的 `Q_OBJECT` 自动处理完成。
10. **PropertySnapshot 的 revision 不是全局版本号**：不能拿不同节点的 revision 判定快照是否相同。
11. **UI 连续处理/结构编辑并发**：初版优先串行单次 Tick；以后连续运行再实测安全边界。
12. **Qt 6 可用不代表启动环境完整**：MSVC Qt 平台插件/运行时 DLL 部署需真实验证。
13. **GUI 更名/拆文件不是借口重写原有底层**：仅改实现编辑器必要的模块。
14. **原版 FlowCV 的 ImGui 端口生成方式无需照搬**：QtNodes 的 ConnectionId 已体现端口索引。

---

## 14. Codex 每阶段输出格式要求

完成每个 Step，向用户报告以下信息：

```text
[阶段名称]
1. 本阶段目标
2. 实际修改的文件列表（新增/修改/删除）
3. 与原版 FlowCV 的行为对应关系
4. 是否修改 managers/sdk/nodes/DSPatch；如有，逐项解释必要性
5. 关键接口/数据流实现说明
6. 编译命令和真实结果（成功 / 失败 / 环境受限）
7. 手工或自动验收项结果（通过 / 失败 / 未验证）
8. 已知问题和下一阶段入口
```

若某一步遇到接口与假设不符，不得自行扩展为大型重构；应采用可编译的最小方案并在阶段报告中列出权衡及尚未完成事项。

---

## 15. 可直接复制给 Codex 的首轮执行指令

> 请阅读仓库当前最新的 `README.md`、`docs/architecture.md`、`managers/flowcv_manager.h/.cpp`、`managers/node_manager.h/.cpp`、`sdk/flowNode.h`、`sdk/flowProperties.h`、`editor` 现有文件，以及固定在 `third_party/nodeeditor` 的 QtNodes 源码。再按《VisionFlowStudio 节点编辑器阶段开发任务书》执行。务必尊重原 FlowCV 的 DSPatch 和 Manager 行为，只完成必要的 Qt/QtNodes 适配。
>
> **第一轮只做 Step 0、Step 1、Step 2（P0）**：实现 `VisionFlowGraphModel` 和可操作的 `VisionFlowEditor`/主窗口，支持 Solid、Add、Viewer 的可视化创建、移动、连线、删除，并保持 UI 与真实 Manager 同步。不要提前修改 Solid/Add/Viewer 算法、扩展属性类型、重构 DSPatch、引入额外状态机或完成 P1/P2/P3 全部功能。对于 QtNodes 内置 Undo/Redo/复制粘贴，若尚未实现正确的真实节点恢复，应彻底禁用对应入口。所有 CMake 变更采用显式文件清单，并保持 Release/`VISIONFLOW_BUILD_STUDIO=OFF` 的现有配置边界。
>
> 完成时说明改动文件、编译是否真实通过、P0 验收结果，以及下一轮实施 P1 的具体入口。若执行环境无法构建 Windows GUI，如实报告，不得假称测试成功。

---

## 16. 最终阶段完成定义（Definition of Done）

本任务书对应的 **节点编辑器阶段** 达成的标志是：

1. 开发者能够用 QtNodes 画布 **不写 C++ 硬编码节点创建语句**，自行搭建现有 `Solid A (30) + Solid B (40) → Add → Viewer`。
2. 在属性面板中设置两路灰度图大小、灰度值等，使用已有 `FlowCV_Properties` 数据通路，UI 和计算逻辑解耦。
3. 使用现有 DSPatch 机制完成单次执行，ViewerPanel 正确显示 `640×480 CV_8UC1`、全部像素值 `70` 的图像。
4. 节点/连线增删和输入替换后，QtNodes 与 `FlowCV_Manager` 的真实图保持一致。
5. 能够保存完整工作流及节点位置，并在重新打开后恢复可再次执行的图。
6. `sdk`、`nodes`、`managers`、DSPatch 未因更换 UI 引入不必要的结构变更；Release 构建依赖关系正确。
7. 未支持的高级交互不会误导用户或破坏真实运行图；文档、阶段报告与实际实现保持一致。

**阶段完成后，再单独计划连续运行、完整 Undo/Redo、复制粘贴、更多节点、插件 UI 和用户体验完善。**
