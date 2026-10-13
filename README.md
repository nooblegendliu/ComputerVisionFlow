# ComputerVisionFlow

ComputerVisionFlow 是一个使用 C++20 和 CMake 的计算机视觉流程项目，目前已完成首个无 UI 执行闭环。

现有构建包含 Utils、Sdk、Nodes、Managers、Editor 五个项目静态库和 `VisionFlowStudio` 控制台入口。入口创建两个 Solid 节点，经 Add 到 Viewer 同步执行并验证输出图像；图形编辑界面尚未接入。已接入 spdlog、DSPatch 静态库和 QtNodes 静态库，Sdk 提供公共类型、属性与节点支持，Managers 管理节点注册和流程执行。

DSPatch 已按 FlowCV 定制版 7.0.1 补齐核心源码并作为静态库接入，当前闭环由 Managers 驱动同步执行。集成说明和历史验证见 [第三方依赖](third_party/README.md)。

## 目录导航

| 目录 | 职责 |
| --- | --- |
| `app/VisionFlowStudio` | 程序入口与模块装配，目前为无 UI 闭环验证程序 |
| `sdk` | 节点公共支持、公共类型和流程图模型；内部 thirdParty 提供 DSPatch |
| `managers` | 流程执行、调度、节点注册与插件加载的归属模块 |
| `editor` | 图编辑、参数面板、可视化和编辑状态的归属模块 |
| `nodes` | 内置 Solid、Add、Viewer 节点实现 |
| `plugins` | 动态插件实现的预留位置 |
| `third_party` | 项目级 spdlog、nlohmann_json 查找及 QtNodes 构建入口 |
| `Utils` | 运行时 C++ 公共辅助功能，目前包含 Logging |
| `tools` | 开发辅助脚本的预留位置 |
| `docs` | 架构说明 |

模块内部按功能聚合，功能目录直接位于模块下，同一功能的头文件和源文件放在一起，例如 `Utils/logging/logger.h` 与 `Utils/logging/logger.cpp`。Sdk 根目录提供 flowTypes、flowProperties 与 flowNode；Managers 根目录按流程、节点和插件组织管理器，Editor 仍保留占位文件。应用入口为 `app/VisionFlowStudio/main.cpp`。依赖方向、公开接口及内部头文件约定见 [架构说明](docs/architecture.md)。

Utils 的 CMake 负责创建公共辅助库和登记功能目录，各功能通过自己的 `CMakeLists.txt` 显式维护源码与链接依赖。例如，Logging 使用 `target_sources` 将文件加入 `VisionFlowUtils`，新增日志文件只需更新 Logging 的清单；新增辅助功能时再在 Utils 中增加 `add_subdirectory`，不扫描目录、不新增功能库。示例见 [架构说明](docs/architecture.md#功能级-cmake)。

构建脚本由顶层和各模块的 `CMakeLists.txt` 管理，属于项目源码；生成的缓存、目标文件和可执行程序统一存放于 `out/`。

## Windows 构建

需要 CMake 3.25 或更新版本、支持 C++20 的 MSVC 工具链，以及 Ninja。使用 Visual Studio 的 x64 开发者命令提示符或开发者 PowerShell，确保 `cl`、`cmake`、`ninja` 可用。

需要预先安装 x64 的 spdlog 和 nlohmann-json，并通过个人预设中的 `VCPKG_ROOT` 指定本机 vcpkg 根目录。共享预设通过该变量加载 vcpkg 工具链，显式关闭清单模式，不自动安装或升级依赖。本机使用 spdlog 1.17.0 和 nlohmann_json 3.12.0（均为 MIT），实际版本由本机安装状态决定，项目未锁定版本。DSPatch 使用按 FlowCV 定制版 7.0.1 整理的源码，QtNodes 使用固定提交的源码快照；详见 [第三方依赖](third_party/README.md)。

内置节点需要 OpenCV 的 `core` 组件；默认开启 Studio 时还需要 x64 的 Qt 6 MSVC SDK（Core、Gui、Widgets、OpenGL）。个人预设通过 `OpenCV_DIR` 和 `Qt6_ROOT` 指定位置，当前机器复用已有 Release OpenCV 和 Qt 6.11.1。QtNodes 随项目编译为静态库，不构建上游测试、示例和文档。

共享的 `CMakePresets.json` 仅保留隐藏的 `windows-base`，提供 Ninja、x64、Release、`x64-windows` triplet 和 SegmentHeap 参数，不作为可选构建入口。`CMakeUserPresets.json` 仅提供 `local-x64-release` 配置预设及同名构建预设，直接继承 `windows-base`；这是当前唯一可选配置。其他机器需自行创建该个人预设，并指定自己的 vcpkg、Qt 和 OpenCV 路径。SegmentHeap 配置依赖 Visual Studio 开发环境提供的 `VSINSTALLDIR`。

Visual Studio 开发者环境可能将 `VCPKG_ROOT` 改为其自带的 vcpkg，个人预设会为配置和构建设置本机路径。若此前选错工具链，可使用 `cmake --fresh --preset local-x64-release` 清除当前配置缓存。头文件搜索路径由 CMake 目标传播，无需手工添加 IDE 包含目录。

若 vcpkg 依赖尚未安装，可在设置好 `VCPKG_ROOT` 的终端中单独执行：

```powershell
& "$env:VCPKG_ROOT/vcpkg.exe" install spdlog:x64-windows nlohmann-json:x64-windows
```

在 Visual Studio x64 开发者 PowerShell 中配置、构建并运行当前无 UI 闭环：

```powershell
cmake --list-presets=all
cmake --preset local-x64-release
cmake --build --preset local-x64-release
& ./out/build/local-x64-release/app/VisionFlowStudio/VisionFlowStudio.exe
```

预设列表应只显示一个配置预设和一个构建预设，名称均为 `local-x64-release`。默认构建 Utils、Sdk、Nodes、Managers、Editor 五个项目静态库、DSPatch 和 QtNodes 静态库，以及 `VisionFlowStudio`。程序执行两个 Solid 节点经 Add 到 Viewer 的同步流程，检查 `640 × 480` 单通道图像的全部像素均为 `70`；成功时输出 `[PASS]` 并以退出码 `0` 结束。

在 Visual Studio 中修改预设后，关闭并重新打开项目文件夹，等待 CMake 配置和 IntelliSense 更新，选择 `local-x64-release` 与启动项 `VisionFlowStudio.exe`。“生成全部”只构建当前预设的目标，不会编译其他预设。错误列表中标为“错误(非活动)”且属于旧配置的条目可能是 IntelliSense 诊断；将来源切换为“生成”可单独查看真实构建错误。若旧诊断仍残留，关闭 VS 后重建项目内的 CMake 与 IntelliSense 缓存，并保留布局和用户设置。

`VISIONFLOW_BUILD_STUDIO` 默认开启；关闭后不查找 Qt，不进入 QtNodes、`editor` 和 `app`，仍需要 OpenCV，构建 `VisionFlowUtils`、`VisionFlowSdk`、`VisionFlowNodes`、`VisionFlowManagers` 及 DSPatch 静态库，不生成可执行程序。`plugins` 目前没有实际目标。

构建输出统一放入 `out/`；本机覆盖配置放入 `CMakeUserPresets.json`。两者及 `.vs/` 均由 Git 忽略，共享的 `CMakePresets.json` 保留在项目中。当前尚未实现 SDK 安装导出、测试框架、CI 或安装打包。

此次只收缩预设入口，未增加项目级 Debug 禁令，仍保留 OpenCV 的 Debug 库匹配检查。已有 Debug 输出目录保留，不会自动触发编译。
