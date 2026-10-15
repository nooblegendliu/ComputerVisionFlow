# 第三方依赖

本文记录依赖来源、构建配置和运行时部署边界。项目入口见[README](../README.md)，模块职责见[架构说明](../docs/architecture.md)，当前能力与待开发事项见[开发状态](../docs/development-status.md)。

## 来源、版本与许可

| 依赖 | 来源与版本管理 | 许可证 |
| --- | --- | --- |
| spdlog | [上游](https://github.com/gabime/spdlog)，通过本机 vcpkg classic 安装；当前包配置声明 1.17.0，项目未锁定版本 | MIT |
| nlohmann_json | [上游](https://github.com/nlohmann/json)，通过本机 vcpkg classic 安装；当前包配置声明 3.12.0，项目未锁定版本 | MIT |
| OpenCV | [上游](https://github.com/opencv/opencv)，使用本机预先构建的 MSVC x64 包，要求 `core` 组件；当前包配置声明 4.14.0，项目未锁定版本 | 当前安装包主许可证为 Apache-2.0，随包第三方组件保留各自许可 |
| DSPatch | 从 [FlowCV](https://github.com/FlowCV-org/FlowCV) 定制版 7.0.1 整理，参考提交 `b09c48edcc9437253da6049365d319fe036eff61`；源码位于 `../sdk/thirdParty/dspatch/` | 对应文件保留 BSD-2-Clause 声明 |
| QtNodes | [nodeeditor](https://github.com/paceholder/nodeeditor) 3.0.16 源码快照，固定提交 `7c6341a66a8e46b8988140b9e60d892b6a3560b3`，位于 `nodeeditor/` | BSD-3-Clause，见[许可证](nodeeditor/LICENSE.rst) |
| Qt | 本机 Qt SDK，当前个人配置使用 Qt 6.11.1 MSVC 2022 x64 套件；项目查找 Qt 6，未锁定小版本 | 以所安装 SDK 及其模块的许可证为准 |

本机包版本是当前环境事实，不是其他机器的版本约束；以包配置文件和实际安装内容为准，不根据目录名称推断版本。CMake 配置不会自动下载、安装或升级依赖，仓库没有 `vcpkg.json`。引入或修改外部源码时，应保留来源、版本、许可及本地改动说明。

## 构建要求与依赖传播

当前共享配置以 Windows、MSVC x64、Ninja、Release 为基线，要求 CMake 3.25 及以上、C++20，以及兼容的 OpenCV 与 Qt 二进制包。在已初始化 MSVC x64 和 Visual Studio 环境变量的开发者 PowerShell 中配置；共享预设还使用 `VSINSTALLDIR` 定位 Visual Studio 的 SegmentHeap CMake 脚本。

`third_party/CMakeLists.txt` 无条件查找 `spdlog` 和 `nlohmann_json`，均使用 `CONFIG REQUIRED GLOBAL`，让同级模块能访问导入目标。`VisionFlow::Utils` 的日志公开头文件使用 spdlog，因此以 `PUBLIC` 链接 `spdlog::spdlog`；fmt 由 spdlog 包配置传递，无需手写包含路径或库名。`VisionFlow::Sdk` 以 `PUBLIC` 链接 `VisionFlow::DSPatch` 和 `nlohmann_json::nlohmann_json`，后者供 `flowProperties.h` 等公开头文件使用。

`nodes/CMakeLists.txt` 无条件执行 `find_package(OpenCV CONFIG REQUIRED COMPONENTS core)`，节点库公开链接 `opencv_core`。`VisionFlow::Managers` 公开链接 Sdk，私有链接 Nodes 和 Utils。`VisionFlow::Editor` 公开链接 Managers、`QtNodes::QtNodes` 和 `Qt6::Widgets`，私有链接 Sdk 和 `Qt6::Core`；其公开头文件涉及的 QtNodes、Qt 与 Manager 使用要求由 CMake 传播。完整目标关系以[架构说明](../docs/architecture.md)为准。

启用 `VISIONFLOW_BUILD_STUDIO`（默认 `ON`）时，顶层查找 Qt 6 的 `Core`、`Gui`、`Widgets`、`OpenGL`，构建 QtNodes、Editor 和 Qt Widgets 程序 VisionFlowStudio。关闭后不查找 Qt、不进入 QtNodes、不生成 Editor 或 Studio，只构建库；spdlog、JSON、OpenCV 和 DSPatch 仍是库构建所需依赖。

Debug 未被全局禁止。MSVC Debug 使用 `/MDd`，Release 使用 `/MD`；节点 CMake 会检查 OpenCV 的 Debug 导入库，缺少时明确报错，不将 Release OpenCV 静默用于 Debug。当前个人预设选择 Release，以匹配现有运行库和部署设置。

## 源码依赖边界

DSPatch 由 `sdk/thirdParty/CMakeLists.txt` 查找 Threads 并引入，构建静态库 `visionflow_dspatch`，别名为 `VisionFlow::DSPatch`。其 CMake 显式登记 8 个实现文件及公共、内部头文件，公共 `include` 使用 `SYSTEM PUBLIC`，内部 `source` 使用 `PRIVATE`，私有链接 `Threads::Threads` 和 `${CMAKE_DL_LIBS}`；MSVC 私有启用 `/utf-8`。DSPatch 不依赖 Qt，也不执行上游安装、测试、教程或 DLL 构建脚本。

本地 DSPatch 保留 `ComponentTypes.h` 文件名、`source` 目录和学习注释，不是逐字一致的原始快照；不包含 fast_any。`DSPatchVersion.h` 声明 7.0.1，保留原有条件 manifest 块，不额外生成版本文件，不定义 `ADD_DSPATCH_DEPENDENCY`。内部目录不会经 Sdk 或 Managers 向消费者传播。

QtNodes 仅在 Studio 开启时构建。在局部变量作用域内启用 `USE_QT6`、关闭 `BUILD_SHARED_LIBS`，并关闭开发者默认选项、示例、测试、文档和调试后缀；`CMAKE_POLICY_DEFAULT_CMP0077=NEW` 让上游选项遵循这些局部设置。其通用 `QT` 包查找通过 `QT_DIR=${Qt6_DIR}` 复用顶层找到的 Qt。当前不需要 Catch2，MOC/RCC 由 QtNodes 自身的 CMake 处理，Editor 单独启用 AUTOMOC。QtNodes 静态构建不等于 Qt 静态链接，当前 Qt SDK 使用动态库。

## 共享预设与个人配置

跟踪的 `CMakePresets.json` 包含两个隐藏预设：`windows-base` 提供 Windows x64 Release、Ninja、`x64-windows` triplet 和 vcpkg classic 工具链；`Qt` 通过环境变量 `QTDIR` 设置 `CMAKE_PREFIX_PATH`，属于 Qt 工具生成的辅助配置。这两个隐藏项不能直接作为配置入口，也不写入本机安装目录。

被 Git 忽略的 `CMakeUserPresets.json` 保存个人入口。当前唯一可直接使用的配置预设和构建预设均名为 `local-x64-release`；配置同时继承 `windows-base` 与 `Qt-Default`，后者经 `6.11.1_msvc2022_64` 继承共享的 `Qt`。`Qt-Default` 和具体套件项均为隐藏的本机 Qt 辅助项，套件路径与 `VCPKG_ROOT`、`Qt6_ROOT`、`OpenCV_DIR` 留在个人配置中，不应固化到共享 CMake。生成目录为 `out/build/local-x64-release`。

其他机器可自行创建最小个人预设，替换以下占位路径；示例仅展示所需字段，不修改任何配置文件，也不要求复制本机 Qt 辅助项：

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "local-x64-release",
      "inherits": "windows-base",
      "environment": {
        "VCPKG_ROOT": "C:/path/to/vcpkg"
      },
      "cacheVariables": {
        "Qt6_ROOT": "C:/path/to/Qt/msvc2022_64",
        "OpenCV_DIR": "C:/path/to/OpenCV/cmake-package"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "local-x64-release",
      "configurePreset": "local-x64-release"
    }
  ]
}
```

`Qt6_ROOT` 指向 Qt MSVC x64 套件根目录，`OpenCV_DIR` 指向包含 `OpenCVConfig.cmake` 的包目录，`VCPKG_ROOT` 指向已安装所需端口的 vcpkg。先准备 OpenCV、Qt 和 vcpkg；需要安装 vcpkg 端口时，先在当前 PowerShell 设置 `VCPKG_ROOT`，再执行：

```powershell
& "$env:VCPKG_ROOT/vcpkg.exe" install spdlog:x64-windows nlohmann-json:x64-windows
```

完成个人配置后使用：

```powershell
cmake --list-presets=all
cmake --preset local-x64-release
cmake --build --preset local-x64-release
```

## Windows 运行时部署

Studio 的 Windows 构建后步骤先通过 `cmake -E copy_if_different` 复制 `$<TARGET_RUNTIME_DLLS:VisionFlowStudio>` 到可执行文件目录，再调用所用 Qt SDK 中的 `windeployqt`。该工具通过 `find_program(... REQUIRED)` 查找，参数固定为 `--release --no-translations --no-compiler-runtime`，用于部署 Qt 库和包括 `platforms/qwindows.dll` 在内的插件。缺少 windeployqt 会使配置失败。

当前没有面向 VisionFlowStudio 的安装包或应用安装规则；构建后复制 DLL 不等同于完整安装分发。QtNodes 上游自带的安装声明不承担 Studio 打包。虽然项目允许满足依赖条件的 Debug 构建，windeployqt 仍固定部署 Release，因此 Debug 的部署尚未随构建配置切换。历史临时探针与旧构建目录的结果不作为当前运行验证；最新能力与验证范围统一记录在[开发状态](../docs/development-status.md)。
