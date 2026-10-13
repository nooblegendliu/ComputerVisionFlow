# 项目架构

> 项目：ComputerVisionFlow（基于 FlowCV）  
> 本文对应仓库 `master` 分支提交 `4d9aa80b14d503a58c2e629d061d45d1b3edcfea`（2026-10-13）。  
> 本文区分**已实现**、**已接入但未完成应用功能**与**后续计划**。实际行为以当前源码为准。

## 1. 当前状态

ComputerVisionFlow 使用 C++20、CMake 和 FlowCV 的 DSPatch 数据流机制。项目已经由工程骨架发展到**第一个无 UI 运行闭环**：

1. `FlowCV::FlowCV_Manager` 创建两个 `Solid`、一个 `Add` 和一个 `Viewer` 节点实例；
2. 通过 `SetNodeProperty()` 将两个 `Solid` 配置为 640 × 480 的单通道图像，像素值分别为 30、40；
3. 使用 `ConnectNodes()` 建立 `Solid A → Add[0]`、`Solid B → Add[1]`、`Add → Viewer` 三条连接；
4. 设置 `SetBufferCount(0)`，调用 `Tick(DSPatch::Component::TickMode::Series)` 同步执行；
5. 通过 `Viewer::GetFrameCopy()` 获取输出，检查图像尺寸、类型及像素值均符合预期（70）。

这一闭环证明了上述**串行、单次执行路径**在已验证环境下可用，**不代表**自动 Tick、并行执行、插件加载、图形编辑或全部状态恢复场景已完成端到端验证。

| 功能 | 当前状态 |
| --- | --- |
| DSPatch 核心 | 已作为静态库接入，并完成串行 Tick 闭环 |
| SDK 公共类型、属性和节点基类 | 已实现，计算路径不依赖 ImGui/Qt |
| `FlowCV_Manager` | 已实现节点、连线、状态序列化和运行控制接口 |
| `InternalNodeManager` | 已实现注册与工厂创建；注册 `Add`、`Solid`、`Viewer` |
| `PluginManager` | 已有 `.fp` 扫描、加载和实例创建代码；尚未完成端到端插件运行验证 |
| 内置节点 | `Add`、`Solid`、`Viewer` 已移植并参与构建 |
| `VisionFlowStudio` | 当前为控制台闭环程序，不创建 Qt 主窗口 |
| Qt / QtNodes | 已接入构建；`VisionFlowEditor` 尚为占位实现 |
| 无界面独立程序 | `app/ProcessEngine` 有占位源文件，未加入可执行目标 |
| 测试、CI、SDK 安装、打包 | 尚未建立正式实现 |

## 2. 设计与移植原则

1. **优先保持 FlowCV 原有逻辑。** DSPatch 的 Circuit、Component、Tick/AutoTick、节点处理和原有状态格式应尽可能保留；不因架构美化而重写计算机制。
2. **仅在必要处解耦 UI。** 原库的 `HasGui()`、`UpdateGui()` 及 ImGui 控件逻辑不进入无界面计算节点；Qt 编辑器通过独立接口读取和修改节点属性、获取处理结果。
3. **修复具体问题，不预先重构。** 若发现非法索引、连接记录不一致、生命周期或资源管理等明确问题，可局部修正；不为尚不存在的需求建立新状态机、新调度器或第二套数据流引擎。
4. **当前实现优先于历史规划。** 文档应记录实际依赖和已实现功能；后续结构优化必须另行论证，不将其表述为已完成或必做事项。
5. **按模块组织，显式管理源码。** 同一功能的头/源文件尽量放在一起；CMake 明确列出源文件，不以递归扫描自动收集业务源码。

## 3. 目录与模块职责

```text
ComputerVisionFlow/
├── CMakeLists.txt
├── CMakePresets.json
├── Utils/
│   └── logging/                  # FlowLogger 与日志宏
├── sdk/
│   ├── flowTypes.h/.cpp           # 节点描述与公共类型
│   ├── flowProperties.h/.cpp      # UI 无关的属性存储及序列化
│   ├── flowNode.h                 # FlowCV::Node 节点基类
│   └── thirdParty/dspatch/        # FlowCV 使用的 DSPatch 7.0.1 定制源码
├── nodes/
│   ├── Add/
│   ├── Solid/
│   ├── Viewer/
│   └── internal_nodes.h          # 当前内置节点头文件入口
├── managers/
│   ├── flowcv_manager.h/.cpp      # 工作流实例、连接、状态与执行
│   ├── node_manager.h/.cpp        # 内置节点注册及工厂
│   └── plugin_manager.h/.cpp      # 动态插件管理
├── editor/
│   └── VisionFlowEditor.h/.cpp    # Qt 编辑器占位实现
├── app/
│   ├── VisionFlowStudio/         # 当前无 UI 闭环入口
│   └── ProcessEngine/            # 未接入构建的无界面入口占位源码
├── plugins/                      # 后续动态插件实现位置
├── third_party/                  # 外部包查找及 QtNodes 源码
├── tools/                        # 开发辅助工具预留目录
└── docs/                         # 项目文档
```

| 模块 | 当前职责 | 当前实际项目依赖 |
| --- | --- | --- |
| `Utils` | 日志封装与公共辅助功能 | spdlog |
| `sdk` | `FlowCV::Node`、`FlowCV_Properties`、公共类型 | DSPatch、nlohmann_json |
| `nodes` | 内置节点及 OpenCV 图像处理 | `sdk`、OpenCV core、nlohmann_json |
| `managers` | 工作流管理、内置节点注册、动态插件加载 | `sdk`、`nodes`、`Utils` |
| `editor` | 未来的图编辑、属性面板和结果显示 | `sdk`、`managers`、QtNodes（当前仅构建接入） |
| `app/VisionFlowStudio` | 程序入口与当前闭环运行验证 | `editor`、`managers`、`Utils`、`nodes` |
| `plugins` | 未来的外部插件实现 | 当前没有插件构建目标 |
| `third_party` | 查找 spdlog、JSON，条件构建 QtNodes | 外部依赖，不承载业务逻辑 |
| `tools` | 开发脚本、模板与离线工具的预留位置 | 不参与当前运行时 |

**依赖约定：** SDK 不反向依赖 Managers、Nodes、Editor 或 Qt；计算节点不依赖图形编辑器。当前 `InternalNodeManager` 在 `node_manager.cpp` 中直接注册具体节点，因此 **Managers 实际依赖 Nodes**；这是现行设计，不应为了符合旧文档而立即改写注册架构。`Utils` 不反向依赖业务模块。UI 相关对象仅应出现在 Editor/App 层。

头文件是否对外公开取决于其使用场景。当前各库以模块目录作为 CMake `PUBLIC` 包含目录；这不等同于已形成可安装、稳定 ABI 的 SDK。必要时可以在功能目录中使用 `detail/` 放置内部头文件，但暂不强制新增目录层级。

## 4. 核心运行结构

### 4.1 SDK 与属性系统

- `flowTypes.h` 定义 `NodeDescription` 和节点分类相关接口。`GuiInterfaceType` 目前仍保留在类型声明中，但不代表当前计算节点存在 ImGui 依赖。
- `flowNode.h` 提供 `FlowCV::Node`，继承 `DSPatch::Component`；节点可通过 `Properties()`、`GetPropertiesSnapshot()` 与 `RefreshProperties()` 维护可编辑参数，并可覆盖 `GetState()` / `SetState()` 保持原库状态格式。
- `flowProperties.h/.cpp` 支持 Bool、Int、Float、Option 属性，具有范围提示、选项、可见性、默认值、快照、修订版本和 JSON 读写功能。
- `Set()` 写入待生效的编辑值；处理路径通过 `Sync()` 更新用于计算的读取值。`GetW()` 读取编辑值，`Get()` 读取计算值。`Snapshot()` 获取一次一致的属性集合；修订版本是**单个属性实例内**的版本号。
- 数值范围和步长主要用于编辑提示，`Set()` 并不统一按范围裁剪；节点可在自身处理逻辑中校验或限制数值。
- `FlowCV_Manager::GetNodePropertiesSnapshot()` 和三个 `SetNodeProperty()` 重载向上层提供 UI 无关的属性访问。**目前没有**通用 String/FilePath 属性类型或 Qt 参数面板。

### 4.2 内置节点

目前 `nodes/internal_nodes.h` 包含三个节点的头文件，`InternalNodeManager` 构造时通过 `RegisterNode<T>()` 注册：

- **Solid**：生成固定尺寸、颜色或灰度值的图像，并保留原库的帧率控制、状态字段及主要算法行为；以属性系统代替原先的 ImGui 控件。
- **Add**：使用 OpenCV 完成两路图像加法，保留可选 Mask 输入和 Mask Mode 选项。
- **Viewer**：缓存处理结果，保留作为终端节点的原有职责；新增 `GetFrameCopy()`，通过互斥锁返回图像深拷贝，供无 UI 调用方读取。

各节点仍使用 DSPatch `Process_()` 与 SignalBus 完成计算和数据传输；不在节点内部创建 Qt 窗口。新增节点时首先对照 FlowCV 原库确认输入输出、算法、参数和序列化行为，再只对需要的 UI 依赖进行解耦。

### 4.3 工作流与管理器

`FlowCV_Manager` 继续作为运行图管理入口：

- **节点：** `CreateNewNodeInstance()`、`RemoveNodeInstance()`、`GetNodeInfoById()` 等；节点实例由 DSPatch Circuit 和 Manager 关联管理。
- **连线：** `ConnectNodes()`、`DisconnectNodes()`、`DisconnectNodeInput()` 及 Wire 查询/移除；实际信号连接由 DSPatch 完成，Manager 同步维护 `wiring_`、`input_conn_map`。
- **状态：** `GetState()`、`SetState()`、`NewState()`、`LoadState()`、`SaveState()`。JSON 主要包含 `nodes`、`connections`；节点自身参数由相应节点的 `GetState()` / `SetState()` 处理。
- **执行：** `SetBufferCount()`、`GetBufferCount()`、`Tick()`、`StartAutoTick()`、`StopAutoTick()`，直接使用 DSPatch Circuit 的既有能力。

原版 FlowCV 没有独立的 `Stopped/Running/Paused` 应用状态机；本项目**不新增**此类调度层。DSPatch 内部的 `PauseAutoTick()` / `ResumeAutoTick()` 用于其既有结构变更及同步路径，保留原有机制。

当前 `SetState()` 沿用原库的“先清空、再按节点和连接恢复”方式，不提供事务式整体回滚；未验证的错误路径不应被表述为已经保证原子恢复。UI 编辑器未来的节点位置、窗口布局等状态不属于现有 Manager 的运行图状态，应在实现编辑器时按需要保存。

### 4.4 插件与日志

`PluginManager` 目前保留 FlowCV 的 `.fp` 插件扫描、描述读取、工厂创建及卸载接口，底层使用 DSPatch 插件机制。当前 `plugins/` 还没有实际插件构建目标，也没有已验证的插件端到端流程；不能将“管理器代码已存在”等同于“插件生态已经可用”。实际使用时须注意插件实例与动态库句柄的生命周期先后关系。

`Utils/logging` 已实现 `FlowCV::FlowLogger` 与 `LOG_DEBUG/INFO/WARN/ERROR` 宏，使用 spdlog；相关管理器可调用日志宏。当前 `VisionFlowStudio` 闭环入口没有显式执行 `LOGGER_INIT`，因此不能将自定义异步日志器初始化描述为已在应用入口完成。

## 5. CMake 目标与依赖边界

| 目标 | 别名 | 类型 | `target_link_libraries` 当前依赖 |
| --- | --- | --- | --- |
| `VisionFlowUtils` | `VisionFlow::Utils` | STATIC | `PUBLIC spdlog::spdlog` |
| `visionflow_dspatch` | `VisionFlow::DSPatch` | STATIC | `PRIVATE Threads::Threads`、`${CMAKE_DL_LIBS}` |
| `VisionFlowSdk` | `VisionFlow::Sdk` | STATIC | `PUBLIC VisionFlow::DSPatch`、`nlohmann_json::nlohmann_json` |
| `VisionFlowNodes` | `VisionFlow::Nodes` | STATIC | `PUBLIC VisionFlow::Sdk`、`nlohmann_json::nlohmann_json`、`opencv_core` |
| `VisionFlowManagers` | `VisionFlow::Managers` | STATIC | `PUBLIC VisionFlow::Sdk`；`PRIVATE VisionFlow::Nodes`、`VisionFlow::Utils` |
| `VisionFlowEditor` | `VisionFlow::Editor` | STATIC | `PRIVATE VisionFlow::Sdk`、`VisionFlow::Managers`、`QtNodes::QtNodes` |
| `VisionFlowStudio` | 无 | EXECUTABLE | `PRIVATE VisionFlow::Editor`、`VisionFlow::Managers`、`VisionFlow::Utils`、`VisionFlow::Nodes` |

`QtNodes::QtNodes` 由 `third_party/nodeeditor` 构建；当前没有单独的 `VisionFlowPlugins` 目标，也未生成 `ProcessEngine` 可执行程序。

主要构建约定：

- 顶层先进入 `third_party`、`Utils`、`sdk`、`nodes`、`managers`、`plugins`；`VISIONFLOW_BUILD_STUDIO=ON` 时再进入 `editor` 和 `app`。
- 由于当前 `node_manager.cpp` 静态引用了内置节点，`nodes` 必须先定义目标，Managers 再链接 `VisionFlow::Nodes`。
- 外部包由合适的 CMake 入口查找；跨模块使用头文件与库的依赖通过 `target_include_directories()`、`target_link_libraries()` 显式传播，不使用项目根目录全局包含路径或全局链接路径。
- `spdlog` 和 `nlohmann_json` 在 `third_party/CMakeLists.txt` 中以 `find_package(... CONFIG REQUIRED GLOBAL)` 查找；OpenCV `core` 在 `nodes/CMakeLists.txt` 中查找；Qt 6 由顶层在启用 Studio 时查找。
- DSPatch 位于 `sdk/thirdParty/dspatch`，当前采用 FlowCV 定制版 7.0.1 的核心源码并编成静态库，不改写其调度与插件加载原理。QtNodes 使用固定源码快照，静态构建，不构建上游示例、测试及文档。
- `VISIONFLOW_BUILD_STUDIO=OFF` 时跳过 Qt 查找、QtNodes、Editor 和 Studio，但**仍会构建** Utils、Sdk、Nodes、Managers 和 DSPatch；仍需要 OpenCV，不自动生成无界面执行程序。
- Windows 下 `VisionFlowStudio` 构建后使用 `$<TARGET_RUNTIME_DLLS:VisionFlowStudio>` 复制可识别的运行时 DLL；这不等于已完成应用安装部署。

## 6. Windows 开发环境和构建配置

| 项目 | 当前约定 |
| --- | --- |
| 编译语言 | C++20（`CMAKE_CXX_STANDARD=20`） |
| CMake | 3.25 及以上 |
| 工具链 | Windows、MSVC、Ninja；当前以 x64 为主 |
| 默认配置 | Release、`x64-windows` vcpkg triplet |
| vcpkg | 非 Manifest 模式；通过 `VCPKG_ROOT` 指定工具链位置 |
| 必需包 | spdlog、nlohmann_json、OpenCV core；Studio ON 时还需 Qt 6 |
| QtNodes / DSPatch | 使用仓库内已固定或已整理的第三方源码 |
| 构建输出 | `out/build/<presetName>`，不提交到 Git |

共享 `CMakePresets.json` 当前只提供隐藏的 `windows-base` 配置基类及 Qt 相关隐藏预设；它们不是公开的可直接选用构建入口。机器相关的 `CMakeUserPresets.json` 不纳入 Git，按照 [README](../README.md) 的约定提供 `local-x64-release` 配置与同名构建预设，指定本机 vcpkg、Qt 和 OpenCV 路径。

常用命令（需要本机存在 `local-x64-release` 用户预设）：

```powershell
cmake --preset local-x64-release
cmake --build --preset local-x64-release
& ./out/build/local-x64-release/app/VisionFlowStudio/VisionFlowStudio.exe
```

当前共享预设选用 Release，适配现有的 Release OpenCV。工程**没有**添加全局 Debug 禁令；`nodes/CMakeLists.txt` 保留在误选 MSVC Debug 且 OpenCV 缺少对应 Debug 库时拒绝继续配置的检查。不得将不同 CRT/编译配置的库强行混用。顶层 `CMP0141` 和 `CMAKE_MSVC_DEBUG_INFORMATION_FORMAT` 保留已有条件配置，不因当前主用 Release 而强制重写。

生成目录 `out/`、IDE 缓存 `.vs/` 和本机配置 `CMakeUserPresets.json` 不属于源码。更换工具链或包位置后应重新配置 CMake；必要时使用 `--fresh` 更新当前构建树缓存，而不是直接修改生成文件。

第三方库版本、许可证和历史验证信息统一以 [第三方依赖](../third_party/README.md) 为准；此处不将特定开发机器的安装路径写入共享工程规则。

## 7. 功能级 CMake 组织

模块先创建库目标，功能子目录通过 `target_sources()` 注册自身文件。现有 Logging 是实际使用的例子：

```cmake
# Utils/CMakeLists.txt
add_library(VisionFlowUtils STATIC)
add_library(VisionFlow::Utils ALIAS VisionFlowUtils)
add_subdirectory(logging)
```

```cmake
# Utils/logging/CMakeLists.txt（节选）
target_sources(VisionFlowUtils PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/logger.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp"
)
target_link_libraries(VisionFlowUtils PUBLIC spdlog::spdlog)
```

`nodes`、`sdk`、`managers` 和 `editor` 目前主要在各自模块的 `CMakeLists.txt` 中显式维护源码清单，并**未**统一改成每个功能一个库或每个功能一个子目录。以后新增功能时按实际需要增设子目录；不提前创建空模块，不使用 `file(GLOB_RECURSE)` 替代显式清单。头文件对下游公开需要相应依赖时使用 `PUBLIC`，仅实现内部使用则优先 `PRIVATE`，以当前接口的实际需求为准。

## 8. 下一阶段边界（尚未实现）

当前下一阶段重点是实现独立于计算核心的 Qt 图形编辑器：节点显示、交互创建/连线、通用属性面板和 Viewer 图像展示。QtNodes 负责编辑交互；DSPatch 和 FlowCV Manager 继续负责运行图和计算，不因为更换 UI 而增加新的执行状态机或调度器。

后续可按实际需求逐步处理工程布局保存、更多内置节点、图像/视频插件、独立无 UI Runner、自动化测试、CI、SDK 安装与发布。这些均属于**待开发事项**，不应在当前状态表中标记为已完成。`app/ProcessEngine/headless_process_engine.cpp` 是已有的占位文件；实际 Runner 的命名和构建目标待功能开发时确认，不预先创建另一套空工程结构。

## 9. 文档维护与验证边界

- **已验证：** 现有 `VisionFlowStudio` 中 Solid + Add + Viewer 的同步运行闭环，依据项目现有入口与已提供的运行结果。
- **已有实现但需在对应阶段验证：** 状态文件完整往返、动态删改连线、自动 Tick / 并行处理、插件加载与卸载、GUI 集成。
- **尚未实现：** 可操作 Qt 编辑器、正式独立无界面程序、自动化测试框架、CI 和安装打包。

每次修改模块目标、第三方依赖、目录结构或功能完成状态时，应同步更新本文件和根目录 README。历史编译实验及早期架构假设保留在对应历史记录中，不再作为当前实现的事实依据。
