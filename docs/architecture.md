# 项目架构

本文描述当前源码的模块关系、运行路径和接口边界。实现状态与验证依据见 [开发进度](development-status.md)，操作步骤见 [Studio 使用指南](studio-guide.md)，构建环境与依赖来源见 [第三方依赖](../third_party/README.md)。

## 模块职责

| 模块 | 职责与主要入口 |
| --- | --- |
| `Utils` | 日志等公共辅助功能；[logging](../Utils/logging/logger.h) 使用 spdlog |
| `sdk` | [公共类型](../sdk/flowTypes.h)、[属性存储](../sdk/flowProperties.h)、[Node 基类](../sdk/flowNode.h)；内部构建 DSPatch |
| `nodes` | Solid、Add、Viewer 的计算实现；依赖 SDK 与 OpenCV，不创建 Qt 窗口 |
| `managers` | [FlowCV_Manager](../managers/flowcv_manager.h) 管理运行图，[InternalNodeManager](../managers/node_manager.h) 管理内置工厂，[PluginManager](../managers/plugin_manager.h) 管理动态插件 |
| `editor` | [VisionFlowEditor](../editor/VisionFlowEditor.h) 组织画布交互，[VisionFlowGraphModel](../editor/Graph/VisionFlowGraphModel.hpp) 适配运行图，[PropertyPanel](../editor/PropertyPanel.h) 编辑参数 |
| `app/VisionFlowStudio` | [入口](../app/VisionFlowStudio/main.cpp) 装配应用，[MainWindow](../app/VisionFlowStudio/MainWindow.h) 组织 Dock，[ViewerPanel](../app/VisionFlowStudio/ViewerPanel.h) 显示结果 |
| `plugins`、`tools` | 分别预留插件实现与开发辅助工具；加载器位于 Managers |
| `third_party` | 查找外部包并构建 QtNodes；DSPatch 由 `sdk/thirdParty` 管理 |

SDK 不依赖 Managers、Nodes、Editor 或 Qt；计算节点不依赖编辑器。Managers 在构造内置工厂时直接注册具体节点，因此实际依赖 Nodes。Utils 不反向依赖业务模块。无界面入口 `app/ProcessEngine` 尚未接入可执行目标。

## 图交互与计算

```mermaid
flowchart TD
    Main["MainWindow"] --> Editor["VisionFlowEditor"]
    Editor --> Model["VisionFlowGraphModel"]
    Main --> Properties["PropertyPanel"]
    Model --> Manager["FlowCV_Manager"]
    Properties --> Manager
    Manager --> Circuit["DSPatch Circuit"]
    Circuit --> Nodes["Solid / Add / Viewer"]
    Main --> Preview["ViewerPanel"]
    Preview -->|"GetFrameCopy()"| Viewer["Viewer 缓存"]
    Nodes --> Viewer
```

QtNodes 处理节点图形、选择、拖动、连线与缩放；计算数据由 DSPatch 的 SignalBus 传递。GraphModel 的 `PortRole::Data` 返回空值，不建立第二套图像传输或计算链路。

[GraphModel](../editor/Graph/VisionFlowGraphModel.cpp) 维护 QtNodes 的 32 位 NodeId 与 Manager 的 64 位节点 ID 映射，位置和尺寸只作为画布状态缓存。节点创建、删除和连线操作调用 Manager，成功后发送 QtNodes 信号更新图形。输入端口最多连接一路，输出允许分发；新连线只接受相同 IoType，Manager 负责输入连接替换和记录同步。`rebuildFromManager()` 可在外部更改运行图后重读节点并通知画布重建，但不负责文件读取或节点位置持久化。

[VisionFlowEditor](../editor/VisionFlowEditor.cpp) 从内置注册表生成节点菜单，转发选择与图变化通知。删除操作直接调用 GraphModel；Copy、Paste、Duplicate、Undo、Redo 被禁用，因为当前没有完整的 `saveNode/loadNode` 恢复路径。它们不能仅凭 QtNodes 自带功能视为已接入。

[FlowCV_Manager](../managers/flowcv_manager.cpp) 关联管理节点实例、Circuit、`wiring_` 和各节点的输入连接映射；提供节点查询、增删、连接、属性访问及状态存取。`Tick()`、`StartAutoTick()`、`StopAutoTick()` 和缓冲设置直接调用 DSPatch，沿用其调度机制。

## 属性与节点接口

`FlowCV::Node` 继承 `DSPatch::Component`，提供 `Properties()`、`GetPropertiesSnapshot()` 和 `RefreshProperties()`。计算节点在 `Process_()` 内通过输入总线读取数据，并向输出总线写入结果；节点可覆盖 `GetState()/SetState()` 保留已有状态格式。

属性系统支持 Bool、Int、Float、Option。值类型为 `bool / int / float`，Option 使用整数索引；声明中的 Separator 目前没有公开添加接口或可编辑控件。参数的数据路径为：

1. PropertyPanel 通过 Manager 获取属性快照，为所有有可编辑属性的节点建立独立折叠分组；选择节点改变标题高亮。
2. 控件写入 `SetNodeProperty()`，进入属性编辑值；Bool / Option 的修改会排队刷新受影响分组，更新动态可见性。
3. 节点处理时调用 `Sync()`，将待生效的编辑值同步到计算读取值。`GetW()` 读取编辑值，`Get()` 读取计算值。
4. `Snapshot()` 在一次锁保护下取得属性集合与 revision。`Sync()` 不消耗界面修订；不同实例的 revision 不能直接互相比较。

属性公共操作各自加锁，不保证多个独立 Get 调用构成同一批快照。数值范围和 step 是编辑提示，`Set()` 不统一裁剪数值；节点可在计算路径中限制范围。整数控件以 1 为步进，IntRange.step 仍表示原有编辑速度提示。

默认 `ToJson()` 保存可见属性的编辑值，`FromJson()` 先验证待应用字段，再更新编辑值与计算值。Solid 使用自己的状态转换，保留 `solid_color` 等原库字段；Viewer 没有可存参数，返回空状态。三类节点的端口与处理条件见 [节点说明](../nodes/README.md)。

## 预览与对象生命周期

MainWindow 根据真实 Viewer 节点 ID 创建、更新和移除独立 Dock，以 `ViewerDock_<id>` 作为对象名，供窗口布局恢复使用。删除 Viewer 节点后，对应 Dock 隐藏并通过 `deleteLater()` 释放；隐藏预览窗口不会删除或停止计算节点。

Viewer 在处理路径缓存输入，`GetFrameCopy()` 加锁后返回深拷贝。ViewerPanel 在 Qt 线程以 33 ms 定时器读取缓存，再转换为 QImage / QPixmap 并等比适应窗口；该定时器不驱动 DSPatch。当前显示类型为 `CV_8UC1/3/4`，源节点帧率与界面刷新周期相互独立。

[main.cpp](../app/VisionFlowStudio/main.cpp) 创建 QApplication 和 Manager，将缓冲数设为 0，再创建 MainWindow，启动 AutoTick 并进入事件循环。事件循环退出后先停止 AutoTick，随后局部对象按 MainWindow、Manager 的顺序析构，Manager 比所有使用它的界面存活更久。Manager 析构再次停止执行、移除 Circuit 组件并清理节点记录。

Editor 先构造 Model，再构造 Scene、View；析构时先断开回调，再按 View → Scene → Model 的顺序释放，避免图形对象清理时访问已销毁的模型。Qt 界面对象不进入计算节点。

## 状态与持久化

运行图与界面布局分别保存，当前职责如下：

| 数据 | 所属位置与接口 | 边界 |
| --- | --- | --- |
| 节点、参数、启用状态与连线 | Manager 的 GetState / SetState / LoadState / SaveState | JSON 包含 `nodes`、`connections`；Studio 尚无文件操作入口 |
| 节点位置与尺寸 | GraphModel 的内存映射 | 尚无工程文件保存与恢复 |
| 窗口尺寸与 Dock 布局 | MainWindow 的 QSettings | 使用 Geometry / DockState 与布局版本；不包含运行图 |

Manager 的节点记录保存名称、ID、实例编号、enabled 及可选 params，连线记录保存两端节点 ID 与端口索引。载入时先检查顶层结构，再清空并逐项恢复；它不提供事务式整体回滚，失败可能留下部分新图。

应用不自动恢复上次运行图。延迟创建的 Viewer Dock 会尝试按对象名恢复布局，但这不会重建 Viewer 节点，也不能据此保证恢复整份流程工程。

## CMake 目标与依赖

下表按各模块 `target_link_libraries()` 的当前声明整理：

| 目标 | 类型 | PUBLIC 依赖 | PRIVATE 依赖 |
| --- | --- | --- | --- |
| `VisionFlowUtils` | STATIC | spdlog::spdlog | — |
| `visionflow_dspatch` | STATIC | — | Threads::Threads、CMAKE_DL_LIBS |
| `VisionFlowSdk` | STATIC | VisionFlow::DSPatch、nlohmann_json::nlohmann_json | — |
| `VisionFlowNodes` | STATIC | VisionFlow::Sdk、nlohmann_json::nlohmann_json、opencv_core | — |
| `VisionFlowManagers` | STATIC | VisionFlow::Sdk | VisionFlow::Nodes、VisionFlow::Utils |
| `VisionFlowEditor` | STATIC | VisionFlow::Managers、QtNodes::QtNodes、Qt6::Widgets | VisionFlow::Sdk、Qt6::Core |
| `VisionFlowStudio` | EXECUTABLE | — | VisionFlow::Editor、VisionFlow::Managers、VisionFlow::Utils、VisionFlow::Nodes、Qt6::Widgets |

项目静态库提供对应的 `VisionFlow::<模块名>` 别名；DSPatch 的别名为 `VisionFlow::DSPatch`。QtNodes 也作为静态库构建，但当前 Qt SDK 使用动态库。

顶层依次进入 third_party、Utils、sdk、nodes、managers、plugins；`VISIONFLOW_BUILD_STUDIO=ON` 时再进入 editor 与 app。关闭 Studio 后跳过 Qt、QtNodes、Editor 和 Studio，只生成基础库，不生成 ProcessEngine。

公开头文件需要的包含路径与依赖通过目标传播；模块目录作为 PUBLIC 包含目录不等于可安装 SDK 或稳定 ABI。Editor 公开 QtNodes 与 Managers 使用要求，并启用 AUTOMOC；SDK 与 DSPatch 不依赖 Qt。

## 扩展与源码组织

保持 FlowCV 的 DSPatch 执行与节点计算机制，通过 UI 无关的参数和图像读取接口接入 Qt。现有 `HasGui()/UpdateGui()` 类型接口仍保留，但当前应用不调用原版 ImGui 控件；插件适配边界见 [插件说明](../plugins/README.md)。

模块按功能聚合头文件与实现，CMake 显式登记源码。Utils 先创建一个公共库，各功能目录用 `target_sources()` 加入该库；[logging/CMakeLists.txt](../Utils/logging/CMakeLists.txt) 是现有示例。其他模块按其实际清单组织，不通过递归扫描自动纳入业务源码，也不提前创建空的功能库。

构建准备、依赖版本和 Windows 部署细节由 [第三方依赖说明](../third_party/README.md) 维护，待开发事项和验证范围由 [开发进度](development-status.md) 维护。
