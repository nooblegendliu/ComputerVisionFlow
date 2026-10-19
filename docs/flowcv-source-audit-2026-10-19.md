# FlowCV 源码复核与剩余工作（2026-10-19）

> **修复追记（同日）：** 下文为修复前的审计证据。C1 Add mask、C2 编辑器自连校验、C3 Viewer 头文件大小写已修复；后续核对发现的 CSV/VideoWriter 运行中编辑覆盖文件、通信参数编辑重连、Solid 状态恢复、Canny 步进、DNN 控件范围及配置错误阻止启动也已修复。验证及边界见[开发进度](development-status.md)。C4 采用读取 null/[]、写出 [] 的现有兼容策略；C5 保持计划明确的显示通道取色语义，不改变 Viewer 空状态。下方“已实现但验收尚未闭合”和延期范围继续有效，不因修复这些代码便自动标为完成。

复核基准：`D:/CodeSpace/projects/VisionFlow/FlowCV`，HEAD 为 `b09c48edcc9437253da6049365d319fe036eff61`。原库工作区只有两份依赖配置和个人 preset 的修改，节点、Manager、Editor 与 SDK 对照源码没有本地修改。当前项目按工作区源码复核，包含尚未提交的实现。本轮只更新核对记录，不修改生产代码。

**结论：71 个生产节点的接入数量齐全，但不能将剩余工作概括为“只剩硬件、模型和人工验收”。** 仍有 Add mask、自连校验、头文件大小写等代码差异；空集合序列化和 Viewer 取色也与原库不同。它们与“节点已实现但验证条件不足”分别记录如下。未发现新增的整节点遗漏，不据此宣称全部参数、模型分支和跨平台行为已经等价。

## 本轮核对范围与证据

| 核对项 | 本轮结果与边界 |
| --- | --- |
| 原库清单与固定矩阵 | 从原库实际源码重新提取的矩阵与 `flowcv-b09c48e-contracts.json` 完全相同：58 内部、13 动态插件；本地同名节点实现 71 个，未发现缺失项 |
| 公共接口与调度 | `stage7_inventory` 实际创建并检查名称、分类、版本、端口顺序/类型及状态往返；逐源码提取的 71 个 ProcessOrder 均与原库一致；Add/Solid/Viewer 的作者署名差异已有记录 |
| 计算与界面 | 对照计算、GetState/SetState、构造默认参数及 Qt Controls/Main；结合复杂操作专项和公共回归评估。原库源码对照不等于运行了原版程序的全部输入组合 |
| 当前构建 | `cmake --build out/build/stage7-clean --parallel 4` 成功，仅运行四项部署命令，没有待重新编译的源码；这次是既有 Release 目录的增量确认，历史干净构建结果另见开发进度 |
| 本轮回归 | 排除 `abs_diff` 与真实摄像头专项后，37 项中 36 项直接通过；ImageWriter 因沙箱临时目录 `Access is denied` 失败，获准在沙箱外单独重跑后通过。包含实际 Windows Qt OpenGL 的 `stage6_depth_gl` 与 `stage7_studio` |
| 补充复现 | 独立临时程序未 Tick 图：空图输出 `{"connections":[],"nodes":[]}`；原库 null 空图可加载；Blur 自连的 `connectionPossible` 返回 true，实际保存 1 条自连 |
| 本轮未重新执行 | 摄像头硬件、真实串口、业务模型/GPU、原生人工操作、Linux/macOS、Studio OFF 与 PluginMaker；已有通过记录继续作为历史证据，不能改写成本轮通过 |

本轮源码提取、大小写扫描、补充复现代码和日志保存在本机 `out/audit/`；该目录属于忽略的临时产物。既有 CTest 的输出在 `out/build/stage7-clean/Testing/Temporary/`，单项重跑会更新最近日志。

## 需要代码修复或明确兼容性策略的项目

优先级以当前 Windows 功能正确性和原库兼容为主；P1 为下一批应处理，P2 为随后收尾。跨平台若成为交付目标，大小写修复提前到该平台构建之前。

| 优先级 / 编号 | 性质 | 证据、影响和剩余工作 |
| --- | --- | --- |
| P1 / C1 | 已实现但行为不一致：Add 的 mask | [当前 Add](../nodes/Add/add.cpp#L74) 只接受同尺寸 `CV_8UC1`，其它 mask 静默按无 mask 计算。[原库 Add](../../VisionFlow/FlowCV/Internal_Nodes/Add/add.cpp#L48) 对非空单通道 mask 调用 `cv::add`，保留 OpenCV 合法的 `CV_8SC1`。因此合法有符号 mask 会失去遮罩；坏尺寸/坏深度也由原库报错变成全图相加。本地旧文档虽记录了更严格策略，仍不能称与原库一致。应与原库及现有 Subtract 的 mask 判定对齐，并对 OpenCV 错误安全结束本次输出；不能把不合法 mask 悄悄扩成整幅处理 |
| P1 / C2 | 缺少原库的编辑器自连校验 | [当前 connectionPossible](../editor/Graph/VisionFlowGraphModel.cpp#L384) 没有拒绝 `outNodeId == inNodeId`；Manager/DSPatch 也允许连接，临时程序已复现。[原库编辑器](../../VisionFlow/FlowCV/Editor_UI/Node_Editor/node_editor.cpp#L845) 明确拒绝同一节点的输入输出互连。应补编辑器校验，并验证拖线与已有输入替换不受影响。不将此项扩大为“禁止所有环”：原库没有这一全局规则 |
| P2 / C3 | 已确认跨平台代码缺陷，不能只写“未验收” | [viewer.cpp](../nodes/Viewer/viewer.cpp#L1) 引用 `Viewer.hpp`；[ViewerPanel.cpp](../app/VisionFlowStudio/ViewerPanel.cpp#L2) 引用 `Viewer/Viewer.hpp`；实际文件是 `nodes/Viewer/viewer.hpp`。在大小写敏感文件系统上无法正常解析这两处项目头文件。修正引用后再做 Linux/macOS 构建；Windows 成功不覆盖此问题 |
| P2 / C4 | 状态格式差异，可读取但不严格相同 | [原库 Manager](../../VisionFlow/FlowCV/Managers/FlowCV_Manager.cpp#L165) 的空 nodes/connections 为 `null`；[当前 Manager](../managers/flowcv_manager.cpp#L375) 输出 `[]`。原库空 editor 同样为 `null`，当前 `getEditorState()` 为 `[]`。两者加载路径都能遍历/接受这些表示，故不是已证实的加载失败，但会改变空图或无连线工程的输出结构。应明确采用“原格式写出”还是“读取兼容、写出规范化”的政策，并覆盖无节点/无连线/空 editor 的往返 |
| P2 / C5 | 已文档化的 Viewer 行为差异 | [当前 Ctrl 提示](../app/VisionFlowStudio/ViewerPanel.cpp#L76) 读取通道选择后的显示图；[原库](../../VisionFlow/FlowCV/FlowCV_SDK/third-party/imgui_wrapper/imgui_opencv.cpp#L285) 读取选择通道前的 BGRA 缓冲。选 R/G/B/A/Lum 时，当前提示灰度 RGB/HSV，原库仍提示转换后的彩色值。功能已有实现与自动化，差异也已记录；剩余是决定是否保持此适配，或恢复原取色语义，不能再笼统标作“完全一致” |

C4 的两种输出示意：

```json
// 原库空 Manager
{"nodes": null, "connections": null}
// 当前空 Manager
{"nodes": [], "connections": []}
```

## 已实现，但验收尚未闭合

| 优先级 / 编号 | 当前已具备 | 剩余验收与完成标准 |
| --- | --- | --- |
| P1 / V1 状态契约 | 71 节点清单、顶层键并集、当前实现自身的状态往返及七份本地生成的代表工程 | [inventory Contract](../tests/stage7_inventory_test.cpp#L50) 主要验证“当前输出键属于原库键集合”，没有要求全部应有键出现；object/null 归类也较宽，默认值、嵌套类型和条件分支未由该矩阵完整锁定。各专项补了一部分，但不能称已完成全部原库状态等价验收。补来自原库的默认/非默认/条件状态金样本，核对必需字段、嵌套结构、数值和空值语义；再做原格式输入→当前保存→按原库读取语义核验。属于契约验收，不需要给每个简单算术节点追加计算专项 |
| P1 / V2 DNN 模型分支 | 七类真实 CPU forward、确定模型的预/后处理、JSON、状态和 Qt 已通过；Region、单人姿态、DB/CTC 等链路有实际执行 | 补 SSD/DetectionOutput、Faster-RCNN/R-FCN 的 im_info、多人 PAF，以及实际使用的数据集/模型变体；分别验证坐标、阈值、类别/关键点、JSON 和错误恢复。代码中已有对应分支，尚无实际模型证明，不应列为“算法未实现”。业务精度、吞吐与可用 GPU 后端再按真实需求验收；任意现代 YOLO 输出解码不属于原库已支持范围 |
| P1 / V3 串口 | 真实设备枚举、配置、失效端口恢复、序列化/发送逻辑和 Qt 已实现 | 使用真实回环设备或可用虚拟端口对验证 bool/int/float/str/JSON、文本/二进制/EOL、波特率/校验位等参数、重配与销毁释放。枚举/连接失败测试不能替代字节收发；目标设备业务协议另记 |
| P2 / V4 原生交互 | 阶段 2/3 Controls、Histogram、DNN、Depth、视频及导出通信界面均有接入；自动化覆盖关闭重开、替换工程和资源生命周期 | 按代表链路手工验证参数编辑、长列表、颜色、滚动与焦点、实际文件选择确认、浏览器文档呈现。About/Help action、Browse 取消和 Ctrl 提示已经有 Qt 自动化，不重列为未实现；也不把自动化写成人工手测 |
| P2 / V5 摄像头及平台 | 本机真实摄像头采帧、None 释放/重开、析构重建已有历史通过记录；三平台枚举源码和构建分支存在 | 摄像头手动属性、驱动原生设置窗口、保存恢复后的驱动实际值、其它型号/后端待验。Linux/macOS 先修 C3，再独立完成依赖、构建、插件发现、设备与 GUI 生命周期验证。其它平台是验收缺口与已知代码缺陷并存 |
| P2 / V6 深度代表工程 | 点云计算、Qt GL 渲染/相机/多实例/GL 重建已有专项通过 | `06-depth-view.flow` 的 depth/intrin 未接真实来源，直接打开不能作为端到端深度演示。Image_Loader 按原库只输出 8 位 BGR，这是样例输入接入缺口；应使用应用注入或原范围外的深度源完成代表演示，不能改称 Depth 节点未实现，也不能用彩色图冒充深度 |
| P3 / V7 模板条件 | 三类模板构建、加载和生命周期已有通过记录 | 两项符号链接相关 Python 检查因权限跳过，使用允许创建链接的测试环境补验。跳过属于验收条件不足，不代表生成器缺少功能 |

## 应保留的原库行为与必要适配

以下项目有实现及相应说明，不应重新安排成“补缺失功能”：

- Viewer/Json_Viewer 返回空状态字符串，因此工程省略 params；Abs_Diff、DCT、Color_Reduce、Histogram 等仍保留原 `null` 状态。Color_Reduce 的 Reduce、DCT 的 Operation、Object_Detection 的 NMS 原本就不持久化，新增保存字段反而会偏离基准。
- Draw_Text 外部字符串接入时省略 text；Draw_Date_Time 按 show 开关保存格式；DNN 按标注开关保存样式；Depth 按内参/视图初始化保存字段。顶层键并集不是每次必须出现的全集。
- VideoLoader 原前两帧 start=true、VideoWriter 每个 true 都切换、CSV 计算不检查 enabled、UDP/TCP/Serial 字符串只发送首字符，以及数据打包/EOL 的原规则均已保留，不能当成新增修复去另改协议。
- VideoLoader 完整解码末帧、有效 seek 和 sleep_until，通信发送缓冲生命周期，DNN 初始化失败废弃旧网络，Depth 两阶段坐标/法线及 GL 线程归属等，是已有记录的修复或适配；不要求恢复原缺陷。
- Qt Dock 替代 ImGui、Width/Height 以缩放加滚动呈现、QSettings 保存布局、配置目录使用 Qt 标准位置、取消 GLFW VSync 控制，以及启动空画布均为当前宿主策略。配置键兼容不等于自动读取原 `%APPDATA%/FlowCV` 配置；需要沿用旧配置时可显式指定 `--cfg`。

## 确实未实现、但此前明确延期的能力

| 建议顺序 | 内容 | 定性 |
| --- | --- | --- |
| P2，按可靠性目标单独安排 | 整工程加载失败的事务恢复 | 当前 Manager 在恢复前 NewState；缺节点、坏 params/位置或连线失败可留下部分图。Studio 已提示此结果，子图粘贴回滚不能替代整工程事务。这是尚未实现的可靠性增强，原库自身也没有完整事务，不能当作新增节点遗漏 |
| P3，需求确定后 | 独立 ProcessEngine | 当前 `app/ProcessEngine/headless_process_engine.cpp` 只有包含语句，未接构建；原库 Processing_Engine 则有实际执行程序。若范围是整个 FlowCV 产品，它确实尚未迁移；在此前明确限定的 Studio+71 节点范围内属于已知延期项 |
| P3，需求确定后 | Undo/Redo、全局暂停/单步、热重载、插件隔离、性能分析、CI、SDK 安装导出、安装包 | 当前未交付。原库 Undo/Redo 菜单本身就是禁用占位；其它能力不能仅因其未实现便推导为相对本次基准的迁移遗漏。它们与跨平台验收分别排期 |

建议执行次序：**C1/C2 与 V1 → V2/V3 → C3/C4/C5 与 V4/V5/V6 → 延期增强项**。先收敛代码差异和契约证据，再进行需要实际模型、设备或人工操作的验收；节点数量不再作为剩余工时比例。
