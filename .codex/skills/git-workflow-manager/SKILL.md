---
name: git-workflow-manager
description: Use when managing Git workflow for the Mini DBMS project, including checking status, reviewing diffs, staging related changes, writing commit messages, creating progress summaries, and preparing push instructions without reverting unrelated user changes.
---

# Git Workflow Manager

中文说明：这是“Git 工作流管理”skill。用于阶段完成后的改动检查、提交拆分、commit message、push 前总结和开发记录整理。

## Mission

Keep Git history readable and aligned with course development phases.

中文任务：让每次提交都对应一个清晰阶段或功能，方便回滚、展示开发过程、写课程报告。

## Safety Rules

- Never run `git reset --hard`.
- Never run `git checkout -- <file>` unless the user explicitly asks.
- Do not revert files you did not change.
- Always inspect `git status` before staging.
- Stage related files together; avoid one giant commit unless it is only planning/docs.
- Do not push unless the user asks.

中文安全规则：

- 不要使用 `git reset --hard`。
- 不要随便 checkout 覆盖文件。
- 不要回退用户或其他 Agent 的改动。
- 提交前必须先看 `git status` 和 diff。
- 尽量按阶段拆 commit。
- 没有用户明确要求时，不要 push。

## Workflow

1. Run `git status --short`.
2. Review changed files with `git diff -- <path>` or `git diff --stat`.
3. Identify which changes belong together.
4. Recommend commit grouping.
5. Stage only the intended files.
6. Write a concise commit message.
7. After commit, show commit hash and summary.
8. If asked to push, run or suggest `git push` only after confirming branch/remote.

中文工作流程：

1. 查看当前状态。
2. 查看 diff。
3. 判断哪些文件属于同一个阶段。
4. 建议如何拆提交。
5. 只暂存本次要提交的文件。
6. 写清晰 commit message。
7. 提交后总结 hash 和改动。
8. 用户要求 push 时，再检查分支和远端后推送。

## Useful Commands

```bash
git status --short
git branch --show-current
git remote -v
git diff --stat
git diff -- <path>
git add <path>
git commit -m "docs: add vibe coding execution plan"
git log --oneline -5
git push
```

## Commit Message Style

Prefer:

- `docs: add AI project context`
- `build: add CMake project skeleton`
- `common: implement dynamic array`
- `sql: add lexer and parser`
- `storage: add file-backed table storage`
- `index: implement B+ tree primary index`
- `executor: wire parser storage and index`
- `network: add TCP client and server`
- `test: add storage persistence tests`
- `report: draft course report`

中文提交风格：

- 前缀说明类型或模块。
- 标题短一点。
- 一个 commit 尽量只做一类事。

## Progress Summary Template

Use this after a phase:

```text
阶段：<Phase>
本次提交：<hash>
主要改动：
- <改动 1>
- <改动 2>

验证：
- <构建/测试命令>
- <结果>

遗留问题：
- <没有则写“无”>
```

## Acceptance

- Worktree status is understood before commit.
- Commit contains only related files.
- Commit message is clear.
- Push happens only when requested.
- Summary can be copied into `docs/AI_USAGE_LOG.md` or course progress notes.

中文验收标准：

- 提交前清楚知道改了什么。
- 一个 commit 文件范围合理。
- commit message 清晰。
- 不误推送。
- 总结能用于课程报告或 AI 使用记录。

