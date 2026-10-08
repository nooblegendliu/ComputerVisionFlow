# 内置节点

这里通过 `sdk` 公共接口实现内置节点。`VisionFlowNodes` 是静态库，通过 `VisionFlow::Nodes` 链接，当前编译 `Add/add.cpp`。

节点目标通过 `find_package(OpenCV CONFIG REQUIRED COMPONENTS core)` 查找 OpenCV，并链接导入目标 `opencv_core`。在本机的 `CMakeUserPresets.json` 中设置 `OpenCV_DIR`，指向与编译器、架构和构建配置匹配的安装包目录。MSVC Debug 需要对应的 Debug 库，不能用仅包含 Release 库的安装包替代。Windows 下应用构建完成后会复制所链接的运行时 DLL 到可执行文件目录。

`Add` 当前只有构造函数，`Process_()` 尚未实现；实际实例化、注册该节点前需要补充计算逻辑。新增节点源文件时更新本目录的 `CMakeLists.txt`。

节点计算逻辑不依赖 `editor`。未来自定义节点界面作为可选部分单独组织，节点注册与执行由应用装配及 `managers` 协作完成。
