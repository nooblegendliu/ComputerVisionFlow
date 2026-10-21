# 可直接复制到 Codex / Claude Code 的第一轮指令

下面这段是**仅执行 A1** 的开始指令；不要一次让代理完成整个路线图。

```text
你正在维护仓库 ComputerVisionFlow。

请首先阅读：
1. 根目录 AGENTS.md（项目开发规则）
2. docs/README.md（文档索引）
3. docs/plans/visionmaster-gap-analysis.md
4. docs/plans/industrial-vision-roadmap.md
5. docs/tasks/A1-node-rename.md

本轮仅执行 A1：节点实例自定义显示名称。

硬性约束：
- 必须先检查当前仓库最新源码与保存恢复实现，不要仅依据任务文档猜测代码；
- 除非确有必要，不修改 FlowCV / DSPatch 内核和任何第三方库；
- 不改变节点类型、端口、实例 ID、连线或运行行为；
- 只对 UI 显示名称和持久化做最小修改；
- 保持旧 .flow 项目兼容，利用现有 editor 状态进行保存恢复；
- 不引入与本任务无关的测试工程、插件或重构；
- 遵守根目录 AGENTS.md；如果同类功能或代码组织与任务文档不同，以当前源码事实为准并说明差异。

执行过程：
① 检索并阅读 Caption、节点右键菜单、工程保存和加载相关代码；
② 简要列出计划修改的文件、方法、数据结构和兼容性问题；
③ 实现 A1；
④ 在具备的环境中按现有构建方式验证，不能执行的验证请说明原因；
⑤ 列出每个文件的具体修改、验收情况与剩余风险。

只完成 A1，不要继续执行 A2/A3/B-F。不要把后续规划误当作本次实施范围。
```

## 推荐后续提问

A1 完成、人工验收后：

```text
A1 已完成。请按 AGENTS.md 的规则阅读 docs/tasks/A2-group-and-comment.md，
复查现有 QtNodes 场景/视图扩展方式，只实施 A2。
未经确认不要修改第三方 QtNodes 或 DSPatch 代码。
```

A2 完成后：

```text
A2 已验收。请按根目录 AGENTS.md 的规则阅读 docs/tasks/A3-single-camera-template.md，
基于当前真实节点端口建立可加载的单相机抓边示例工程。
无法运行的功能用 TODO 视觉标记表示，不允许制造伪造的检测结果。
```
