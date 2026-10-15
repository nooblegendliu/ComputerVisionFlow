# 动态插件

这里放置动态插件实现。[PluginManager](../managers/plugin_manager.cpp) 已实现 `.fp` 文件扫描、动态库加载、节点描述读取、实例创建和卸载；加载器属于 `managers`，底层使用 DSPatch 插件机制。

当前应用入口尚未调用 `LoadPlugins()`，[plugins/CMakeLists.txt](CMakeLists.txt) 没有插件实现目标。[编辑器的节点类型枚举](../editor/VisionFlowEditor.cpp) 只读取 `InternalNodeManager`，因此节点库和新增菜单当前仅提供内置节点。接通插件功能还需要应用加载路径、编辑器枚举、参数适配及实际运行验证。

[Manager 属性接口](../managers/flowcv_manager.cpp) 通过 `dynamic_pointer_cast<FlowCV::Node>` 访问属性。仅继承 `DSPatch::Component` 的原版插件不能直接通过这套接口获取或编辑参数；原版 ImGui 控件也不能直接用于当前 Qt 属性面板，需要适配或迁移。现有接口的保留不代表已验证原版 `.fp` 的二进制兼容性，兼容情况需用实际插件与匹配的构建环境核对。

新增插件时显式添加 CMake 目标和依赖，并验证节点实例与动态库的创建、销毁顺序。架构约定见 [架构说明](../docs/architecture.md)，实现状态与验证范围统一记录在 [开发进度](../docs/development-status.md)。
