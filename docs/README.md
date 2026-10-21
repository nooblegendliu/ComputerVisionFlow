# ComputerVisionFlow 项目文档

更新日期：2026-10-21。项目规划、任务和构建说明统一在本目录维护；项目开发规则统一维护在根目录 [AGENTS.md](../AGENTS.md)。

ComputerVisionFlow 基于 FlowCV / DSPatch，使用 C++20、Qt / QtNodes 构建视觉流程编辑器。本套文档将 VisionMaster 案例差距分析转为可逐任务执行的工业视觉规划，不代表已经修改或完成了项目功能。

本文档集由原交接包整理而来，已替换旧计划、历史记录、示例工程及脚本。原交接目录已移除，后续按当前根目录规则和本目录中的计划、任务维护。

本文档是项目文档入口，仓库根目录不再保留 README。根目录 `AGENTS.md` 包含项目目标、技术边界、执行步骤、交付要求和高风险变更约束。构建说明见 [Windows 构建](build.md)。

## 阅读入口

1. 阅读根目录 [AGENTS.md](../AGENTS.md) 中的项目开发规则。
2. 开展规划或任务开发时，读取 [案例对标与能力差距](plans/visionmaster-gap-analysis.md) 和 [工业视觉演进路线](plans/industrial-vision-roadmap.md)。
3. 根据用户当前请求读取对应任务；实施前核对相关源码、保存格式和构建环境。

| 文档 | 用途 |
| --- | --- |
| [Windows 构建](build.md) | 环境要求、个人预设和构建命令 |
| [A1：节点实例命名](tasks/A1-node-rename.md) | 下一项建议实施任务，状态为 todo |
| [A2：Group / Comment](tasks/A2-group-and-comment.md) | A1 完成并验证后实施 |
| [A3：单相机抓边模板](tasks/A3-single-camera-template.md) | A1、A2 完成并验证后实施 |
| [启动提示](prompts/codex-start.md) | 用户决定启动 A1 时可使用的指令 |
| [参考图 1](references/visionmaster-case-01.jpg)、[参考图 2](references/visionmaster-case-02.jpg)、[参考图 3](references/visionmaster-case-03.jpg) | VisionMaster 案例证据 |

## 当前状态与维护

当前两份规划的文档状态为 proposed，A1–A3 的任务状态为 todo，验收项未勾选。文档整理不代表任务已实施或验收；实际实现须核对源码。路线图中的工期为估计，历史节点数量和候选源码路径需在实施时重新核实。

旧项目测试目录 `tests/`、`editor/Theme/tests/` 及其 CMake 入口已按用户要求移除。旧测试代码、夹具、示例和通过记录不作为当前验收依据；未来按具体任务的验收标准进行必要验证。

后续完成任务时，在对应任务文件记录实际验证、更新状态和验收项，再同步路线图。新增计划、任务和验证记录放在 `docs/`；项目开发规则只在根目录 `AGENTS.md` 维护，不另建重复规则文件。

本目录中的代码路径统一相对仓库根目录解释；Markdown 链接相对所在文档解释。

## 目录组织

| 目录 / 文件 | 用途 |
| --- | --- |
| `AGENTS.md` | 项目开发规则与代理阅读入口 |
| `docs/README.md`、`docs/build.md` | 文档索引与构建说明 |
| `docs/plans/`、`docs/tasks/`、`docs/prompts/`、`docs/references/` | 演进规划、任务验收、启动指令、参考图 |
| `Utils/`、`sdk/`、`nodes/`、`managers/` | 公共功能、DSPatch 节点、计算实现与运行图管理 |
| `editor/`、`app/VisionFlowStudio/` | 编辑器、主程序及 Viewer 界面 |
| `plugins/`、`tools/`、`Templates/` | 动态插件、生成器与模板 |
| `third_party/`、`sdk/thirdParty/` | 依赖配置与第三方源码 |
| `out/` | 本机构建和运行产物 |
