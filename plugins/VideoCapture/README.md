# Video_Capture

项目规划与任务统一从 [项目文档](../../docs/README.md) 读取。旧项目测试与示例已移除；本文中的历史测试描述不代表当前可运行的测试或本次验收结果。

从 FlowCV `b09c48e` 的 `Plugins/VideoCapture` 移植，复用其 Windows/Linux/macOS 枚举实现，不引入 Qt Multimedia。保持 InOrder 调度、输出 `src`/`fps`、11 项分辨率、10 项 FPS、14 项手动属性，以及 Windows DShow/Linux V4L 的 OpenCV 捕获后端。

状态键保持 `src_index`、`list_index`、`force_index`、`src_name`、`src_mode`、`fps`、`fps_index`、`set_load_man`、`camera_settings`。索引 0 是 None，实际设备索引在状态中加 1；默认按设备名匹配，Force Use Index 时保留数字索引。只有成功打开真实设备后才应用和读取驱动属性。

Qt Controls 提供刷新设备、选择摄像头/分辨率/FPS、Force Use Index、原生驱动设置、手动属性与恢复设置。计算和全部控件操作通过实例互斥锁同步，选择 None 会释放设备。不会用测试设备替代不存在的摄像头。

必要修复：

- Windows 枚举初始化 COM 枚举指针，兼容 GUI 线程已有 STA，只有本次 COM 初始化成功才配对释放；设备名称使用 UTF-8。移除原枚举过程中无意义的 FriendlyName 写操作。
- 排除 NDI 时不改变后续 DShow 设备原始索引。Linux 保持实际 `/dev/videoN` 索引，修复原库对 video0 与 None 的混淆；macOS 释放首轮枚举迭代器。
- 保护非法分辨率/源索引、错误的属性 JSON；选择 None、切换设备和实例析构释放旧捕获句柄。图像帧在每次读取中独立持有，避免后续读取覆盖下游使用的 Mat。

`stage4_video_test` 验证真实枚举、None、按名称恢复/缺失设备、非法状态、Qt 控件及释放。独立 `stage4_capture_hardware` 的 2026-10-19 历史记录为本机真实摄像头通过：3帧初始采集、None释放后重开1帧、析构并新建实例再采1帧，检查协商后的尺寸/类型/FPS，仅处理内存而不保存图像。手动属性和原生驱动窗口、其它设备仍需实际验收；没有设备时该专项明确返回77。 平台枚举代码保留，当前 Windows 环境不代表 Linux/macOS 已完成运行验证。
