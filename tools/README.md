# 开发工具

这里放置代码生成、资源处理等开发辅助脚本，不放应用运行时逻辑。

## PluginMaker

`PluginMaker.py` 沿用 FlowCV 原工具的“模板生成 + 显式注册”流程，适配当前 SDK 与纯 Qt 界面。使用 Python 3.10 或更新版本，无第三方 Python 依赖。类名、节点名、CMake 目标名均采用 `--name` 的值；名称必须是以字母开头的非保留 C++ 标识符。

```powershell
python -B tools/PluginMaker.py --kind plugin --name MyFilter
python -B tools/PluginMaker.py --kind qt-plugin --name MyGuiFilter
python -B tools/PluginMaker.py --kind internal --name MyInternalFilter
```

三种模板均提供一入一出的图像增益最小示例（默认增益 1，等同透传），禁用时透传输入，不改变上游图像。生成器拒绝覆盖已有目录，拒绝大小写冲突的节点名称，并检查输出及注册文件不能通过符号链接或目录联接逃离项目根目录。

| 模板 | 输出目录 | 参数和界面 | 构建依赖 |
| --- | --- | --- | --- |
| `plugin` | `plugins/<Name>` | `properties_`、通用属性面板、基类状态序列化 | SDK、OpenCV core，无 Qt |
| `qt-plugin` | `plugins/<Name>` | 自定义 Controls、百分比增益和复位按钮、自定义状态序列化 | 可选 QtNodeUi 适配层、OpenCV core |
| `internal` | `nodes/<Name>` | `properties_`、通用属性面板、基类状态序列化 | 已有 VisionFlowNodes 依赖，无 Qt |

普通和 Qt 插件保留 `EXPORT_PLUGIN` 导出的 `Create` 和 `.fp` 格式。内部节点不导出动态入口。Qt 模板在 `VISIONFLOW_BUILD_STUDIO=OFF` 时跳过；不会使基础 SDK 或其它计算节点依赖 Qt。

### 注册与构建

默认仅生成文件，不修改现有清单。生成时添加 `--register`，会为插件追加 `plugins/CMakeLists.txt` 的 `add_subdirectory`；内部节点还会追加 `nodes/CMakeLists.txt`、`nodes/internal_nodes.h` 和 `managers/node_manager.cpp` 的显式注册。所有注册位置先检查，再写入；写入失败时恢复本次变动。`--root <path>` 可以指定另一份完整项目。

```powershell
python -B tools/PluginMaker.py --kind plugin --name MyNewFilter --register
cmake --preset local-x64-release
cmake --build --preset local-x64-release --parallel 4
```

若已使用默认模式生成并审查文件，请手工注册（生成器不会再次打开或覆盖目录）：插件在 `plugins/CMakeLists.txt` 添加 `add_subdirectory(MyFilter)`；内部节点在 `nodes/CMakeLists.txt` 添加 `add_subdirectory(MyInternalFilter)`，在 `nodes/internal_nodes.h` 包含 `MyInternalFilter/MyInternalFilter.hpp`，并在 `InternalNodeManager` 构造函数现有注册旁添加 `RegisterNode<DSPatch::DSPatchables::MyInternalFilter>();`。

模板调用项目的 `visionflow_register_plugin`（如存在），由 Studio 构建统一部署到 `Plugins`。独立集成模板时，可把生成的 `.fp` 与所需运行库复制到 Studio 的 `Plugins`，或在 Settings 添加插件所在目录后重启。插件必须使用与宿主匹配的 SDK、Qt（仅 Qt 插件）、OpenCV、编译器与运行库重新构建，不兼容原版 ImGui 插件的二进制接口。

### 状态与线程

普通及内部模板在 `Process_` 开头调用 `properties_.Sync()`，只从处理快照读参数；参数编辑使用 `Properties().Set`，由基类 `GetState/SetState` 保存恢复。请为实际算法添加与其参数对应的范围及状态验证，勿在处理线程直接读取控件。

Qt 模板只提供 Controls，`hasNodeUi(Main)` 返回 false。需要主界面时，通过同一 `NodeUiProvider` 创建 Main QWidget；宿主负责 Dock。所有 QWidget 和 QTimer 都在 GUI 线程创建、使用和销毁。示例状态通过原子变量共享；回调捕获 `weak_ptr`，不会捕获插件裸 `this`。状态保存仍走 `GetState/SetState`。节点删除、换工程或退出时，宿主必须先释放界面及其中的回调，再释放节点和动态库；仅有 `weak_ptr` 不能使已卸载 DLL 内的回调代码继续有效。

### 检查

旧测试脚本已移除。修改模板后，按当前任务验证生成产物的构建、加载、参数保存恢复及界面生命周期，记录实际结果。

参与程序编译的公共 C++ 辅助功能位于 [Utils](../Utils/CMakeLists.txt)，例如 [日志模块](../Utils/logging/logger.h)。项目计划统一从 [docs](../docs/README.md) 读取。
