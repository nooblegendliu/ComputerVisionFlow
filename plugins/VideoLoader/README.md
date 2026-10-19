# Video_Loader

从 FlowCV `b09c48e` 的 `Plugins/VideoLoader` 移植。计算库使用 OpenCV videoio，只有 Studio 构建增加可选 Qt Controls，仍通过 `Create` 导出 `.fp`。

- 输出保持 `video`、`start`、`frame`、`fps`；`frame` 保持解码后的一基帧位置，`start` 保留原库前两帧为 true 的行为。
- 状态只包含 `movie_path`、`looping`、`use_fps_speed`。播放/停止与当前帧不写入工程。
- 保留默认循环播放、影片 FPS 和未使用影片 FPS 时的约 60 Hz 更新上限。忙等待改为 `sleep_until`，FPS 未知/无效时使用 16 ms，避免死循环或除零。
- 必要修复：原库非循环模式在最后一帧之前停止；现在完整解码到末帧后停止并保持末帧。seek 控件统一使用一基帧，播放和停止状态均可跳转，避免原控件越界及播放时跳转无效。
- 视频打开、读帧、播放命令和状态由同一实例互斥锁保护；文件失败给出 Controls 状态且不输出旧影片。Qt 控件使用弱会话引用，宿主的 NodeUiRegistry 管理节点/插件租约。

`stage4_video_test` 使用实际生成的 MJPG 视频覆盖解码、EOF、循环、seek、状态恢复、错误恢复与可选 Qt Controls；不需要外部视频文件。
