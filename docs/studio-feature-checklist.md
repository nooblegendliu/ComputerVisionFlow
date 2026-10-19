# Studio 公共功能对照与验收清单

核对日期：2026-10-19；原库基准：本机 FlowCV `b09c48e`。此表记录原库范围内节点与公共功能，排除独立 ProcessEngine 和原库外扩展。原库路径以其仓库根目录为起点；当前入口链接指向本项目。

本日追加[源码全面复核与剩余工作](flowcv-source-audit-2026-10-19.md)：71 节点接入数保持不变；Add mask、自连校验、大小写引用属于代码差异，空集合与 Viewer 取色属于格式/行为差异。历史自动化通过范围不因此扩大，也不能将这些项目归为仅未人工验收。

“代码已接入”只说明当前实现存在，实际通过结果必须由执行日志补录。自动化按 CTest 名称列出；GUI 手测未经执行不得勾选。

## 阶段 3D–7 验收（2026-10-19）

同日源码复核后的局部修复已完成：CSV/视频运行中编辑保留文件、通信非连接参数编辑保留连接、Solid 非默认状态、DNN 控件步进的复杂回归，加上图编辑、Studio 和71节点清单，共8/8通过。Add、Canny、自连拦截、Viewer include 大小写及损坏配置恢复仅源码/构建检查；未增加简单计算专项。详细结果见[开发进度的修复记录](development-status.md)。这批结果不替代真实串口、实际模型、人工与跨平台验收。

同批 Studio OFF 构建通过，Solid 状态、视频、DataOutput 三项复杂回归3/3通过。

| 当前入口 | 验收结果 |
| --- | --- |
| Histogram / Depth_Viewer_3D | 确定数值与状态、Qt Controls通过；Depth原生GL实际像素/相机/多实例/资源重建通过 |
| Video_Loader / Video_Writer / Video_Capture | 实际MJPG录放和Qt通过；本机真实摄像头内存采集、None释放/重开、析构重建通过；手动驱动属性未验收 |
| 五个DataOutput插件 | CSV与UDP/TCP/OSC本机回环、重连/坏状态/Qt生命周期通过；Serial真实往返未验收 |
| 七个DNN节点 | 确定小模型实际CPU推理与原后处理/状态/Qt通过；实际模型精度、GPU与未覆盖模型分支单列 |
| 71节点契约 / 七代表flow | 原库名称、分类、版本、端口、状态键及完整混合工程恢复、释放通过 |
| 全部新UI与公共交互 | stage7_studio验证Controls/Main关闭重开、同ID换工程、三缓冲、About、Documentation URL、Browse取消、Ctrl像素提示；来源为Qt自动化 |
| 构建与回归 | 新目录干净Release；37项复杂/公共回归+新增Studio收尾1项，共38项通过；Studio OFF五专项5/5通过；简单Abs_Diff专项未运行；PluginMaker 7项通过、2项符号链接权限不足而跳过 |
| 人工与外部条件 | 真实串口、实际模型/GPU、摄像头手动属性/驱动窗口、文件确认/文档网页呈现和其它平台仍待实际验收；不标为通过 |

复现命令、必要偏离与首轮失败修复见 [开发进度](development-status.md)。

## 节点迁移阶段 3C 验收（2026-10-19）

| 当前入口 | 验收结果 |
| --- | --- |
| Draw_Text / Draw_Number；nodes/DrawTools | 原算法、输入优先级、默认值及状态核对；计算和 Controls 构建通过，未加简单节点专项 |
| Draw_Date_Time / DrawDateTimeControls | 原格式及条件状态、复杂格式错误、实际像素、Qt 草稿/开关/销毁专项通过 |
| Draw_JSON / DrawJsonControls | JSON Pointer、列表/数组/浮点/像素、缺空输入、状态与 Qt 增删/旧行回调专项通过 |
| 混合工程 | Shape_Counter → Draw_JSON → Viewer 结果标注、复制、保存重开、三缓冲及 Controls 释放通过 |
| 构建与回归 | Studio Release 完整构建通过；本批两项复杂专项及 phase1_pipeline、graph_edit、studio，共 5/5 通过；Studio OFF 计算构建及两专项 2/2 通过 |
| 原生手测 | 本批未执行；复杂 Controls 验证来源为 Qt offscreen 自动化 |

原状态契约与必要修复见 [内部节点](../nodes/README.md) 和 [开发进度](development-status.md)。

## 节点迁移阶段 3B 验收（2026-10-19）

| 当前入口 | 验收结果 |
| --- | --- |
| ShapeColorizer.fp / Shape_Colorizer | 三种颜色模式、原 JSON/状态列表、复杂 ROI、Qt 编辑/旧行回调与真实动态库生命周期，专项通过 |
| ShapeCounter.fp / Shape_Counter | 尺寸/类别分支、overlay 像素与边界、隐藏状态/原位置回写、Qt RGBA/恢复，专项通过 |
| SimpleBlobTracker.fp / Simple_Blob_Tracker | 原跨帧 ID/匹配/过期/历史/复位、状态边界、Qt 与模块租约，专项通过 |
| 混合工程 | Blob→Colorizer→Counter、Blob→Tracker 的图像/JSON、复制、保存重开、多缓冲及 Controls 释放，通过扩展 phase1_pipeline 验证 |
| 构建/回归 | Studio Release 完整构建和本批选定测试 6/6 通过；关闭 Studio 后三插件专项 3/3 通过，fp 无 Qt DLL 依赖 |
| 原生手测 | 本批未执行；新 Qt 界面来源为 offscreen 自动化。简单节点未额外测试 |

状态契约、必要修复与复现命令见 [插件说明](../plugins/README.md) 和 [开发进度](development-status.md)。

## 节点迁移阶段 3 首批验收

本节为节点迁移阶段，不改变下方公共能力的阶段编号。Background_Subtraction 先独立实现并测试通过，随后完成本批其余四节点。

| 原库节点与当前入口 | 验收结果 |
| --- | --- |
| Background_Subtraction；`nodes/BackgroundSubtraction` | `stage3_background`：跨帧 MOG2/KNN、切换保留历史、实时参数/状态、阴影、禁用不学习、支持类型及错误恢复通过 |
| Blob_Detector；`nodes/BlobDetector`、BlobControls | `stage3_blob`：OpenCV 参考检测与绘制、过滤、JSON、隐藏状态、Qt 条件编辑/颜色/生命周期通过 |
| Hough_Circles / Hough_Lines；`nodes/HoughCircles`、`nodes/HoughLines`、HoughControls | `stage3_hough`：三种检测路径、原 JSON 与边界端点、颜色/文本绘制、空结果、状态、错误恢复及 Qt 操作通过 |
| Line_Intersections；`nodes/LineIntersect`、LineIntersectControls | `stage3_intersections`：无限直线、方向角、距离阈值、200 条上限、data:null、绘制、状态、Qt 计数/条件显示及销毁通过 |
| 混合工程 | 扩展 `phase1_pipeline` 包含上述五节点，真实插件工程复制、保存重开、隐藏状态与 UI 生命周期通过 |
| 构建与回归 | Windows Release 完整构建通过；上述专项/工程与 graph_edit、studio 共 7/7 通过；未重跑全部历史测试 |
| 原生界面手测 | 本批未执行；新 Controls 验证来源为 Qt offscreen 自动化 |

必要偏离、Blob 新版 OpenCV 零下限兼容及剩余范围见 [开发进度](development-status.md) 和 [移植计划](flowcv-migration-plan.md)。

## 原库能力与当前入口

| 阶段 | 原库能力 / 原实现位置 | 当前入口 | 验收方法与本轮结果 |
| --- | --- | --- | --- |
| 0 | 工程存取、位置及未保存提示；`Editor_UI/Common/entry.cpp`、Node Editor | [MainWindow](../app/VisionFlowStudio/MainWindow.cpp) 的 File New/Open/Save/Save As，`.flow` | 本轮前已实现；`studio` 往返测试通过 |
| 0 | 动态 `.fp` / `Create`；SDK PluginManager | [插件管理](../managers/plugin_manager.cpp)、ImageLoader、启动扫描 | 历史核心与 UI 两项测试通过；本轮回归通过（CTest 5/5 中的两项） |
| 1 | `HasGui/UpdateGui(Controls/Main)`；SDK 与 `entry.cpp` | [NodeUiProvider](../editor/NodeUi/NodeUiProvider.h)、[Registry](../editor/NodeUi/NodeUiRegistry.cpp)，Properties / Node Windows | 首轮自动化通过；`studio` 检查插件优先、Controls 重开、Main Dock、同 ID 换工程、删除和库租约 |
| 2 | 应用配置、额外插件目录、缓冲数、日志级别；`app_settings.cpp`、`entry.cpp` | [StudioSettings](../app/VisionFlowStudio/StudioSettings.cpp)、Edit → Settings、`--cfg` | `studio` 验证配置保存/取消、目录加载、buffer 1→0 与 2/3、重启生效及 debug/info/warn/off 映射；正式程序启动日志及 Settings 配置显示已原生检查 |
| 2 | Load Recent；`entry.cpp` | File → Load Recent | `studio` 数量、顺序与实际 action 打开通过；Windows 重启后菜单打开完整工程通过 |
| 3 | 通道、比例、高位深伪彩、Ctrl 检查；`FlowCV_SDK/third-party/imgui_wrapper/imgui_opencv.cpp` | [ViewerPanel](../app/VisionFlowStudio/ViewerPanel.cpp)、[ViewerImage](../app/VisionFlowStudio/ViewerImage.cpp) | 首轮自动化通过；`viewer` 检查全部支持类型、通道、scale/色图、源数据不变、UV/边缘、滚动、tooltip、多面板独立 |
| 4 | Cut/Copy/Paste；`Editor_UI/Node_Editor/node_editor.cpp` | [VisionFlowEditor](../editor/VisionFlowEditor.cpp)，Edit 菜单和 Ctrl+X/C/V | 首轮自动化通过；`graph_edit` 检查内部/动态混合子图、无状态节点、参数、位置、内部边、重编号、重复粘贴及失败回滚 |
| 4 | D 禁用、信息窗口、F 定位；Node Editor | 画布 D/F、节点右键 Enabled / Node Information | 自动化通过；实际窗口 D 切换及禁用交叉标记、F 全图定位通过；Node Information 描述由自动化检查 |
| 4 | Tab 搜索、类别菜单、拖线到空白创建；Node Editor | 画布 Tab / 右键 Add Node / 端口拖空，Node Library 搜索框 | 自动化通过；原生 Tab 搜索 Image_Loader、鼠标拖 Blur.out 至空白 → Views → Viewer 创建并自动连线通过 |
| 5 | 分类颜色、端口标识、Show Flow；Node Editor | [图绘制适配](../editor/Graph/GraphPainters.hpp)，View → Show Flow / S | 自动化绘制与端口检查通过；原生 S 切换成功，实际截图确认动画箭头朝向下游 |
| 5 | UI FPS、Help、About；`entry.cpp` | Edit → Settings / 状态栏 / Help | 原生窗口观察 UI FPS 约 21，仅为该次界面绘制统计；Help/About 点击未记录手测 |
| 5 | PluginMaker 和节点模板；`Tools/PluginMaker.py` | [本地 PluginMaker](../tools/PluginMaker.py)、[工具说明](../tools/README.md) | Python 7 pass / 2 skip；隔离三类模板 Release 构建 20/20 完成，并通过实际加载运行 |

## 集成验收记录

| 验收项 | 结果 |
| --- | --- |
| 本轮前 ImageLoader 基线 | 本轮开始时已重新执行，CTest 2/2 通过 |
| 公共功能基线 Release 完整构建 | 原子状态修复后最终干净构建 93 项通过；文档入口和对话框配色调整增量构建通过；后续节点构建见下表 |
| 公共功能基线 CTest | 5/5 通过，包括扩充后的设置和双 Viewer 完整工程重开测试；后续节点回归见下表 |
| PluginMaker Python 测试 | 7 项通过、2 项因 Windows 符号链接权限跳过 |
| 生成的普通插件、Qt 插件、内部节点编译 | 隔离工程 Release 构建 20/20 完成 |
| 模板加载、状态编辑保存恢复与销毁 | 三种模板实际运行通过；两种真实 .fp 加载、状态往返、25→50 增益、禁用透传、原图不变、Qt Controls 编辑/复位及控件先于库销毁通过 |
| 原生文件选择与中文路径 | Ctrl+O 原生工程文件选择并载入通过；ImageLoader Browse 图片对话框未记录原生手测，中文图片路径由原有自动化覆盖 |
| 实际鼠标拖线、Tab/D/F/S 和剪贴板快捷键 | 原生全部通过；Ctrl+C/V 新建 Blur_1 且保留禁用状态，Ctrl+X 仅删除副本；拖 Blur.out → Views → Viewer 后为 5 节点、4 连线 |
| 多 Dock 关闭重开与拖动 | 原生 Viewer_2 关闭并经 Node Windows 重开通过；拖动形成 Viewer_1/Viewer_2 标签通过 |
| Controls 关闭重开 | 原生关闭分组后双击 Blur 重开通过 |
| 保存布局、重启恢复与最近文件重开 | Windows 正常退出并重启后，经 Load Recent 恢复工程和 Viewer_1/Viewer_2 标签布局，通过 |
| Viewer 显示与通道独立 | 原生多 Viewer 正常显示；Viewer_0 选 Lum，其它 RGB 不变 |
| Viewer 多格式、Ctrl 检查与滚动末端 | 转换、Qt 事件 tooltip 和坐标自动化通过；未逐格式原生手测，Ctrl 因工具无法保持按键未标原生通过 |
| `ImageLoader → Blur → 多 Viewer`，保存/关闭/重开/退出 | 自动化验证两路精确像素及全部状态；Windows 完成打开、编辑至三个 Viewer、保存、关闭、重启、最近文件重开及再次退出（exit 0） |

## 节点迁移阶段 1 验收

本节与上文公共功能阶段编号分别计数。计算、状态、端口及必要差异说明见 [开发进度](development-status.md)、[节点](../nodes/README.md) 和 [插件](../plugins/README.md)。

| 原库节点与当前入口 | 验收结果 |
| --- | --- |
| 本批 Release 与完整回归 | MSVC x64 Release 构建通过；最终 CTest 10/10 通过 |
| Convert_Color；`nodes/ConvertColor`、ConvertColorControls | 独立先验收；完整代码表对照 OpenCV、多类型、错误透传、状态与 Qt 编辑通过 |
| Threshold、Morphology；对应 nodes 目录、FilterControls | 方法/操作/形状、确定像素、RGB 嵌套状态、非法输入和控件生命周期通过；窄属性栏及实际滚动布局回归通过 |
| Contours；`nodes/Contours`、AnalysisControls | 四种模式/四种方法、洞、严格面积、原库 JSON、双输出、输入不变及状态往返通过 |
| Json_Viewer；`nodes/JsonViewer`、通用 Main Dock | 表格、原文、复制、非数组 JSON、大整数、多实例、空状态与销毁通过；实际窗口表格/原文显示通过 |
| Get_Size；`nodes/GetSize` | `[width,height]`、空输入和禁用；完整链路连接 Resize.size，输出像素通过 |
| Image_Writer；`plugins/ImageWriter`、可选 Qt Controls | 真实 .fp、中文路径、输出像素、请求/禁用/错误、null 状态、文件选择及库生命周期通过；实际窗口选择中文输出路径并写出 PNG 通过 |
| 完整静态分析链路 | `phase1_pipeline` 验证 11 节点/12 连线、图像及 JSON、混合粘贴、保存重开、同 ID 控件释放、全窗口/Manager 销毁后 buffer=3 重开通过 |
| 无 Qt 构建 | `VISIONFLOW_BUILD_STUDIO=OFF` 构建通过，对应 4/4 CTest 通过；普通计算插件不依赖 Qt |
| 模板回归 | Python 7 项通过，2 项因 Windows 符号链接权限跳过；本批未修改模板 |
| 原生属性栏与退出 | 颜色选择对话框打开/取消通过；最终重启确认 RGB 无裁切、Threshold Simple 无多余空白，正常退出码为 0 |

## 节点迁移阶段 2A 验收

| 原库节点 / 当前入口 | 本批结果 |
| --- | --- |
| Abs_Diff；`nodes/AbsDiff` | 按明确要求首先独立测试通过，保留 null 状态、双输入及禁用语义 |
| Subtract；`nodes/Subtract` | 三种 mask 合成、CV_8S、无效 mask、状态、输入匹配与禁用测试通过 |
| Bitwise；`nodes/Bitwise` | AND/NOT/OR/XOR、可选 in2、resize、mask、状态及错误恢复通过 |
| Normalize；`nodes/Normalize` | 四种模式、Min Max 忽略 mask、输出位深、隐藏 Beta 持久化及错误恢复通过 |
| Mean；`nodes/Mean` | 均值/标准差开关、scale、JSON 格式与分支精度、状态/实例隔离通过 |
| Add_Weighted、Multiply、Divide、Max、Min、Scale_Abs | 源码核对、构建注册完成；按用户要求不做简单节点计算测试 |
| 本批构建与回归 | Release 构建通过；abs_diff、phase2a_modes、phase2a_mask_json、graph_edit、studio 共 5/5 通过 |
| 界面范围 | 沿用通用属性面板，无新增专属 Qt 控件；本批未做原生窗口手测 |

本批完整状态与必要适配见 [开发进度](development-status.md)。上述记录不替代前批测试，也不声称重新执行全部历史测试。

## 节点迁移阶段 2B 验收

| 原库节点 / 当前入口 | 本批结果 |
| --- | --- |
| Crop；`nodes/Crop`、CropControls | JSON bbox 索引/合并、padding、区域裁剪写回、状态和异常、Qt 条件显隐及生命周期通过 |
| Transform；`nodes/Transform`、通用 Properties | 240 组复合操作与插值、动态参数、可见字段保存、首帧前锁比例恢复、异常恢复通过 |
| Perspective_Warp；`nodes/PerspectiveWarp`、PerspectiveControls | 四点几何、自动/固定尺寸、JSON 与条件状态、五种插值、Qt 提示/参数/生命周期通过 |
| Copy_Make_Border；`nodes/CopyMakeBorder`、BorderControls | 可选 size、绝对差扩边、居中截断、边界和颜色、状态/溢出、Qt 颜色与生命周期通过 |
| 本批构建与回归 | Release 构建通过；四项 phase2b 测试加 graph_edit、studio，最终 6/6 通过 |
| 界面与测试边界 | 复用 Controls 注册表，未给主窗口或公共属性面板添加类型判断；Qt offscreen 自动化通过，未做原生桌面手测；未追加简单节点测试 |

原库语义与必要修复记录见 [开发进度](development-status.md) 和 [内部节点](../nodes/README.md)。

## 节点迁移阶段 2C 验收

| 原库节点 / 当前入口 | 本批结果 |
| --- | --- |
| Color_Correct；`nodes/Colorcorrect`、ColorCorrectControls | 首先独立执行 `phase2c_color_correct`，1/1 通过；计算顺序、mask=255/缩放、RGB 范围、alpha、gamma=0、状态、异常恢复及 Qt 编辑/Reset/生命周期 |
| Color_Reduce；`nodes/ColorReduce`、通用 Properties | 源码核对完成，保留运行时 Reduce 与 null 状态、逐字节量化；仅做构建检查，不新增专项计算测试 |
| Split、Combine；对应 nodes 目录、Split 提示 Controls | 源码核对完成，保留通道映射、merge、可选 alpha、null 状态及自启用；仅做构建检查，不新增专项计算测试 |
| 本批构建与公共回归 | 最终 Release 构建通过；phase2c_color_correct、graph_edit、studio 合计 3/3 通过（1.71 秒） |
| 界面范围 | 复用现有 Qt 工厂注册，Color_Correct offscreen 自动化通过；未做原生桌面手测 |

原库状态与必要修复详见 [开发进度](development-status.md) 和 [内部节点](../nodes/README.md)。构建检查不替代三个简单节点的专项运行验收。

## 节点迁移阶段 2D 验收

| 原库节点 / 当前入口 | 本批结果 |
| --- | --- |
| Sobel；`nodes/SobelFilter`、DerivativeControls | 首先独立执行 `phase2d_sobel`，1/1 通过；梯度、边界、深度、核/ROI、状态和错误恢复；Qt 核增减/恢复/生命周期测试通过 |
| Sharpen；`nodes/Sharpen`、通用 Properties | 两种复杂模式、原位 Laplacian 顺序、多位深/饱和、状态、输入复制及错误恢复专项通过 |
| Scharr、Laplacian；对应 nodes 目录，分别使用通用 Properties / DerivativeControls | 原库计算和状态核对、Release 构建通过；不新增专项计算测试 |
| 本批构建与公共回归 | 最终 Release 构建通过；phase2d_sobel、phase2d_sharpen、graph_edit、studio 合计 4/4 通过（1.71 秒） |
| 界面范围 | Sobel/Laplacian 可选 Controls 保留核步长 2，Scharr/Sharpen 使用通用属性；没有 Main Dock，未修改主窗口/公共属性面板；未做原生桌面手测 |

原库状态、模式限制及必要防护见 [开发进度](development-status.md) 和 [内部节点](../nodes/README.md)。简单节点的构建检查不替代专项运行验收。

## 节点迁移阶段 2E 验收

| 原库节点 / 当前入口 | 本批结果 |
| --- | --- |
| Get_Optimal_DFT_Size；`nodes/GetOptimalDft` | 优先完成源码核对和 Nodes/Managers 构建；图像优先、尺寸数组与禁用规则保留，无专项计算测试 |
| DFT、DCT；对应 nodes 目录、通用 Properties | 幅值输出/变换顺序、浮点转换、尺寸/输入边界及原状态专项通过；DCT 尺寸支持与本机 OpenCV 一致 |
| Rnd_Noise；`nodes/RndNoise`、通用 Properties | 三模式类型/范围、状态恢复、FPS 节奏、UI 默认范围外状态、非法状态与错误恢复专项通过 |
| Magnitude；`nodes/Magnitude` | 源码核对及 Release 构建通过；不增加简单节点专项测试 |
| 本批构建与公共回归 | 最终 Release 构建通过；phase2e_frequency、phase2e_noise、graph_edit、studio 合计 4/4 通过（1.30 秒） |
| 界面范围 | 全部复用通用属性或无参数入口；没有新增 Controls/Main，不修改主窗口或公共属性面板；未做原生桌面手测 |

DFT 输出幅值、DCT 运行模式不保存、随机源恢复修复等契约见 [内部节点](../nodes/README.md)；测试结果见 [开发进度](development-status.md)。简单节点仅构建检查，不据此宣称专项运行通过。

## 必要差异与排除项

- 原库直接提供 ImGui 界面；当前使用可选 Qt 适配层。插件提供器、注册工厂、通用属性面板按优先级选择；只迁移节点界面，不让基础计算 SDK 依赖 Qt。
- 原库 Width/Height 调整窗口尺寸；Qt Dock 按指定维缩放并通过另一维滚动完整访问图像，避免干扰其它 Dock。
- 无参数 Viewer 必须参与复制。粘贴事务只清理本次新增节点和连线，并恢复原 AutoTick 状态；这项补全不改变 `.flow` 格式，也不提供整个工程加载事务回滚。
- 节点实例持有动态库租约，宿主先释放控件及回调再释放节点和库，补全 Qt 控件生命周期要求。插件仍需与当前 SDK、编译器及相关依赖匹配构建。
- DSPatch 启用标记改为 `atomic<bool>`，消除源节点自动启用与 GUI 绘图/工程状态读取并发时的数据竞争；接口与调度语义保持不变。
- Ctrl 提示基于所选显示通道；高位深按原库顺序转换，不读取原始科学数值。界面 FPS 和 Show Flow 不表示算法吞吐。
- 不纳入此次完成标准：ProcessEngine、剩余业务节点、Undo/Redo、运行暂停/单步、热重载、插件隔离、性能分析、CI、SDK 安装导出和安装包。GLFW VSync 不迁入纯 Qt 后端。

完成标准是上述公共能力通过与其风险相称的测试，未执行的 GUI 项被明确列出；不能因代码接入就宣称所有验收通过。后续新增节点应仅需实现计算、状态、参数或专属 Qt UI，并显式注册构建。
