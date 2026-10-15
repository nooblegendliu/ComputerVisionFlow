# 内置节点

`VisionFlowNodes` 是内置节点静态库，通过 `VisionFlow::Nodes` 链接。当前 [构建清单](CMakeLists.txt) 与 [节点工厂](../managers/node_manager.cpp) 均已接入 `Solid`、`Add`、`Viewer`。节点继承 [FlowCV::Node](../sdk/flowNode.h)，使用 DSPatch `Process_()` 和 SignalBus 计算、传递数据，不依赖 Qt 或 `editor`。模块关系见 [架构说明](../docs/architecture.md)，整体完成状态与验证范围见 [开发进度](../docs/development-status.md)。

## Solid

[实现](Solid/Solid.cpp) 生成固定尺寸的纯色或灰度图像。没有输入，输出端口 `0: out` 为 `CvMat`。

| 参数 | 默认值 | 条件与限制 |
| --- | --- | --- |
| `width` / `height` | `640` / `480` | 计算时分别裁剪到 `[2, 10000]` |
| `is_color` | `true` | `true` 输出彩色图，`false` 输出灰度图 |
| `has_alpha` | `true` | 彩色模式下显示；决定输出三通道或四通道图像 |
| `color_r` / `color_g` / `color_b` | 均为 `0.0` | 彩色模式下显示；计算时裁剪到 `[0.0, 1.0]` |
| `color_a` | `1.0` | 彩色且启用 Alpha 时显示；计算时裁剪到 `[0.0, 1.0]` |
| `value` | `0` | 灰度模式下显示；计算时裁剪到 `[0, 255]` |
| `fps_index` | `7`，对应 `30 FPS` | 索引从 `0` 开始；选项依次为 `1, 3, 5, 10, 15, 20, 25, 30, 60, 120 FPS` |

默认输出为 `640 × 480` 的黑色 `CV_8UC4` 图像，Alpha 为 `255`。关闭 Alpha 后输出 `CV_8UC3`，灰度模式输出 `CV_8UC1`；彩色像素按 OpenCV 的 BGR / BGRA 顺序存储。帧率控制目前保留原库的忙等待方式。

`Solid` 自定义状态读写，保留 `solid_color`、`fps` 等原库字段；恢复时也接受 `fps_index`。参数编辑值在处理函数调用 `Sync()` 后用于计算。

## Add

[实现](Add/add.cpp) 使用 `cv::add` 计算两路图像之和，端口均声明为 `CvMat`。

| 方向与索引 | 名称 | 要求 |
| --- | --- | --- |
| 输入 `0` | `in1` | 必须有非空图像 |
| 输入 `1` | `in2` | 必须有非空图像，正常计算时须与 `in1` 尺寸、类型一致 |
| 输入 `2` | `mask` | 可选；有效 Mask 必须为 `CV_8UC1`，且与输入图像尺寸一致 |
| 输出 `0` | `out` | 加法结果 |

唯一参数 `mask_mode` 默认是 `0: Default`。使用有效 Mask 时，`0: Default` 将 Mask 外像素设为黑色，`1: In1` 保留第一路输入，`2: In2` 保留第二路输入；Mask 内进行加法。缺少或无效的 Mask 按无 Mask 处理，整幅图像参与加法。

缺少任一路有效输入时不产生输出；正常计算时两路图像尺寸或类型不一致也不产生输出。底层节点被禁用时，在两路输入均非空的前提下直接输出 `in1`。当前这些输入检查没有向编辑器提供具体错误状态。

## Viewer

[计算节点](Viewer/Viewer.cpp) 是终端缓存节点：输入 `0: in` 为 `CvMat`，没有输出端口，也没有可编辑节点参数。初始缓存为 `640 × 480` 的黑色 `CV_8UC3` 图像。收到非空图像时复制到缓存；输入信号缺失且距上次有效输入超过 `500 ms` 时将缓存清零。节点状态不包含图像缓存。

`Viewer::GetFrameCopy()` 在互斥锁保护下返回图像深拷贝。[Qt ViewerPanel](../app/VisionFlowStudio/ViewerPanel.cpp) 可见时按 `33 ms` 定时器读取对应节点的缓存，当前支持预览 `CV_8UC1`、`CV_8UC3`、`CV_8UC4`，并按比例缩放显示。这个定时器只刷新界面，DSPatch 计算由 [应用入口](../app/VisionFlowStudio/main.cpp) 启动的 AutoTick 驱动；隐藏 Viewer 窗口不会停止节点计算。

## 新增节点

新增节点需同时接入以下三处：

1. 在 [nodes/CMakeLists.txt](CMakeLists.txt) 中显式加入头文件、源文件及必要依赖。
2. 在 [internal_nodes.h](internal_nodes.h) 中包含节点头文件。
3. 在 [InternalNodeManager 构造函数](../managers/node_manager.cpp) 中调用 `RegisterNode<T>()`。节点类型须继承 `FlowCV::Node`、可默认构造，并提供非空且不重复的组件名。

迁移原库节点时核对端口、参数、处理和状态格式，将界面逻辑放在 Editor / App 层。当前属性系统支持 Bool、Int、Float、Option，文件路径等复杂参数仍需扩展，详见 [SDK 属性定义](../sdk/flowProperties.h)。

节点库当前查找并链接 OpenCV `core`。本机通过 `CMakeUserPresets.json` 设置 `OpenCV_DIR`，指向与编译器、架构及构建配置匹配的安装包；MSVC Debug 需要对应的 Debug 库。具体构建方式见 [根目录 README](../README.md)。
