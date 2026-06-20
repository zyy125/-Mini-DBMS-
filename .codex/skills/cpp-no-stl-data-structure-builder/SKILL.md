---
name: cpp-no-stl-data-structure-builder
description: Use when implementing the project-owned data structures required by the Mini DBMS course project, such as DynamicArray, HashTable, linked structures, Status, and small string helpers without forbidden STL containers.
---

# C++ No-STL Data Structure Builder

中文说明：这是“自研基础数据结构”skill。用于实现课程要求的动态数组、哈希表、状态类型等基础设施。

## Mission

Build small, reliable project-owned data structures that unblock the rest of the Mini DBMS.

中文任务：先搭好项目自己的基础容器，避免后续 Parser、Storage、B+树偷偷依赖 STL 容器。

## Required First Step

Read existing `include/common`, `src/common`, and `tests/common` before creating new abstractions.

中文要求：新增抽象前先看已有 `common` 目录，避免重复造不同风格的轮子。

## Implement Conservatively

Prefer:

- `Status` and `StatusOr<T>` if needed.
- `DynamicArray<T>` with copy/move support.
- `HashTable<K, V>` only for simple key types needed by the project.
- Fixed-size arrays where maximum sizes are known.
- `StringUtils::Split` or an equivalent helper returning project-owned arrays.
- A lightweight in-project test framework if third-party test frameworks are not explicitly allowed.

Avoid:

- Over-engineered generic containers.
- Iterator systems unless already needed.
- Hidden STL container usage.

中文实现原则：

- 优先实现够用的小工具，不要写一个过度复杂的通用库。
- `DynamicArray<T>` 用来替代 `std::vector`。
- `HashTable<K, V>` 用来替代 `std::unordered_map`。
- 固定长度场景可以直接用原生数组。
- 不要在内部偷偷用 STL 容器。
- 实现字符串切分工具，避免后续 Lexer 为了 split 又引入 STL 容器。
- 默认不要使用 GoogleTest/Catch2，因为它们可能依赖 STL 容器；除非课程明确允许测试目录使用第三方测试库。

## Suggested Interfaces

`DynamicArray<T>` should support:

- construction/destruction
- `push_back`
- `remove_at`
- `clear`
- `size`
- `capacity`
- `operator[]`
- copy/move if useful

中文建议接口：

- `DynamicArray<T>` 至少支持构造、析构、追加、删除、清空、大小、容量、下标访问。
- 需要时再加 copy/move。

`HashTable<K, V>` should support:

- `put`
- `get`
- `contains`
- `remove`
- clear/destruction

Test framework should support:

- `TEST_CASE(name)`
- `EXPECT_TRUE(expr)`
- `EXPECT_EQ(expected, actual)`
- a simple test runner

ASan/UBSan:

- Add or use a CMake option such as `MINI_DBMS_ENABLE_ASAN`.
- Prefer `-fsanitize=address,undefined -g` in Debug sanitizer builds.

中文哈希表接口：

- `put`
- `get`
- `contains`
- `remove`
- `clear`

## Testing

Add tests for:

- empty structure behavior
- growth
- removal
- copy/move, if implemented
- collision handling for hash table
- destructor behavior for owned objects
- `StringUtils::Split` edge cases
- test framework success/failure reporting
- sanitizer build if available

中文测试重点：

- 空结构。
- 扩容。
- 删除。
- 拷贝/移动。
- 哈希冲突。
- 析构释放。

## Acceptance

- No forbidden STL containers.
- Tests pass.
- Interfaces are enough for Parser, Storage, Index, and Executor.
- Memory ownership is clear.
- Test framework does not depend on forbidden STL containers unless explicitly allowed.
- ASan/UBSan tests pass or the reason they were not run is recorded.

中文验收标准：

- 没有 forbidden STL 容器。
- 测试通过。
- 接口足够后续模块使用。
- 内存所有权清楚。
