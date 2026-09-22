# 第三方依赖

本目录负责 spdlog、nlohmann_json 查找与 QtNodes 构建；DSPatch 由 `sdk/thirdParty` 管理。本文统一记录项目依赖的来源与配置。spdlog 和 nlohmann_json 使用本机 vcpkg 经典模式；DSPatch 使用按 FlowCV 定制版 7.0.1 整理的头文件与实现源码，QtNodes 使用固定提交的源码快照。Qt 由顶层在启用 Studio 时查找，保证编辑器和应用所在目录均能使用 Qt 的 CMake 配置。配置过程不自动下载、安装或升级依赖，不提供 `vcpkg.json`。

| 依赖 | 来源 | 版本 | 许可证 | 使用目标 |
| --- | --- | --- | --- | --- |
| spdlog | vcpkg 官方端口，<https://github.com/gabime/spdlog> | 1.17.0 | MIT | `spdlog::spdlog` |
| nlohmann_json | vcpkg 官方端口，<https://github.com/nlohmann/json> | 本机 x64 已安装 3.12.0，未锁定版本 | MIT | `nlohmann_json::nlohmann_json` |
| DSPatch | FlowCV 中的定制版本，<https://github.com/FlowCV-org/FlowCV> | 参考 7.0.1，FlowCV 提交 `b09c48edcc9437253da6049365d319fe036eff61`；核心源码已补齐 | 对应源码保留原有 BSD-2-Clause 声明 | `VisionFlow::DSPatch`（静态库别名） |
| QtNodes | <https://github.com/paceholder/nodeeditor> | 3.0.16，`7c6341a66a8e46b8988140b9e60d892b6a3560b3` | BSD-3-Clause | `QtNodes::QtNodes` |
| Qt | 本机 Qt SDK | 6.11.1，MSVC 2022 x64 套件 | 以所安装 Qt SDK 的许可证为准 | `Qt6::Core/Gui/Widgets/OpenGL` |

spdlog、nlohmann_json 与 Qt 的实际版本由本机安装状态决定，QtNodes 的源码版本已固定，DSPatch 为本地整理版本，保留文件命名与注释调整，并非逐字一致的原始快照。共享预设通过 `VCPKG_ROOT` 找到工具链，不写入本机绝对路径。x86 构建需要对应架构的依赖；当前 Qt SDK 只支持 x64，此次不验证 x86 Studio。

`CMakeLists.txt` 使用 `find_package(spdlog CONFIG REQUIRED GLOBAL)`，使导入目标可被同级模块消费。Utils 的 Logging 功能以 `PUBLIC` 方式链接该目标，将公开头文件需要的编译与链接要求传播给消费者。spdlog 的包配置负责查找和传递 fmt，不手工添加 fmt 路径或库名。保留 vcpkg 默认的运行时依赖复制行为。

JSON 通过无条件的 `find_package(nlohmann_json CONFIG REQUIRED GLOBAL)` 查找，Studio OFF 同样需要它。Sdk 以 `PUBLIC` 链接 `nlohmann_json::nlohmann_json`，因为公开头文件 `flowProperties.h` 包含 `<nlohmann/json.hpp>`；下游无需手工添加包含路径。JSON 是头文件库，不新增二进制库构建，不使用 FetchContent 或另一份源码副本。本机 x64 已有 3.12.0；其他机器或 x86 环境需先安装对应依赖：

```powershell
& "$env:VCPKG_ROOT/vcpkg.exe" install nlohmann-json:x64-windows
# 仅在需要 x86 构建时安装：
& "$env:VCPKG_ROOT/vcpkg.exe" install nlohmann-json:x86-windows
```

## 源码快照与构建边界

`../sdk/thirdParty/dspatch/` 包含 FlowCV 定制版 7.0.1 的核心公共头文件、8 个实现文件及4 个内部头文件，不包含 fast_any。参考 FlowCV 提交为 `b09c48edcc9437253da6049365d319fe036eff61`。本地保留 `ComponentTypes.h` 文件名、`source` 目录和学习注释；已有版权与许可声明按对应原文件恢复。`DSPatchVersion.h` 固定声明 7.0.1 并保留原版条件 manifest 块，不额外生成文件，也不定义 `ADD_DSPATCH_DEPENDENCY`。`nodeeditor/` 继续保留上述固定提交的源码与许可证。

Sdk 内部第三方入口查找 Threads，并通过 `add_subdirectory(dspatch)` 引入静态库 `visionflow_dspatch`，别名为 `VisionFlow::DSPatch`。子目录显式登记源码与头文件，`include` 使用 `SYSTEM PUBLIC`，`source` 使用 `PRIVATE`。目标使用 C++20，私有链接 `Threads::Threads` 与 `${CMAKE_DL_LIBS}`；MSVC 私有启用 `/utf-8`，保持默认 Debug `/MDd`、Release `/MD`。VisionFlowSdk PUBLIC 链接该目标，消费者链接 VisionFlow::Sdk 即可获得 DSPatch 公共使用要求；Managers 私有链接 Sdk，不再重复直接链接 DSPatch，内部 source 路径不向消费者传播。DSPatch 不链接 Qt，不修改其原有导出宏、插件加载或调度逻辑，不执行上游的安装、测试、教程或 DLL 构建脚本。

QtNodes 仅在 `VISIONFLOW_BUILD_STUDIO=ON` 时作为子目录构建。局部变量作用域内设置 `USE_QT6=ON`、`BUILD_SHARED_LIBS=OFF`，关闭 `QT_NODES_DEVELOPER_DEFAULTS`、`BUILD_EXAMPLES`、`BUILD_TESTING`、`BUILD_DOCS` 和调试后缀。`CMAKE_POLICY_DEFAULT_CMP0077=NEW` 使上游选项遵循局部值，不通过全局缓存强制覆盖其他模块设置。上游先查找通用包名 `QT`，该查找不使用 `Qt6_ROOT`，因此局部设置 `QT_DIR=${Qt6_DIR}`，复用顶层已找到的 Qt。无需 Catch2；MOC/RCC 由 QtNodes 自身的 CMake 处理。Editor 私有链接 `QtNodes::QtNodes`。

关闭 Studio 后不查找 Qt、不进入 QtNodes，只构建基础业务库。QtNodes 静态链接不代表 Qt 静态链接；当前 Qt SDK 仍使用动态库，MSVC 运行库保持 Debug `/MDd`、Release `/MD`。

## 本机配置与使用

在 Visual Studio x64 开发者 PowerShell 中使用个人预设。当前被忽略的 `CMakeUserPresets.json` 提供 `local-x64-debug`、`local-x64-release`，分别继承共享的对应预设，指定本机 Qt 根目录和 vcpkg 环境变量；生成文件进入各自全新的 `out/build/local-x64-*` 目录。

```powershell
cmake --preset local-x64-debug
cmake --build --preset local-x64-debug
cmake --preset local-x64-release
cmake --build --preset local-x64-release
```

其他机器需自行建立个人预设：`Qt6_ROOT` 指向包含 `bin`、`include`、`lib` 的 Qt MSVC 套件根目录，`VCPKG_ROOT` 指向已安装 spdlog 和 nlohmann-json 的 vcpkg。不要在共享 CMake 中设置绝对路径或全局包含/链接目录。

当前正式程序仍为控制台占位入口，不创建 Qt 窗口。后续实际使用 Qt 界面时再配置 Qt DLL、插件部署；本次不增加应用打包逻辑。

## 当前变更（2026-09-20，Managers 迁移与 JSON 接入）

- Managers 替代 Engine 构建目标，三个管理器的六个文件仅为空占位，不包含业务实现。
- SDK PUBLIC 链接 nlohmann_json，头文件使用标准包含路径；配置阶段复用本机 vcpkg 包。
- 本次仅静态审阅，未执行配置、编译、应用或测试，未删除缓存。重新配置 CMake 后 IDE 才能读取更新的依赖路径。
- 下方历史结果保留原有模块名和验证结论，不代表当前修改已经验证。

## 历史验证（2026-09-18，Logging 迁移至 Utils）

- Logging 位于 `Utils/logging`，编入独立静态库 `VisionFlowUtils`，别名 `VisionFlow::Utils`；日志头文件与实现迁移前后 SHA256 一致。
- x64 Debug、Release 及既有 Studio OFF 构建目录均重新配置、编译通过，产物为 `Utils/VisionFlowUtils.lib`；每套规则仅编译一次 logger.cpp。
- Sdk 和 Engine 编译规则不再携带 spdlog 使用要求或 Utils 路径；Utils 获得 spdlog 和私有 `/utf-8`，Studio 通过私有链接 Utils 获得日志公共包含路径。Studio OFF 不生成 QtNodes、Editor 或 Studio。
- 本次不运行应用或日志初始化，不添加调用测试，不产生运行日志。配置和编译记录保存在各构建目录的 `utils-configure.log`、`utils-build.log`。
- 以下为迁移前的历史验证，依赖边界以本节及当前架构说明为准。

## 历史验证（2026-09-18，Logging 迁移前的 Sdk 结构）

- `local-x64-debug`、`local-x64-release` 重新配置与构建通过，生成 `sdk/VisionFlowSdk.lib` 和 `sdk/thirdParty/dspatch/visionflow_dspatch.lib`。
- 复用既有 `out/build/local-x64-core-only` 目录验证 Studio OFF（目录名保留历史名称）：Sdk、Engine、DSPatch 构建通过，不查找 Qt，不生成 QtNodes、Editor 或 Studio。
- 三套生成规则均登记 8 个 DSPatch 实现文件；Engine 经由 Sdk 获得 DSPatch 公共 include 路径，不获得内部 source 路径，生成规则不再引用旧 Core 目标或 DSPatch 源码路径。
- DSPatch 目录全部 22 个文件迁移前后 SHA256 一致。应用入口、个人预设和 QtNodes 未修改，未新增调用程序，未运行应用或测试。
- 本次配置和编译日志为上述构建目录中的 `sdk-configure.log`、`sdk-build.log`。以下记录属于迁移前验证，旧路径仅用于说明历史状态。

## 历史验证（2026-09-18，Sdk 迁移前静态库接入）

- `local-x64-debug`、`local-x64-release` 均配置和构建通过；8 个 DSPatch 实现文件全部参与编译，生成 `third_party/dspatch/visionflow_dspatch.lib`。
- `local-x64-core-only` 使用 Debug 个人预设并设置 `VISIONFLOW_BUILD_STUDIO=OFF`、`CMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE`，配置和构建通过；生成 DSPatch、Core、Engine，不生成 QtNodes、Editor 或 Studio。
- 核对生成的编译规则：DSPatch 使用公开 `include`、私有 `source`、C++20 和 `/utf-8`；Engine 不包含 DSPatch 的内部 `source` 路径。Debug/Release 保持 `/MDd`、`/MD`。
- 对照参考源码，8 个实现文件及4 个内部头文件的代码逻辑保持一致，差异限于注释、排版及等价的同目录包含方式。
- 本次只验证配置与编译，没有添加调用示例或探针工程，没有运行应用、串行数据传递、并行调度或插件加载测试。构建成功不代表这些运行行为已验证。
- 配置和构建日志保存在上述 `out/build` 目录的 `configure.log`、`build.log` 中。

## 历史验证（2026-09-17，旧版抄写阶段）

- 修正 `Signal::GetValue()` 的成员名笔误和现有 `Circuit::AddComponent()` 声明缺少分号的问题，未补写其他接口或实现源码。
- 临时程序包含 `DSPatch.h`，实际实例化 `Signal::GetValue<int>()`；MSVC C++20 Debug `/MDd` 与 Release `/MD` 仅编译检查均通过，没有执行 DSPatch 链接或运行验证。
- 现有 `local-x64-debug`、`local-x64-release` 预设配置与占位工程构建通过；本次不修改应用入口、QtNodes 或个人预设。
- 独立构建目录设置 `VISIONFLOW_BUILD_STUDIO=OFF`、`CMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE` 后 Core/Engine 构建通过，生成文件不包含 QtNodes、Editor 或 Studio 目标。首次沙箱内 ABI 探测停滞，终止后在沙箱外重新配置验证通过。
- 临时编译程序、检查脚本与日志位于 `out/validation/dspatch-minimal-fix/`。当时 DSPatch 尚缺实现源码，以上历史结果不代表当前静态库的运行验证。

## 历史验证（2026-09-16，新版快照）

以下结果属于此前的新版 DSPatch 快照及临时补丁，不适用于当前 FlowCV 旧版静态库。

- x64 Debug/Release 的主工程及 QtNodes 静态库构建通过，正式程序仍输出 `VisionFlowStudio`。
- 关闭 Studio 并禁用 Qt6 查找后，Core、Engine 构建通过，不生成 QtNodes、Editor 或 Studio 目标。
- **DSPatch 原始快照的实际 API 编译未通过**：MSVC 19.51 拒绝 `Component.h` 的 `std::atomic_flag flag = { true }`（C2665）；Qt 的 `UNICODE` 编译定义使 `Plugin.h` 的 `LoadLibrary(std::string::c_str())` 参数类型不匹配（C2664）。主工程中的占位 Engine 尚未包含该头文件，因此主工程构建成功不能替代 API 验证。
- 兼容性修正目前只在 `out/validation/dspatch-compat/` 临时副本中验证，当时的正式上游快照未修改。当前已切换为旧版静态库，不继续应用这些新版补丁。
- 临时修正使用标准原子标记初始化，并在普通/移动构造时恢复原有初始等待状态；Windows 插件加载显式使用 `LoadLibraryA`。该副本的 Debug/Release 串行数据传递、分发、缺失插件处理及 QtNodes 模型保存/加载已通过。额外的 Debug 并行探测出现停滞并被终止，未完成原因定位；这些结果不代表多线程调度或完整插件功能已验证。
- 临时验证工程及日志位于 `out/validation/dependency-smoke/`、`out/validation/dependency-logs/`。完整验证脚本显式清除临时头文件覆盖选项，避免将候选补丁结果误认为原始源码已通过。

上述历史临时验证文件及日志已随用户要求清空 `out`，所列路径仅作历史记录，本次不重新创建探针工程。

引入依赖时记录来源、验证版本、版本管理方式和许可证；修改过的外部源码应记录上游版本与本地补丁。不在 SDK 中复制另一份依赖实现。
