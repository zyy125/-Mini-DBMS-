---
name: cpp-no-stl-reviewer
description: Use when reviewing or modifying the Mini DBMS C++ codebase to enforce the course requirement that STL containers and container adapters are forbidden, and to propose replacements using project-owned data structures.
---

# C++ No-STL Reviewer

中文说明：这是“禁止 STL 容器审查”skill。每个编码阶段结束后都应该使用一次，专门检查有没有违反课程限制。

## Mission

Prevent accidental use of forbidden STL containers and keep the code defensible for course review.

中文任务：防止 AI 或人工不小心写出 `std::vector`、`std::map` 等 forbidden 容器，保证代码能通过课程审查。

## Forbidden

Do not use:

- `std::vector`
- `std::map`
- `std::set`
- `std::unordered_map`
- `std::unordered_set`
- `std::list`
- `std::deque`
- `std::array`
- `std::forward_list`
- `std::span`
- `std::stack`
- `std::queue`
- `std::priority_queue`

Also avoid including the corresponding headers unless the project has a documented exception.

中文禁止项：

- 不允许使用上面列出的 STL 容器和容器适配器。
- 一般也不要包含 `<vector>`、`<map>`、`<queue>` 等对应头文件。
- 注释或文档里提到这些词不算代码违规，但要能解释。

## Usually Allowed

These are not STL containers and are usually acceptable unless the course says otherwise:

- `std::string`
- `std::string_view`, if allowed by the team
- `std::iostream`, `std::fstream`
- `std::filesystem`
- `std::unique_ptr`, `std::shared_ptr`
- `std::thread`, `std::mutex`
- algorithms for raw ranges only, if not hiding container usage

中文通常允许：

- `std::string`
- 文件流、输入输出流
- `std::filesystem`
- 智能指针
- 线程和锁
- 注意：这些不是 STL 容器，但如果老师额外限制，就按老师要求来。

## Workflow

1. Scan code before and after changes:

```bash
rg "std::vector|std::map|std::set|std::unordered|std::list|std::deque|std::array|std::forward_list|std::span|std::stack|std::queue|std::priority_queue|#include <vector>|#include <map>|#include <set>|#include <unordered_map>|#include <unordered_set>|#include <list>|#include <deque>|#include <array>|#include <span>|#include <stack>|#include <queue>" include src tests
```

2. Classify each hit:
   - Real violation.
   - False positive in text/comment.
   - Allowed explanation in docs.

3. Replace violations with project-owned structures:
   - `DynamicArray<T>` for sequential storage.
   - `HashTable<K, V>` for lookup.
   - Raw fixed-size arrays for B+ tree node slots.
   - Linked structures only when ownership is clear.

4. Add or update tests when replacement behavior is non-trivial.

中文工作流程：

1. 先用 `rg` 扫描 forbidden 容器。
2. 判断是真违规、注释误报，还是文档说明。
3. 用项目自己的 `DynamicArray`、`HashTable`、固定数组替换。
4. 替换逻辑复杂时补测试。

## Output Format

Lead with findings:

- File and line.
- Forbidden construct.
- Replacement.
- Risk if left unresolved.

Then provide patch or implementation notes.

中文输出格式：

- 先列问题。
- 每个问题给出文件、行号、违规类型、替代方案。
- 再给修复补丁或实现建议。

## Acceptance

- Source and test code have no forbidden containers.
- Any exception is documented and justified.
- Replacement code has basic tests.

中文验收标准：

- `include/`、`src/`、`tests/` 中没有 forbidden STL 容器。
- 如果有例外，必须写清楚原因。
- 替换后的自研结构有基本测试。
