---
name: mini-dbms-test-report-recorder
description: Use after each Mini DBMS development phase to turn real build, test, sanitizer, no-STL scan, performance, and error-test results into report-ready entries in docs/TEST_LOG.md and, when useful, docs/AI_USAGE_LOG.md. This skill records testing evidence; it must not invent passing results.
---

# Mini DBMS Test Report Recorder

中文说明：这是“阶段测试记录整理”skill。每个开发阶段完成测试后使用，把真实运行过的测试结果整理到 `docs/TEST_LOG.md`，方便最终汇总成课程报告第五章“系统测试”。

## Mission

Keep a factual, phase-by-phase testing record for the Mini DBMS course report.

中文任务：按阶段记录测试用例、测试结果、性能测试、错误测试和 AI 辅助测试说明；最终报告第五章从 `docs/TEST_LOG.md` 汇总，而不是每阶段重复写完整章节。

## Required Context

Read if available:

- `docs/PROJECT_SPEC.md`
- `docs/AI_PROJECT_CONTEXT.md`
- `docs/TEST_LOG.md`
- `docs/AI_USAGE_LOG.md`
- Current stage files under `include/`, `src/`, `tests/`, `scripts/`
- Current Git status
- Actual command output from build/test/no-STL/ASan/performance runs

中文需要先读：

- 课程需求和项目上下文。
- 已有测试记录和 AI 使用记录。
- 当前阶段相关源码、测试和脚本。
- 当前 Git 状态。
- 真实命令输出或用户明确提供的测试结果。

## Core Rules

- Do not invent test cases that were not designed, run, or explicitly planned.
- Do not mark a test as passed unless there is command output or user confirmation.
- If a command was not run, write `未运行` and explain why.
- If a result is unknown, write `待验证`.
- Performance data must include the actual command, dataset size, environment notes if known, and measured result. Without measurement, record it as `计划测试项`, not a conclusion.
- Error tests must include invalid input and expected error behavior, not only success paths.
- Keep report wording clean and course-ready; do not mention private planning files.
- Do not run destructive commands or modify production code while recording tests.

中文硬规则：

- 不编造测试结果。
- 没有真实运行或用户确认，不能写“通过”。
- 没运行就写“未运行”和原因。
- 性能测试没有实际计时数据时，只能写计划，不能写性能结论。
- 错误测试必须记录非法输入和预期错误行为。
- 记录要能直接用于课程报告。

## Workflow

1. Identify the current phase and changed modules.
2. Read existing tests and relevant docs.
3. Collect actual verification commands and outputs.
4. Classify tests:
   - Unit tests
   - Integration tests
   - Error tests
   - Performance tests
   - No-STL scan
   - ASan/UBSan run
5. Append a phase entry to `docs/TEST_LOG.md`.
6. If AI helped design or summarize tests, append or update `docs/AI_USAGE_LOG.md` with a concise note.
7. Report missing tests and next validation commands.

中文流程：

1. 确认当前阶段和改动模块。
2. 阅读已有测试和相关文档。
3. 收集实际执行过的验证命令和输出。
4. 把测试分类为单元测试、集成测试、错误测试、性能测试、no-STL 扫描、ASan/UBSan。
5. 追加到 `docs/TEST_LOG.md`。
6. 如果 AI 参与了测试设计或整理，同步补充 `docs/AI_USAGE_LOG.md`。
7. 最后列出缺失测试和下一步验证命令。

## Required Verification Commands

Use the commands that match the current stage. If the project is not ready for a command, record why it was skipped.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
rg "std::vector|std::map|std::set|std::unordered|std::list|std::deque|std::array|std::forward_list|std::span|std::stack|std::queue|std::priority_queue" include src tests
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

For performance tests, prefer a repeatable command or script and record input size:

```bash
./build/tests/perf_insert_select
./scripts/run_demo.sh
```

## TEST_LOG Format

If `docs/TEST_LOG.md` does not exist, create it with this title:

```markdown
# Mini DBMS Test Log

本文档按阶段记录真实测试过程，最终用于汇总课程报告第五章“系统测试”。
```

Append this format for each phase:

```markdown
## <日期> <Phase 名称>

### 测试范围

- <模块或功能 1>
- <模块或功能 2>

### 测试用例与结果

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| <测试项> | <命令、SQL、函数输入或场景> | <预期行为> | <通过/失败/未运行/待验证 + 简短说明> |

### 性能测试

| 测试内容 | 输入规模 | 验证命令 | 预期结果 | 实际结果 |
| -------- | -------- | -------- | -------- | -------- |
| <性能项> | <数据量> | `<command>` | <预期> | <实测结果或未运行原因> |

### 错误测试

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| <错误项> | <非法 SQL 或非法操作> | <错误码/错误消息/不崩溃> | <实际结果> |

### 验证命令

```bash
<实际执行的命令>
```

结果摘要：
- <命令 1>: <通过/失败/未运行，原因>
- <命令 2>: <通过/失败/未运行，原因>

### AI 辅助测试说明

- AI 如何辅助测试：<设计测试点/整理测试表/分析失败等>
- AI 给出的建议：<建议摘要>
- 最终选择：<人工最终采纳或舍弃的内容>

### 遗留测试问题

- <没有则写“无”>
```

## Report Chapter Guidance

When the user asks for Chapter 5, summarize `docs/TEST_LOG.md` instead of copying every phase verbatim:

- 5.1 测试方法
- 5.2 单元测试
- 5.3 集成测试
- 5.4 错误测试
- 5.5 性能测试
- 5.6 AI 辅助测试说明
- 5.7 测试结论与遗留问题

中文建议：阶段日志写细，最终报告写汇总。报告中选代表性表格和关键结果，不需要把每个阶段的全部原始记录重复粘贴。

## Acceptance

- `docs/TEST_LOG.md` exists and has a new phase entry.
- Each table separates expected and actual result.
- Unrun tests are clearly marked.
- AI testing assistance is recorded when applicable.
- Missing verification is not hidden.

