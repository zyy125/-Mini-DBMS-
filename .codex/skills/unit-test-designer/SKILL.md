---
name: unit-test-designer
description: Use when designing, adding, or reviewing Mini DBMS unit tests, integration tests, demo scripts, edge cases, and final course acceptance checks.
---

# Unit Test Designer

中文说明：这是“测试设计和验收”skill。用于补单元测试、集成测试、演示脚本，并检查课程要求是否满足。

## Mission

Validate the project against the course requirements and catch real regressions.

中文任务：站在课程验收者角度找真实问题，不只是总结项目已经做了什么。

## Test Areas

- Common data structures.
- SQL lexer/parser.
- Storage persistence.
- B+ tree index.
- Executor integration.
- TCP server/client smoke behavior.
- Error handling.
- No-STL-container scan.
- Test framework dependency review.
- ASan/UBSan sanitizer run.

中文测试范围：

- 自研基础数据结构。
- SQL Lexer/Parser。
- 存储持久化。
- B+树索引。
- Executor 集成。
- TCP server/client。
- 错误处理。
- 禁止 STL 容器扫描。
- 测试框架依赖检查，避免 GoogleTest/Catch2 违反课程限制。
- ASan/UBSan 内存检查。

## Workflow

1. Read `docs/PROJECT_SPEC.md`.
2. List required behaviors.
3. Map each behavior to an existing or missing test.
4. Add focused tests.
5. Run build and tests.
6. Report failures with likely cause and fix order.

中文工作流程：

1. 先读课程需求。
2. 列出必须满足的行为。
3. 检查哪些已有测试覆盖了，哪些没覆盖。
4. 补重点测试。
5. 运行构建和测试。
6. 对失败项给出原因和修复顺序。

## Required Checks

Run or recommend:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
rg "std::vector|std::map|std::set|std::unordered|std::list|std::deque|std::array|std::forward_list|std::span|std::stack|std::queue|std::priority_queue" include src tests
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

中文必跑检查：

- CMake 配置。
- 编译。
- CTest。
- forbidden STL 容器扫描。
- ASan/UBSan 构建和测试。

## Acceptance

- Each required SQL command has at least one success test.
- Parser has invalid input tests.
- Storage has restart/persistence tests.
- B+ tree has split and duplicate-key tests.
- End-to-end demo covers create/use/create table/insert/select/update/delete/drop.
- Test framework is project-owned or explicitly allowed by course policy.
- Sanitizer run has no errors or missing run is documented.

中文验收标准：

- 每种课程 SQL 至少有一个成功测试。
- Parser 有非法输入测试。
- Storage 有重启后读取测试。
- B+树有分裂和重复主键测试。
- 端到端 demo 覆盖 create/use/create table/insert/select/update/delete/drop。
- 测试框架是自研的，或有课程允许第三方测试框架的记录。
- Sanitizer 无报错，或记录未运行原因。
