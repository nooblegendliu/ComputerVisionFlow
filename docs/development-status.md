# 开发进度

源码基线：`b422647dd09458537b0ffb43560d911a018cc1de`，核对日期：2026-10-15。

当前项目已接入 Qt 图编辑、参数面板、DSPatch 自动执行和多 Viewer 预览。本文集中记录实现状态与验证依据；操作步骤见 [Studio 使用指南](studio-guide.md)，模块关系见 [架构说明](architecture.md)。

## 状态与证据

“已实现”表示源码中已有对应实现并接入当前构建；“部分接入”表示底层能力存在，但应用使用链路尚未完成；“待开发”表示没有可用实现。验证依据另行记录，不能由编译产物或接口存在推断运行测试通过。

| 功能 | 实现状态 | 源码与验证依据 |
| --- | --- | --- |
| DSPatch 与运行图管理 | 已实现 | [Manager](../managers/flowcv_manager.cpp) 已管理节点、连线、Tick、AutoTick 与缓冲数；本轮仅核对源码，未重跑并行调度或压力测试 |
| Solid、Add、Viewer | 已实现 | [节点注册](../managers/node_manager.cpp) 与 [构建清单](../nodes/CMakeLists.txt) 均包含三类节点；历史记录曾验证 Solid + Add + Viewer 串行输出 |
| Qt 主窗口与图编辑 | 已实现 | [MainWindow](../app/VisionFlowStudio/MainWindow.cpp)、[Editor](../editor/VisionFlowEditor.cpp) 和 [GraphModel](../editor/Graph/VisionFlowGraphModel.cpp) 已接入；历史手测来源见下文 |
| 通用属性面板 | 已实现 | [PropertyPanel](../editor/PropertyPanel.cpp) 支持 Bool、Int、Float、Option，按节点分组，可折叠并高亮选中节点；参数可见性随配置更新 |
| 多 Viewer 预览 | 已实现 | [MainWindow](../app/VisionFlowStudio/MainWindow.cpp) 按真实 Viewer ID 管理独立 Dock；[ViewerPanel](../app/VisionFlowStudio/ViewerPanel.cpp) 定时读取图像；历史手测来源见下文 |
| 窗口与 Dock 布局保存 | 已实现 | MainWindow 使用 QSettings 保存窗口尺寸与 Dock 布局；本轮未重新执行跨次启动恢复测试 |
| 运行图 JSON 存取 | 部分接入 | Manager 已有 GetState / SetState / LoadState / SaveState；Studio 没有 New / Open / Save / Save As，未完成当前入口的完整往返验证 |
| 工程与画布持久化 | 待开发 | GraphModel 的节点位置、尺寸保存在内存；QSettings 布局不包含运行图、节点参数或连线 |
| 动态插件 | 部分接入 | [PluginManager](../managers/plugin_manager.cpp) 已扫描、加载和创建 `.fp` 实例；应用未调用加载，节点菜单只枚举内置节点，无实际插件目标及端到端验证 |
| 复杂参数与自定义视图 | 待开发 | [属性值类型](../sdk/flowProperties.h) 仅含 bool / int / float，Option 使用 int；主窗口只识别具体 Viewer，尚无通用文件路径或自定义视图接入 |
| 图片、视频、相机与结果输出节点 | 待开发 | 当前节点构建清单只有 Solid、Add、Viewer，没有真实媒体输入或文件、网络输出实现 |
| 更多内置算法节点 | 待开发 | FlowCV 原库中的其他处理、检测、绘制与 DNN 节点尚未迁入当前注册与构建清单 |
| 剪切、复制、粘贴与复制节点 | 待开发 | Editor 主动禁用 QtNodes 的 Copy / Paste / Duplicate；GraphModel 尚未实现对应节点保存与恢复路径，也没有应用层剪切功能 |
| Undo / Redo | 待开发，增强项 | 当前禁用；所参考 FlowCV 原库中的 Undo / Redo 也为禁用菜单项，不属于已有原库功能的遗漏移植 |
| 节点禁用、运行控制与诊断界面 | 待开发 | 当前节点菜单只提供删除；应用启动 AutoTick，退出时停止，没有启停或单步控件；节点验证和处理状态尚未接入图模型显示 |
| 独立无界面程序 | 待开发 | [ProcessEngine](../app/ProcessEngine/headless_process_engine.cpp) 只有占位内容，未接入可执行目标；Studio OFF 只生成库 |
| 自有测试、CI、SDK 安装与打包 | 待开发 | 当前 CMake 未建立自有测试或安装导出；QtNodes 自带测试被关闭；Windows 依赖复制与 Qt 部署不等于安装包 |
| 开发辅助工具 | 待开发 | [tools](../tools/README.md) 仍为预留目录，无工具实现 |

## 验证记录的适用范围

- **本轮文档更新：** 只做源码、文档相对链接、章节锚点及 `cmake --list-presets=all` 核对，不重新运行构建或 GUI。使用指南中的示例是按当前实现编写的操作步骤，不是本轮测试报告。
- **历史手测：** `916073d`（2026-10-15）的提交说明记录了图像输出、实时预览、删除 Viewer 和正常退出；`b422647` 同日的提交说明记录了多 Dock 独立显示修改与测试完成。这里引用提交说明，不将其扩展成全部格式、布局恢复、并行调度或插件生命周期已通过测试。
- **历史串行闭环：** 先前项目文档记录过两个 Solid 经 Add 到 Viewer 的同步检查，输出为 640 × 480、单通道、像素值 70。正式入口现为 Qt 应用，该记录不代表当前入口仍自动创建测试图或输出 `[PASS]`。

状态文件完整往返、导入失败恢复、AutoTick 中连续编辑、插件创建与卸载，以及不同部署环境仍需在对应功能开发时建立可重复验证。

## 已知限制

- `SetState()` 在顶层结构检查后先清空旧图，再恢复节点和连线；缺少节点或参数异常可能返回失败并留下部分新图，没有整体回滚。
- ViewerPanel 只显示 `CV_8UC1`、`CV_8UC3`、`CV_8UC4`。其他图像类型直接跳过刷新，可能保留上一帧；尚无格式转换或错误提示。
- Add 的两路输入缺失、为空或尺寸 / 类型不一致时不产生新输出，界面尚无对应节点错误说明。
- Solid 沿用忙等待限帧方式；源节点帧率和 ViewerPanel 的 33 ms 界面刷新周期是两个不同设置，不代表实际吞吐保证。
- 加载器通过 DSPatch 接口创建插件实例；原版插件能否直接加载运行仍待验证。当前参数访问要求节点继承 `FlowCV::Node`，旧插件的 ImGui 控件不能直接作为 Qt 参数面板使用。
- 应用入口没有显式初始化自定义 FlowLogger；不能据此认为每日文件日志已在正式程序中启用。

## 后续工作建议

以下为开发依赖顺序建议，不是已经承诺的发布计划：

1. 接通工程保存与恢复，同时处理导入失败、画布状态及未保存提示。
2. 扩充字符串 / 路径参数，迁移 ImageLoader，形成真实图片输入与预览流程。
3. 迁移常用预处理节点及 ImageWriter，补充必要的计算错误反馈。
4. 接通插件路径、加载、节点枚举与参数适配，再扩展视频、相机及其他插件。
5. 开发独立 ProcessEngine，并按部署需求补齐测试、SDK 导出与打包。

分类搜索、子图复制、节点启用 / 禁用、运行控制和 Undo / Redo 可随编辑器需求逐项推进。

## 文档维护

行为变化时更新使用指南与本页；接口、模块或依赖变化时更新对应说明；新增节点时同步节点文档。README 只保留概览与入口，不重复完整进度表。记录验证时注明来源与范围，将未执行的步骤与实际结果分开；删除失效路径和过时过程描述。
