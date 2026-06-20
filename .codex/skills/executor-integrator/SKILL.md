---
name: executor-integrator
description: Use when connecting the Mini DBMS parser, storage engine, and B+ tree index into executable SQL behavior with QueryResult responses and integration tests.
---

# Executor Integrator

中文说明：这是“执行器集成”skill。用于把 Parser、Storage、B+树串起来，让 SQL 真正执行。

## Mission

Make parsed SQL commands actually run against storage and indexes.

中文任务：把解析后的 SQL 命令变成实际数据库操作，并返回结构化结果。

## Required Inputs

Read:

- Parser command structures.
- Storage engine public API.
- B+ tree index API.
- Existing result/status types.

中文需要先读：

- Parser 的 Command 结构。
- StorageEngine 的公开接口。
- B+树 Index 的接口。
- 当前项目的 Status/Result 类型。

## Responsibilities

- Manage current database selection.
- Execute DDL.
- Execute insert/select/delete/update.
- Enforce schema checks.
- Enforce primary key uniqueness.
- Use primary index when possible.
- Return structured result data for CLI/network.

中文职责：

- 管理当前使用的数据库。
- 执行 DDL 和 DML。
- 检查表结构和字段。
- 检查主键唯一性。
- 能用主键索引时优先使用索引。
- 返回给 CLI/Network 使用的结构化结果。

## Boundaries

- Executor does not tokenize SQL unless a top-level helper explicitly combines parse + execute.
- Executor does not own socket code.
- Executor does not print CLI tables directly.

中文边界：

- Executor 不负责 socket。
- Executor 不直接打印 CLI 表格。
- Executor 可以提供一个“parse + execute”的上层辅助函数，但核心执行逻辑应消费 Parser 输出。

## Query Behavior

- `select *` returns all columns.
- `select column` returns one column.
- `where primary = value` should use B+ tree index.
- `where primary < value` and `where primary > value` should use range index if available.
- Non-indexed where can scan.

中文查询规则：

- `select *` 返回所有列。
- `select column` 返回单列。
- 主键 `=`、`<`、`>` 查询优先走索引。
- 非索引 where 可以全表扫描。

## Acceptance

- All required SQL commands work through one execution API.
- Error messages are clear.
- Integration tests cover DDL and DML.
- Index path is tested separately from scan path.

中文验收标准：

- 所有课程 SQL 都能通过统一接口执行。
- 错误信息清楚。
- 集成测试覆盖 DDL 和 DML。
- 索引路径和全表扫描路径都要测。
