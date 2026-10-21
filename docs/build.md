# Windows 构建

以下命令均在仓库根目录执行。返回 [项目文档入口](README.md)。

准备 CMake 3.25 及以上、MSVC x64 开发者环境、Ninja、Qt 6.8 及以上与匹配的 OpenCV。依赖与个人预设示例见 [第三方依赖](../third_party/README.md)。`CMakeUserPresets.json` 保存本机路径，不提交到仓库。

在 Visual Studio x64 Developer PowerShell 中执行；以下以已创建的个人预设 `local-x64-release` 为例：

```powershell
cmake --list-presets=all
cmake --preset local-x64-release -DVISIONFLOW_BUILD_STUDIO=ON
cmake --build --preset local-x64-release --parallel 4
& ./out/build/local-x64-release/app/VisionFlowStudio/VisionFlowStudio.exe
```

`VISIONFLOW_BUILD_STUDIO` 默认 ON；关闭后构建计算库及普通插件，不生成独立无界面执行程序。旧项目测试和对应构建入口已移除；后续验证按 `docs/tasks/` 中的具体任务组织。
