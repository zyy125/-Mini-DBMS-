---
name: sql-parser-builder
description: Use when implementing or revising the Mini DBMS SQL lexer, parser, command structures, syntax validation, and parser tests for the course-required SQL subset.
---

# SQL Parser Builder

中文说明：这是“SQL 解析器”skill。用于实现 Lexer、Parser、Command/AST 结构和 SQL 语法测试。

## Mission

Implement only the required SQL subset cleanly and predictably.

中文任务：只实现课程要求的 SQL 子集，不扩展复杂 SQL，保证解析结果稳定、可供 Executor 使用。

## Supported SQL

- `create database <dbname>`
- `drop database <dbname>`
- `use <dbname>`
- `create table <table> (<column> <type> [primary], ...)`
- `drop table <table>`
- `insert <table> values (...)`
- `select <column|*> from <table> [where <column> <op> <const-value>]`
- `delete <table> [where <column> <op> <const-value>]`
- `update <table> set <column> = <const-value> [where <column> <op> <const-value>]`

中文支持范围：

- 只支持上面这些课程要求 SQL。
- 不支持 join、order by、group by、聚合函数、子查询。

## Syntax Rules

- Names use lowercase English letters only.
- No `_` in names.
- Types: `int`, `string`.
- Strings use double quotes.
- Where operators: `=`, `<`, `>`.
- Do not add joins, order by, aggregation, nested expressions, or semicolon requirements unless explicitly requested.

中文语法规则：

- 数据库名、表名、列名只能是小写英文字母。
- 不允许下划线和特殊字符。
- 类型只有 `int` 和 `string`。
- 字符串必须用双引号。
- where 只支持 `=`、`<`、`>`。

## Workflow

1. Inspect existing command/result types.
2. Define token kinds.
3. Define command structures before parser logic.
4. Implement lexer.
5. Implement parser by statement type.
6. Add tests for every supported command and common errors.

中文工作流程：

1. 先看已有命令结构和结果类型。
2. 定义 Token 类型。
3. 先定义 Command/AST，再写 parser。
4. 写 Lexer。
5. 按 SQL 类型分别写 Parser。
6. 每种合法 SQL 和常见非法 SQL 都要有测试。

## Design Guidance

- Parser should not touch files, indexes, or network.
- Parser returns structured command data and a `Status`.
- Keep error messages precise enough for CLI display.
- Use project-owned arrays for column/value lists.

中文设计原则：

- Parser 不碰文件、不碰索引、不碰网络。
- Parser 只把 SQL 字符串变成结构化命令。
- 错误信息要能直接给 CLI 展示。
- 列和值列表使用项目自研数组。

## Acceptance

- Every required SQL form parses.
- Invalid SQL fails with useful error.
- Parser uses no forbidden STL containers.
- Executor can consume command structures without reparsing strings.

中文验收标准：

- 课程要求的每种 SQL 都能解析。
- 非法 SQL 能返回清楚错误。
- 不使用 forbidden STL 容器。
- Executor 可以直接消费解析结果，不需要重新解析字符串。
