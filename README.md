# ComputerVisionFlow

ComputerVisionFlow 是基于 FlowCV 数据流机制的 C++20 计算机视觉流程项目，使用 CMake 构建、Qt / QtNodes 编辑流程、DSPatch 执行计算。

VisionFlowStudio 已接入工程保存恢复、通用 Qt 节点界面、动态插件、应用设置、子图复制粘贴和多 Viewer 图像检查。后台通过 DSPatch AutoTick 持续执行；`.flow` 保存流程与节点位置，QSettings 保存窗口与 Dock 布局。本轮公共功能已通过 Release 构建、集成测试及主要 Windows 窗口交互验收，具体覆盖与边界见 [开发进度](docs/development-status.md) 和 [原库功能验收清单](docs/studio-feature-checklist.md)。

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

Studio 构建包含 Utils、Sdk、Nodes、Managers、Editor、可选 QtNodeUi 适配层、DSPatch、QtNodes、VisionFlowStudio 及已注册动态插件。关闭 `VISIONFLOW_BUILD_STUDIO` 后不查找 Qt，只构建基础计算功能，不生成无界面执行程序。Windows 构建后会复制运行时 DLL 并使用 windeployqt 部署 Qt 依赖，当前没有安装包。

启用项目集成测试：

```powershell
cmake --preset local-x64-release -DVISIONFLOW_BUILD_TESTS=ON
cmake --build --preset local-x64-release --parallel 4
ctest --test-dir out/build/local-x64-release --output-on-failure
python -B tests/test_plugin_maker.py
```

## 文档导航

| 文档 | 内容 |
| --- | --- |
| [Studio 使用指南](docs/studio-guide.md) | 工程操作、快捷键、设置、Viewer、插件使用 |
| [开发进度](docs/development-status.md) | 完成状态、验证依据、已知限制与后续建议 |
| [源码全面复核](docs/flowcv-source-audit-2026-10-19.md) | 原库对照发现的代码/格式差异、待验收项与剩余工作优先级 |
| [剩余移植计划](docs/flowcv-migration-plan.md) | 71 个节点的移植记录、验证范围与剩余环境验收 |
| [原库功能验收清单](docs/studio-feature-checklist.md) | FlowCV 功能与当前入口、阶段和验收状态的对照 |
| [架构说明](docs/architecture.md) | 模块关系、图适配、属性同步、预览、生命周期与持久化边界 |
| [第三方依赖](third_party/README.md) | 依赖来源、许可、环境准备、个人预设和 Windows 部署 |
| [内置节点](nodes/README.md) | 58 个内部节点的端口、参数、状态与接入方法 |
| [动态插件](plugins/README.md) | 加载器、应用接入和原版插件适配边界 |
| [开发工具](tools/README.md) | PluginMaker 普通插件、Qt 插件和内部节点模板 |

## 目录导航

| 目录 | 用途 |
| --- | --- |
| `Utils/`、`sdk/`、`nodes/`、`managers/` | 公共辅助功能、节点支持、内置计算与运行图管理 |
| `editor/` | QtNodes 图适配、画布交互与参数面板 |
| `app/VisionFlowStudio/` | Qt 应用入口、主窗口与 Viewer 预览 |
| `app/ProcessEngine/` | 尚未接入构建的无界面入口占位 |
| `plugins/`、`tools/` | ImageLoader、ImageWriter 动态插件与 PluginMaker 生成器 |
| `third_party/`、`sdk/thirdParty/` | 包查找、QtNodes 与 DSPatch 源码 |
| `docs/` | 架构、使用指南与开发进度 |

节点迁移阶段 1 支持静态图片分割、轮廓 JSON、多个 Viewer 与图片输出闭环；阶段 2A 又接入 Abs_Diff 等 11 个算术与数值节点。阶段 2B 已接入 Crop、Transform、Perspective_Warp、Copy_Make_Border；阶段 2C 已接入 Color_Correct、Color_Reduce、Split、Combine。阶段 2D 已接入 Sobel、Scharr、Laplacian、Sharpen。阶段 2E 已接入 Get_Optimal_DFT_Size、DFT、DCT、Magnitude、Rnd_Noise，阶段 2 的 28 个节点全部接入。现有 58 个内部节点、13 个动态插件，阶段 3 首批 Background_Subtraction、Blob_Detector、Hough_Circles、Hough_Lines、Line_Intersections 已接入，阶段 3B 的 Shape_Colorizer、Shape_Counter、Simple_Blob_Tracker 已接入，阶段 3C 的四个 Draw 节点、3D Histogram、视频/通信插件、7 类 DNN 与 Depth_Viewer_3D 均已接入。71 个节点的契约及代表工程见 docs/examples；实际模型精度、GPU、真实串口及尚未执行的人工界面验收单列记录。测试按要求集中于复杂操作，简单节点仅做源码核对与构建检查；本批最终构建和回归状态见开发进度。独立 ProcessEngine、Undo/Redo、热重载、插件隔离和性能分析不在本轮范围内。
