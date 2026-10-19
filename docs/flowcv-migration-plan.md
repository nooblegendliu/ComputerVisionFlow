# FlowCV 剩余移植计划

更新日期：2026-10-19；沿用本机 FlowCV `b09c48e` 功能基准。阶段 3D–7 的剩余代码已接入，原库 58 个内部节点和 13 个插件均可构建、注册和恢复；追加[源码全面复核](flowcv-source-audit-2026-10-19.md)仍发现若干行为与格式差异，剩余工作包括代码修正、契约核验及特定设备、模型和人工验收。构建与验收见 [开发进度](development-status.md)。

## 结论与统计口径

Studio 公共接入框架、静态图像分析输出链路和阶段 2A 算术节点已具备，节点计算、状态、参数和专属 Qt 界面迁移已完成，后续主线是特定环境验收。阶段 2 的 28 个基础图像处理节点均已接入，阶段 3 首批五个检测节点已接入，阶段 3B/3C/3D、视频、导出通信、DNN 与三维显示均已接入；本机自动化与未验证条件分别记录。

| 类型 | 本机原库 | 当前已接入 | 待接入 |
| --- | ---: | ---: | ---: |
| 内部节点 | 58 | 58 | 0 |
| 动态插件节点 | 13 | 13 | 0 |
| 合计 | 71 | 71 | 0 |

内部节点按原库 `Managers/Internal_Node_Manager.cpp` 中的 58 项注册核对，并与 `Internal_Nodes/**/*.cpp` 的 `SetComponentName_` 对照。动态插件按根 `CMakeLists.txt` 与各插件构建文件核对：9 个插件目录实际生成 13 个节点，其中 DataOutput 生成 5 个独立插件。不能按目录数统计。当前注册依据为 [内部节点管理器](../managers/node_manager.cpp)、[节点构建](../nodes/CMakeLists.txt) 和 [插件构建](../plugins/CMakeLists.txt)。

已接入内部节点：`Add`、`Blur`、`Canny`、`Resize`、`Solid`、`Viewer`、`Convert_Color`、`Threshold`、`Morphology`、`Contours`、`Json_Viewer`、`Get_Size`，以及阶段 2A 的 `Abs_Diff`、`Add_Weighted`、`Subtract`、`Multiply`、`Divide`、`Max`、`Min`、`Bitwise`、`Mean`、`Scale_Abs`、`Normalize`，阶段 2B 的 `Crop`、`Transform`、`Perspective_Warp`、`Copy_Make_Border`，阶段 2C 的 `Color_Correct`、`Color_Reduce`、`Split`、`Combine`，阶段 2D 的 `Sobel`、`Scharr`、`Laplacian`、`Sharpen`，阶段 2E 的 `Get_Optimal_DFT_Size`、`DFT`、`DCT`、`Magnitude`、`Rnd_Noise`，阶段 3 首批 `Background_Subtraction`、`Blob_Detector`、`Hough_Circles`、`Hough_Lines`、`Line_Intersections`，阶段 3C `Draw_Text`、`Draw_Number`、`Draw_Date_Time`、`Draw_JSON`，以及 `Histogram`、`Classification`、`Object_Detection`、`Segmentation`、`Human_Pose`、`Text_Detection`、`Text_Recognition`、`Image_Processing`、`Depth_Viewer_3D`；已接入插件：`Image_Loader`、`Image_Writer`、`Shape_Colorizer`、`Shape_Counter`、`Simple_Blob_Tracker`、`Video_Loader`、`Video_Writer`、`Video_Capture`、`CSV_File`、`UDP_Send`、`TCP_Send`、`OSC_Send`、`Serial_Send`。测试专用插件、生成器演示节点不计入生产节点数。这是接入数量，不代表工时完成比例，也不替代逐节点完整行为验收。

已完成的公共能力不再安排重做：工程保存恢复、动态插件、通用 Controls/Main 接入、多 Viewer、设置、最近文件、图编辑、节点检索、方向动画及三类接入模板。公共功能基线与阶段 1 新增计算、状态、真实插件及完整工程测试记录见 [开发进度](development-status.md)。

## 代码交付与验收顺序

本机原库范围内待接入节点数量已归零。阶段 3D–7 的生产代码、可选 Qt 界面、独立插件、契约矩阵与代表工程已交付；干净 Release、复杂专项、真实摄像头、原生 OpenGL 及 Qt 工程生命周期自动化已通过。该数量不代表所有设备、真实模型或全部后端已经验收。剩余条件按优先级为：实际业务模型/输入验收 → 真实串口回环 → 摄像头手动属性/驱动窗口 → 代表人工操作与其它平台。详见开发进度。

| 优先顺序 | 批次 | 本批内容（原执行顺序） | 完成标准 | 完成后累计 |
| --- | --- | --- | --- | ---: |
| 已完成 | 3B 结果消费插件 | Shape_Colorizer → Shape_Counter → Simple_Blob_Tracker | 检测 JSON 跨真实 `.fp` 传递，颜色分类/计数正确，跟踪 ID 和启停/跨帧历史符合原库；状态、Qt 列表编辑与卸载安全 | 50/71 |
| 已完成 | 3C 绘制 | Draw_Text → Draw_Number → Draw_Date_Time → Draw_JSON | 原字体、位置、颜色、格式及状态保留，已有图像/JSON 可直接标注；复杂格式/边界路径测试，简单绘制仅构建核对 | 54/71 |
| 代码已完成 | 3D 直方图 | Histogram | 计算与原模式一致，Qt Main Dock 可多实例独立显示，参数恢复、更新与销毁正确 | 55/71 |
| 代码已完成 | 4 视频链路 | Video_Loader → Video_Writer → Video_Capture | 文件读取/结束/循环/写出可复现，随后验收真实摄像头枚举、采集和设备释放 | 58/71 |
| 代码已完成 | 5 导出与通信 | CSV_File → UDP_Send → TCP_Send → OSC_Send → Serial_Send | CSV 临时文件与本机回环验证格式/失败恢复/重连；串口使用虚拟端口对或真实设备 | 63/71 |
| 代码已完成 | 6A DNN | Classification → Object_Detection → Segmentation → Human_Pose → Text_Detection → Text_Recognition → Image_Processing | 每类模型至少完成 CPU 推理、预/后处理、原 JSON 与模型路径状态验证，专属 Qt Controls 可用 | 70/71 |
| 代码已完成 | 6B 三维显示 | Depth_Viewer_3D | 深度/颜色/内参到点云正确；Qt Main 中相机交互、多实例和 GL 资源释放验收 | 71/71 |
| 代码已完成 | 7 整体收尾 | 原库工程兼容、代表链路、窗口手测、文档 | 干净 Release、既有自动化回归、代表 `.flow` 保存重开、节点/控件/插件/设备退出释放；补齐此前未记录的 GUI 手测 | 代码完成，环境验收单列 |

3B 的跟踪先使用确定的连续帧，不等待视频插件；视频录放先用文件，不等待摄像头。模型资产、摄像头和串口条件可提前准备，缺少某项条件时继续可验证部分，保留具体未验收项。节点专属 Qt 界面和必需依赖随节点交付，不集中拖到最终收尾。

节点接入完成后，仍需补齐的验收包括：阶段 2/3 新 Controls 的代表性人工操作、实际文件选择确认、浏览器文档页呈现及其它硬件/平台；ImageLoader Browse 取消、Help/About action、Ctrl 像素提示和完整节点工程资源释放已补 Qt 自动化。已经通过的自动化不重复标为“未实现”，未做的原生手测也不标为通过。完整工程加载失败的整体事务回滚仍是已知限制，按现有边界单列可靠性增强，不混入原库节点接入数量。

## 原阶段安排与已完成记录

| 顺序 | 阶段 | 新增内部 / 插件 | 交付结果 |
| --- | --- | ---: | --- |
| 0 | 固定节点清单和验收约定 | 0 / 0 | 本文完成清单；首批开发前固定测试图、原库状态样例和输出期望 |
| 1 | 静态图像分析闭环，已实现 | 6 / 1 | 彩色输入、二值化、轮廓 JSON、预览和图片输出贯通 |
| 2 | 常用算术、滤波、颜色和变换，2A–2E 已接入 | 28 / 0 | 28 项均已接入，复杂操作专项与简单节点构建按批记录 |
| 3 | 检测、标注、统计和跟踪，十内部节点与三插件已接入 | 10 / 3 | 结构化检测结果能够查看、绘制、计数和跟踪 |
| 4 | 视频输入、采集和输出 | 0 / 3 | 文件视频与摄像头进入同一处理链，结果可录制 |
| 5 | 数据导出与通信 | 0 / 5 | JSON 结果可写 CSV，或经 UDP/TCP/OSC/串口输出 |
| 6 | DNN 与三维查看 | 8 / 0 | 7 类模型节点及深度点云界面完成，全部原库节点接入 |
| 7 | 原库工程兼容与整体收尾 | 0 / 0 | 完整节点矩阵、代表工程和资源释放验收完成 |

阶段顺序按可用价值、依赖和验证成本安排。每阶段内部按下面的小批次推进，不等全部节点写完才验证。阶段 4 的摄像头、阶段 5 的串口和阶段 6 的模型有额外验收条件；缺少条件时推进同阶段其它项，并明确保留未验收项。

### 阶段 0：固定验收约定

统计清单已经完成。首批端口、默认值、状态键、启用行为及 UI 角色已记录在 [内部节点](../nodes/README.md) 和 [插件](../plugins/README.md)；确定输入、原库格式状态及输出期望纳入测试。后续逐批补充，不要求先完成全部剩余节点的详细设计。

沿用现有自动化基线；发生代码改动后执行受影响测试与必要回归。按用户阶段 2A 的要求，后续新增节点只对复杂操作编写和运行专项测试，简单算术/数值节点做源码核对和构建检查；Abs_Diff 按明确要求单独完成测试。不能因为注册存在就认定其所有参数组合已验证。

### 阶段 1：静态图像分析闭环

本阶段已实现。Convert_Color 首先独立通过计算及 Qt 测试，再接入其余节点；完整链路测试见 `tests/phase1_pipeline_test.cpp`。Image_Writer 保留原库 `null` 状态，输出路径和写入命令不随工程保存。必要适配及验收边界见 [开发进度](development-status.md)。

内部节点按顺序接入：`Convert_Color`、`Threshold`、`Morphology`、`Contours`、`Json_Viewer`、`Get_Size`。动态插件接入：`Image_Writer`。

具体要求：

- Convert_Color 保留原库转换代码表、选择值与错误处理语义；不能只实现 BGR→Gray。
- Threshold 包含原库 Simple、Color Range、Adaptive、Otsu 四种模式。其颜色范围计算当前依赖 ImGui RGB→HSV 辅助函数，移植时用不依赖 UI 的等价实现，验证数值范围与转换结果。颜色选择等复杂交互放在可选 Qt Controls。
- Morphology 保留操作、核形状和迭代参数；Contours 保留 `viz`/`in` 输入及图像/JSON 双输出，字段结构不另起一套格式。
- Json_Viewer 通过通用 Main 接口提供结构化查看；Image_Writer 通过可选插件 Controls 提供原库的路径设置和 Write Image 操作。计算插件不因按钮而依赖 Qt。
- Get_Size 验证数组输出与已有 Resize 的尺寸输入衔接。

验收链路：`Image_Loader → Convert_Color → Threshold → Morphology → Contours`，原图按原端口契约接入 Contours 的 `viz`；图像结果送 `Viewer`/`Image_Writer`，JSON 送 `Json_Viewer`。另验证 `Get_Size → Resize.size`。

完成标准：固定图片的掩膜、轮廓 JSON 和写出图片符合预期；参数编辑、复制粘贴、保存重开及多个实例互不干扰；关闭自定义面板、删除节点和退出不遗留回调。不得修改 MainWindow/PropertyPanel 的节点类型判断。

### 阶段 2：基础图像处理扩展

按以下小批次接入，共 28 个内部节点：

| 批次 | 节点 | 重点验收 |
| --- | --- | --- |
| 2A 算术与数值，已实现 | `Abs_Diff`、`Add_Weighted`、`Subtract`、`Multiply`、`Divide`、`Max`、`Min`、`Bitwise`、`Mean`、`Scale_Abs`、`Normalize` | Abs_Diff 先独立通过；Subtract/Bitwise/Normalize/Mean 复杂路径测试通过；其余六个简单节点仅构建核对 |
| 2B 几何处理，已实现 | `Crop`、`Transform`、`Perspective_Warp`、`Copy_Make_Border` | 坐标、边界、插值、变换参数与状态恢复 |
| 2C 颜色与通道，已实现 | `Color_Correct`、`Color_Reduce`、`Split`、`Combine` | Color_Correct 复杂计算、状态和 Qt 专项通过；其余三个简单节点仅源码核对及构建检查；Release 与选定 CTest 3/3 通过 |
| 2D 空间滤波，已实现 | `Sobel`、`Scharr`、`Laplacian`、`Sharpen` | Sobel 计算先独立 1/1 通过，随后 Qt 核编辑测试通过；Sharpen 复杂模式专项通过；Scharr/Laplacian 仅源码核对及构建检查；最终 Release 与选定 CTest 4/4 通过 |
| 2E 频域与生成，已接入 | `Get_Optimal_DFT_Size`、`DFT`、`DCT`、`Magnitude`、`Rnd_Noise` | Get_Optimal_DFT_Size 先构建；DFT/DCT 复杂转换与 Rnd_Noise 状态/节奏专项通过；Magnitude 仅源码与构建检查；最终 Release 与选定 CTest 4/4 通过 |

默认使用通用属性；确实需要交互操作的节点注册专属 Controls。复杂节点测试覆盖确定输出或合理数值容差、原库状态样例恢复、禁用语义及必要混合图回归；简单节点按用户要求不额外测试。阶段 2A 全部使用既有通用属性，未新增节点专属 UI 或公共框架。阶段 2D 的 Sobel/Laplacian 使用可选 Controls 保留核大小步长 2，Scharr/Sharpen 仍用通用属性；不修改公共属性面板。阶段 2E 全部复用通用属性，DFT 输出幅值图而非复数矩阵，DCT 的 Operation 不持久化；随机源保留原三模式、五个状态字段与 FPS 周期。浮点预览仍沿用已有 Viewer 的输入不变保证。

### 阶段 3：检测、绘制和跟踪

首批五个内部节点已接入：Background_Subtraction 先独立测试，再接入 Blob_Detector、Hough_Circles、Hough_Lines、Line_Intersections；保留背景模型历史及原 JSON 契约。阶段 3B 的 Shape_Colorizer、Shape_Counter、Simple_Blob_Tracker 已接入，阶段 3C 四个绘制节点已接入，阶段 3D Histogram 也已接入，不把本批完成等同于整个阶段完成。专项及集成结果见 [开发进度](development-status.md)。

内部节点共 10 个：`Background_Subtraction`、`Blob_Detector`、`Hough_Circles`、`Hough_Lines`、`Line_Intersections`、`Draw_Text`、`Draw_Number`、`Draw_Date_Time`、`Draw_JSON`、`Histogram`。

动态插件共 3 个，3B 已完成：`Shape_Colorizer`、`Shape_Counter`、`Simple_Blob_Tracker`。

先迁移检测和 JSON 结果，再迁移结果消费插件与绘制节点。Histogram 使用通用 Main Dock；颜色、文本、列表等复杂参数使用专属 Controls，不预先扩展所有通用属性类型。Background_Subtraction 和 Simple_Blob_Tracker 需保留跨帧状态行为；测试可直接输入确定的连续帧，不依赖阶段 4 的摄像头。

完成标准：轮廓/形状结果能跨内部节点和 `.fp` 插件传递，JSON 字段与原库一致；跟踪编号、背景更新及复位行为按原实现验证；多个 Histogram/Json_Viewer 独立，参数保存恢复不混入临时检测历史。

### 阶段 4：视频链路

依次迁移 `Video_Loader`、`Video_Writer`、`Video_Capture`，保留原库对应控制行为和状态键。按需引入 OpenCV videoio 及原库摄像头枚举实现；当前优先验证 Windows 后端，不将其它平台自动标为已支持。

先用短视频验证可重复的读取、结束/循环行为和写出，再验证摄像头枚举、设备选择、打开失败及释放。文件选择、播放或写入操作属于节点 Controls，不是新增全局运行暂停工具栏。源节点 FPS、处理速度和 UI FPS 保持各自语义。

完成标准：视频经处理后能预览和写出，输出可重开；更换源、删除节点、换工程和退出均释放文件/设备；不同 Flow Buffer Count 下不发生销毁竞态。摄像头实际采集需要可用设备，模拟输入不能代替这项验收。

### 阶段 5：数据导出与通信

按 `CSV_File → UDP_Send → TCP_Send → OSC_Send → Serial_Send` 迁移，共 5 个动态插件。保留 DataOutput 的五个独立 `.fp` 产物，不合并成一个新节点。

复用原库协议实现及必要依赖，按需接入 asio2、OSC 和平台串口枚举；不用 Qt 网络类重写原库协议层。连接、发送、路径及串口参数通过节点 UI 提供，保存的配置与运行时连接句柄分离。

完成标准：CSV 用临时目录，网络用本机回环接收端验证数据格式、失败处理和重连语义；串口需虚拟串口对或实际设备完成往返验证。覆盖删除节点、工程切换、退出时的连接及后台任务清理，不向真实外部服务发送测试数据。

### 阶段 6：DNN 与三维查看

此阶段有两条相互独立的工作，放在后期是因为需要模型资产或专属渲染界面，并非二者存在依赖。

**6A：7 个 DNN 节点。** 按 `Classification`、`Object_Detection`、`Segmentation`、`Human_Pose`、`Text_Detection`、`Text_Recognition`、`Image_Processing` 推进。先完成一个 Classification 纵向样例，再复用已经证实相同的模型文件、配置文件、类别文件及后端选择界面逻辑；不先创建新的通用推理架构。

接入 OpenCV dnn 所需模块，保留原库预处理、后处理、输出 JSON 和状态格式。至少用 CPU 后端及每类对应的可用模型/输入验证完整推理；GPU 和其它后端仅对实际构建支持的组合验收。模型路径存在、网络能加载不等于推理已验证；缺少模型时保留明确的验收缺口，不宣称该节点完成。

**6B：Depth_Viewer_3D。** 原实现直接依赖 `imgui_3d_opengl.hpp`、OpenGL 对象和 ImGui 控件，是剩余界面中需要单独适配的一项。保留深度/颜色/内参输入、点云计算、参数和交互行为；通过 Qt Main Dock 承载，优先评估 QOpenGLWidget，必要时增加可选适配层对 Qt OpenGLWidgets 的依赖。不能为移植此节点恢复 GLFW/ImGui 主窗口，也不能把 Qt 依赖传入计算 SDK。

完成标准：确定深度图生成的点坐标/颜色正确，无内参路径按原算法验证；相机交互、多个三维 Dock、GL 资源随上下文正确释放通过窗口验收。资源创建、上传、销毁在 GUI/GL 上下文所属线程执行，计算线程只提供线程安全数据。

### 阶段 7：完整性收尾

- 逐项核对 58 个内部节点与 13 个插件均可构建、注册、创建，普通插件仍只需原 `Create`，UI 插件通过可选接口接入。
- 按已迁移节点组成代表 `.flow` 样例：静态分割、结构化检测/跟踪、视频读写、通信、DNN、深度显示。覆盖原库状态载入、编辑后保存、重启恢复及失效资源反馈；不承诺未重新编译的原版 ImGui 插件二进制兼容。
- 完成参数、状态、端口、禁用行为和生命周期矩阵；空输入、错误类型、坏参数及缺文件的处理只做必要修复，并记录偏离原实现的原因。
- 执行干净 Release、所有项目测试、模板检查和代表性 GUI 操作；补齐已有尚未记录的 Help/About、ImageLoader Browse 和 Ctrl 像素检查手测，明确自动化与原生手测来源。
- 更新开发进度、节点接入说明、样例及功能对照清单。计划启动时待迁移的 64 个节点全部达到各自完成标准后，才能称为本计划范围内的节点移植完成。

## 所有阶段共同遵守

1. 保留节点运行时名称、端口顺序/类型、状态键、默认值、计算顺序及原有启用语义。遇到原库缺陷，只修复阻碍当前功能正确运行的部分并写明原因。
2. 计算、状态和参数留在 SDK/节点/插件；简单属性用现有面板，复杂 Controls/Main 用可选 Qt 适配。控件先于节点与插件库释放，计算线程不访问 QWidget。
3. 新节点显式加入构建与注册；PluginMaker 只提供骨架，不能把模板生成等同于算法移植完成。
4. 每个小批次完成“计算与状态 → Qt 参数/界面 → 构建注册 → 自动化与必要手测 → 文档”的闭环，再扩大范围。

## 不计入当前节点移植主线

独立 ProcessEngine 继续延期。Undo/Redo、全局运行暂停/单步、热重载、插件隔离、性能分析、CI、SDK 安装导出和安装包不因本次计划自动纳入。跨平台构建另列验收，不从 Windows 成功推断 Linux/macOS 成功。

完整工程加载失败的整体事务回滚仍是已知运行限制，当前子图粘贴回滚不能替代它；作为后续可靠性增强单独评估，不阻塞首批节点迁移。原库之外或本机快照不含源码的扩展插件，不纳入这 13 个插件的完成承诺。

## 下一步工作

生产节点的数量接入已完成。先处理[源码复核报告](flowcv-source-audit-2026-10-19.md)的 Add mask、自连校验和状态契约证据，再使用实际业务模型和输入复核 DNN 各任务的结果与性能，并接真实串口回环设备验证 Serial_Send。CPU 合成模型已覆盖七类实际推理，但不替代任务精度、GPU、SSD 分支或 Human Pose 多人 PAF 的模型专项；大小写引用与空集合/取色差异按报告单独收尾。

本机真实摄像头采集/释放/重开及原生 Qt OpenGL 点云/相机/多实例/重建已经通过；摄像头手动属性和驱动设置窗口、其它设备型号、Linux/macOS 及完整人工交互仍需按实际环境补验。阶段 7 的自动化来源与复现命令见 [开发进度](development-status.md)，七份工程和原库契约矩阵见 [examples](examples/README.md)。
