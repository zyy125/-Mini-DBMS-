---
name: bplustree-index-builder
description: Use when implementing or fixing the Mini DBMS primary-key B+ tree index, including insert, exact lookup, range lookup, duplicate-key handling, index persistence, and tests under the no-STL-container constraint.
---

# B+Tree Index Builder

中文说明：这是“B+树索引”skill。用于实现主键索引、精确查询、范围查询、重复主键检查和 `.idx` 持久化。

## Mission

Implement a course-feasible B+ tree primary index without turning it into a full database-grade indexing subsystem.

中文任务：实现课程项目可接受的 B+树主键索引，不追求工业数据库级别复杂度。

## Scope

Required:

- `int` key support.
- key -> `RowId` mapping.
- insert.
- exact lookup.
- `<` and `>` range lookup.
- duplicate primary key rejection.
- persistence to `.idx`.

中文必须支持：

- `int` 主键。
- 主键映射到 `RowId`。
- 插入。
- 精确查找。
- `<` 和 `>` 范围查询。
- 拒绝重复主键。
- 保存到 `.idx` 文件并能重新加载。

Optional or simplified:

- Delete may use tombstones or index rebuild if documented.
- Complex concurrency is not required unless the course demands it.

中文可简化项：

- 删除可以采用 tombstone 或重建索引，但必须在报告中说明。
- 不要求复杂并发控制。
- 不要求 string 主键，除非你主动扩展。

## Workflow

1. Read `RowId` definition from storage.
2. Define B+ tree node layout.
3. Use fixed-size arrays inside nodes.
4. Implement search first.
5. Implement insert and split.
6. Implement leaf links for range query.
7. Implement save/load.
8. Add tests before integrating with Executor.

中文工作流程：

1. 先读取 Storage 的 `RowId` 定义。
2. 设计 B+树节点结构。
3. 节点内部优先用固定数组。
4. 先实现查找。
5. 再实现插入和分裂。
6. 用叶子链表支持范围查询。
7. 实现保存和加载。
8. 先单测通过，再接入 Executor。

## Design Guidance

- Keep tree order small and constant for easier testing.
- Separate in-memory logic from file serialization where practical.
- Do not use STL containers.
- Do not support string keys unless the project explicitly adds that requirement.
- Run ASan/UBSan tests when available because node split/merge code is memory-risky.

中文设计原则：

- 阶数用固定常量，方便测试。
- 尽量分离内存逻辑和文件序列化。
- 不使用 forbidden STL 容器。
- 不主动支持 string key。
- 如果项目已有 ASan/UBSan 构建，必须优先运行索引测试。

## Acceptance

- Empty tree lookup fails cleanly.
- Many inserts trigger splits.
- Exact lookup returns correct `RowId`.
- Range query returns expected row ids.
- Duplicate key returns error.
- Reloaded index still works.
- ASan/UBSan tests pass or the reason they were not run is recorded.

中文验收标准：

- 空树查询能正常失败。
- 多次插入能触发节点分裂。
- 精确查询返回正确 `RowId`。
- 范围查询结果正确。
- 重复主键报错。
- 重启加载后索引仍可用。
- ASan/UBSan 测试通过，或记录未运行原因。
