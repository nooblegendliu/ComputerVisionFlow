# 项目架构

## 当前状态与组织原则

项目已接入 DSPatch、Types、Properties 与 Logging。Managers 的三个管理器仅有六个空占位文件，Editor 和应用仍处于骨架阶段。本文中的管理器、节点和图编辑职责表示后续功能归属，不表示已实现。

保留 `sdk / managers / editor / app` 顶层分层，模块内部按功能聚合，功能目录直接位于模块下，同一功能的头文件和源文件放在一起，不再维护 `include/` 与 `source/` 两棵目录树。例如，日志文件集中于 `Utils/logging/`，编入 `VisionFlowUtils`，不单独建立日志库。

Sdk 根目录提供 `flowTypes.h/.cpp` 和 `flowProperties.h/.cpp`；Managers 根目录提供 `flowcv_manager`、`node_manager`、`plugin_manager` 各自的空 `.h/.cpp`，不包含类声明或实现；Editor 保留 `VisionFlowEditor.h/.cpp` 占位文件；桌面入口为 `app/VisionFlowStudio/main.cpp`。后续实际功能出现时，再按职责创建功能目录，不提前建立空目录。

头文件是否公开由用途决定：面向其他模块使用的头文件属于公开接口，其余属于内部实现，后续接口开发时应明确标注用途。功能内部头文件可按需放入该功能的 `detail/` 目录；这是内部使用约定，外部模块不得依赖，目录布局本身不强制访问隔离。未来 SDK 导出应显式选择公开头文件。

当前 `Utils/logging/logger.h` 包含 `spdlog/spdlog.h`，`logger.cpp` 包含自身头文件。CMake 已接入本机 vcpkg 的 spdlog，已有 FlowCV::FlowLogger、日志宏、异步初始化和级别管理；应用尚未调用初始化。

## 模块职责

| 模块 | 职责 | 允许的项目内依赖 |
| --- | --- | --- |
| `sdk` | 节点公共支持、公共类型、参数和图模型；统一提供 DSPatch 和 JSON 依赖 | DSPatch、nlohmann_json；不依赖 Utils、spdlog、其他业务模块或 Qt |
| `managers` | 流程执行、调度、启停、节点类型注册、插件加载 | `sdk` |
| `editor` | 图编辑、参数面板、结果展示，以及节点位置、选中状态、窗口状态等编辑信息 | `sdk`、`managers` |
| `nodes` | 基于公开节点接口的内置功能实现 | `sdk`，以及实际需要的外部算法库 |
| `plugins` | 基于公开接口的动态扩展实现 | `sdk` 的公开接口，以及实际需要的外部库 |
| `app` | 程序入口、启动配置、节点注册与模块装配 | 当前链接 `editor`、`managers`；未来按应用需要装配节点模块 |
| `Utils` | 参与编译的公共 C++ 辅助功能，内部按 logging 等职责划分 | spdlog；不依赖 Sdk、Managers、DSPatch 或 Qt |
| `tools` | 开发脚本、生成器及离线处理工具 | 不承载应用运行时逻辑 |
| `third_party` | 项目级 spdlog、nlohmann_json 查找及 QtNodes 构建入口 | 不反向依赖业务模块 |

上层可以使用下层接口，下层不得反向包含上层头文件。尤其是 `managers` 不依赖编辑器或具体节点实现，而应通过 `sdk` 的公开接口工作。未来由应用选择节点实现并完成注册。

`Utils` 是运行时公共辅助功能容器，内部按 logging 等具体职责组织；`tools` 仅放开发脚本。`managers` 承接运行管理职责：流程协调、节点注册与插件加载归 `managers`，界面状态归 `editor`，应用装配归 `app`。确有跨模块复用需求时，再按具体职责提取模块。

## 功能归属与扩展规则

- 公共接口、端口与参数约定归 `sdk`；节点算法实现归 `nodes` 或具体插件，不进入公共模型。
- 流程图模型属于 `sdk`。流程序列化的初期实现可放入 `managers` 的专门文件；序列化逻辑不得依赖编辑器，编辑器布局等状态由 `editor` 单独管理。
- 插件实现归 `plugins`，插件发现、加载和生命周期管理归 `managers`。内置节点与动态插件共用公开接口；本次不提前设计插件 ABI 或版本协议。
- 节点计算与自定义界面分离。未来界面扩展作为可选部分组织，基础计算路径不要求 ImGui、窗口系统或编辑器。
- 后续无界面入口约定为 `app/VisionFlowRunner`，负责命令行输入和运行库装配。本次不创建该目录或目标；关闭桌面构建开关并不代表已提供流程运行器。
- SDK 未来由现有公开头文件和库目标导出，不单独复制一份核心源码或依赖。当前的 CMake 别名仅供构建树内使用，不代表已有可安装 SDK。
- 节点分类、生成器及模板目录随实现按需创建，不提前建立空算法目录。未来生成器模板放入 `tools/node_generator/templates/`。

## 构建边界

| 实际目标 | 构建树内别名 | 类型 | 当前直接项目依赖 |
| --- | --- | --- | --- |
| `VisionFlowUtils` | `VisionFlow::Utils` | 静态库 | `spdlog::spdlog`，`PUBLIC` |
| `VisionFlowSdk` | `VisionFlow::Sdk` | 静态库 | `VisionFlow::DSPatch`、`nlohmann_json::nlohmann_json`，`PUBLIC` |
| `VisionFlowManagers` | `VisionFlow::Managers` | 静态库 | `VisionFlow::Sdk`，`PRIVATE` |
| `VisionFlowEditor` | `VisionFlow::Editor` | 静态库 | `VisionFlow::Sdk`、`VisionFlow::Managers`，`PRIVATE` |
| `VisionFlowStudio` | 无 | 可执行程序 | `VisionFlow::Editor`、`VisionFlow::Managers`、`VisionFlow::Utils`，`PRIVATE` |

四个项目库均以各自模块根目录作为 `PUBLIC` 包含路径，桌面程序以自身目录作为 `PRIVATE` 包含路径。Sdk 公开头文件为 `<flowTypes.h>` 和 `<flowProperties.h>`，`<DSPatch.h>` 由 Sdk 提供，`<logging/logger.h>` 由 Utils 提供；不提供旧 Core 目标或头文件兼容层。跨模块使用头文件必须通过对应 CMake 目标依赖获取包含路径；不得将项目根目录加入全局搜索路径，也不得用全局链接目录绕过目标边界。

Sdk 作为节点开发支持入口，以 `PUBLIC` 方式提供 DSPatch 和 nlohmann_json 使用要求，不再提供日志或 spdlog；Managers 和 Editor 对 Sdk 保持私有链接。后续公共接口如暴露项目依赖类型，再调整对应链接可见性。各目标继续显式列出源码文件，不使用递归扫描。

顶层始终纳入 `third_party`、`Utils`、`sdk`、`managers`、`nodes` 和 `plugins`。`third_party` 在业务模块之前通过 `find_package(spdlog CONFIG REQUIRED GLOBAL)` 和 `find_package(nlohmann_json CONFIG REQUIRED GLOBAL)` 无条件查找依赖；`GLOBAL` 使导入目标能够被同级模块使用。`nodes`、`plugins` 当前只保留说明，不产生虚假的占位库目标。

DSPatch 使用按 FlowCV 定制版 7.0.1 整理的核心源码，不使用 fast_any。`sdk/thirdParty/dspatch` 创建静态库及别名 `VisionFlow::DSPatch`，显式登记 8 个实现文件和现有头文件，公开 `include` 使用要求，私有包含 `source` 并链接线程和平台动态加载依赖。目标使用 C++20，MSVC 私有启用 `/utf-8`，不依赖 Qt。Sdk 通过内部 `thirdParty` 入口登记 DSPatch，并 PUBLIC 链接该目标；Managers 仅私有链接 Sdk，经由 Sdk 获得 DSPatch 公共头文件和链接要求，不获得内部 source 路径。允许节点通过 Sdk 使用 DSPatch，不再限定 DSPatch 只能存在于 Managers 内部。此配置不引入 DSPatch DLL、测试、安装或运行调用。开启 Studio 时顶层先查找 Qt 6，使配置对所有消费者目录可见；`third_party` 再以隔离的局部构建选项引入 QtNodes 静态库，Editor 私有链接 `QtNodes::QtNodes`。关闭 Studio 后不查找 Qt、不构建 QtNodes。此接入只提供编译依赖，不改变公开接口或实现调度、图编辑等业务功能。

Utils 的 Logging 功能通过 `PUBLIC spdlog::spdlog` 链接编译版本的 spdlog，因为其公开头文件直接包含 spdlog。日志消费者必须显式链接 `VisionFlow::Utils` 才能获得日志头文件路径、编译定义和链接依赖；fmt 依赖由 spdlog 的导入目标传递。不手工添加依赖路径或库文件，Studio 私有链接 Utils，Managers、Editor、nodes 当前不使用日志，不提前添加依赖。

共享预设在 `project()` 初始化前通过 `VCPKG_ROOT` 加载 vcpkg 工具链，使用经典模式，x64/x86 分别选择 `x64-windows`/`x86-windows`。spdlog、nlohmann-json 和启用 Studio 时需要的 Qt 需预先安装，配置阶段不自动安装或升级；QtNodes 的源码提交已固定，DSPatch 的参考来源和接入状态详见第三方依赖说明。此前验证的 spdlog 为 1.17.0（MIT）；本机 x64 已安装 nlohmann_json 3.12.0（MIT），版本不锁定，本次未编译验证。保留 vcpkg 默认的运行时依赖复制行为。切换工具链后使用新的构建目录验证，并重新生成 IDE 的 CMake 缓存。

`VISIONFLOW_BUILD_STUDIO` 默认 `ON`，同时控制编辑器库与桌面入口；设置为 `OFF` 时跳过 `editor`、`app`。未来独立消费者出现后，再考虑进一步细化构建选项。

编译基线为 CMake 3.25、C++20，项目版本为 `0.0.1`。MSVC 热重载策略直接在顶层 `CMakeLists.txt` 的 `project()` 前设置，确保策略作用于工具链初始化。

保留的调试格式条件同时检查 C 和 C++ 编译器；当前项目仅启用 C++，因此 MSVC Debug 构建实际选择 `ProgramDatabase`（`-Zi`），不会选择 `EditAndContinue`（`-ZI`）。本次目录精简保持这一既有行为。

构建脚本由顶层和各模块的 `CMakeLists.txt` 管理，属于项目源码。生成的缓存、目标文件和可执行程序统一存放于 `out/`。

共享预设与输出路径保持原有约定。`out/`、`.vs/` 和个人预设不属于源代码；目录迁移后重新生成 CMake 配置，必要时使用 `--fresh` 刷新缓存，不清空整个 out。

## 开发辅助与验收

`tools` 存放开发脚本，当前仅用说明文件保留该目录。暂不设置正式示例和自动化测试目录，不引入测试框架、CI 或安装打包功能。依赖传播与运行时验证工程可放在 `out/` 中，不加入正式源码。

本次 Managers 迁移与 JSON 接入仅做静态审阅，未运行 CMake 配置、构建、应用或测试，也未删除已有构建缓存。静态核对六个管理器文件为空、旧目标引用已移除、JSON 通过 Sdk PUBLIC 传播、Studio 开关两条依赖路径完整。后续重新配置 CMake 以刷新 IDE 包含路径；x86 需预先安装对应依赖。此前配置和构建结果见第三方依赖说明的历史验证，不代表本次修改已编译通过。

## 功能级 CMake

模块创建目标并登记功能，功能负责源码清单与自身依赖。Utils 的模块级 CMake 创建静态库并设置公共包含路径，通过 `add_subdirectory(logging)` 加载 Logging。Logging 使用 `target_sources(VisionFlowUtils PRIVATE ...)` 追加源码，不创建单独的静态库或对象库。

后续增加 Graph 功能时，在 Sdk 的目标创建之后登记：

```cmake
add_subdirectory(graph)
```

在 `sdk/graph/CMakeLists.txt` 中显式列出该功能实际创建的文件：

```cmake
target_sources(VisionFlowSdk PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/graph.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/graph.cpp"
)
```

此处仅为扩展示例，不提前创建 Graph 目录。功能新增文件只修改自身清单；新增项目级依赖由 `third_party` 查找，Sdk 内部依赖由 `sdk/thirdParty` 登记，再在功能 CMake 中通过 `target_link_libraries` 声明。公开头文件需要依赖时使用 `PUBLIC`，仅实现需要时使用 `PRIVATE`。不使用 GLOB、递归扫描或自动发现子目录。
