# 内置节点

`VisionFlowNodes` 是内置节点静态库，通过 `VisionFlow::Nodes` 链接。当前 [构建清单](CMakeLists.txt) 与 [节点工厂](../managers/node_manager.cpp) 已注册 58 个节点：`Solid`、`Add`、`Blur`、`Canny`（类名 CannyFilter）、`Resize`、`Viewer`、`Convert_Color`、`Threshold`、`Morphology`、`Contours`、`Json_Viewer`、`Get_Size`，以及阶段 2A 的 11 个节点、阶段 2B 的 Crop、Transform、Perspective_Warp、Copy_Make_Border、阶段 2C 的 Color_Correct、Color_Reduce、Split、Combine、阶段 2D 的 Sobel、Scharr、Laplacian、Sharpen，以及阶段 2E 的 Get_Optimal_DFT_Size、DFT、DCT、Magnitude、Rnd_Noise，以及阶段 3 首批 Background_Subtraction、Blob_Detector、Hough_Circles、Hough_Lines、Line_Intersections，以及阶段 3C 的 Draw_Text、Draw_Number、Draw_Date_Time、Draw_JSON，以及 Histogram、7 个 DNN 节点和 Depth_Viewer_3D。节点继承 [FlowCV::Node](../sdk/flowNode.h)，使用 DSPatch `Process_()` 和 SignalBus 计算、传递数据，不依赖 Qt 或 `editor`。模块关系见 [架构说明](../docs/architecture.md)，整体完成状态与验证范围见 [开发进度](../docs/development-status.md)。注册数量不代表所有行为已经验收，阶段安排见 [移植计划](../docs/flowcv-migration-plan.md)。

## 阶段 3：背景与检测

以下节点均保留原库名称、OutOfOrder 调度、参数状态和 JSON 字段。Background_Subtraction 使用通用属性；其它四个节点通过 Studio 可选 Controls 编辑颜色及条件参数，计算库仍不依赖 Qt。新增 OpenCV `video`、`features2d` 构建依赖。

| 节点 | 输入 → 输出 | 计算与空结果约定 |
| --- | --- | --- |
| Background_Subtraction | `in` 图像 → `out` 图像、`mask` 图像 | 持久 MOG2/KNN 模型，模式切换保留各自历史；mask 非零均保留原像素，127 阴影也保留 |
| Blob_Detector | `in` 图像 → `viz` 图像、`blobs` JSON | SimpleBlobDetector 和 DRAW_RICH_KEYPOINTS；无检测结果只输出图像 |
| Hough_Circles | `orig` 可选图像、`canny` 必需图像 → `vis` 图像、`circles` JSON | HOUGH_GRADIENT；8UC3 检测输入转灰度；缺少 orig 时绘制在黑色 BGR 图上 |
| Hough_Lines | `in` 图像 → `out` 图像、`lines` JSON | 启用时只接受 8UC1；Standard/Probabilistic；输出转 BGR，绘线可关闭 |
| Line_Intersections | `in` 可选图像、`lines` 必需 JSON → `out` 图像、`points` JSON | 最多 200 条输入线；计算无限直线的非负交点，方向夹角严格大于阈值；无交点不输出 JSON |

Background_Subtraction 状态为 `mode=0`、`history=500`、`threshold=40`、`detect_shadows=false`。参数更新应用到两套已有模型，不清除历史；历史不写入工程。禁用时只透传图像，不更新模型或输出 mask。KNN 的 CPU 算法读取字节样本，拒绝非 8U 输入以免将浮点内存误作字节；MOG2 保留 OpenCV 的深度转换支持。模型访问加锁，防止多缓冲并行更新同一模型。

Blob_Detector 保留 `blob_viz_color:{R,G,B}`、五个 `filter_by_*`、`color`，以及 `min/max_area`、`min/max_circularity`、`min/max_convexity`、`min/max_inertia`、`min/max_threshold`、`min_dist`、`min_repeat`、`threshold_step`。默认白色检测、面积筛选开启，面积 0–5000，其余几何筛选关闭，阈值 10–200、步长 10、距离 10、重复次数 2；可视化颜色沿原 ImVec4 默认值为黑色。新版 OpenCV 强制部分下限大于零，调用时将零下限替换为最小正 float，保存值仍为零；退化零值特征会因此被排除。无效参数或输入不输出，后续有效输入可恢复。

Hough_Circles 保留 `dp`、`min_dist`、`param1/2`、`min/max_radius`、`fill_circle`、`show_center/radius`、`circle_thickness`、`text_scale/thickness/offset` 和三个 RGB 对象 `circle_color/center_color/text_color`。主要默认值为 2、150、200/100、0/0，红圈、绿中心、白色半径文字；`circles.data` 每项为 `{x,y,size}`，size 为直径。无圆时 `data:null`，禁用时透传 canny 输入，不输出 JSON。

Hough_Lines 保留 `hough_mode=0`、`rho=1`、`thresh=150`、`min_theta=0`、`max_theta=360`、`min_line_len=100`、`max_line_gap=10`、`draw_lines=false`、`line_thickness=3` 与红色 `line_color`。角度单位为度；标准模式沿原算法按左、上、下、右边界求端点，边界包括 width/height，不改为像素范围裁线。JSON 每项为 `{line:{x0,y0,x1,y1}}`；无直线时 `data:null`。禁用透传输入，不输出 JSON。

Line_Intersections 状态为 `min_angle=60`、`min_dist=10`、`draw_points=true`、`point_radius=4`、`point_thickness=1`、`point_solid=false` 与青色 `point_color`。保留有方向的 0–180 度夹角，不改成锐角；交点不限制在图像内部，距离恰好等于 min_dist 时保留。JSON 为 `{data_type:"points",ref_frame:输入引用,data:[{x,y},...]}`；错误 data_type 或超过 200 条线抑制所有输出。禁用且有非空 lines JSON 时输出 JSON null，并透传可用图像；缺少 lines 信号时图像也不输出。所有检测 JSON 使用原 `ref_frame:{w,h}` 结构，条件隐藏的设置继续保存。

## Solid

[实现](Solid/solid.cpp) 生成固定尺寸的纯色或灰度图像。没有输入，输出端口 `0: out` 为 `CvMat`。

| 参数 | 默认值 | 条件与限制 |
| --- | --- | --- |
| `width` / `height` | `640` / `480` | 默认编辑范围 `[2, 10000]`；恢复的有效正尺寸按原值计算，界面扩展显示范围 |
| `is_color` | `true` | `true` 输出彩色图，`false` 输出灰度图 |
| `has_alpha` | `true` | 彩色模式下显示；决定输出三通道或四通道图像 |
| `color_r` / `color_g` / `color_b` | 均为 `0.0` | 彩色模式下显示；计算时裁剪到 `[0.0, 1.0]` |
| `color_a` | `1.0` | 彩色且启用 Alpha 时显示；计算时裁剪到 `[0.0, 1.0]` |
| `value` | `0` | 灰度模式下显示；计算时裁剪到 `[0, 255]` |
| `fps_index` | `7`，对应 `30 FPS` | 索引从 `0` 开始；选项依次为 `1, 3, 5, 10, 15, 20, 25, 30, 60, 120 FPS` |

默认输出为 `640 × 480` 的黑色 `CV_8UC4` 图像，Alpha 为 `255`。关闭 Alpha 后输出 `CV_8UC3`，灰度模式输出 `CV_8UC1`；彩色像素按 OpenCV 的 BGR / BGRA 顺序存储。帧率控制目前保留原库的忙等待方式。

`Solid` 自定义状态读写，保留 `solid_color`、`fps` 等原库字段。`fps` 与 `fps_index` 独立保存恢复；只有交互选择新的 FPS 选项时才更新 `fps`。恢复的正 FPS 用于实际节拍，修复原库恢复后未更新等待周期的问题；非法 FPS 在修改状态前拒绝。参数编辑值在处理函数调用 `Sync()` 后用于计算。

## Add

[实现](Add/add.cpp) 使用 `cv::add` 计算两路图像之和，端口均声明为 `CvMat`。

| 方向与索引 | 名称 | 要求 |
| --- | --- | --- |
| 输入 `0` | `in1` | 必须有非空图像 |
| 输入 `1` | `in2` | 必须有非空图像，正常计算时须与 `in1` 尺寸、类型一致 |
| 输入 `2` | `mask` | 可选；有效单通道 Mask 支持 `CV_8UC1` / `CV_8SC1`，且与输入图像尺寸一致 |
| 输出 `0` | `out` | 加法结果 |

唯一参数 `mask_mode` 默认是 `0: Default`。使用有效 Mask 时，`0: Default` 将 Mask 外像素设为黑色，`1: In1` 保留第一路输入，`2: In2` 保留第二路输入；Mask 内进行加法。缺少、空或多通道 Mask 沿用原库的无 Mask 分支；单通道 Mask 的尺寸或深度无效时不输出，不再静默改成全图相加。

缺少任一路有效输入时不产生输出；正常计算时两路图像尺寸或类型不一致也不产生输出。底层节点被禁用时，在两路输入均非空的前提下直接输出 `in1`。当前这些输入检查没有向编辑器提供具体错误状态。

## Viewer

[计算节点](Viewer/Viewer.cpp) 是终端缓存节点：输入 `0: in` 为 `CvMat`，没有输出端口，也没有可编辑节点参数。初始缓存为 `640 × 480` 的黑色 `CV_8UC3` 图像。收到非空图像时复制到缓存；输入信号缺失且距上次有效输入超过 `500 ms` 时将缓存清零。节点状态不包含图像缓存。

`Viewer::GetFrameCopy()` 在互斥锁保护下返回图像深拷贝。[Qt ViewerPanel](../app/VisionFlowStudio/ViewerPanel.cpp) 经通用 Main 工厂创建，可见时按 `33 ms` 定时器读取缓存。支持 `CV_8UC1/3/4` 和 `CV_16UC1/16SC1/32FC1/64FC1`、通道选择、伪彩缩放、Free/Width/Height 与 Ctrl 像素检查。显示转换不修改节点缓存，选项属于独立面板，不写入 Viewer 空状态。使用方法见 [Studio 指南](../docs/studio-guide.md)。该定时器只刷新界面，隐藏窗口不会停止计算；Viewer 仍保留原库处理时自动启用的行为。

## Convert_Color

[计算实现](ConvertColor/convert_color.cpp) 的输入 `0: in` 和输出 `0: out` 均为 `CvMat`，按原库完整 [转换表](ConvertColor/color_types.hpp) 调用 `cv::cvtColor`。[Qt Controls](../app/VisionFlowStudio/ConvertColorControls.cpp) 提供 Conversion Type 选择和不兼容转换提示。

状态只有 `conv_code`，默认 `0`。它是原库转换表的**索引**，不是 OpenCV 的颜色转换枚举：例如 BGR→Gray 保存为 `12`，对应的 OpenCV 转换值为 `6`。不要用 OpenCV 枚举直接替换已保存的索引。禁用或转换失败时透传输入；失败时显示错误提示，后续有效转换清除错误。缺少有效图像时不输出。接阈值与轮廓链路时，按输入通道选择 BGR→Gray 或 BGRA→Gray。

## Threshold

[计算实现](Threshold/threshold.cpp) 的端口为 `0: in → 0: out`，均为 `CvMat`。[Qt Controls](../app/VisionFlowStudio/FilterControls.cpp) 保留 Simple、Color Range、Adaptive、Otsu 四种模式及对应参数页。Simple 直接对输入执行阈值；Adaptive 和 Otsu 对多通道输入先转灰度。构建轮廓链路时，应确保最终输出为单通道 8 位掩膜。

原库状态键为 `thresh_method`、`thresh_type`、`thresh_amt`、`adapt_type`、`adapt_thresh`、`adapt_block`、`low_range`、`high_range`。默认分别为 `0`、`0`、`127`、`0`、`2`、`11`，上下颜色范围分别为黑和白。两个颜色对象继续使用归一化的 `R/G/B` 字段；计算时按原库 RGB→HSV 算法转换后用于 `inRange`，保存值不是 HSV 数值。Adaptive 的 Block Size 为不小于 3 的奇数。禁用时透传输入；不兼容的输入或参数不输出，并在 Controls 显示错误。

## Morphology

[计算实现](Morphology/Morphology.cpp) 的端口为 `0: in → 0: out`，均为 `CvMat`。[Qt Controls](../app/VisionFlowStudio/FilterControls.cpp) 提供 Erode、Dilate、Open、Close、Gradient、Tophat、Blackhat，以及 Rect、Cross、Ellipse 核形状。

状态保留 `operation`、`shape`、`amt`、`iter`、`border_type`、`border_color`；默认值依次为 `0`、`2`、`1`、`1`、`4` 和黑色 `R/G/B` 对象。核的宽高均为 `2 * amt + 1`，因此默认是 `3 × 3` 椭圆核。边界值沿用 OpenCV 原值，颜色保存为归一化 RGB、计算转换为 BGR 标量。原库菜单中的 Wrap 和 Transparent 仍保留；OpenCV 拒绝的组合会显示错误且不产生输出。禁用时透传输入。

## Contours

[计算实现](Contours/contours.cpp) 保留两个输入和两个输出：

| 方向与索引 | 名称 / 类型 | 用法 |
| --- | --- | --- |
| 输入 `0` | `viz` / `CvMat` | 可选绘制底图，处理时复制；未连接时按分析图尺寸创建黑色 BGR 图 |
| 输入 `1` | `in` / `CvMat` | 必需分析图；只有 `CV_8UC1` 执行轮廓提取 |
| 输出 `0` | `out` / `CvMat` | 绘制结果，可连接 Viewer 或 Image_Writer |
| 输出 `1` | `contours` / `JSON` | 结构化结果，可连接 Json_Viewer |

状态保留 `mode`、`method`、`filter_by_area`、`area_min`、`area_max`、`draw_contours`、`fill_contours`、`draw_bbox`，以及 `contour_color`、`bbox_color` 的归一化 `R/G/B` 对象。默认 COMP/Simple、面积过滤开启、范围 100–1000、绘制绿色轮廓、不填充、不绘制蓝色边框。[Qt Controls](../app/VisionFlowStudio/AnalysisControls.cpp) 提供全部参数和颜色选择。面积判断沿用严格大于下限且小于上限；轮廓遍历保留原库顶层兄弟节点规则。

JSON 顶层字段为 `data_type: "contours"`、`ref_frame: {w, h}` 和 `data`。`draw_contours` 开启时条目才包含 `contour: [{x, y}, ...]`，`draw_bbox` 开启时才包含 `bbox: {x, y, w, h}`；这些开关同时影响图像和 JSON。没有合格轮廓时 `data` 为 `null`；两种绘制均关闭时，合格轮廓条目仍为 `null`。禁用时只透传 `in` 到 `out`，不输出 JSON，也不透传 `viz`。原图接 `viz`、二值掩膜接 `in` 可在原图上显示分析结果而不改写上游图像。

## Json_Viewer

[计算节点](JsonViewer/json_viewer.cpp) 只有输入 `0: JSON`，没有输出。有效非空 JSON 复制到互斥锁保护的缓存，`GetJsonCopy()` 返回独立快照；缺失或空输入保留上次内容，处理时与原库一样自动启用。

[Qt Main 面板](../app/VisionFlowStudio/AnalysisControls.cpp) 默认按 `data` 数组首个对象的字段显示表格，嵌套数组和对象显示类型占位文字。Show Raw Data 展示完整 JSON，Copy JSON 按钮或原文右键菜单复制完整内容。不符合表格结构的 JSON 仍可通过原文查看。每个面板独立切换显示模式；`GetState()` 保留空字符串，缓存和显示选项不写入工程状态。

## Get_Size

[计算实现](GetSize/get_size.cpp) 输入 `0: in` 为 `CvMat`，输出 `0: size` 为 `Int_Array`，内容按 `[图像宽度, 图像高度]` 排列，可直接连接 `Resize` 的输入 `2: size`。没有参数或专属界面，`GetState()` 保留原库的 JSON `null`。仅在节点启用且输入图像非空时输出尺寸；**禁用时不输出**，不作图像透传。

## 阶段 2A：算术与数值

本批沿用 FlowCV `b09c48e` 的 `OutOfOrder`、默认启用、作者 Richard 和版本 0.1.0。有参数的节点使用现有通用属性面板，没有专属 Qt Controls 或 Main。浮点参数仍为原库 float；无上下限的原控件使用有限 float 全范围作为编辑范围，不人为改为非负数或百分比。

| 节点 / 实现 | 端口 | 计算及原库状态默认值 |
| --- | --- | --- |
| [Abs_Diff](AbsDiff/abs_diff.cpp) | `in1, in2 → out`，均为 CvMat | `cv::absdiff`；状态 `null` |
| [Add_Weighted](AddWeighted/add_weighted.cpp) | `in1, in2 → out` | `cv::addWeighted`；`alpha=0.5, beta=0.5, gamma=0` |
| [Subtract](Subtract/subtract.cpp) | `in1, in2, mask → out` | `cv::subtract`；`mask_mode=0`，选项 Default / In1 / In2 |
| [Multiply](Multiply/multiply.cpp) | `in1, in2 → out` | `cv::multiply`；`scale=1` |
| [Divide](Divide/divide.cpp) | `in1, in2 → out` | `cv::divide`；`scale=1`；保留 OpenCV 按位深处理除零的规则 |
| [Max](Max/max.cpp)、[Min](Min/min.cpp) | `in1, in2 → out` | 对应 `cv::max/min`；状态 `null` |
| [Bitwise](Bitwise/bitwise.cpp) | `in1, in2, mask → out` | `mode=0`，AND / NOT / OR / XOR |
| [Mean](Mean/mean.cpp) | `in: CvMat → json: JSON` | `scale=1, calc_mean=true, calc_std_dev=false` |
| [Scale_Abs](ScaleAbs/scale_abs.cpp) | `in → out` | `cv::convertScaleAbs`；`alpha=1, beta=0`；启用时输出 8 位图像 |
| [Normalize](Normalize/normalize.cpp) | `in, mask → out` | `norm_type=0, alpha=1, beta=10`；Inf / L1 / L2 / Min Max |

除 Mean 的 JSON 端口外，上表端口均为 CvMat。Abs_Diff、Add_Weighted、Subtract、Multiply、Divide、Max、Min 均要求两路图像非空；启用时还要求类型和尺寸一致，不一致则无输出。禁用仍要求双输入有效，再透传第一路，不额外检查两图匹配。Bitwise、Scale_Abs、Normalize 禁用时仅需第一路图像并透传；Mean 禁用时不输出。

Subtract 的 Default 模式在 mask 外为零，In1 / In2 分别保留对应输入。与原库一致，空或多通道 mask 被忽略，单通道 mask 交给 OpenCV 校验，因此保留合法 `CV_8S` mask 支持。坏尺寸/深度导致无输出，异常不逃出处理线程。

Bitwise 第二输入可选，缺失时 AND/OR 与自身运算、XOR 得零、NOT 取反。已连接且尺寸不同的第二路先用默认 `cv::resize` 匹配第一路，NOT 也保留这个处理顺序。缺失 mask 使用全 255；连接的 mask 保持原样，不自动转换或缩放。无效输入由 OpenCV 异常边界处理为无输出。

Normalize 的 `norm_type` 保存菜单索引，映射 OpenCV 的 `1/2/4/32`。**Min Max 忽略 mask**，其它模式使用非空 mask；输出位深保持输入。Beta 仅在 Min Max 显示，但始终保存，切换模式和重开工程不会丢失 Beta。

Mean 输出 `{"data_type":"mean","ref_frame":{"w":宽,"h":高},"data":[{"mean":[...],"std_dev":[...],"scale":数值}]}`。两个数组由各自开关决定是否存在；均关闭时仍输出含 scale 的条目。通道顺序保持 OpenCV 顺序，不转 RGB。只计算均值时保留 double；启用标准差时，均值和标准差在乘 scale 后按原库转 float 再写 JSON。

测试范围按本批要求缩减：Abs_Diff 作为明确指定的首项单独测试；只为 Subtract、Bitwise、Normalize、Mean 的复杂路径新增测试，其余六个简单节点仅做源码核对和 Release 构建，不宣称通过运行验收。

## 阶段 2B：几何处理

四个节点均保留 Transform 分类、作者 Richard、版本 0.1.0、OutOfOrder 和默认启用。计算不依赖 Qt；Crop、Perspective_Warp、Copy_Make_Border 的专属 Controls 使用现有工厂注册，Transform 使用通用属性。没有新增 Main Dock。

### Crop

[实现](Crop/crop.cpp)：输入 `0: in / CvMat`、`1: bbox / JSON`，输出 `0: out / CvMat`。状态为 `bbox: {X:0,Y:0,W:100,H:100}`、`calc_total_bbox=false`、`json_bbox_index=0`、`adjust_size_x=0`、`adjust_size_y=0`。

无 JSON data 时使用手动区域；存在 data 时按索引读取 `data[index].bbox` 的小写 `x/y/w/h`，或合并所有 bbox。单框先将原点减去整数 padding/2，再给宽高加 padding；合并框沿用原库先调整原点、再用该原点计算宽高的公式，不能改成标准矩形扩张。没有命中框时沿用上次区域。

裁剪前把原点限制在图内，宽高至少为 1，再截到图像边界。**处理后的区域写回运行状态并随工程保存**，断开 JSON 后手动界面也显示该区域。原图先复制再取 ROI，输出不会修改上游。禁用透传图像，不要求 JSON。畸形 JSON 不输出并在 Controls 显示错误，旧区域不被部分覆盖。

### Transform

[实现](Transform/transform.cpp)：单个 `in / CvMat → out / CvMat`。按原库顺序翻转、旋转、平移、缩放；Horizontal 标签对应 `cv::flip(...,0)`，Vertical 对应 1，保持原行为。自由旋转和平移均使用原图大小的画布及默认 warpAffine 插值/边界，Interpolation 只控制最后的 resize。

参数默认值：`trans_x/trans_y=0`、`flip_mode/rot_mode=0`、`angle=0`、`scale_mode=0`、`scale_x/scale_y=100`、`interp=0`、`aspect_mode=0`。Percentage 模式按旋转后的宽高计算百分比；Pixels 使用指定像素值，输出尺寸沿用整数截断。六种 resize 插值保留。

交互切换至 Pixels 时重置为原始输入尺寸，切回 Percentage 时重置为 100% 并解除宽高比锁；SetState 恢复不执行交互重置。锁比例使用输入图像比例，与原库一致。首帧到达前暂缓锁比例推导，避免重开/粘贴时用默认比例改写保存尺寸；随后按实际输入比例更新。

状态沿用原库属性的可见字段规则：仅 Free 旋转保存 angle，仅 Pixels 保存 aspect_mode；内部 `res_x/res_y/aspect_ratio` 不保存。禁用透传原图。零尺寸、越界浮点转整数及 OpenCV 错误不逃出处理线程。

### Perspective_Warp

[实现](PerspectiveWarp/perspective_warp.cpp)：输入 `0: in / CvMat`、`1: points / JSON`，输出 `0: out / CvMat`。JSON 为 `{"data_type":"points","data":[{"x":数值,"y":数值}, ...]}`，必须恰好四点；data_type 可省略。保留 approxPolyDP 四边形检查、按中心分上下后排序，以及目标点采用 `(width,height)` 而非减一的规则，不替换为新的角点排序算法。

状态默认 `aspect_ratio_adj=0`、`interp_mode=1`、`use_fixed_res=false`；仅固定分辨率模式保存 `fixed_width/fixed_height`（默认 640/480）。自动尺寸分别截断四条边长，再平均，比例调整只改变高度；最后宽高至少为 2。无有效四边形时 Controls 显示提示，有效时显示五种插值、固定分辨率或比例调整。

禁用仍要求图像和 JSON 两个输入非空才透传，保持原库要求。畸形 JSON、退化几何、零宽计算或异常不输出；每次重新计算检测状态，避免上一帧四点计数影响后续无 data 输入。

### Copy_Make_Border

[实现](CopyMakeBorder/copy_make_border.cpp)：输入 `0: in / CvMat`、`1: size / Int_Array`，输出 `0: out / CvMat`。size 仅在恰好两个元素时使用 `[width,height]`，可连接 Get_Size。默认状态 `add_width/add_height=0`、`border_type=0`、`border_color:{R:0,G:0,B:0}`、`center=true`。

有 size 时按目标尺寸加增量，否则按输入尺寸加增量，再取与输入尺寸差的绝对值。因此指定尺寸较小时也扩边；不会裁剪。Center 开启时两侧分别取半差并截断，奇数差最终少 1 像素；关闭时只加右侧、下侧。保留 Constant/Replicate/Reflect/Wrap/Default 五种边界方式。颜色保存归一化 RGB，传给 OpenCV 时为 BGR，四通道图像的常量边框 alpha 为 0。

Controls 的颜色仅在 Constant 显示，但所有颜色字段一直保存。状态仍接受原库负增量；尺寸加减使用较宽整数防溢出，不支持的输入或过大输出不逃出计算线程。禁用直接透传图像。

本批测试限定为上述复杂 JSON、组合变换、动态尺寸/颜色和控件生命周期，未扩充简单算术节点测试。详细执行范围见 [开发进度](../docs/development-status.md)。

## 阶段 2C：颜色与通道

四个节点均保留作者 Richard、版本 0.1.0、OutOfOrder 和原端口顺序。Color_Correct 与 Color_Reduce 属于 Color 分类，Split 与 Combine 属于 Utility 分类；计算库不依赖 Qt。

### Color_Correct

[实现](Colorcorrect/ColorCorrect.cpp) 保留类名 `Colorcorrect`，输入 `0: in / CvMat`、`1: mask / CvMat`，输出 `0: out / CvMat`。默认状态为 `brightness=0`、`contrast=1`、`gamma=1`、`gain=1`、`saturation=1`，`low_color:{R:0,G:0,B:0}`、`high_color:{R:1,G:1,B:1}`。专属 [Qt Controls](../app/VisionFlowStudio/ColorCorrectControls.cpp) 提供五个数值参数、上下 RGB 范围和 Reset，不创建 Main Dock。

仅对 8 位 BGR/BGRA 彩色输入进行校正，alpha 保持原值。缺少 mask 时处理全图；连接时要求非空 `CV_8UC1`，不同尺寸按原库默认线性插值缩放，**仅 mask 字节恰为 255 的像素生效**。RGB 上下限按 BGR 存储顺序映射，以每个原始通道值作含边界判断。选中通道依次执行饱和度、gamma LUT、对比度、增益、亮度；饱和度使用三个颜色通道的算术平均值，不替换为加权亮度。

Gamma 仍允许原界面的 0：LUT 将小于 255 的输入映射为 0，255 保持 255。低于三通道的图像与禁用状态均直接透传；缺少或空输入不输出。非法 mask 或不支持的彩色类型不输出并显示错误，后续合法输入恢复。参数和 LUT 在锁保护下同步更新，处理读取一致快照，坏状态不部分提交；不增加状态字段。

### Color_Reduce

[实现](ColorReduce/color_reduce.cpp)：`in / CvMat → out / CvMat`。通用 Properties 的 Reduce 默认 64，界面范围 1–255，计算保留原库最小值 1 的限制。复制输入后逐字节执行 `value / div * div + div / 2`，包括 BGRA 的 alpha；结果转 `uchar` 时按原库截断，不作饱和转换。仅处理 `CV_8U`，其它位深不输出，避免原实现把高位深数据误解释为字节。

**原库状态始终为 JSON `null`，Reduce 是运行时参数，不随工程保存。** 新建、重开工程或粘贴出的新实例使用 64；向现有实例恢复 null 状态不重置其当前 Reduce。禁用透传输入。

### Split 与 Combine

[Split](Split/split.cpp)：输入 `in / CvMat`，输出按 `r, g, b, a` 排列，对应输入平面索引 `2, 1, 0, 3`。至少三通道时使用 `cv::split`，保留输入位深；仅恰好四通道时输出 alpha。无输入或通道不足时不输出，专属 Controls 提示 `No Color Image Input Detected`；空输入清除旧的有效颜色提示状态。

[Combine](Combine/combine.cpp)：输入 `r, g, b, a / CvMat`，输出 `out / CvMat`。r/g/b 必须非空且尺寸、类型一致，按 `b, g, r` 顺序交给 `cv::merge`；alpha 只有非空且尺寸、类型匹配时加入，否则忽略并输出前三路合并结果。不额外限制输入必须为单通道，保留原库 merge 行为。

Split 和 Combine 均保留 `null` 状态，且每次处理时自动启用；界面上的禁用不会让它们持续停止。Combine 没有专属界面。

本批只为 Color_Correct 的复杂计算、状态、异常与 Qt 生命周期增加专项测试。Color_Reduce、Split、Combine 仅源码核对和构建检查，不声称完成专项运行验收；最终执行记录见 [开发进度](../docs/development-status.md)。

## 阶段 2D：空间滤波

四个节点均属于 Filter 分类，保留作者 Richard、版本 0.1.0、OutOfOrder、默认启用，以及单个 `0: in / CvMat → 0: out / CvMat` 端口。Sobel/Laplacian 通过可选 [DerivativeControls](../app/VisionFlowStudio/DerivativeControls.cpp) 提供参数界面，保留核大小步长 2；Scharr/Sharpen 使用通用 Properties。四个节点均无 Main Dock。

### Sobel、Scharr、Laplacian

[Sobel](SobelFilter/sobel_filter.cpp)、[Scharr](ScharrFilter/scharr_filter.cpp)、[Laplacian](LaplacianFilter/laplacian_filter.cpp) 对多通道输入先执行 `COLOR_BGR2GRAY`，单通道保持原样，再调用对应 OpenCV 导数函数，边界均为 `BORDER_DEFAULT`。输出保留计算深度和符号，不自动取绝对值或转成 8 位。

| 节点 | 原库状态与默认值 |
| --- | --- |
| Sobel | `derivative_order=0, depth_out=2, ksize=3, scale=1, delta=0` |
| Scharr | `derivative_order=0, depth_out=2, scale=1, delta=0` |
| Laplacian | `depth_out=2, ksize=3, scale=1, delta=0` |

`derivative_order` 的 0/1 分别表示 X/Y 一阶导数；`depth_out` 的 0/1/2/3 分别映射 Auto（-1）、CV_16S、CV_32F、CV_64F。Scale/Delta 保留原库有限 float 范围，允许负数，不加额外非负限制。

Sobel/Laplacian 的 Kernel Size 在界面编辑时修正为 1–7 的奇数；**恢复工程不改写保存的 ksize**，由 OpenCV 判断计算是否支持，Sobel 因而保留原状态中的 9 与 -1 等值的计算语义。恢复时临时扩展属性显示范围以显示真实保存值，避免控件显示 7 而实际使用 9；首次交互编辑再按原规则修正并恢复 1–7 范围。Qt 核控件使用步长 2，避免通用整数步长 1 与偶数向上修正叠加后无法从 3 减到 1。参数快照与核编辑记录通过已有锁机制同步，坏状态通过属性校验原子拒绝。缺失或空输入不输出；禁用直接透传原图。不支持的输入/参数产生 OpenCV 异常时本帧不输出，后续有效输入可恢复。

### Sharpen

[实现](Sharpen/sharpen.cpp) 状态仅为 `sharpen_mode=0, sharpen_amt=0`。Mode 1 的 Sharpen 为 0–3；0 或负值复制原图，1 使用中心 5、四邻域 -1 的核，2 使用中心 9、八邻域 -1 的核，3 使用中心 17、八邻域 -2 的核。`filter2D` 保留输入深度及默认边界。

Mode 2 的界面范围为 0–5：先转灰度，再以原深度、3×3 核和 `scale=-amount` 原位执行 Laplacian；结果转 BGR 后调用 `addWeighted(original, 1, result, amount, 0)`。保留原库两次使用 amount 和中间饱和的顺序，不替换为其它锐化算法。**该模式仍不支持灰度/BGRA 输入**：原库合并的通道数不匹配，当前捕获异常并使本帧无输出，不自动改写输入通道。

切回 Mode 1 时沿原 UI 将大于 3 的值限制为 3；尚未经过界面刷新而直接传入的 Mode 1 超限参数会被计算侧拒绝，避免原库使用未初始化卷积核。禁用与 Mode 1 的 amount≤0 均复制原图，输出拥有独立缓冲。缺失/空输入无输出，错误后可由有效输入恢复。

Sobel 按明确要求首先单独完成计算测试，并补充 Qt 核增减、恢复和控件生命周期测试；Sharpen 覆盖两种复杂模式。Scharr/Laplacian 仅源码核对和构建检查，不增加专项计算测试。实际执行结果见 [开发进度](../docs/development-status.md)。

## 阶段 2E：频域与生成

五个节点显式加入 Nodes 与 InternalNodeManager，保留作者 Richard、版本 0.1.0、OutOfOrder 和默认启用。DFT/DCT/Rnd_Noise 使用已有通用 Properties，Get_Optimal_DFT_Size/Magnitude 无参数；没有新增专属 Qt Controls 或 Main Dock。

### Get_Optimal_DFT_Size

[实现](GetOptimalDft/get_optimal_dft.cpp) 输入 `0: in / CvMat`、`1: size / Int_Array`，输出 `0: size / Int_Array`。非空图像优先，否则读取长度恰好为 2 的 `[width, height]` 数组，分别调用 `cv::getOptimalDFTSize`。它只计算尺寸，不对图像补边或缩放；无有效输入或禁用时无输出。状态为原库 JSON `null`。

### DFT

[实现](DiscreteFourierTransform/discrete_fourier_transform.cpp) 的端口为 `in → dft`，均为 CvMat。多通道输入先按 `COLOR_BGR2GRAY` 转灰度，再转换为 32F 实部、配零虚部并调用 `cv::dft`。**输出是单通道幅值图，不是双通道复数矩阵**；Inverse 选项也继续执行后续幅值处理，不构成可逆的复数频谱传递接口。

状态保留 `log_view=false, shift_center=true, op_mode=0`。Operation 的 Default / Inverse / Rows / Complex / Real / Complex In 分别对应原库的 `0`、`DFT_INVERSE + DFT_SCALE`、`DFT_ROWS`、`DFT_COMPLEX_OUTPUT`、`DFT_REAL_OUTPUT`、`DFT_COMPLEX_INPUT`，不另行组合标志。先计算幅值，Shift DC to Center 开启时裁去奇数末行/末列并交换象限，再按 Logarithmic View 决定是否执行 `log(1 + magnitude)`、MinMax 到 0–1、乘 255 转 8U。关闭 Logarithmic View 时输出原始 32F 幅值，不附加归一化或 1/255 缩放。单行或单列开启 Shift 后会被偶数裁剪成空图，因而无输出；关闭 Shift 保留其尺寸。缺失/空输入或禁用时无输出，不透传原图。

### DCT

[实现](DiscreteCosineTransform/discrete_cosine_transform.cpp) 的端口为 `in → out`，均为 CvMat。多通道先转灰度；灰度结果已为 CV_32F 时保持数值，其余深度（包括 CV_64F）均转换为 CV_32F 并乘 `1/255`，再调用 `cv::dct`。Operation 的 Default / Inverse / Rows 对应 `0 / DCT_INVERSE / DCT_ROWS`。不自动补边，也不人为限定偶数尺寸；尺寸支持由链接的 OpenCV 决定，本机 5×7 奇数尺寸已与直接 cv::dct 对照通过。OpenCV 拒绝的输入本帧不输出，后续合法输入可恢复。

**Operation 仅为运行时参数，状态沿用原库 JSON `null`。** 重开工程或粘贴新实例后回到 Default；对现有实例调用 SetState 仅解析 JSON，不重置已选模式。禁用或无有效输入时无输出。

### Magnitude

[实现](Magnitude/magnitude.cpp) 输入 `in1, in2`，输出 `out`，均为 CvMat。两路必须非空；启用时还要求尺寸、类型完全相同，且仅允许 CV_32FC1 或 CV_64FC1，然后调用 `cv::magnitude`。多通道浮点不纳入原库支持范围。禁用仍要求两路非空，再直接透传第一路，不要求两路尺寸和类型匹配。状态为 JSON `null`。

### Rnd_Noise

[实现](RndNoise/rnd_noise.cpp) 没有输入，输出 `0: out / CvMat`。状态保留五个键：`mode=0, width=1280, height=720, fps=30, fps_index=7`。Mask Mode 的 Color 生成独立 BGR 随机值，BW_8U 生成单通道灰度，BW_8UC3 先生成灰度再复制为 BGR 三通道。三种模式都调用 `cv::randu`，每个字节范围为 **0–254**，不固定随机种子或承诺逐帧像素一致。

Width/Height 的默认界面范围沿原库为 1–8000；恢复状态只要求尺寸与 fps 为正 int，不把 UI 上限当作文件格式限制，超限尺寸会扩展控件范围以显示真实值。Output FPS 的索引对应 `1, 3, 5, 10, 15, 20, 25, 30, 60, 120`。保存仍分别保留 `fps` 与 `fps_index`，恢复时不强行用索引覆盖 fps；交互切换索引时才映射对应 FPS。输出周期继续采用 `floor(1000 / fps)` 毫秒（按原 float 算式），第一帧也等待周期，多个并发 Tick 通过 try-lock 跳过竞争帧。按原库规则，处理函数不读取也不改变 enabled 标记，因此禁用不会停止随机源。

必要适配：参数由同步快照读取，非法状态原子拒绝；OpenCV 异常不退出工作线程。Rnd_Noise 修复原 SetState 恢复值被临时默认值覆盖、恢复 fps 未更新等待周期的问题；用 `sleep_until` 替代忙等但保留截断周期和 try-lock 语义。随机源临时帧与时间戳不写入工程。

本批只为 DFT/DCT 的复杂转换和 Rnd_Noise 的状态、模式及节奏增加专项测试；Get_Optimal_DFT_Size 与 Magnitude 仅源码核对和构建检查。最终运行结果见 [开发进度](../docs/development-status.md)。

## 阶段 3C：文字、数值、日期与 JSON 标注

四个 [DrawTools](DrawTools) 节点均在 Draw 分类，使用原 `cv::putText`、OutOfOrder、原端口和状态格式。可选 Qt Controls 只在 Studio 注册；计算库不依赖 Qt。输入图像复制后绘制，禁用时透传原图；缺失/空图不输出。共同默认值为 `position:{x:10,y:30}`、`scale:1`、`font:0`、`thickness:2`、`color:{R:1,G:1,B:1}`，RGB 状态转 BGR 255 绘制，使用原八种 Hershey 字体。

| 节点 | 输入 → 输出 | 原状态与行为 |
| --- | --- | --- |
| Draw_Text | `in:CvMat, str:String → out:CvMat` | 外部 str（包括空串）优先，否则使用本地 text。存在 str 输入时 GetState 省略 text；SetState 缺 text 时清空本地文本。Controls 局部改样式保持本地文本 |
| Draw_Number | `in:CvMat, bool:Bool, int:Int, float:Float → out:CvMat` | 按 bool→int→float 优先取一个；false 也优先，显示 True/False；数字沿用 std::to_string，不新增精度状态。没有数字输入仍输出图像副本 |
| Draw_Date_Time | `in:CvMat → out:CvMat` | 主机本地时间，先日期后时间，默认 show_date/show_time 均 true，date_fmt 为 `%Y-%m-%d`、time_fmt 为 ` %X`（含前导空格）。对应 show 开启时才保存格式键；编辑格式采用原日期/时间白名单，非法草稿保留上一有效格式 |
| Draw_JSON | `in:CvMat, json:JSON → out:CvMat` | 从 data 数组选 json_data_index，按 out_list 顺序显示。decimal_places 默认 3；空 out_list 保存 null；每项 is_array/item_path，只有 is_array=true 保存 item_index |

Draw_JSON 的 JSON Pointer 相对于 `data[json_data_index]`；实际解析到数组时显示 `path [index]: value`，否则显示 `path: value`。浮点先转 float 再按 fixed 精度输出，其余使用 JSON 序列化（字符串保留引号）。缺失字段跳过，数组索引越界仍绘制前缀；每行间距为原 getTextSize 高度加 14。字段树递归对象、将数组作为可选叶子，双击添加/移除；数组允许重复选择不同索引，普通字段去重。

沿用原输出门槛：启用的 Draw_JSON 在缺少或空 JSON 输入时不输出；非空 JSON 但没有 data、data 为空、数据索引越界或列表为空时输出未标注的图像副本。临时 JSON、错误状态、列表版本和界面草稿不写入工程。恢复合法状态不以界面范围截断；GUI 编辑范围保留原设置。

必要适配仅限参数同步、原子恢复、固定字符缓冲安全、格式/JSON/OpenCV 错误保护，以及字段名含 `/`、`~` 时正确转义 JSON Pointer。Draw_Text/Draw_Number 按要求仅源码核对与构建；复杂日期格式、JSON 解析/列表和真实插件工程链路的验证结果见 [开发进度](../docs/development-status.md)。

## 阶段 3D / 6：直方图、DNN 与三维显示

Histogram 输入 `in:CvMat`，无输出，Main Dock 显示灰度或 BGR 三通道曲线，BGRA 忽略 alpha。保留原 `calcHist` 的 256 个 `[0,256)` 分箱，各通道按图像高度进行 MinMax 归一化；原界面实际只显示 x=1..255、对应 bin=0..254 的 255 个点，这里不擅自补最后一个 bin。支持 OpenCV calcHist 的 8U、16U、32F；无信号保留旧数据，空图清除，非法格式清除并显示错误。节点每次 Tick 自动启用，状态为 `null`。Qt 提供通道开关、拖动/滚轮、双击自适应和重置轴；这些界面选项按原库不写节点状态。

7 个 DNN 节点的完整端口、默认值、模型布局、JSON、条件状态和必要修复见 [DNN 说明](DNN/README.md)。保留原逐节点 OpenCV dnn 预处理/推理/后处理；计算库增加 dnn 模块，Qt Controls 只位于 App。模型路径、配置、类别/字典及 Initialize 操作在对应 Controls 配置。小型模型夹具执行真正 CPU forward，不替代用户实际模型精度、GPU 或所有可选模式验收。

Depth_Viewer_3D 输入 `color:CvMat, depth:CvMat, intrin:JSON`，无输出，Controls + Main。深度要求 `CV_16UC1`，颜色可选 `CV_8UC3`；先水平翻转，颜色按最近邻缩放到深度尺寸，深度乘 0.01，按原公式投影并减 `pos_offset`。内参位置为 `data[0].intrinsics.depth.{fx,fy,ppx,ppy}`，缺少时由 `no_int_hfov` 推算。有效点取 BGR→RGB 或 `diff_color`，零深度透明。节点自动启用。

状态保留 `diff_shading`、`diff_color:{R,G,B}`、`point_size`、`pos_offset:{x,y,z}`；无内参时保存 `no_int_hfov`。Main 初始化后保存原相机字段：view_lat/view_lon/yaw/pitch/zoom/fov/orbit_mode/fly_speed/show_grid/grid_size/msaa/view_pos/view_center/cam_fwd/cam_up/bg_color。恢复不创建 GL 资源，首次 GUI 显示时在当前上下文初始化。右拖轨道/视角、中拖平移、滚轮缩放/移动、WASD/QE 飞行、H 回到初始相机；Main 控件提供原相机/网格/MSAA/背景选项。

必要修复：原深度循环读取尚未更新的右邻居法线、上一行零深度后下一行有效点坐标可能缺失，改为先完整计算位置再按原叉乘分支算本帧法线；单行/单列避免邻居越界；非法类型/内参安全报告。零深度透明点在 shader 丢弃，避免写深度遮挡其它内容。QOpenGLWidget 仅在 GUI 线程创建/更新/销毁 VAO/VBO/program，销毁上下文前清理并断开回调，支持重新创建。请求原 8x MSAA，实际样本数由驱动协商；未恢复 GLFW/ImGui，也未给计算 SDK 增加 Qt 依赖。

## 新增节点

新增节点需同时接入以下三处：

1. 在 [nodes/CMakeLists.txt](CMakeLists.txt) 中显式加入头文件、源文件及必要依赖。
2. 在 [internal_nodes.h](internal_nodes.h) 中包含节点头文件。
3. 在 [InternalNodeManager 构造函数](../managers/node_manager.cpp) 中调用 `RegisterNode<T>()`。节点类型须继承 `FlowCV::Node`、可默认构造，并提供非空且不重复的组件名。

迁移原库节点时核对端口、参数、处理和状态格式，将界面逻辑放在可选 Editor / App 适配层。当前属性系统支持 Bool、Int、Float、Option、String、FilePath，详见 [SDK 属性定义](../sdk/flowProperties.h)。复杂 Controls 或 Main 使用 NodeUiRegistry 注册 QWidget 工厂，主窗口不新增具体节点判断。可使用 [PluginMaker 内部节点模板](../tools/README.md) 生成并显式注册。

节点库当前查找并链接 OpenCV `core`、`imgproc`、`video`、`features2d`、`dnn`。本机通过 `CMakeUserPresets.json` 设置 `OpenCV_DIR`，指向与编译器、架构及构建配置匹配的安装包；MSVC Debug 需要对应的 Debug 库。具体构建方式见 [根目录 README](../README.md)。
