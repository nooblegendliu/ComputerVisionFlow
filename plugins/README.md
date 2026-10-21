# 动态插件

原 FlowCV 基准的十三个插件 ImageLoader.fp、ImageWriter.fp、ShapeColorizer.fp、ShapeCounter.fp、SimpleBlobTracker.fp、VideoLoader.fp、VideoWriter.fp、VideoCapture.fp、CsvFile.fp、UdpSend.fp、TcpSend.fp、OscSend.fp、SerialSend.fp 已接入构建、启动加载、节点菜单及 Qt 属性面板。Studio 启动时递归扫描程序旁的 `Plugins`，并非递归扫描 Edit → Settings 保存的额外目录；设置修改后重启生效。节点名称沿用 `Image_Loader`、`Image_Writer`、`Shape_Colorizer`、`Shape_Counter`、`Simple_Blob_Tracker`。创建实例仍通过 `DSPatch::Plugin` 的 `Create` 导出，Manager 保留插件优先于同名内置节点的规则。

当前另外构建 `DemoWaferLocator.fp`，组件名为 `Demo_Wafer_Locator`，共 14 个动态插件目标。该演示使用通用 Properties，没有专属 Qt Main；接口、样本和未验收边界见 [IndustrialDemo](IndustrialDemo/README.md)。原库 13 插件的固定契约不因增加演示而改变。

## ImageLoader

来源：[FlowCV ImageLoader](https://github.com/FlowCV-org/FlowCV/tree/b09c48edcc9437253da6049365d319fe036eff61/Plugins/ImageLoader)，按仓库 GPL-3.0 许可证迁移。保留 `DSPatch::DSPatchables::ImageLoader`、Source 分类、作者 Richard、版本 0.1.0、0 输入及单个 `out / CvMat` 输出。

在节点库或新增菜单创建 `Image_Loader`，在 Properties 的 Image Path 输入图片路径并按 Enter，或用 Browse 选择文件；连接 `out → Viewer.in` 即可预览。Output FPS 默认为 30，选项为 1、3、5、10、15、20、25、30、60、120。

保留原版行为：图片按 8-bit BGR 彩色模式解码、缓存并按 FPS 重复输出；空路径或读取失败不产生图像；处理时自动启用源节点；使用 `InOrder`、互斥锁及原有忙等限帧方式。重新提交同一路径会重新读取文件。读取失败写入 stderr，尚无节点内错误提示。

ImageLoader 的必要适配包括：派生现有 `FlowCV::Node` 接入属性桥，用通用 String / FilePath 属性和 Qt 控件替代 ImGui；以 UTF-8 路径打开文件并调用 `imdecode(IMREAD_COLOR)`，支持 Windows 中文文件名；恢复状态时校验并实际应用 FPS。JSON 继续保存 `image_path`、`fps`、`fps_index`，支持原版格式。

## ImageWriter

来源：[FlowCV ImageWriter](https://github.com/FlowCV-org/FlowCV/tree/b09c48edcc9437253da6049365d319fe036eff61/Plugins/ImageWriter)，按仓库 GPL-3.0 许可证迁移。保留 `Image_Writer` 名称、Output 分类、作者 Richard、版本 0.1.0、单个 `in / CvMat` 输入和 0 输出，以及 `OutOfOrder` 调度方式。

将图像输出连接到 `Image_Writer.in`，在 Properties 的自定义 Controls 中点击 **Set Output Path**，选择包含扩展名的输出文件路径，再点击 **Write Image**。下一次收到有效图像且节点启用时写入一次；同一帧处理前重复点击会合并为一个请求。空输入或禁用不会消耗待写请求；启用后收到有效图像才处理。未设置路径时处理请求但不写文件，沿用原库行为。

输出格式由文件扩展名决定，使用 OpenCV 默认编码参数。为了支持 Windows 中文路径，写入过程使用 `imencode` 和原生 `std::filesystem` 文件路径；不修改输入图像。实际写入在节点内部串行执行，路径、命令和状态通过互斥锁共享，计算线程不操作 QWidget。编码或文件写入失败会消费本次请求，在 Controls 显示错误并写入 stderr，不让异常退出 AutoTick；修正路径后可再次点击 Write Image。成功后显示完成提示。

**状态格式保持原库的 JSON `null`。输出路径、待写命令及完成计数均不保存到工程或剪贴板。** 重新打开工程或粘贴生成的新节点需要重新设置输出路径，不会自动写文件。`SetState` 仅检查 JSON 能否解析，不改变已有实例的路径或待写请求。

Qt-free 的 [ImageWriterControl](ImageWriter/image_writer_control.hpp) 提供 `SetOutputPath`、`RequestWrite` 和 `GetWriteStatus`，可通过组件的 `dynamic_cast` 查询；仅这些运行时命令经过接口，不增加插件导出入口。`GetWriteStatus` 返回路径、待写标记、已处理请求数、成功写入数及最近错误。接口裸指针仅在持有对应组件共享指针或 UI 宿主租约时有效。

## 阶段 3B：结果消费插件

三个实现分别位于 [ShapeColorizer](ShapeColorizer/shape_colorizer.cpp)、[ShapeCounter](ShapeCounter/shape_counter.cpp)、[SimpleBlobTracker](SimpleBlobTracker/simple_blob_tracker.cpp)，源自本机 FlowCV `b09c48e` 同名插件目录，按 GPL-3.0 迁移。均保留 Feature_Detection、Richard、0.1.0、OutOfOrder；各自构建独立 `.fp` 并导出 Create。计算部分只依赖 SDK/OpenCV core、imgproc，Studio 开启时另编译可选 Qt Controls 提供器；不修改主窗口或公共属性面板。

### Shape_Colorizer

输入 `0:in/CvMat`、`1:shapes/JSON`，输出 `0:out/CvMat`、`1:shapes/JSON`。两路输入信号均需存在。处理 data 中的 `{x,y,size}`，将 size 转整数后除 2 作为半径，适合 Blob/Hough 圆结果，不直接解析 Contours 的轮廓数组。

Mean 取圆形掩膜内均值；Max 保留原 ImGui 等价 RGB/HSV 换算，将 S/V 设为 1；Closest 使用 RGB 距离或原 D50 CIE76 算式。保留形状之间累计 mask、Overlay 改图对后续均值的影响、列表匹配后更新颜色再比较后续条目的顺序，以及严格距离阈值。保留原 JSON 并补 `color`，Closest 再补 `dist`、`class_id`，未匹配为 -1、None。Overlay 关闭输出独立颜色图，开启则绘制到原图副本，输入不变。禁用只透传图像。

状态保持 `method=0`、`dist_method=0`、`threshold=75`、`overlay=true`、`show_labels=false`、`ignore_none=true`；`color_match` 非空时保存 `[{color:{R,G,B},label},...]`，空时省略。部分恢复未提供列表时保留现有列表，显式空数组清空。Controls 支持模式、阈值、颜色/标签增删和清空。列表版本只防止失效行回调写错项目，不保存；标签改 std::string 避免原固定缓冲区越界。

必要适配为同步快照、原子恢复、坏 JSON/OpenCV 异常隔离、坐标检查、退化/越界 ROI 跳过；ROI 同时限制在图像与参考掩膜内，保留原右下边缘裁剪规则。计算颜色换算不依赖 ImGui 或 Qt。

### Shape_Counter

输入 `0:in/CvMat`、`1:shapes/JSON`，输出 `0:out/CvMat`、`1:counts/JSON`，需要两路信号及非空图像。By Size 将浮点 size 截断为整数后，按 `size_match-size_thresh < size < size_match+size_thresh` 计数；By ID 按 class_id 分组，不读取 Tracker 的 id。保留 unordered_map 分类顺序。

输出仅为 `{data:...}`，不新增 data_type/ref_frame。尺寸模式为 `[{size,count}]`，分类模式为 `[{class_id,count},...]`；无 data、空分类、零个尺寸匹配等 JSON 区别沿原库。九个状态全部保存，包括隐藏参数：`size_match=100`、`size_thresh=5`、`count_mode=0`、`show_overlay=true`、`position:{x:20,y:30}`、`scale=0.35`、`font=0`、`thickness=1`、`color:{R:1,G:1,B:1,A:0.8}`。

Alpha 用于原灰色背景混合，字体/边框、位置修正顺序及修正后状态回写保留；同步写回不覆盖期间用户编辑的位置。Qt 保留字体、位置、RGBA、样式、计数方式和条件尺寸控件；超 UI 默认范围的合法恢复值可准确显示。Overlay 仍使用原 8UC3 背景，不扩展灰度/BGRA 混合能力；坏 JSON 或不支持组合本帧不输出，后续可恢复。空 ROI 跳过混合，整数运算做必要防溢出。禁用只透传图像。

### Simple_Blob_Tracker

输入 `0:in/CvMat`、`1:start/Bool`、`2:blobs/JSON`，输出 `0:vis/CvMat`、`1:tracks/JSON`。图像信号必需，其余可不接。保留逐检测搜索最近轨迹，同帧多个检测可关联同一轨迹；只有一个历史位置时不筛尺寸/年龄；最近距离在尺寸/年龄判断前更新，不替换为新算法。缺少 blobs 时仍处理已有轨迹。

输出 `{data_type:"tracking",ref_frame:{w,h},data:...}`，每项 `{id,x,y,size}`；需累计至少两个位置样本才输出轨迹，否则 data 为 null。参考尺寸优先来自 blobs，否则取图像。随机轨迹颜色、历史线和编号保留，输入图像不变。历史超长时每帧只移除一个最旧位置。

七个状态键为 `show_tracking=true`、`show_ids=true`、`tracking_duration=300`、`drop_lost_tracks=true`、`lost_track_duration=120`、`max_pos_var=8`、`max_size_var=8`。轨迹、颜色、ID/帧计数和复位命令不保存；恢复参数到已有实例不清空历史，新实例从空历史开始。

Qt Reset Tracking 和 start=true 请求复位；start 持续 true 会每帧复位，不是上升沿触发。有图像信号时先处理复位，再检查空图/禁用；禁用只透传图像，不推进帧计数。必要修复：原删除循环跳过相邻过期轨迹，改同帧完整删除，仍用原 `age > lost_track_duration` 条件；历史访问加锁，补数值/JSON 异常边界。

## 构建与部署

`plugins/ImageLoader/CMakeLists.txt` 显式构建 SHARED 目标 `ImageLoader`，链接 SDK 和 OpenCV core / imgcodecs，使用 `.fp` 后缀。构建 Studio 时自动复制到其 `Plugins` 目录，并在 Windows 下复制插件运行库依赖到应用目录；即使仅插件更新，也会重新部署。关闭 `VISIONFLOW_BUILD_STUDIO` 仍可生成插件，不依赖 Qt。

`plugins/ImageWriter/CMakeLists.txt` 同样显式构建 SHARED 目标 `ImageWriter` 并注册部署。`VISIONFLOW_BUILD_STUDIO=ON` 时追加 `image_writer_qt.cpp`、链接可选 `VisionFlow::QtNodeUi`，通过提供器创建 Controls，不提供 Main Dock。关闭该选项时，仅编译 Qt-free 的计算实现，保留 `.fp`、`Create`、状态和运行时控制接口；不会引入 Qt 依赖，也不提供按钮界面。此时宿主可通过 `ImageWriterControl` 设置路径并发出写入请求。

插件与宿主必须使用匹配的 SDK、架构、C++ 运行库和 OpenCV 配置重新构建。这里验证的是当前源码构建的插件，不保证未经迁移的原版 `.fp` 二进制兼容；原版 ImGui 控件也不会自动成为 Qt 控件。

插件只在启动时扫描，不包含热重载。实例带有共享模块租约；Manager 清除加载记录后，外部组件共享指针及 UI 宿主仍保持动态库有效，直到最后一个实例租约释放。节点销毁先于其动态库，UI 宿主先销毁控件、QTimer 和连接回调再释放实例。外部共享指针无需提前释放；裸组件指针和保存的动态库函数地址不具备此保证。

## 可选 Qt 界面

普通插件继续只依赖 SDK，并使用通用属性控件。需要复杂界面时，额外链接 `VisionFlow::QtNodeUi`、实现 [NodeUiProvider](../editor/NodeUi/NodeUiProvider.h)，按 Controls/Main 角色返回 QWidget。Controls 进入属性分组，Main 进入独立 Dock；没有 Main 就不建 Dock。宿主优先使用插件提供器，其次按名称注册的适配工厂，再回退通用属性。

所有 QWidget/QTimer 必须在 GUI 线程创建、使用和销毁，计算线程不得操作控件。节点专属界面使用节点自身的线程安全接口；状态保存仍通过 GetState/SetState。完整示例和显式注册方法见 [PluginMaker](../tools/README.md)。Qt 插件必须使用与 Studio 匹配的 Qt 和运行库重新构建；旧 ImGui 二进制不直接兼容。

## 验证

旧插件测试与夹具已移除。后续修改应按任务验证真实插件加载、状态保存恢复、计算结果和界面生命周期，实际结果记录在 [项目文档](../docs/README.md) 的对应任务中。

## 阶段 4 / 5：视频与导出通信

三个视频插件复用原 OpenCV videoio 和平台设备枚举。契约、所有状态字段与必要修复分别见 [VideoLoader](VideoLoader/README.md)、[VideoWriter](VideoWriter/README.md)、[VideoCapture](VideoCapture/README.md)。Loader 保留逐帧播放、循环、帧号、FPS 和 start 信号；Writer 保留原 codec 列表与 start 每个 true 都切换录制的规则，不能把持续 true 当成持续录制。Capture 选择 None 会释放设备，刷新和驱动属性在可选 Controls 中操作。

CSV_File、UDP_Send、TCP_Send、OSC_Send、Serial_Send 保持五个独立 fp。协议、优先级、EOL/打包、CSV 触发与轮换、必要安全修复见 [DataOutput](DataOutput/README.md)。依赖复用原固定版本 asio2、oscpack 与串口枚举，不使用 Qt 网络替代计算层。默认不会向未配置端点发送。真实串口往返与目标端硬件协议需另行实测。

本批可选 Controls 仍通过 NodeUiProvider 提供，关闭 Studio 构建时仅计算插件。配置/状态和实时连接、录制句柄分离，界面先释放再销毁节点和动态库。
