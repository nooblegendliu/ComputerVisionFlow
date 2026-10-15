# Studio 使用指南

本指南按 [当前源码基线](development-status.md) 编写，说明已有界面及手动搭建流程的方法；这些步骤未在本轮文档更新中重新执行。构建准备见 [第三方依赖](../third_party/README.md)，启动命令见 [项目 README](../README.md)。

## 启动与界面

运行 VisionFlowStudio 后打开 Qt 主窗口，后台自动执行 DSPatch。应用没有自动创建示例节点，也不加载上次运行图，因此启动时画布为空；窗口尺寸和 Dock 布局会尝试恢复。

| 区域 | 用途与入口 |
| --- | --- |
| Editor | 放置、移动节点，建立与删除连线；通过 View → Editor 显示或隐藏 |
| Properties | 按节点列出可编辑参数；点击分组标题折叠或展开，选中节点时高亮对应标题 |
| Node Library | 双击节点名称创建实例；默认隐藏，可通过 View → Node Library 打开 |
| Viewer / Viewer Windows | 没有 Viewer 节点时显示占位区；创建后每个 Viewer 对应一个独立预览 Dock |
| Graph 工具栏 | 提供 Add Node、删除选择、Fit All Nodes 和 Reset Zoom |
| 状态栏 | 显示选择信息、部分编辑操作错误，以及节点和连线数量 |

也可以通过 Node → Add Node、工具栏 Add Node，或右键画布空白处的 Add Node 创建节点。当前可选类型为 Add、Solid、Viewer。

## 搭建灰度加法示例

### 创建并设置节点

1. 创建两个 Solid、一个 Add 和一个 Viewer，拖动节点将两个 Solid 放在 Add 左侧，Viewer 放在右侧。下文将两个 Solid 分别记作 A、B，仅用于说明，不要求在界面中重命名。
2. 打开 Properties，在两个 Solid 对应的分组中都取消勾选 Color。灰度模式会隐藏颜色通道参数并显示 Solid Value；Viewer 没有可编辑参数，不出现空分组。
3. 按下表配置。输入数值时可编辑数值框，再按 Enter 或切换焦点完成输入。

| 参数标签 | Solid A | Solid B |
| --- | --- | --- |
| Width | 640 | 640 |
| Height | 480 | 480 |
| Color | 取消勾选 | 取消勾选 |
| Solid Value | 30 | 40 |
| Output FPS | 30 | 30 |

Add 的 Mask Mode 保持 Default。完整端口、参数与处理条件见 [内置节点说明](../nodes/README.md)。

### 建立连线

从输出端口拖动到目标输入端口，建立以下三条连接：

| 起点 | 终点 |
| --- | --- |
| Solid A 的 out | Add 的 in1 |
| Solid B 的 out | Add 的 in2 |
| Add 的 out | Viewer 的 in |

Add 的 mask 是可选输入，此示例不连接。连线只接受相同端口类型；两路 Solid 都必须存在、非空且具有相同尺寸与图像类型。

当前输入端最多连接一路。将新输出连接到已占用输入端会替换原连接；如需改接，也可先选中连线删除，再建立新连接。当前没有拖出输入端以拆线的交互。

### 观察与修改结果

后台已经自动运行，无需点击 Run。Viewer Dock 预期显示 640 × 480 的均匀灰度图；按 Add 的计算逻辑，像素值为 30 + 40 = 70，画面按 Dock 大小等比缩放。

程序不会自动读取像素并断言结果，也不会打印 `[PASS]`。当前预览没有像素数值检查工具；预期值来自节点算法。

将 Solid B 的 Solid Value 改为 80，下一批有效输出预期变为像素值 110 的均匀灰度图。Properties 支持所有可编辑节点独立展开；选中节点只改变标题高亮，不会隐藏其他节点分组。

## 多 Viewer 与窗口布局

再创建一个 Viewer，并将 Solid A 的 out 连接到它的 in。输出端允许连接多个输入，因此原 Add 连线会保留；两个预览窗口分别显示加法结果与 Solid A。每个 Dock 标题采用 Viewer 的实例编号。

- 可拖动 Dock 标题改变位置或浮动，多个 Viewer 可独立显示。
- 关闭 Dock 只隐藏预览，不删除 Viewer 节点，也不停止计算。通过 View → Viewer Windows → 对应窗口重新显示。
- 删除 Viewer 节点会移除它的连线、预览 Dock 和菜单项。
- View → Reset Layout 恢复基础停靠布局，并重新排列当前 Viewer 窗口，不删除运行图。

退出时保存窗口尺寸与 Dock 布局。重新启动不会恢复节点、连线、节点位置或参数；延迟创建的 Viewer Dock 仅尝试按其对象名恢复位置。当前没有 New / Open / Save / Save As，也没有退出前保存流程的提示。

## 删除、移动与缩放

- 拖动节点调整位置；在空白画布按住鼠标左键拖动可平移视图，滚轮用于缩放。
- 点击选择节点或连线后，按 Delete、使用工具栏删除动作或 Edit 菜单删除选择。右键节点的 Delete Node 删除该节点及其连接。
- View → Fit All Nodes 将全部节点纳入视野；View → Reset Zoom 将画布缩放恢复为 1 倍。这些操作调整画布，不改变 Viewer 图像数据。
- Copy、Paste、Duplicate、Undo、Redo 当前被禁用，尚无应用层剪切功能。误删后需要手动重建。

## 排查预览问题

| 现象 | 检查方法 |
| --- | --- |
| 显示 No Viewer preview | 先创建 Viewer 节点；它是无 Viewer 时的占位文本 |
| 看不到预览 Dock | 在 View → Viewer Windows 中显示对应窗口，或使用 Reset Layout |
| 没有新结果或预览变黑 | 检查 Add 的 in1 / in2 是否都已连接且尺寸、类型一致；确认两个 Solid 的 Color 设置相同 |
| 新建 Viewer 暂时显示黑图 | Viewer 有默认黑色缓存；检查输入连线后等待有效帧 |

预览刷新周期为 33 ms，不代表节点必然以相同帧率运行。ViewerPanel 当前只显示 8 位单通道、BGR、BGRA 图像；其他格式可能保留旧画面。更详细的运行与功能限制见 [开发进度](development-status.md)。

通过 File → Exit 或关闭主窗口退出；事件循环结束后应用停止 AutoTick 并清理对象。当前没有界面启停或单步运行控件。
