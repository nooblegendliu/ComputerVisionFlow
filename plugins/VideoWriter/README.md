# Video_Writer

从 FlowCV `b09c48e` 的 `Plugins/VideoWriter` 移植。输入保持 `in`、`fps`、`start`，无输出。全部 19 项 FourCC 名称、顺序、对应扩展名和 FPS 选项保持原库。默认 H264/30 FPS；实际编码器可用性由 OpenCV 后端决定，不以其它编码器静默代替。

状态键保持 `filename`、`codec`、`codec_str`、`fps`、`fps_index`、`auto_ext`。恢复工程不会自动开始录制。运行时开始/停止接口不改变状态格式。

保留原库语义：每次收到 true 的 `start` 输入都切换录制状态；没有改成边沿触发。`fps` 输入覆盖选择的 FPS。FPS/codec 变更会重新打开输出文件，写入不额外限速，原库不按节点 enabled 状态屏蔽写入的行为也保留。修改 `auto_ext` 只影响下次正常打开文件，不会重开、覆盖当前录制。

必要保护：序列化所有写入与停止/释放；拒绝非法 codec 索引和非四字符 FourCC；无效 FPS、图像类型、录制中尺寸变化及文件失败显示错误，不让异常终止调度。扩展名处理不再把父目录中的点当作文件扩展名。仅支持原库 color=true 对应的 8 位 BGR 录制，不隐式转换算法图像。

`stage4_video_test` 实际写出并重新解码 MJPG 视频，检查帧数/颜色、录制中关闭再开启自动扩展名后的累计帧、true 输入切换、自动扩展名、失败恢复、状态和析构完成文件写入。Studio 构建额外检查 Qt Controls 和插件租约释放。
