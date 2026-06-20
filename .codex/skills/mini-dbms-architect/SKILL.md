---
name: mini-dbms-architect
description: Use when designing or revising the overall architecture of the C++ Mini DBMS course project, including module boundaries, directory layout, class responsibilities, MVP scope, and report-ready OOA/OOD material under the no-STL-container constraint.
---

# Mini DBMS Architect

中文说明：这是“系统架构设计”skill。你在项目刚开始、调整模块边界、写课程报告中的系统设计章节时使用它。

## Mission

Design a course-feasible Mini DBMS architecture that can actually be implemented, tested, and explained in the report.

中文任务：设计一个真的能完成、能测试、能写进报告的 Mini DBMS 架构，不追求数据库工业级复杂度。

## Required Context

Before answering or editing, read:

- `docs/PROJECT_SPEC.md`
- Existing `CMakeLists.txt`, `include/`, `src/`, and `tests/` if present

中文要求：回答或改代码前，先读课程需求、开发计划和已有项目结构。

## Hard Constraints

- C++20 or C++23.
- CMake.
- Linux.
- Client/Server architecture.
- TCP/IP between CLI client and DB server.
- File-system persistence.
- SQL support exactly as required by the course unless explicitly expanded.
- Primary key index uses B+ tree.
- Do not use STL containers: `vector`, `map`, `set`, `unordered_map`, `unordered_set`, `list`, `deque`, `array`, `forward_list`, `span`, or container adapters.
- Prefer a narrow MVP before expanding.
- Treat third-party test frameworks as risky unless the course explicitly allows STL usage in tests.
- Plan ASan/UBSan support for hand-written containers and storage/index code.

中文硬约束：

- 必须满足课程要求，不要自己扩展复杂数据库功能。
- 必须 C/S 架构，客户端和服务端通过 TCP/IP 通信。
- 必须文件持久化。
- 主键索引用 B+树。
- 禁止 STL 容器。
- 先做最小可运行版本，再逐步补齐。
- 测试框架默认自研轻量版本，避免 GoogleTest/Catch2 引入 STL 容器风险。
- CMake 要规划 ASan/UBSan 选项，用来检查手写容器和 B+树内存问题。

## Workflow

1. Restate the current phase and implementation goal.
2. Identify affected modules.
3. Define interfaces before implementation.
4. Keep Parser, Executor, Storage, Index, Network, and CLI separated.
5. For each design choice, prefer the simpler option that satisfies the course.
6. Add report-ready explanations while the design is fresh.

中文工作流程：

1. 先说明当前处于哪个阶段。
2. 找出会影响哪些模块。
3. 先定接口，再写实现。
4. 保持 Parser、Executor、Storage、Index、Network、CLI 分离。
5. 能满足课程要求就用简单方案。
6. 设计完成后顺手留下能写进报告的说明。

## Expected Outputs

- Directory structure.
- Module responsibility table.
- Class responsibility table.
- Data flow from client SQL to result output.
- MVP sequence.
- Risks and mitigations.
- Acceptance checks.

中文产出：

- 目录结构。
- 模块职责表。
- 核心类职责表。
- 从客户端 SQL 到结果输出的数据流。
- MVP 实现顺序。
- 风险和规避方法。
- 验收标准。

## Architecture Defaults

Use this module split unless the existing project strongly suggests otherwise:

- `common`: status, dynamic array, hash table, string helpers.
- `sql`: lexer, parser, command structures.
- `storage`: database/table metadata, row format, file I/O.
- `index`: B+ tree and `.idx` persistence.
- `executor`: command execution and query result formatting data.
- `network`: request/response protocol and sockets.
- `client`: CLI.
- `server`: DB server entrypoint.
- `tests`: unit and integration tests.

中文默认模块划分：

- `common`：状态码、自研动态数组、哈希表、字符串辅助。
- `sql`：词法分析、语法分析、SQL 命令结构。
- `storage`：数据库/表元数据、行格式、文件读写。
- `index`：B+树和 `.idx` 文件。
- `executor`：执行 SQL 命令，协调 Parser、Storage、Index。
- `network`：网络协议和 socket。
- `client`：命令行客户端。
- `server`：服务端入口。
- `tests`：测试。

## Review Checklist

- No module depends on CLI formatting except CLI.
- Network layer does not parse or execute SQL.
- Executor is the only layer coordinating Parser, Storage, and Index.
- Storage exposes stable `RowId` or equivalent.
- Index maps primary key to `RowId`.
- Report can explain every major class.

中文检查清单：

- CLI 之外的模块不要负责表格显示。
- 网络层不要解析或执行 SQL。
- Executor 负责协调 Parser、Storage、Index。
- Storage 必须提供稳定的 `RowId`。
- Index 用主键映射到 `RowId`。
- 每个主要类都能在报告里解释清楚。
