# Studio 使用指南

本指南说明当前源码接入的功能。实际测试结果见 [开发进度](development-status.md)，本页操作步骤不是 GUI 手测通过记录。环境准备见 [第三方依赖](../third_party/README.md)。

## 启动、工程与布局

运行 VisionFlowStudio 后打开空画布，DSPatch AutoTick 自动执行。程序不自动搭建示例或打开上次工程；窗口和 Dock 布局会尝试从 QSettings 恢复。

File 菜单提供 New、Open、Load Recent、Save、Save As 和 Exit。工程以 `.flow` 保存节点类型、实例编号、启用状态、参数、连线和画布位置。New、Open 和退出前会检查未保存改动，提示保存、放弃或取消。成功打开或保存的路径进入 Load Recent。

窗口位置和 Dock 布局由 QSettings 单独保存，不在工程文件里。View → Reset Layout 恢复基础停靠布局，不删除流程。完整工程打开失败可能留下成功恢复的部分新图，尚无整体事务回滚。

| 区域 | 用法 |
| --- | --- |
| Editor | 创建、连接、选择和移动节点 |
| Properties | 参数或节点自定义 Controls；标题折叠，选择节点时高亮；× 关闭分组，双击节点或右键 Open Controls 重开 |
| Node Library | 按名称搜索，双击创建；View 菜单控制显示 |
| Node Windows | View → Node Windows 控制 Viewer 或插件 Main 界面的独立 Dock |
| Graph 工具栏 | Add Node、删除选择、Fit All Nodes 和 Reset Zoom |
| 状态栏 | 图统计、操作错误；设置启用后显示 UI FPS |

## 创建与编辑流程

空白画布右键 → Add Node 按类别创建；Tab 打开名称搜索。Node 菜单和工具栏也可创建节点。当前名称以节点库实际枚举为准，包含已有内部节点及启动时加载成功的插件。

从输出端拖到目标输入端连接，只接受相同端口类型。输入端最多一路连接；新连接会替换原连接，输出端可分支至多个节点。将端口连线拖到空白处会打开新增菜单：创建节点后仅当它的首个对应端口类型匹配时自动连接，否则保留新节点供手动连接。

选择节点后可剪切、复制和粘贴子图。复制保留参数、相对位置、启用状态，以及选中节点之间的内部连线，不复制连到未选中节点的外部边。Viewer 等无参数节点也参与复制。粘贴会重分配 ID 和实例编号；快捷键粘贴以画布鼠标位置为锚点，菜单粘贴以视口中心为锚点。失败会移除本次新增对象，保留原图并恢复此前 AutoTick 状态。

右键节点可打开 Node Information 查看作者、版本、类别及端口数量，或通过 Enabled 切换启用状态。禁用状态在节点上标出，但计算行为由各节点定义：例如 Viewer 沿用原库的自动重新启用行为。

| 快捷键（Windows） | 操作 |
| --- | --- |
| Ctrl+N | 新建工程 |
| Ctrl+O / Ctrl+L | 打开工程 |
| Ctrl+S / Ctrl+Shift+S | 保存 / 另存为 |
| Ctrl+X / Ctrl+C / Ctrl+V | 画布中剪切 / 复制 / 粘贴 |
| Delete | 删除选中节点或连线 |
| D | 切换选中节点启用状态 |
| F | 定位选中节点；未选择节点时定位全图 |
| Tab | 搜索并创建节点 |
| S | 切换 Show Flow 连线方向动画 |
| Ctrl + 指向 Viewer 像素 | 显示图像坐标、UV、RGB、HSV |

画布操作快捷键在画布拥有焦点时生效，不替代参数输入框内的文本编辑快捷键。滚轮缩放，拖动空白画布平移；View 提供 Fit All Nodes 和 Reset Zoom。Undo/Redo 与独立 Duplicate 动作没有实现。

View → Show Flow 显示连线方向动画，它不表示实际流量或性能。节点分类颜色与端口类型文字帮助识别连接。Help → Documentation 打开 FlowCV 网站，About 显示当前应用和原库基线信息。

## 加载图片与处理

构建 Studio 会把 `ImageLoader.fp` 部署至程序旁的 `Plugins`。启动后创建 `Image_Loader`，在 Properties 中使用 Image Path / Browse 选择图片，或输入路径并按 Enter 提交；支持中文图片路径。Output FPS 默认 30。

将 `Image_Loader.out → Viewer.in` 连接即可预览。也可以接入已有处理节点，例如 `Image_Loader → Blur → Viewer`；从源或中间输出再连接第二个 Viewer，可对照处理前后图像。修改参数由后台自动执行，不需要 Run 按钮。

ImageLoader 按原库规则输出 8-bit BGR。空路径或解码失败时不再输出；Viewer 在没有有效输入时沿用原库约 500 ms 清空行为。插件参数保存为原版 `image_path`、`fps`、`fps_index` 字段。

也可以创建两个 Solid，均设置 640 × 480、取消 Color，Solid Value 分别设为 30 和 40，将两者 out 接入 Add.in1 / Add.in2，再将 Add.out 接入 Viewer.in。Mask Mode 保持 Default，不接 mask。预期灰度值为 70，可用 Ctrl 检查；这是一份操作示例，不是程序自动执行的断言。

## 静态图像分析与写出

创建 `Image_Loader`、`Convert_Color`、`Threshold`、`Morphology`、`Contours`、两个 `Viewer`、`Json_Viewer` 和 `Image_Writer`，按以下方式连接：

1. `Image_Loader.out → Convert_Color.in → Threshold.in → Morphology.in → Contours.in`。每一步使用前一节点的图像输出；Contours 的分析输入是第二个端口 `in`。
2. 原图同时连接 `Contours.viz` 和第一个 Viewer，Contours 的图像输出连接第二个 Viewer 与 Image_Writer；JSON 输出 `contours` 连接 Json_Viewer。
3. Convert_Color 选择 BGR→Gray，Threshold 使用 Simple / Binary，按图片调整阈值。Morphology 按需要选择 Open / Close 等操作；核宽高为 `2 × Morph Amt + 1`。
4. Contours 按图片调整面积范围或取消 Filter by Area。Draw Contours / Draw BBox 同时决定绘制内容及 JSON 中是否包含 `contour` / `bbox`；面积比较是严格大于最小值、严格小于最大值。
5. Json_Viewer 默认显示表格，Show Raw Data 查看完整 JSON，Copy JSON 复制内容。每个 Dock 的显示模式独立。
6. Image_Writer 点击 Set Output Path，选择带扩展名的文件，再点击 Write Image，下一次有效输入写出一次。失败会显示错误；修正后可以重试。

可另接 `Image_Loader.out → Get_Size.in`，将其 `size` 输出接到 `Resize.size`（输入索引 2），同时给 Resize 接图像，验证动态尺寸传递。

**Image_Writer 沿用原库 `null` 状态，输出路径和待写命令不保存。** 重开工程或粘贴新节点后需要重新选路径，不会自动写文件。禁用或空输入时保留待写请求。Windows 中文路径可用；格式取决于扩展名和 OpenCV 编码器。

Threshold 包含 Simple、Color Range、Adaptive、Otsu；Color Range 的颜色值按原库保存为 RGB，计算时转 HSV。Adaptive 的 Block Size 必须是至少 3 的奇数。Morphology 保留原库边界选项，OpenCV 不支持的组合会显示错误且不产生输出。详细状态和禁用语义见 [内部节点说明](../nodes/README.md)。

## 算术、归一化与统计

阶段 2A 节点可从分类菜单或 Tab 搜索创建，参数直接使用 Properties。Abs_Diff、Add_Weighted、Subtract、Multiply、Divide、Max、Min 均需要两路非空、同尺寸同类型图像；禁用时仍需两路有效输入才透传第一路。Subtract 的 Mask Mode 决定 mask 外为零、保留第一路或第二路。

Bitwise 第二路可不连接，此时与自身运算；连接不同尺寸图片时会缩放第二路。Normalize 的 Normal Type 为 Inf / L1 / L2 / Min Max，只有 Min Max 显示 Beta，且该模式忽略 mask；Scale_Abs 输出 8 位图像。

统计可连接 `Image_Loader.out → Mean.in`、`Mean.json → Json_Viewer.JSON`，选择 Calc Mean / Calc Std Dev 与 Scale。Json_Viewer 的 Show Raw Data 可查看逐通道数组和完整字段；通道按输入的 OpenCV 顺序排列。各节点状态、边界和禁用差异见 [内部节点说明](../nodes/README.md)。

## 裁剪与几何变换

四个新节点都在 Transform 分类下，也可通过 Tab 搜索名称。

- **Crop**：图像接 `in`。未接 bbox 时在 Controls 编辑起点和宽高；接入 `Contours.contours → Crop.bbox` 后，改为选择 BBOX Index 或 Calculate Total BBOX from All，并可调整 Padding X/Y。Contours 需开启 Draw BBox 才有 bbox 字段。处理后的实际裁剪区域随工程保存，断开 bbox 后保留该区域。
- **Transform**：在通用 Properties 设置 Flip、Rotate、Translate、Scale。Free 旋转显示 Angle；Pixels 显示宽高比锁。操作顺序为翻转、旋转、平移、缩放；Interpolation 只用于最后的缩放。沿用原库标签含义，Horizontal 对应 OpenCV flip code 0。
- **Perspective_Warp**：图像接 `in`，`points` 需要 `data` 数组中的四个 `{x,y}` 点，可带 `data_type:"points"`。无有效四边形时显示提示；检测有效后可设置插值、固定尺寸或比例调整。它不直接接受 Contours 的 bbox/contour 嵌套格式，也不新增手工点编辑器。
- **Copy_Make_Border**：图像接 `in`，可把 Get_Size 的输出接到 `size`，再设置 Border Width/Height、Center 和 Border Type。Constant 模式可以选择边框颜色。该节点按原库绝对差扩边，小于原图的目标尺寸不会裁剪；居中时奇数差向下截断，最终尺寸可能少 1 像素。

常见链路为 `Image_Loader → Transform → Crop → Copy_Make_Border → Viewer`；也可让原图接 Crop.in、Contours 的 JSON 接 Crop.bbox，实现按检测结果裁剪。详细 JSON、状态与计算规则见 [内部节点说明](../nodes/README.md)。

## 颜色校正与通道处理

Color 分类可创建 Color_Correct 和 Color_Reduce，Utility 分类可创建 Split 和 Combine，也可以直接 Tab 搜索名称。

- **Color_Correct**：图像接 `in`，在 Controls 调整 Saturation、Gamma、Contrast、Gain、Brightness，计算按这个顺序执行；Reset 恢复全部默认值。Low Range / High Range 使用 RGB 数值，分别限制原始颜色通道的处理范围。可把 8 位单通道掩膜接 `mask`；尺寸不同时会线性缩放，只有值恰好为 255 的位置参与处理。输入应为 8 位 BGR/BGRA，alpha 保持原样；单/双通道输入直接透传。Gamma=0 保留原库行为，将选中通道中小于 255 的 LUT 输入变为 0。
- **Color_Reduce**：通用 Properties 的 Reduce 默认 64，调整颜色量化步长。处理所有 8 位通道，包括 alpha；非 8 位输入不输出。**Reduce 不随工程保存**，重开工程或粘贴新节点后恢复默认 64，这是原库 null 状态的行为。
- **Split**：彩色图接 `in`，`r/g/b` 输出分别是红、绿、蓝平面；只有四通道输入才输出 `a`。原位深保持不变。无有效彩色输入时 Controls 显示提示。
- **Combine**：把三个同尺寸、同类型的平面接 `r/g/b`，得到 BGR 图像；可选 `a` 必须同尺寸同类型，否则忽略。Split 与 Combine 每次处理会自行启用，不能通过禁用状态使其持续停止。

可连接 `Image_Loader → Color_Correct → Color_Reduce → Viewer`，也可将 `Split.r/g/b` 分别接到 `Combine.r/g/b` 后预览重组图像。完整状态格式、通道规则和异常边界见 [内部节点说明](../nodes/README.md)。

## 空间滤波与锐化

在 Filter 分类或 Tab 搜索创建 Sobel、Scharr、Laplacian、Sharpen，在 Properties 中调参，端口均为 `in → out` 图像。Sobel/Laplacian 使用专属 Controls 保留核大小的步长 2；Scharr/Sharpen 使用通用属性。

- **Sobel / Scharr**：Derivative Order 选择 X 或 Y；Depth Out 默认 CV_32F，可选 Auto、CV_16S、CV_64F。多通道输入先转灰度，输出保留梯度符号和位深。Sobel 可调 Kernel Size，Scharr 使用自身固定核。
- **Laplacian**：设置 Depth Out、Kernel Size、Scale、Delta。Sobel/Laplacian 的 Kernel Size 界面编辑为 1–7 的奇数；加载工程时保留原保存值。三个导数节点的 Scale/Delta 均允许负数。
- **Sharpen**：Mode 1 的 Sharpen 为 0–3，0 复制输入；Mode 2 为 0–5，沿用原库 Laplacian 与原图加权合成。切回 Mode 1 时大于 3 的值会降至 3。Mode 2 使用 BGR 输入，不支持灰度或 BGRA，错误输入本帧不输出。

可连接 `Image_Loader → Sobel → Viewer`，通过 Viewer 的浮点预览观察梯度；若下游需要 8 位幅值，显式接入 `Scale_Abs`。Viewer 的伪彩只影响预览，不改变流经节点的数据。四个节点禁用时输出原图，Sharpen 按原库规则复制图像。详细状态和错误处理见 [内部节点说明](../nodes/README.md)。

## 频域与随机图像

Utility 分类可创建 Get_Optimal_DFT_Size、DFT、DCT、Magnitude，Source 分类可创建 Rnd_Noise，也可 Tab 搜索名称。有参数节点直接使用通用 Properties。

- **Get_Optimal_DFT_Size**：图像接 `in`，或把 `[width,height]` 数组接 `size`；同时有输入时非空图像优先。`size` 输出只是建议尺寸，可接 Copy_Make_Border.size 手动构建补边链路，节点自身不修改图像。禁用时不输出。
- **DFT**：图像接 `in`，幅值输出 `dft` 接 Viewer。Shift DC to Center 默认开启，将频谱裁为偶数尺寸并移动直流位置；Logarithmic View 开启时输出归一化 8 位图，否则保持 32F 幅值。Operation 保留原六个选项，但各模式最终都输出幅值，不能将它当作复数频谱接逆变换来恢复原图。禁用时不输出。
- **DCT**：Operation 可选 Default、Inverse、Rows。彩色先转灰度；32F 原值输入，其它深度先转 32F 并除 255。输出浮点结果，不自动补边。**Operation 沿用原库不保存的行为**，重开工程或粘贴新节点后回到 Default。禁用时不输出。
- **Magnitude**：`in1/in2` 接同尺寸、同类型的 32F 或 64F 单通道图像，输出两路的幅值。即使禁用也需要两路非空输入，随后透传第一路。
- **Rnd_Noise**：设置 Width、Height、Mask Mode 和 Output FPS；Color 为独立彩色噪声，BW_8U 为单通道灰度，BW_8UC3 为三通道相同灰度。像素范围 0–254；尺寸、模式和 FPS 随工程保存。它沿原库忽略 enabled 标记，禁用不会停止产生图像。

可连接 `Rnd_Noise → DFT → Viewer` 观察频谱，或以 `Sobel/Scharr` 的 X/Y 浮点梯度分别接 Magnitude。高位深 Viewer 的伪彩仍只改变预览。各节点类型、保存格式和不支持的输入条件见 [内部节点说明](../nodes/README.md)。

## 背景分割、检测与交点

Filter 分类创建 Background_Subtraction，其余四个节点在 Feature Detection 分类，也可 Tab 搜索完整名称。Background_Subtraction 设置 MOG2/KNN、History、Threshold 和 Detect Shadows；切换模式保留两种模型各自学到的背景。静态图片反复输入后会成为背景，检查前景应输入连续变化帧。

- 将 Background_Subtraction.mask 接 Blob_Detector.in，viz 接 Viewer，blobs 接 Json_Viewer；设置阈值、面积/圆度/凸度/惯性及颜色筛选。默认绘制颜色是黑色，可在 Controls 中调整。
- 将 Canny.out 接 Hough_Lines.in，lines 接 Line_Intersections.lines；原图另接 Line_Intersections.in，out 接 Viewer、points 接 Json_Viewer。Hough Lines 可切换 Standard/Probabilistic 并选择是否绘线；交点设置最小夹角、距离、点颜色和实心显示。
- Hough_Circles 的检测输入是第 2 个端口 canny，原图可接第 1 个端口 orig；不接 orig 时在黑色底图上显示。可设置圆半径范围、中心和半径文字，circles JSON 中的 size 是直径。

检测 JSON 的空结果语义沿用原库：Hough 输出 `data:null`，Blob/交点无结果时不发送新 JSON；Json_Viewer 因此可能仍显示上次结果。Line_Intersections 求无限直线的交点，结果可能在图像外；输入超过 200 条线时不处理，并在 Controls 提示。状态格式和禁用规则见 [内部节点说明](../nodes/README.md)。本批参数界面通过可选 Controls 注册，不需要修改主窗口。

## 颜色分类、计数和跟踪

在 Feature Detection 分类或 Tab 搜索创建 Shape_Colorizer、Shape_Counter、Simple_Blob_Tracker。它们通过启动时扫描的 `.fp` 加载，带自己的 Qt Controls；需要使用本次构建部署的 Plugins 目录。

1. 原图接 Shape_Colorizer.in，Blob_Detector.blobs 或 Hough_Circles.circles 接 shapes。Mean 显示平均颜色，Max 将色相增强至满饱和/亮度；Closest 在颜色列表中添加颜色和标签，选择 RGB/CIE76 及阈值。Overlay 选择在原图上绘制或输出独立颜色图。
2. Shape_Colorizer.shapes 接 Shape_Counter.shapes，原图接 Shape_Counter.in；Counting Mode 选 By ID，按 class_id 标签计数。By Size 则按尺寸严格范围筛选。图像结果接 Viewer，counts 接 Json_Viewer；字体、RGBA、位置和 Overlay 在 Controls 编辑。
3. 原图接 Simple_Blob_Tracker.in，Blob/Hough 的 `{x,y,size}` 结果接 blobs；vis 接 Viewer，tracks 接 Json_Viewer。用确定连续帧或后续视频源观察移动，设置最大位置/尺寸差、历史长度和丢失时长。静态图也可建立稳定 ID，但不能代替运动跟踪验证。

Reset Tracking 清空学习历史并从 ID 1 开始；start 布尔输入持续 true 会每帧清空，不能当作持续运行开关。轨迹至少有两个位置样本才进入 tracks JSON，初始 data:null 正常。计数的 By ID 使用颜色分类的 class_id，不直接消费跟踪输出的 id。参数、颜色列表随工程保存，跟踪历史和待复位命令不保存。

颜色列表支持增删和清空；原库匹配依赖列表顺序。复杂算法与空结果、禁用规则见 [插件说明](../plugins/README.md)。

## 文字、数字、时间与结果标注

在 Draw 分类或 Tab 搜索创建 Draw_Text、Draw_Number、Draw_Date_Time、Draw_JSON。图像接 in，out 接 Viewer 或 Image_Writer；四个节点均在 Controls 编辑字体、位置、缩放、厚度和颜色。

- Draw_Text 编辑 String；外部 str 输入存在时隐藏本地文字框，并使用输入字符串，空串也会覆盖本地文字。
- Draw_Number 将数值接 bool/int/float，多个输入同时存在时按这个顺序取值；布尔显示 True/False，浮点沿用原 std::to_string 格式。
- Draw_Date_Time 设置 Show Date/Time，在格式框编辑后按 Enter 提交。无效格式显示错误并继续使用上一个有效格式。默认日期 `%Y-%m-%d`、时间 ` %X`，使用主机本地时间；关闭对应显示后该格式不写入工程，重建节点时使用默认值。
- Draw_JSON 接收已有检测、计数或跟踪 JSON。例如原图接 in，Shape_Counter.counts 接 json；收到数据后双击可用字段 `/count` 加入绘制列表，out 即标注计数。Data Index 选择 data 数组元素；数组字段可重复添加并分别编辑索引，双击已选项删除。Decimal Places 只影响浮点值。

Draw_JSON 未收到非空 JSON 时不输出图像，Viewer 可能先保留旧帧再超时；收到非空 JSON 但没有可绘制字段时输出原图副本。文字保持 OpenCV 原绘制能力，不新增字体引擎。字段、参数和位置随工程保存，实时 JSON 与日期格式的未提交草稿不保存。

## Viewer 显示与像素检查

每个 Viewer 对应独立 Dock，显示选项互不影响，也不修改上游或下游的图像数据。关闭 Dock 仅隐藏界面，节点继续运行；从 View → Node Windows 重开。删除节点会释放其界面。

| 控件 | 行为 |
| --- | --- |
| Channel：RGB | 显示彩色；保持原库忽略 BGRA alpha 的显示语义 |
| R / G / B / A | 将所选通道作为灰度显示；无 alpha 的输入按不透明 alpha 显示 |
| Lum | 显示亮度灰度 |
| Aspect：Free | 图像伸展填满当前视口 |
| Width / Height | 分别按视口宽 / 高缩放，保持比例；另一维超出时可滚动查看 |
| Color map / Scale | 仅在支持的高位深单通道图像上显示；默认 JET、0.15 |

支持 `CV_8UC1/3/4`，以及 `CV_16UC1`、`CV_16SC1`、`CV_32FC1`、`CV_64FC1`。高位深预览依次执行 scale 转 8 位饱和、归一化至 1..255、伪彩，再执行通道选择，沿用 FlowCV 的转换顺序。它是显示映射，不用于读取原始深度数值。其它格式或空图显示 `No supported image`，不会保留误导性的旧画面。

在图像内按住 Ctrl，可查看分辨率、原图整数坐标、归一化 UV，以及所选显示通道的 RGB/HSV。滚动后仍按实际图像位置取色。RGB 为 0..255，HSV 为归一化值；取色对应图像像素，不是平滑缩放后屏幕插值颜色。

原版 Width/Height 会改变窗口另一维；Qt Dock 采用滚动画布，避免改变其它停靠区域。显示选项仅保存在当前面板，不写入 Viewer 的空节点状态。

## 直方图、视频、通信与 DNN

Histogram 位于 Views 分类，图像接 in 后显示独立 Main Dock。曲线可分别开关，左拖平移、滚轮缩放、双击自适应，Reset axes 恢复原初始范围。显示开关与轴范围不随工程保存。

Video_Loader 在 Controls 选择文件、播放/暂停、循环及定位帧；Video_Writer 选择输出路径、codec、FPS，再 Start Write。不要将 Loader.start 直接持续接 Writer.start：沿用原库，Loader 前两帧为 true，而 Writer 每个 true 都切换录制。Video_Capture 从 None 切换到设备，选择分辨率/FPS等参数，Refresh 更新枚举；切回 None 释放设备。

CSV 与 UDP/TCP/OSC/Serial 均在 Output 分类配置。先选择路径、地址/端口或串口，再操作写入/连接/发送开关；状态和错误在 Controls 显示。数值/JSON 的打包和发送优先级沿用原库，具体见插件文档。示例通信工程默认禁用输出，使用前先设置自己的接收端。

DNN 分类包含七类原模型节点，依次配置模型/配置文件、类别或字典、输入尺寸、预处理、后端/目标，然后 Initialize。模型输出布局必须与节点的解析方式匹配，不能只凭扩展名判断可用；模型加载和推理失败会显示错误。详情见 [DNN说明](../nodes/DNN/README.md)。

Depth_Viewer_3D 的 depth 输入必须是 16 位无符号单通道深度，不能直接连接 Image_Loader 的 BGR 输出。color 可选，intrin 接原内参 JSON；无内参时 Controls 显示 H-FOV。Main 中右拖旋转、中拖平移、滚轮缩放，关闭 Orbit 后可用 WASD/QE 飞行，H 恢复初始相机。每个 Dock 独立保存相机与显示参数。

[七份代表工程](examples/README.md)提供静态分析、结果消费、视频、通信、DNN、深度和直方图链路及可复现资源生成命令；深度输入及实际模型需按说明补充。硬件和人工验收范围见开发进度。

## 设置与插件目录

Edit → Settings 提供以下配置，保存至 Qt 应用配置目录中的 `flowcv_editor.cfg`，窗口布局仍独立使用 QSettings。

| 设置 | 默认及生效方式 |
| --- | --- |
| Extra Plugin Paths | 默认空；默认 `Plugins` 递归扫描，额外目录不递归；保存后重启生效 |
| Flow Buffer Count | 默认 1，保持原库默认执行方式；大于 1 设置相应缓冲数，重启生效 |
| Log Level | Info；Debug / Info / Warning / Off，保存后应用 |
| Recent File History Size | 默认 8，限制 Load Recent 数量 |
| Show UI FPS | 默认关闭；显示画布界面刷新统计，不是算法吞吐 |

可指定配置文件：

```powershell
& ./out/build/local-x64-release/app/VisionFlowStudio/VisionFlowStudio.exe --cfg ./my-studio.cfg
```

保留原库配置键 `ext_plugin_dir`、`buffer_count`、`log_level`、`recent_size`、`recent_files`、`show_fps`。日志等级配置值为 0=Debug、1=Info、2=Warning、3=Off。正式入口初始化日志，文件位于运行工作目录下的 `logs/`。Qt 后端不提供 GLFW VSync 选项。

插件需要与宿主匹配的 SDK、Qt（仅自定义 Qt 界面插件）、OpenCV、编译器与运行库。旧 ImGui 插件界面不能直接嵌入 Qt。加载失败查看日志；增加目录后重启。开发者可用 [PluginMaker](../tools/README.md) 创建普通插件、Qt 插件或内部节点模板。

## 排查与限制

预览黑屏时检查源是否有输出、连接类型、处理节点输入尺寸与类型，以及是否禁用了节点。看不到 Dock 时使用 View → Node Windows 或 Reset Layout。Viewer 每 33 ms 尝试刷新，但隐藏窗口不刷新；这不保证每秒处理 30 帧。

当前不含运行暂停、单步、Undo/Redo、热重载或插件崩溃隔离。保存前确认流程内容；关闭 Studio 会停止 AutoTick，再释放节点界面和运行对象。
