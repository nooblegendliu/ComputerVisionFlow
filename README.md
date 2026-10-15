# ComputerVisionFlow

ComputerVisionFlow 是基于 FlowCV 数据流机制的 C++20 计算机视觉流程项目，使用 CMake 构建、Qt / QtNodes 编辑流程、DSPatch 执行计算。

当前 VisionFlowStudio 已接入图编辑、通用参数面板和多 Viewer 实时预览。用户从空画布创建 Solid、Add、Viewer 并连线，后台通过 AutoTick 持续执行；窗口尺寸与 Dock 布局使用 QSettings 保存。详细实现状态、验证依据与后续工作见 [开发进度](docs/development-status.md)。

## Windows 快速开始

准备 CMake 3.25 及以上、MSVC x64 开发者环境、Ninja，以及匹配的 x64 Qt 6 和 OpenCV 包。项目使用 vcpkg classic 查找 spdlog、nlohmann_json；Studio 还需要 Qt 的 Core、Gui、Widgets、OpenGL 和 windeployqt。

在项目根目录创建个人 `CMakeUserPresets.json`，设置自己的 `VCPKG_ROOT`、`Qt6_ROOT`、`OpenCV_DIR`，并提供 `local-x64-release` 配置与构建预设。配置示例、依赖版本来源和部署限制见 [第三方依赖说明](third_party/README.md)。

在 Visual Studio x64 开发者 PowerShell 中执行：

```powershell
cmake --list-presets=all
cmake --preset local-x64-release
cmake --build --preset local-x64-release
& ./out/build/local-x64-release/app/VisionFlowStudio/VisionFlowStudio.exe
```

当前本机可用的配置与构建预设均为 `local-x64-release`；共享的 `windows-base` 和 `Qt` 是隐藏辅助项。个人配置和 `out/` 构建产物由 Git 忽略，不将安装目录固化到共享工程。

启动后打开 Qt 主窗口，画布为空。按照 [Studio 使用指南](docs/studio-guide.md) 手动搭建两个 Solid → Add → Viewer，修改灰度值并观察预览；程序不会自动搭建示例或输出 `[PASS]`。

默认构建五个项目静态库 Utils、Sdk、Nodes、Managers、Editor，DSPatch 与 QtNodes 静态库，以及 VisionFlowStudio。关闭 `VISIONFLOW_BUILD_STUDIO` 后不查找 Qt，只构建基础库，不生成无界面执行程序。Windows 构建后会复制运行时 DLL 并使用 windeployqt 部署 Qt 依赖，当前没有安装包。

## 文档导航

| 文档 | 内容 |
| --- | --- |
| [Studio 使用指南](docs/studio-guide.md) | 界面操作、灰度加法示例、多 Viewer、删除、缩放与布局 |
| [开发进度](docs/development-status.md) | 完成状态、验证依据、已知限制与后续建议 |
| [架构说明](docs/architecture.md) | 模块关系、图适配、属性同步、预览、生命周期与持久化边界 |
| [第三方依赖](third_party/README.md) | 依赖来源、许可、环境准备、个人预设和 Windows 部署 |
| [内置节点](nodes/README.md) | Solid、Add、Viewer 的端口、参数与接入方法 |
| [动态插件](plugins/README.md) | 加载器、应用接入和原版插件适配边界 |
| [开发工具](tools/README.md) | 辅助工具目录用途 |

## 目录导航

| 目录 | 用途 |
| --- | --- |
| `Utils/`、`sdk/`、`nodes/`、`managers/` | 公共辅助功能、节点支持、内置计算与运行图管理 |
| `editor/` | QtNodes 图适配、画布交互与参数面板 |
| `app/VisionFlowStudio/` | Qt 应用入口、主窗口与 Viewer 预览 |
| `app/ProcessEngine/` | 尚未接入构建的无界面入口占位 |
| `plugins/`、`tools/` | 插件实现与开发工具预留位置 |
| `third_party/`、`sdk/thirdParty/` | 包查找、QtNodes 与 DSPatch 源码 |
| `docs/` | 架构、使用指南与开发进度 |

当前 Studio 尚无工程文件保存与加载，关闭窗口只保存布局；真实媒体输入、更多算法、完整插件使用链路和独立 ProcessEngine 仍待开发。
