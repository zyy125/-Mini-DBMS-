---
name: storage-engine-designer
description: Use when designing or implementing the Mini DBMS file storage layer, including database/table directories, metadata files, row serialization, CRUD operations, RowId design, persistence tests, and report-ready storage format documentation.
---

# Storage Engine Designer

中文说明：这是“存储引擎”skill。用于设计和实现数据库目录、表元数据、行数据文件、CRUD 和持久化测试。

## Mission

Create a simple persistent file storage layer that is reliable enough for the course project and easy to explain.

中文任务：实现一个简单可靠、能落盘、能在报告中讲清楚的文件存储层。

## Required Concepts

- `data/` root directory.
- One directory per database.
- Per-table metadata file.
- Per-table row data file.
- `RowId` or `RecordId` stable enough for indexes.
- `int` and `string` values, with string max length 256.

中文核心概念：

- `data/` 是数据根目录。
- 每个数据库一个目录。
- 每个表至少有元数据文件和数据文件。
- 必须有 `RowId` 或 `RecordId`，供索引定位记录。
- 支持 `int` 和最长 256 字符的 `string`。

## Workflow

1. Read existing schema/command/common types.
2. Design file formats before coding.
3. Document file format in `docs/STORAGE_FORMAT.md`.
4. Implement database and table lifecycle.
5. Implement row insert/scan/update/delete.
6. Add persistence tests that recreate the storage engine after writes.

中文工作流程：

1. 先读已有 schema、command、common 类型。
2. 先设计文件格式，再写代码。
3. 把文件格式写进 `docs/STORAGE_FORMAT.md`。
4. 实现数据库和表的创建/删除。
5. 实现行插入、扫描、更新、删除。
6. 写“重启后还能读取”的持久化测试。

## Storage Defaults

Prefer a simple format:

- Metadata file stores column count, column names, types, primary column.
- Data file stores fixed or length-prefixed rows.
- Deleted rows can use tombstones for MVP.
- `RowId` can be file offset or numeric slot id, but must be documented.
- Prefer manual field-by-field serialization.
- Do not directly persist ordinary C++ structs with `fwrite` as the long-term format unless struct packing and initialization are explicitly handled.

中文默认存储方案：

- 元数据文件记录列数量、列名、类型、主键列。
- 数据文件保存行。
- 删除可以先用 tombstone 标记。
- `RowId` 可以是文件偏移或槽位编号，但必须文档化。
- 优先手动按字段序列化。
- 不要直接把普通 C++ struct 当作长期文件格式写入文件；如果使用 packed struct，必须处理 `#pragma pack(push, 1)` / `#pragma pack(pop)` 并写入文档。

## Boundaries

- Storage does not parse SQL.
- Storage does not format CLI tables.
- Storage may maintain primary index hooks but B+ tree logic belongs in `index`.

中文模块边界：

- Storage 不解析 SQL。
- Storage 不负责 CLI 表格输出。
- B+树逻辑属于 `index` 模块。
- Storage 可以暴露接口让 Index 根据 `RowId` 定位数据。

## Acceptance

- Data survives process restart.
- Duplicate database/table creation returns errors.
- Drop operations remove files/directories.
- Update/delete behavior is documented.
- Tests cover normal and error paths.
- File format documentation explains padding/alignment strategy.
- ASan/UBSan tests pass or the reason they were not run is recorded.

中文验收标准：

- 数据写入后重启仍能读取。
- 重复创建数据库/表会报错。
- drop 会删除对应文件或目录。
- update/delete 策略写清楚。
- 正常路径和错误路径都有测试。
- 文件格式文档说明如何避免 struct padding / 字节对齐问题。
- ASan/UBSan 测试通过，或记录未运行原因。
