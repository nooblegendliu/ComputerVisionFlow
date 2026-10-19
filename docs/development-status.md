# 开发进度

核对日期：2026-10-19。功能基准为本机 FlowCV `b09c48e`；当前工程以工作区实际源码为准。

本日[源码全面复核](flowcv-source-audit-2026-10-19.md)及后续审计确认的代码问题已按下节修复。空集合写出 `[]`、Viewer 按显示通道取色继续保留既定 Qt 宿主策略；不宣称与原库字节级或所有交互严格等价。实际模型、串口和其它平台等未完成验收仍单独记录。

VisionFlowStudio 公共功能及本机 FlowCV `b09c48e` 范围内全部生产节点的接入代码已完成：58 个内部节点、13 个真实动态插件，共 71 个。阶段 3D–7 补齐 Histogram、视频、导出通信、7 类 DNN、Depth_Viewer_3D、契约矩阵和代表工程。干净 Release、复杂专项与整体自动化结果见下节；真实串口、实际模型精度/GPU和剩余人工/跨平台验收未标为通过。操作见 [使用指南](studio-guide.md)，逐项验收见 [功能对照清单](studio-feature-checklist.md)。

## 源码复核后的局部修复（2026-10-19）

保持原节点计算路径、持久化字段及 `.fp` 接入方式；运行时 Controls 与工程恢复的资源操作作必要区分，没有重构主窗口或公共参数系统。

| 修复 | 行为及验证范围 |
| --- | --- |
| CSV_File | 运行中修改容量上限保留已有内容、计数和重复帧抑制；路径、表头及轮转配置仍按原规则重开。实际文件和 Qt 编辑回归通过 |
| Video_Writer | 录制中切换 auto_ext 不再立即重开；实际四帧录制、中途开关、文件重解码及颜色检查通过 |
| UDP/TCP/OSC/Serial | Qt 实时参数走专用 patch；仅连接配置变化重开，SetState 保留工程恢复语义。UDP/OSC 源端点、TCP 原连接持续发送及 UDP 更换目标回归通过；真实串口保活仍待设备验收 |
| Solid | fps/fps_index 独立恢复和回写；正尺寸不按界面范围截断。非默认及部分状态往返、1×10001 输出、实际节拍、非法 FPS 原子拒绝通过。让恢复的 FPS 实际生效是对原库未更新 fps_time 的必要修复 |
| DNN Controls | 恢复原拖动范围对应的箭头/滚轮边界，保留显式文本输入和旧工程范围外值；七类控件初始化不改状态及边界检查通过，原 CPU 推理回归通过 |
| 简单修复 | Add 恢复 CV_8SC1 mask，坏单通道 mask 不再变全图运算；Canny 复用专属 Controls，核步长 2；画布拒绝同节点自连；Viewer 引用匹配实际文件大小写；损坏配置记录日志、显示提示并用默认值继续启动。仅源码核对与构建检查，没有新增简单计算专项 |

Release 完整构建成功；针对性复杂及公共回归 `solid_state / graph_edit / studio / stage4_video / stage5_data / stage6_dnn / stage7_inventory / stage7_studio` **8/8 通过（14.14 秒）**。这是本次选择的回归集合，不冒充全量历史测试；现有 71 节点契约测试仍主要检查注册、端口和自身状态往返，不证明所有原库非默认条件状态均已覆盖。测试日志：`out/audit-fixes-tests.log`。

关闭 Studio 的已有构建配置中，受影响计算库与插件构建通过；`solid_state / stage4_video / stage5_data` **3/3 通过（3.72 秒）**，确认普通计算插件不依赖 Qt。日志：`out/audit-fixes-core-tests.log`。本批没有重新运行摄像头、实际串口、GPU或原生人工验收。

```powershell
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release -R '^(solid_state|stage4_video|stage5_data|stage6_dnn|stage7_inventory|studio|graph_edit|stage7_studio)$' --output-on-failure
```

## 阶段 3D–7：剩余代码与整体收尾（2026-10-19）

按本轮要求先完成剩余 17 个节点的计算、状态、Controls/Main 与构建注册，再集中测试并修复失败。保留 DSPatch、原节点名/端口/状态和 `.fp` / Create 机制；计算节点仍无 Qt，原库未保存的参数继续仅在运行时存在。MainWindow、PropertyPanel 和 SDK 未为新增节点添加类型分支。全部节点的固定原库契约与七份代表 `.flow` 位于 [examples](examples/README.md)。

| 内容 | 验证与实际边界 |
| --- | --- |
| Histogram | 原 255 样本、按图高归一化、灰度/BGR/BGRA/16U/32F、空/缺输入与错误恢复、独立 Qt Main；状态仍 null |
| Video_Loader / Writer / Capture | 真实 MJPG 文件读写、末帧/循环/seek/FPS、codec/路径/坏状态/销毁及 Controls；本机摄像头真实采集 3 帧、两次重开各 1 帧、None 与析构释放通过，未保存图像 |
| CSV / UDP / TCP / OSC / Serial | 隔离 CSV、本机 UDP/TCP/OSC 收包与格式、TCP 自动重连、状态/Qt/库释放通过；串口仅真实枚举和失效配置恢复，未完成硬件往返 |
| 7 类 DNN | 小型确定 ONNX / Darknet 模型实际 CPU forward，分类/softmax、Region/NMS、分割、单姿态、DB→CTC 裁剪识别、Style Transfer、状态/Qt/失效资源通过；真实模型精度、GPU、SSD 与多人 PAF 分支尚未实测 |
| Depth_Viewer_3D | 确定深度点坐标/翻转/缩放/内参或FOV/偏移/颜色/法线/尺寸变化/边界状态通过；原生 Windows Qt OpenGL 实际点云像素、轨道/飞行/平移/滚轮/Home、多实例及GL销毁重建通过，未跳过 |
| stage7_inventory | 独立原库矩阵核对 58+13 名称、分类、版本、端口与状态键；71 节点混合图保存、重复恢复、实例释放，以及七份示例工程通过；不 Tick 简单节点 |
| stage7_studio | 剩余17节点的实际 UI 角色、Controls/Main关闭重开、13插件同ID工程替换、旧控件/节点释放、三缓冲及多Viewer/Histogram通过；About实际对话框、Documentation URL拦截、Browse实际文件框取消、Ctrl像素提示通过Qt自动化 |
| 干净 Release | `out/build/stage7-clean` 新目录完整配置/337步构建成功；复杂操作与公共回归 37/37（35.85秒），随后新增 stage7_studio 1/1（2.16秒）；总计38项通过。按本轮要求排除简单 Abs_Diff 专项，仅构建 |
| Studio OFF | `out/build/phase1-core` 新节点/13插件构建；视频、通信、DNN、视图计算、71节点契约五项 5/5（8.03秒） |
| PluginMaker | 9项检查中7项通过，2项因本机无创建符号链接权限而明确跳过；本轮未修改模板，未重做简单模板节点计算测试 |

必要偏离：Histogram 保留原少最后一箱的显示采样，不“修正”成另一算法。Depth 把 GL 移到 GUI 上下文，先计算所有本帧位置再按原叉乘求法线，修复原右邻居旧值、零深度邻接缺坐标和单行/列越界；透明点 discard 避免错误深度遮挡，GL上下文回调在清理时断开。相机切换仍沿原轨道/飞行算法，状态恢复不创建GL资源；UI请求原8xMSAA，由驱动协商。其余节点必要适配详见 [视频](../plugins/VideoLoader/README.md)、[Writer](../plugins/VideoWriter/README.md)、[Capture](../plugins/VideoCapture/README.md)、[DataOutput](../plugins/DataOutput/README.md)、[DNN](../nodes/DNN/README.md)，包括固定缓冲/输入边界、线程同步、发送缓冲生命周期及失效资源释放，没有公共架构重写。

首轮失败与修复：测试目标漏链接 NodeUiRegistry 已补齐；DNN TextDetection/TextRecognition 的原 SetState 保留已有绘制开关，Qt入口现显式同步开关，不改变原工程恢复语义；原 Json_Viewer GetState 返回空字符串，矩阵提取脚本误判为JSON，已改为检查实际 dump，仅修测试期望。修复后专项通过。

剩余验收不等于未实现代码：实际串口/目标设备协议、用户实际模型精度与GPU/模型变体、摄像头手动属性/原生驱动设置、人工文件确认/浏览器呈现及其它平台仍未验证。GUI自动化不是人工手测；也没有将深度示例中的BGR源冒充16U深度源。独立 ProcessEngine、整工程加载事务增强、Undo/Redo等仍按原边界不纳入。

```powershell
# 在 VS x64 Developer PowerShell 中
cmake --preset local-x64-release -B out/build/stage7-clean -DVISIONFLOW_BUILD_TESTS=ON
cmake --build out/build/stage7-clean --parallel 4
ctest --test-dir out/build/stage7-clean -E '^abs_diff$' --output-on-failure
# 下面的独立目录已经配置 VISIONFLOW_BUILD_STUDIO=OFF / TESTS=ON
cmake --build out/build/phase1-core --target stage4_video_test stage5_data_test stage6_dnn_test stage6_views_test stage7_inventory_test --parallel 4
ctest --test-dir out/build/phase1-core -R '^(stage4_video|stage5_data|stage6_dnn|stage6_views|stage7_inventory)$' --output-on-failure
python -B tests/test_plugin_maker.py
```

## 阶段进度

| 阶段 | 本轮实现内容 | 实现状态与验收入口 |
| --- | --- | --- |
| 0：完成基线 | 校正工程保存、位置恢复、未保存提示和插件已接通的过时描述，建立统一功能清单 | 文档已更新；基线及最终测试结果见下文 |
| 1：Qt 节点界面 | 可选 `NodeUiProvider`、Controls/Main 工厂注册、Viewer 适配、属性组关闭与重开、节点与插件库生命周期 | 代码已接入；`studio` 测试覆盖自定义界面插件 |
| 2：应用设置 | `flowcv_editor.cfg`、`--cfg`、插件目录、缓冲数、日志等级、最近文件、UI FPS | 代码已接入；`studio` 测试覆盖配置与运行时应用 |
| 3：Viewer | 通道选择、Free/Width/Height、16 位与浮点伪彩、缩放、Ctrl 像素检查、滚动和多实例独立选项 | 代码已接入；`viewer` 测试覆盖转换、坐标和控件 |
| 4：图编辑 | Cut/Copy/Paste、节点启用状态、分类与搜索、端口拖空创建、节点信息、F 定位 | 代码已接入；`graph_edit` 测试覆盖子图及失败处理 |
| 5：辅助与模板 | 分类颜色、端口类型、Show Flow、Help/About、三类 PluginMaker 模板 | 生成器与模板验证通过；实际窗口已验证方向动画和 UI FPS；其余 GUI 范围见清单 |

工程文件的 New/Open/Save/Save As、节点位置和未保存提示在本轮开始前已经实现，并非待开发项。QSettings 仅负责窗口与 Dock 布局，不承担运行图保存。

当前全部 58 个内部节点与 13 个插件的原库名称、端口和状态键清单见 [固定契约矩阵](examples/flowcv-b09c48e-contracts.json)。计算、接口和 Qt 专属界面均按节点交付，具体算法验证范围不混入公共功能缺口。

## 节点迁移阶段 3C：绘制（2026-10-19）

Draw_Text 先完成计算、状态及 Qt Controls，并独立构建 Studio 通过，随后接入 Draw_Number、Draw_Date_Time、Draw_JSON。四个内部节点显式加入 Nodes/Managers 构建与注册，Controls 通过 StudioNodeUis 工厂提供；没有改 MainWindow、PropertyPanel、SDK 或公共界面机制。阶段 3 剩余 Histogram。

保留原 cv::putText、八种字体、位置/颜色/缩放/厚度、端口及 OutOfOrder。Draw_Text 外部字符串优先且空串有效，保存省略 text 与恢复缺 text 清空的规则不变；Draw_Number 保留 bool→int→float 优先级和 std::to_string。Draw_Date_Time 保留本地 strftime、先日期后时间、原编辑白名单及显示开启才保存对应格式；Draw_JSON 保留 float32 固定精度、JSON 字符串引号、数组前缀、缺字段跳过、文字高度加 14 行距、空列表 null 状态和缺/空 JSON 无输出的门槛。完整契约见 [内部节点](../nodes/README.md)。

必要适配：参数同步快照、状态原子恢复、Qt 弱引用与父对象定时器；std::string 替代原不安全固定缓冲。日期格式化检查返回值，动态缓冲支持长状态格式，Windows 通过临时线程局部 invalid-parameter handler 防止坏格式终止进程并恢复调用方处理器。JSON Pointer 正确转义字段名的 `/`、`~`；非法 pointer 按项跳过，畸形非空 data 安全输出图像副本并提示，正常 data:null 不报错；仅运行时列表版本阻止旧行回调修改恢复后的条目。以上不改变合法输入的计算或工程字段。

| 验收入口 | 结果 |
| --- | --- |
| Draw_Text / Draw_Number | 原库源码契约核对与 Release 构建通过；按要求不增加专项测试 |
| `stage3c_datetime` | 固定闰日与格式顺序、系统本地时间实际像素、原图不变、条件状态、长格式、非法格式/处理器恢复、禁用/错误恢复、Qt 条件显示/草稿/编辑/销毁通过 |
| `stage3c_json` | 原绘制像素、float32 精度、嵌套与转义路径、数组重复/索引/越界、缺空 JSON、原状态恢复、Qt 双击增删/数据选择/旧行回调/销毁通过 |
| `phase1_pipeline` | 真实 Shape_Counter.counts → Draw_JSON → Viewer 标注；包含日期节点状态与两个 Controls，混合图复制、保存重开、同 ID 面板释放及三缓冲执行通过 |
| Studio Release | 完整构建、部署通过；上述三项加 `graph_edit`、`studio`，5/5 通过（3.16 秒） |
| Qt-free 计算构建 | `out/build/phase1-core`（Studio OFF）构建通过，日期时间/JSON 复杂专项 2/2 通过（0.17 秒） |
| 验收范围 | Qt 界面以 offscreen 自动化验证，本批未执行原生桌面手测；未重跑全部历史测试 |

```powershell
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release -R '^(stage3c_.*|phase1_pipeline|graph_edit|studio)$' --output-on-failure
# 独立目录已配置 VISIONFLOW_BUILD_STUDIO=OFF、VISIONFLOW_BUILD_TESTS=ON
cmake --build out/build/phase1-core --target stage3c_datetime_test stage3c_json_test --parallel 4
ctest --test-dir out/build/phase1-core -R '^stage3c_' --output-on-failure
```

## 节点迁移阶段 3B：结果消费（2026-10-19）

Shape_Colorizer、Shape_Counter、Simple_Blob_Tracker 已作为三个独立 `.fp` 接入。先完成并通过 ShapeColorizer 专项，再验证计数、跟踪和集成；宿主沿已有可选 Qt NodeUiProvider 加载 Controls，MainWindow、公共 PropertyPanel 与 SDK 未因本批添加具体插件判断。关闭 Studio 时只构建计算代码。至此共 45 个内部节点、5 个生产插件，剩余 13 个内部节点、8 个插件；阶段 3 尚余四个 Draw 节点和 Histogram。

颜色插件保留 Mean/Max/Closest、原 ImGui 等价 HSV 和 D50 CIE76、累计 mask、依序修改匹配颜色、原 JSON/颜色列表格式；计数保留 size 截断后的严格边界、class_id 分组、原背景/字体绘制和位置回写；跟踪保留逐检测匹配顺序、同帧重复关联、首次位置特殊规则、严格距离/尺寸/年龄边界、持有 start=true 每帧复位，以及历史每帧只移除一个位置。详见 [插件契约](../plugins/README.md)。

必要修复限于参数/运行状态同步、状态原子恢复、坏 JSON/OpenCV 异常不逃出计算线程、ROI/整数边界保护、原标签固定缓冲区溢出防护，以及跟踪器原删除循环跳过相邻过期轨迹的缺陷。没有增加新的颜色匹配或跟踪算法。Qt 恢复合法超范围值时扩展显示范围；颜色列表使用不持久化的版本号阻止旧行回调写到新条目，销毁旧行前断开回调。首次颜色专项失败源于测试错误假设浮点取整，按原 ImGui 计算得到的 127.499969 修正预期为 127，没有修改算法去迎合测试。

| 验收入口 | 结果 |
| --- | --- |
| `stage3b_colorizer` | 真实 fp；三模式、RGB/CIE76 参考值、累计 mask/依序匹配、JSON/原状态、ROI/异常恢复、Qt 列表/颜色/删除恢复回调/阈值/租约销毁通过 |
| `stage3b_counter` | 真实 fp；严格 size/class_id 分支、空 JSON、原 overlay 像素/位置修正/边界、隐藏状态、Qt RGBA/条件参数/超范围恢复/销毁通过 |
| `stage3b_tracker` | 真实 fp；ID 关联/历史、丢失边界与相邻剔除、start/Reset、禁用不推进、状态不混入轨迹、坏数据恢复、Qt 编辑/范围/租约通过 |
| `phase1_pipeline` | 原静态图链路增加 Blob→Colorizer→Counter、Blob→Tracker；真实插件输出 JSON、预览原图不变、复制/粘贴、保存重开、三缓冲执行、插件 Controls 先于节点释放通过 |
| Studio Release | 完整构建、插件部署通过；三专项加 phase1_pipeline、graph_edit、studio，共 6/6 通过（4.59 秒） |
| Qt-free 计算构建 | 复用 `out/build/phase1-core`（Studio OFF），三插件及专项构建通过；CTest 3/3（0.10 秒），dumpbin 确认三个 fp 无 Qt DLL 依赖 |
| 验收范围 | 本批为颜色列表/ROI、绘制分类、跨帧历史等复杂操作；未新增简单算术测试，未重跑全部历史测试。Qt 界面以 offscreen 自动化验证，本批未做原生桌面手测 |

```powershell
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release -R '^(stage3b_.*|phase1_pipeline|graph_edit|studio)$' --output-on-failure
# 下列使用已配置为 VISIONFLOW_BUILD_STUDIO=OFF、VISIONFLOW_BUILD_TESTS=ON 的独立目录
cmake --build out/build/phase1-core --target stage3b_colorizer_test stage3b_counter_test stage3b_tracker_test --parallel 4
ctest --test-dir out/build/phase1-core -R '^stage3b_' --output-on-failure
```

## 节点迁移阶段 3：首批背景与检测

按要求先完成 Background_Subtraction 并独立通过专项测试，再接入 Blob_Detector、Hough_Circles、Hough_Lines、Line_Intersections。本批五个节点显式加入构建与注册；Background_Subtraction 使用通用属性，其余四个节点通过 BlobControls、HoughControls、LineIntersectControls 提供颜色与条件界面，不修改 MainWindow/PropertyPanel 的节点判断。计算库增加 OpenCV video/features2d 依赖，仍不依赖 Qt。

保留原算法、默认值、端口顺序、状态键和嵌套 RGB 对象，隐藏参数继续保存。背景模型各自长期保留，模式切换不重建；Hough 空检测仍输出 `data:null`，Blob/交点无结果不发 JSON。直线端点沿原边界顺序求解，交点仍按无限直线和方向夹角计算，不改成线段或锐角算法。详细规则见 [节点契约](../nodes/README.md)。

必要适配限于：背景模型并行访问加锁，状态恢复后的设置同步至两个已有模型；KNN 非 8U 输入拒绝，避免其 CPU 字节采样误读浮点数据，MOG2 仍支持 BGRA 和浮点输入；新版 OpenCV 要求 Blob 参数下限大于零，在计算副本中用最小正正规 float 替换零，状态保持原零值（退化零测量会被排除）；状态原子更新、异常恢复和几何溢出/非有限值保护、acos 输入修正。Line_Intersections 正常接受 Hough 的 `data:null`，并使用本帧计数执行 200 条限制，UI 计数及时清零。没有新增业务算法或修改公共架构。

| 验收入口 | 本批结果 |
| --- | --- |
| `stage3_background` | 首先独立通过；持续 MOG2/KNN 参考模型对照、模式切换保留历史、状态及实时参数更新、127 阴影、禁用不学习、灰度/BGRA/浮点及异常恢复通过 |
| `stage3_blob` | 固定图对 OpenCV 检测/绘制、五类过滤、JSON、嵌套颜色、隐藏参数状态与原子恢复、Qt 条件显示/颜色及销毁通过 |
| `stage3_hough` | 圆/标准线/概率线、端点和直径 JSON、颜色/文本/填充绘制、空检测 null、禁用及错误恢复、Qt 编辑和生命周期通过 |
| `stage3_intersections` | 无限线/方向角/严格边界/距离去重、200 条上限、Hough 空结果、图外点、绘制、JSON、隐藏状态及 Qt 计数/条件控件/销毁通过 |
| `phase1_pipeline` | 在真实 ImageLoader/ImageWriter 混合工程中加入五节点及非默认隐藏字段/颜色，复制、粘贴、保存、重开、UI 释放及三缓冲运行通过 |
| Windows MSVC x64 Release | 五节点、Studio、插件及全部测试目标构建通过 |
| 选定回归 | 上述五项与 `graph_edit`、`studio` 共 7/7 通过（3.42 秒）；未重跑全部历史测试 |
| 界面边界 | 新 Controls 以 Qt offscreen 自动化验证，本批未执行原生桌面手测 |

本批均含跨帧状态、检测/绘制或几何边界，属于复杂操作；未新增或运行简单算术/数值节点专项。阶段 3 尚余五个绘制/显示内部节点和三个结果消费插件，不能标记整个阶段完成。

```powershell
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release -R '^(stage3_.*|graph_edit|studio|phase1_pipeline)$' --output-on-failure
```

## 节点迁移阶段 2E

先接入 Get_Optimal_DFT_Size 并完成 Nodes/Managers 构建检查，再接入 DFT、DCT、Magnitude、Rnd_Noise。五个节点复用通用属性或无参数入口，不新增 Qt 界面，不修改 MainWindow/PropertyPanel。至此阶段 2 的 28 个内部节点均已接入。

DFT 保留灰度、32F 复数计算、幅值、偶数裁剪/象限交换及可选 log/归一化/8U 顺序，输出仍是幅值图。DCT 保留只有原 32F 不缩放、其它深度转 32F/255 的规则，其 Operation 仍不持久化。Get_Optimal_DFT_Size 图像优先于 size；Magnitude 保留单通道浮点门槛及禁用双输入条件。Rnd_Noise 保留三模式、五字段状态、随机字节 0–254、try-lock 和忽略 enabled 的源行为。详见 [节点契约](../nodes/README.md)。

必要修复限于线程安全参数快照、非法状态校验及计算异常边界；Rnd_Noise 修复原 SetState 的恢复值被临时默认值覆盖、fps 恢复不影响等待周期，用 sleep_until 替代忙等并保留原截断周期。无自动补边、可逆频谱接口或随机种子新设计。DCT 尺寸支持由链接的 OpenCV 决定，本机 5×7 奇数尺寸与直接 cv::dct 结果一致，不人为限定偶数。DFT 单行在开启 Shift 时按原偶数裁剪变空、无输出，关闭 Shift 后保留 1×N。

| 验收入口 | 本批结果 |
| --- | --- |
| Get_Optimal_DFT_Size | 首先完成源码契约核对、Nodes/Managers 构建检查；不增加专项计算测试 |
| `phase2e_frequency` | DFT 六模式、解析信号/象限/奇数裁剪/单行、log、位深/ROI、状态和错误恢复；DCT 三模式、缩放、正逆变换及当前 OpenCV 尺寸支持，通过 |
| `phase2e_noise` | 三模式类型/范围、原状态恢复、首帧/连续帧及恢复 10 FPS 节奏、超出 UI 默认范围的状态/尺寸显示、非法状态与错误恢复，通过 |
| Magnitude | 原库源码契约核对及 Release 构建通过；不增加专项计算测试 |
| Windows MSVC x64 Release | 最终五节点、Studio 和相关测试集成构建通过 |
| `graph_edit`、`studio` | 两项公共回归通过；与 phase2e_frequency、phase2e_noise 合计 4/4 通过（1.30 秒），未重跑全部历史测试 |
| 界面边界 | 参数沿用通用 Properties，无新增 Qt 专属界面；本批未进行原生桌面手测 |

```powershell
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release -R '^(phase2e_frequency|phase2e_noise|graph_edit|studio)$' --output-on-failure
```

## 节点迁移阶段 2D

先完成 Sobel 并独立通过专项测试，再接入 Scharr、Laplacian、Sharpen。四个节点显式加入构建和注册；Sobel/Laplacian 通过可选 DerivativeControls 提供步长为 2 的核大小编辑，Scharr/Sharpen 使用通用 Properties。不修改 MainWindow 或 PropertyPanel 的节点类型判断。

Sobel/Scharr/Laplacian 保留原库转灰度、输出深度、导数方向、scale/delta 和默认边界，不自动转 8 位。Sobel/Laplacian 的核大小仅在界面编辑时修正为 1–7 的奇数，恢复原状态时不擅自改变 ksize。Sharpen 保留三种卷积核以及 Mode 2 的灰度 Laplacian、转 BGR、addWeighted 顺序和原深度中间结果。原状态、端口和禁用规则见 [节点契约](../nodes/README.md)。

必要防护包括参数快照和核编辑记录同步、坏状态原子拒绝，以及 OpenCV 异常使本帧无输出并允许后续恢复。Sobel/Laplacian 恢复核大小时临时扩展显示范围，避免控件限制掩盖真实保存值；首次编辑再恢复原 UI 范围。核控件沿原库步长 2，避免通用整数步长 1 与偶数修正叠加后无法向下调整；界面放入已有可选工厂，不扩展公共属性系统。Sharpen Mode 1 在界面切换时沿原行为限制 amount≤3，计算侧再拒绝未经刷新而超限的参数，避免使用未初始化核。Mode 2 的灰度/BGRA 通道不匹配仍按不支持处理，不改变原算法来扩展输入格式。

| 验收入口 | 本批结果 |
| --- | --- |
| `phase2d_sobel` | 首先独立执行，1/1 通过；梯度、边界、深度、核、颜色转换、ROI、原状态及错误恢复；随后补充 Qt 核增减/恢复/生命周期，最终回归通过 |
| `phase2d_sharpen` | 两模式、三种卷积核、多位深/饱和、状态与动态范围、复制语义、非法输入恢复通过；Mode 2 对照原库原位 Laplacian 路径 |
| Scharr、Laplacian | 原库源码/状态核对及 Release 构建通过；不新增专项计算测试 |
| Windows MSVC x64 Release | 最终四节点、可选 Controls、Studio 及相关测试集成构建通过 |
| `graph_edit`、`studio` | 两项公共回归通过；与 phase2d_sobel、phase2d_sharpen 合计 4/4 通过（1.71 秒） |
| 界面边界 | Sobel Qt offscreen 控件测试通过；Laplacian 同类 Controls 仅构建检查，Scharr/Sharpen 使用通用属性；本批未进行原生桌面手测 |

```powershell
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release -R '^(phase2d_sobel|phase2d_sharpen|graph_edit|studio)$' --output-on-failure
```

## 节点迁移阶段 2C

先完成 Color_Correct 并通过专项测试，再接入 Color_Reduce、Split、Combine。Color_Correct 通过现有可选 Qt 工厂提供参数、RGB 范围及 Reset，Split 提供颜色输入提示，Color_Reduce 使用通用属性；Combine 无专属界面。计算层不引入 Qt，不修改 MainWindow 或 PropertyPanel 的节点类型判断。

Color_Correct 保留饱和度 → gamma LUT → 对比度 → 增益 → 亮度顺序、算术平均亮度、仅 mask=255 生效、RGB 范围状态及 gamma=0 语义。Color_Reduce 保留逐字节量化，包括 alpha 和 uchar 截断，Reduce 默认 64 且不持久化，状态仍为 null。Split 保留 r/g/b/a 对应 2/1/0/3、仅四通道输出 alpha；Combine 按 BGR 合并，不匹配 alpha 被忽略。Split/Combine 保留 null 状态及自启用行为。详见 [节点契约](../nodes/README.md)。

必要修复仅包括 Color_Correct 参数/LUT 的同步快照、坏状态原子拒绝，以及非法 mask 或不支持的彩色类型不输出，避免未初始化或越界访问；Color_Reduce 拒绝非 8 位输入，避免错误解释高位深字节；Split 空输入清除陈旧颜色提示。原计算顺序、有效输入行为和状态格式保持不变。

| 验收入口 | 本批结果 |
| --- | --- |
| `phase2c_color_correct` | 首先独立执行，1/1 通过；固定像素转换顺序、mask/缩放、RGB 范围、alpha、gamma=0、输入不变、状态/坏状态、非法输入恢复及 Qt 编辑/Reset/生命周期 |
| Color_Reduce、Split、Combine | 原库源码/状态核对及 Release 构建通过；按用户要求不新增简单节点专项测试 |
| Windows MSVC x64 Release | 最终四节点、Studio 及相关测试集成构建通过 |
| `graph_edit`、`studio` | 最终两项公共回归通过；与 phase2c_color_correct 合计 3/3 通过（1.71 秒） |
| 界面边界 | Color_Correct 使用 Qt offscreen 控件自动化；本批未进行原生桌面手测 |

```powershell
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release -R '^(phase2c_color_correct|graph_edit|studio)$' --output-on-failure
```

## 节点迁移阶段 2B

先接入 Crop 并通过 JSON 合并、padding、运行区域保存和 Controls 测试，再完成 Transform、Perspective_Warp、Copy_Make_Border。四个节点显式加入构建与注册；三个专属 Controls 经现有 Qt 适配注册表接入，Transform 使用通用属性。本批没有修改 MainWindow、PropertyPanel、基础 SDK 或 DSPatch。

保留原库状态格式和易混淆行为：Crop 的合并框 padding 公式及区域写回，Transform 的翻转标签/代码、操作顺序和可见字段状态，Perspective_Warp 的角点排序/尺寸截断/禁用双输入门槛，以及 Copy_Make_Border 的绝对差扩边、居中奇数截断和 BGR/alpha 规则。详见 [节点契约](../nodes/README.md)。

必要修复：参数和运行区域用锁/快照同步；畸形 JSON、非法坐标转换、尺寸溢出和 OpenCV 异常不退出 AutoTick；Perspective 不再沿用上一帧角点计数；Transform 的交互模式变更不再依赖可能被 Sync 消耗的标记，恢复状态不触发交互重置，且首帧前不使用默认宽高比覆盖保存尺寸。这些修复不改变合法输入的原计算顺序或另建状态格式。

| 验收入口 | 本批结果 |
| --- | --- |
| Windows MSVC x64 Release | 四个节点、Studio 和相关测试构建通过 |
| `phase2b_crop` | bbox 索引/合并、padding 差异、空数据与断开后保留、边界/溢出、运行区域状态、输入不变、异常恢复、Qt 显隐/编辑/生命周期通过 |
| `phase2b_transform` | 240 组复合操作、六种插值、可见字段状态、模式切换、锁比例恢复及首帧、无效尺寸/恢复通过 |
| `phase2b_perspective` | 四点排序、自动尺寸/比例、固定尺寸、五种插值、JSON 异常、条件状态、禁用门槛、Qt 控件/生命周期通过 |
| `phase2b_border` | size 输入、较小目标、奇偶居中、五种边界、RGB/BGRA、状态、尺寸溢出及 Qt 颜色/生命周期通过 |
| `graph_edit`、`studio` | 既有图编辑与窗口/插件生命周期自动化回归通过 |
| 最终选定 CTest | 6/6 通过，包括 Transform 恢复问题修复后的回归 |

四个节点都有上述复杂分支，因此仅围绕这些分支增加测试；不重新执行简单节点专项测试，不扩展为全部历史回归。本批 Qt 界面由 offscreen 控件测试验证，未进行原生桌面手测。

```powershell
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release -R '^(phase2b_.*|graph_edit|studio)$' --output-on-failure
```

## 节点迁移阶段 2A

新增 Abs_Diff、Add_Weighted、Subtract、Multiply、Divide、Max、Min、Bitwise、Mean、Scale_Abs、Normalize，均显式加入 Nodes 构建与 InternalNodeManager 注册。先单独构建并通过 Abs_Diff 测试，随后接入其余节点。

保留原库计算函数、输入要求、输出类型、默认参数、状态键和禁用语义。Abs_Diff/Max/Min 仍保存 JSON `null`；Normalize 的 Beta 即使隐藏也保存；Mean 保留 JSON 字段及均值/标准差分支的 float/double 区别。参数使用现有属性快照和通用面板，基础层不引入 Qt，本批未修改 SDK、MainWindow、PropertyPanel 或专属 Qt UI。

必要适配仅包括节点继承现有 FlowCV::Node、参数同步、通过既有属性校验拒绝非法枚举/非有限数值，以及 OpenCV 异常不退出处理线程。原库的有效路径保持不变：Subtract 仍支持合法有符号 8 位 mask，多通道 mask 被忽略；Bitwise 按原顺序缩放第二路输入；Normalize 的 Min Max 始终忽略 mask。不统一改写这些节点各自的规则。

用户要求本批只测试复杂操作，并明确先测试 Abs_Diff，因此验证范围为：

| 入口 | 本批结果 |
| --- | --- |
| Windows MSVC x64 Release | 全部新增节点及 Studio 构建通过 |
| `abs_diff` | 先独立通过；输出像素/类型、输入不变、注册、null 状态、双输入要求与禁用透传 |
| `phase2a_modes` | Bitwise 四模式、可选输入、缩放、mask；Normalize 四模式、Min Max 忽略 mask、隐藏 Beta 保存、非法状态和恢复，通过 |
| `phase2a_mask_json` | Subtract 三种 mask 模式、CV_8S/无效 mask；Mean 多开关、多 scale、多类型、JSON 精度与状态/实例隔离，通过 |
| `graph_edit`、`studio` | 保留的公共图编辑、插件及窗口/工程生命周期自动化回归通过 |
| 本批选定 CTest | 上述 5 项全部通过；没有重新执行全部历史测试 |
| Add_Weighted、Multiply、Divide、Max、Min、Scale_Abs | 仅源码核对与构建检查，按要求未添加或运行专项计算测试 |

本批不增加原生 GUI 手测，不将构建通过等同于全部参数组合或桌面操作验收。复现本批测试：

```powershell
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release -R '^(abs_diff|phase2a_modes|phase2a_mask_json|graph_edit|studio)$' --output-on-failure
```

## 节点迁移阶段 1

先完成 Convert_Color 并通过独立计算、状态和 Qt 控件测试，再接入本批其余节点。Convert_Color 使用原库完整转换表及表索引状态 `conv_code`；Threshold 保留四种方法及 RGB 范围转 HSV 的计算；Morphology 保留七种操作、三种核形状与边界参数；Contours 保留原端口、顶层轮廓遍历、严格面积比较以及绘制开关控制 JSON 字段的语义。

复杂 Controls 和 Json_Viewer Main Dock 均经现有可选 Qt 注册层接入；ImageWriter 通过真实 `.fp` 的可选提供器创建 Controls。本批没有向 MainWindow 或 PropertyPanel 添加节点类型判断。状态、默认值、端口与禁用语义见 [节点说明](../nodes/README.md) 和 [插件说明](../plugins/README.md)。

新增验收入口：

| 测试 | 覆盖 |
| --- | --- |
| `convert_color` | 原库完整转换表与 6 种输入类型，对照 OpenCV；无效组合透传/错误提示、状态恢复、原图不变、禁用及 Qt 编辑 |
| `phase1_filters` | Threshold 四种方法与各阈值类型、Morphology 操作/形状、精确像素、嵌套颜色状态、非法参数、控件编辑与销毁、窄属性栏布局 |
| `phase1_analysis` | Contours 模式/方法、洞与面积过滤、精确图像和 JSON、禁用；Get_Size/Resize 数据契约；JSON 表格/原文/复制、多实例及控件生命周期 |
| `image_writer` | 真实动态库、中文路径及像素、单次请求、禁用/空输入、失败恢复、`null` 状态；Qt 文件选择/写入按钮和动态库租约 |
| `phase1_pipeline` | 11 节点、12 连线完整链路；精确输出、混合子图粘贴、参数/位置/连线保存恢复、同 ID 换工程释放控件、销毁整个窗口与 Manager 后重开、buffer=3 |

Windows MSVC x64 Release 构建通过；最终 CTest 10/10 通过（原有 5 项与新增 5 项），包含最后的滚动属性组布局修复。关闭 Studio 的独立构建已通过，计算层不引入 Qt；对应 CTest 4/4 通过（convert_color、image_loader_integration、image_writer、phase1_filters）。PluginMaker Python 测试本批重跑，7 项通过，2 项因 Windows 符号链接权限跳过。

原生 Windows 窗口已通过 Ctrl+O 打开完整静态分析工程，显示两个 Viewer、轮廓表格与完整 JSON；ImageWriter 的 Set Output Path 文件选择和 Write Image 实际写出中文文件名 PNG。颜色选择对话框可正常打开和取消；最终重启复查颜色控件无裁切、Simple 参数页无多余空白，正常退出码为 0。算法像素和保存重开由自动化进一步校验，不将所有参数组合都标为原生手测。

本批必要适配：

- 用可选 QWidget Controls/Main 替换 ImGui，RGB→HSV 保留原算式；计算线程只读参数快照或互斥锁保护的数据，GUI 使用定时器和弱引用。
- 校验转换索引、参数范围和非有限数值；捕获 OpenCV 不兼容输入，避免异常退出 AutoTick。Convert_Color 保留原库错误时透传；Threshold、Morphology 出错时不输出并显示错误；Contours 将 findContours 纳入原有异常边界。Morphology 原库 Wrap/Transparent 选项仍保留，不伪装成其它边界方式。
- Json_Viewer 的空字符串状态允许往返恢复，解决无参数节点保存、粘贴时的空状态解析问题；临时 JSON 缓存与显示模式不写入工程。
- ImageWriter 保留 `OutOfOrder`、`.fp`/`Create`、单次请求和 `null` 状态。为支持 Windows 中文路径，以相同默认编码参数 `imencode` 后用原生路径写入；实际写入加锁，失败提示并消耗请求，避免每帧重复失败。输出路径和待写请求仍不持久化。
- 颜色控件在窄属性栏竖向排列，Threshold 只占当前参数页的高度，避免新增复杂控件裁切其它属性组；不修改公共属性布局架构。

## 验证记录

- ImageLoader 基线：本轮开始时已重新执行原有测试，CTest 2/2 通过，覆盖真实 `.fp`、像素、中文路径、FPS 状态恢复、多实例、AutoTick 参数变更与 Qt 参数编辑。这两项在最终集成测试中继续保留。
- 最终构建与自动化：启用状态原子化后，干净 Windows MSVC x64 Release 构建 93 项完成；CTest 5/5 通过（`image_loader_integration`、`image_loader_ui`、`graph_edit`、`viewer`、`studio`）。`studio` 进一步覆盖四级日志映射、Settings 保存/取消、重启参数应用，以及确定图片经 Blur 后送入双 Viewer，保存并销毁整个窗口/Manager，再从最近文件恢复参数、位置、连线与精确像素。生成器 Python 测试 7 项通过、2 项因 Windows 符号链接权限跳过；三类生成模板在隔离工程中 Release 构建 20/20 项完成，并通过实际加载与运行验证。最终文档入口与对话框配色调整也已增量构建通过。
- 本轮原生 GUI：在实际 Studio Windows 窗口通过 Ctrl+O 文件选择打开 `ImageLoader → Blur → 双 Viewer`；Tab 搜索 Image_Loader、F 全图定位、D 启用/禁用及交叉标记、S 方向动画均可用，实际截图箭头朝向下游，UI FPS 约 21（仅该次观测）。Ctrl+C/V 创建 Blur_1 并保留禁用状态，Ctrl+X 删除副本后原图保留。实际鼠标将 Blur.out 拖至空白处，经 Views → Viewer 创建并自动连线，得到 5 节点、4 连线及第三个正常显示的 Viewer。
- 原生界面生命周期与保存：Viewer_0 选 Lum 不影响其它 RGB 预览；Viewer_2 Dock 关闭后可经 View → Node Windows 重开；拖动可形成 Viewer_1/Viewer_2 标签；关闭 Controls 后双击 Blur 可重开。Ctrl+S 保存后正常退出，退出码为 0。重启后通过 File → Load Recent 恢复 5 节点、4 连线、三个 Viewer 及此前标签布局，退出码仍为 0。Settings 显示的配置与 `--cfg` 文件一致。
- 验证边界：Ctrl 像素检查由 Qt 事件测试验证，本轮原生操作工具无法保持 Ctrl 按键，因此不标为原生手测通过。高位深、多格式转换及滚动末端由自动化覆盖，未逐格式进行原生窗口手测。工程 Open 的原生文件选择已验证，ImageLoader 的 Browse 图片对话框与中文路径仍分别按实际测试来源记录。

可重复执行的验证命令：

```powershell
cmake --preset local-x64-release -DVISIONFLOW_BUILD_TESTS=ON
cmake --build --preset local-x64-release --clean-first --parallel 4
ctest --test-dir out/build/local-x64-release --output-on-failure
python -B tests/test_plugin_maker.py
```

## 兼容与必要适配

- 保留 DSPatch 调度、`.fp`/`Create` 插件入口、`.flow` 中的 `nodes`/`connections`/`editor` 结构，基础 SDK、Managers 和计算节点不因自定义界面而依赖 Qt。
- 原库 Width/Height 调整窗口另一维以保持整幅图像。Qt Dock 中按所选宽/高缩放画布，另一维超出时提供滚动条；不强制改变其它 Dock 或裁掉不可访问的图像区域。
- 子图复制包含无参数节点（如 Viewer），粘贴重分配节点 ID 和实例编号。失败只清理本次新增对象，并恢复 AutoTick；这是避免丢节点、破坏已有图和执行永久停止的必要补全。
- 插件 UI 先于节点及动态库释放；实例通过共享模块租约保持库存活，防止自定义控件、定时器和回调进入已卸载代码。这不等于插件热重载或崩溃隔离。
- DSPatch 的启用标记由普通 bool 改为 `atomic<bool>`，保留 SetEnabled/IsEnabled 及节点自启用语义，消除工作线程写入与 GUI 绘制/工程 GetState 并发读取的竞态；不改变调度机制。
- 高位深预览沿用原库“scale 转 8 位并饱和 → 归一化到 1..255 → 伪彩”的顺序，不改成原始数值直接归一化。显示转换不修改输入图像；Ctrl 检查反映所选显示通道的颜色。

## 当前边界与下一步

原库范围内生产节点已全部接入；下一步按 [验收安排](flowcv-migration-plan.md) 使用真实业务模型/输入、串口回环设备补齐外部条件，再执行剩余人工与跨平台验收。后续库外扩展继续使用 [PluginMaker](../tools/README.md) 与通用 Qt 接口，不在主窗口追加节点判断。

以下是本轮明确排除的能力，不能算作此轮遗漏：独立 ProcessEngine、Undo/Redo、运行暂停与单步工具栏、热重载、插件崩溃隔离、性能分析、CI、SDK 安装导出和安装包。GLFW VSync 不适用于当前纯 Qt 渲染后端。

仍需保留的运行限制：完整工程 `SetState()` 失败可能留下部分新图，没有整体事务回滚；子图粘贴回滚不能扩大解释为工程加载回滚。Viewer 的显示选项按面板独立保存在内存中，不写入算法节点参数。ImageLoader 仍按原库规则输出 8-bit BGR；高位深 Viewer 能力供其它图像源使用。

日志由正式入口初始化，沿用日志模块的工作目录下 `logs/` 路径。源节点 FPS、Viewer 的 33 ms 刷新和 UI FPS 统计是不同概念，不代表算法吞吐保证。
