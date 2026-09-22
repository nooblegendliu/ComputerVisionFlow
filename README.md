# ComputerVisionFlow

ComputerVisionFlow 是一个使用 C++20 和 CMake 的计算机视觉流程项目，目前处于工程骨架阶段。

现有构建包含 Utils、Sdk、Managers、Editor 四个项目静态库和 `VisionFlowStudio` 控制台占位入口，运行时输出 `VisionFlowStudio`。节点、调度器、图形编辑界面和插件加载功能尚未实现；已接入 spdlog、DSPatch 静态库和 QtNodes 静态库，已有 FlowCV::FlowLogger 日志封装，但应用尚未调用初始化，Sdk 已提供 Types 和 Properties；Managers 的三个管理器仅有六个空占位文件，尚无业务实现。

DSPatch 已按 FlowCV 定制版 7.0.1 补齐核心源码并作为静态库接入；该接入不增加项目业务调用，编译通过不代表串行执行、并行调度或插件加载已经验证。具体状态见 [第三方依赖](third_party/README.md)。

## 目录导航

| 目录 | 职责 |
| --- | --- |
| `app/VisionFlowStudio` | 桌面程序入口与模块装配，目前为控制台占位程序 |
| `sdk` | 节点公共支持、公共类型和流程图模型；内部 thirdParty 提供 DSPatch |
| `managers` | 流程执行、调度、节点注册与插件加载的归属模块 |
| `editor` | 图编辑、参数面板、可视化和编辑状态的归属模块 |
| `nodes` | 内置节点实现的预留位置 |
| `plugins` | 动态插件实现的预留位置 |
| `third_party` | 项目级 spdlog、nlohmann_json 查找及 QtNodes 构建入口 |
| `Utils` | 运行时 C++ 公共辅助功能，目前包含 Logging |
| `tools` | 开发辅助脚本的预留位置 |
| `docs` | 架构说明 |

模块内部按功能聚合，功能目录直接位于模块下，同一功能的头文件和源文件放在一起，例如 `Utils/logging/logger.h` 与 `Utils/logging/logger.cpp`。Sdk 根目录提供 flowTypes 与 flowProperties；Managers 根目录保留 flowcv_manager、node_manager、plugin_manager 各自的空 .h/.cpp，Editor 仍保留占位文件。桌面入口为 `app/VisionFlowStudio/main.cpp`。依赖方向、公开接口及内部头文件约定见 [架构说明](docs/architecture.md)。

Utils 的 CMake 负责创建公共辅助库和登记功能目录，各功能通过自己的 `CMakeLists.txt` 显式维护源码与链接依赖。例如，Logging 使用 `target_sources` 将文件加入 `VisionFlowUtils`，新增日志文件只需更新 Logging 的清单；新增辅助功能时再在 Utils 中增加 `add_subdirectory`，不扫描目录、不新增功能库。示例见 [架构说明](docs/architecture.md#功能级-cmake)。

构建脚本由顶层和各模块的 `CMakeLists.txt` 管理，属于项目源码；生成的缓存、目标文件和可执行程序统一存放于 `out/`。

## Windows 构建

需要 CMake 3.25 或更新版本、支持 C++20 的 MSVC 工具链，以及 Ninja。使用 Visual Studio 的 x64 开发者命令提示符或开发者 PowerShell，确保 `cl`、`cmake`、`ninja` 可用。

还需要将 `VCPKG_ROOT` 环境变量设置为本机 vcpkg 根目录，并预先安装对应架构的 spdlog 和 nlohmann-json。共享预设通过该变量加载 vcpkg 工具链，显式关闭清单模式，不自动安装或升级依赖。此前验证的 spdlog 为 1.17.0（MIT）；本机 x64 已安装 nlohmann_json 3.12.0（MIT），此次未编译验证。二者实际版本由本机安装状态决定，项目未锁定版本。DSPatch 使用按 FlowCV 定制版 7.0.1 整理的源码，QtNodes 使用固定提交的源码快照；详见 [第三方依赖](third_party/README.md)。

默认开启 Studio 时还需要匹配架构的 Qt 6 MSVC SDK（Core、Gui、Widgets、OpenGL）。通过个人预设或 `-DQt6_ROOT=<Qt套件根目录>` 指定位置。当前机器可直接使用 `cmake --preset local-x64-debug` 和 `cmake --build --preset local-x64-debug`，Release 对应 `local-x64-release`，复用已安装 Qt 6.11.1；无需重新安装 Qt。QtNodes 随项目编译为静态库，不构建上游测试、示例和文档。关闭 Studio 时不需要 Qt。

Visual Studio 开发者环境可能将 `VCPKG_ROOT` 改为其自带的 vcpkg。初始化开发者环境后，应确认 `$env:VCPKG_ROOT` 仍指向安装了依赖的那一份 vcpkg；必要时在当前终端重新设置该变量。若此前选错工具链，需要新建构建目录或使用 `cmake --fresh --preset x64-debug` 清除对应预设的配置缓存，避免继续使用其他软件环境中的同名库。

若对应依赖尚未安装，可单独执行以下命令；x86 构建不能使用 x64 库：

```powershell
& "$env:VCPKG_ROOT/vcpkg.exe" install spdlog:x64-windows nlohmann-json:x64-windows
# 仅在需要 x86 构建时安装：
& "$env:VCPKG_ROOT/vcpkg.exe" install spdlog:x86-windows nlohmann-json:x86-windows
```

共享预设保留 `x64-debug`、`x64-release`、`x86-debug`、`x86-release`。预设使用外部初始化的工具链环境；选择 x86 预设时也应切换到 x86 开发环境。预设中的 SegmentHeap 配置依赖 Visual Studio 开发环境提供的 `VSINSTALLDIR`。

x64 与 x86 预设分别选择 `x64-windows`、`x86-windows`，Release 继承对应架构设置。首次接入或更换工具链后，应使用新的构建目录，例如在配置命令中添加 `-B out/build/feature-cmake-vcpkg-debug`，并对该目录执行构建。已有 IDE 配置需要重新生成 CMake 缓存；若 IDE 尚未读取 `VCPKG_ROOT`，重新启动 IDE 使其继承环境变量。头文件搜索路径由 CMake 目标传播，无需手工添加 IDE 包含目录。

默认构建 Utils、Sdk、Managers、Editor 四个项目静态库及桌面占位程序（另有第三方静态库）：

```powershell
cmake --preset x64-debug
cmake --build out/build/x64-debug
& ./out/build/x64-debug/app/VisionFlowStudio/VisionFlowStudio.exe
```

关闭桌面程序，使用独立目录构建基础库：

```powershell
cmake --preset x64-debug -B out/build/x64-sdk-only -DVISIONFLOW_BUILD_STUDIO=OFF
cmake --build out/build/x64-sdk-only
```

`VISIONFLOW_BUILD_STUDIO` 默认开启；关闭后不查找 Qt，不进入 QtNodes、`editor` 和 `app`，构建 `VisionFlowUtils`、`VisionFlowSdk` 与 `VisionFlowManagers`，以及 DSPatch 静态库。`nodes`、`plugins` 目前没有实际目标；`third_party` 提供外部依赖目标，开启 Studio 时还构建 QtNodes 静态库，不生成额外业务库。

也可在上述开发环境中绕过共享预设直接配置：

```powershell
cmake -S . -B out/build/manual-x64-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=cl.exe "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_MANIFEST_MODE=OFF -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build out/build/manual-x64-debug
```

构建输出统一放入 `out/`；本机覆盖配置放入 `CMakeUserPresets.json`。两者及 `.vs/` 均由 Git 忽略，共享的 `CMakePresets.json` 保留在项目中。当前尚未实现 SDK 安装导出、测试框架、CI 或安装打包。

本次 Managers 迁移与 JSON 接入仅静态审阅，未运行配置、构建、应用或测试，未删除已有缓存。后续需重新配置 CMake，让 IDE 读取新的目标和包含路径；Properties 使用 `<nlohmann/json.hpp>`，包含路径由 Sdk 的 PUBLIC 依赖传播，无需手工设置 IntelliSense 路径。
