# 内置节点

这里通过 `sdk` 公共接口实现内置节点。`VisionFlowNodes` 是静态库，通过 `VisionFlow::Nodes` 链接，
当前编译 `Add/add.cpp`。

节点目标通过 `find_package(OpenCV CONFIG REQUIRED COMPONENTS core)` 查找 OpenCV，并链接导入
目标 `opencv_core`。在本机的 `CMakeUserPresets.json` 中设置 `OpenCV_DIR`，指向与编译器、架构和
构建配置匹配的安装包目录。MSVC Debug 需要对应的 Debug 库，不能用仅包含 Release 库的安装包替代。Wind
ows 下应用构建完成后会复制所链接的运行时 DLL 到可执行文件目录。

已经完成实现的节点包含：
- Add节点：实现图像加法
- Solid节点：给出指定形态的背景图片
- Viewer节点：无图界面的编辑器节点

节点计算逻辑不依赖 `editor`。未来自定义节点界面作为可选部分单独组织，节点注册与执行由应用装配及
`managers` 协作完成。
