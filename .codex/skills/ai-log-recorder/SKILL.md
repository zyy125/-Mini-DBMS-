---
name: ai-log-recorder
description: Use after each Mini DBMS development phase to append a clean, report-ready AI usage entry to docs/AI_USAGE_LOG.md, including skill, prompt summary, AI output, human review, verification results, Git commit, and remaining issues without mentioning private planning files.
---

# AI Log Recorder

中文说明：这是“AI 使用记录”skill。每个阶段结束后使用，用来把本阶段 AI 参与过程写入 `docs/AI_USAGE_LOG.md`，方便最后生成课程报告。

## Mission

Append accurate, report-ready AI usage records after every development phase.

中文任务：每阶段结束后留下可追溯记录，说明用了哪个 Agent/Skill、Prompt 是什么、AI 做了什么、人如何检查、测试结果如何。

## Required Context

Read if available:

- `docs/PROJECT_SPEC.md`
- `docs/AI_PROJECT_CONTEXT.md`
- current `docs/AI_USAGE_LOG.md`
- current Git status or commit hash
- stage output files and test results

中文需要先读：

- 课程需求。
- AI 项目上下文。
- 已有 AI 使用记录。
- 当前阶段改动、测试结果、Git 状态或 commit hash。

## Rules

- Do not mention private planning files.
- Do not claim tests passed unless test output or user confirmation says so.
- Do not invent model names, platform names, commit hashes, or test results.
- Prompt summary should be concise and report-safe.
- Record human review honestly, including unresolved issues.

中文规则：

- 不要提个人自用计划文件。
- 没有真实运行测试，不要写“测试通过”。
- 不要编造模型、平台、commit hash 或测试结果。
- Prompt 摘要要干净，适合写进报告。
- 人工审查和遗留问题要如实记录。

## Log Entry Template

Append this format to `docs/AI_USAGE_LOG.md`:

```markdown
## <日期> <Phase 名称>

Skill:
<skill-name>

平台/模型:
<如果未知，写“待补充”>

Prompt 摘要:
<报告可展示的 Prompt 摘要，不提个人自用计划文件>

AI 产出:
- <文件或设计/代码/测试产出>

人工审查:
- <人工检查点>
- <人工修改点，没有则写“无”>

验证结果:
- <构建命令和结果>
- <测试命令和结果>
- <no-STL 扫描结果>
- <ASan/UBSan 结果或未运行原因>

Git:
<commit hash；未提交则写“未提交，原因：...”>

遗留问题:
- <没有则写“无”>
```

## Acceptance

- `docs/AI_USAGE_LOG.md` exists.
- New entry has phase name and skill.
- Verification results are factual.
- Entry is suitable for course report.

中文验收标准：

- `docs/AI_USAGE_LOG.md` 存在。
- 新记录包含阶段名和 skill。
- 验证结果真实。
- 记录可以直接用于课程报告。
