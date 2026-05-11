# 记录模板：

## <日期> <阶段>

Skill:
<skill-name>

Prompt 摘要:
<粘贴本阶段实际使用的干净 Prompt，不要写 VIBE_CODING_PLAN.md>

AI 产出:
<设计 / 代码 / 测试 / 文档>

验证结果:
<构建、测试、no-STL、ASan/UBSan 结果>

Git:
<commit hash 或未提交原因>

遗留问题:
<没有则写“无”>

## 2026-05-11 Phase 1 架构与目录设计

Skill:
mini-dbms-architect, git-workflow-manager

Prompt 摘要:
要求 AI 先阅读 `docs/PROJECT_SPEC.md`、`docs/AI_PROJECT_CONTEXT.md` 和当前项目结构，在 Phase 1 输出并写入 `docs/ARCHITECTURE.md`：推荐目录结构、模块职责表、核心类职责表、跨模块调用关系、CLI 输入 SQL 到返回结果的数据流、MVP 实现顺序、后续阶段接口契约，以及禁止 STL 容器、测试框架、ASan、存储序列化方面的风险说明。明确要求不要实现 Parser、Storage、Index。

Git 收尾 Prompt 要求 AI 使用 `git-workflow-manager`，阅读 `docs/AI_PROJECT_CONTEXT.md`，查看 `git status` 和相关 diff，判断本阶段应进入同一 commit 的文件，给出建议 commit message；仅当工作区只有本阶段相关改动时才执行 `git add` 和 `git commit`，且不要 push、不要回退未确认改动。

AI 产出:
- 新增 `docs/ARCHITECTURE.md`，记录 Phase 1 架构设计、模块边界、数据流、MVP 顺序、接口契约和风险说明。
- 创建模块目录骨架：`include/`、`src/`、`tests/`、`scripts/` 下的模块目录，并用 `.gitkeep` 占位。
- 未实现 Parser、Storage、Index 或其他业务逻辑。
- Git 收尾阶段判断本阶段建议 commit message 为 `docs: add phase 1 architecture design`。

验证结果:
- 已阅读 `docs/PROJECT_SPEC.md` 和 `docs/AI_PROJECT_CONTEXT.md`。
- 已检查 `docs/ARCHITECTURE.md` 内容覆盖用户要求的 8 项。
- 已执行 `find include src tests scripts -type f -name .gitkeep | sort` 确认目录占位文件。
- 已执行 `git status --short --untracked-files=all` 和相关 diff 检查。
- 未运行构建、单元测试或 ASan/UBSan，因为本阶段只新增文档和空目录占位，没有 C++ 业务代码。

Git:
Git 收尾检查时未提交。原因是当时工作区除本阶段架构改动外，还存在未确认改动：`docs/AI_PROMPT_TEMPLATES.md` 删除和 `docs/AI_USAGE_LOG.md` 新增。为避免提交不属于本阶段或未确认的内容，AI 未执行 `git add` / `git commit`，也未 push。

遗留问题:
- 需要确认 `docs/AI_PROMPT_TEMPLATES.md` 的删除是否保留。
- 需要确认 `docs/AI_USAGE_LOG.md` 是否与 Phase 1 架构改动同提交，或单独提交。
- `data/.gitkeep` 被 `.gitignore` 中的 `data/` 规则忽略，当前不会进入 Git；运行期数据目录保持忽略是合理的。

## 2026-05-11 Phase 2 基础设施、测试框架和 CMake

Skill:
cpp-no-stl-data-structure-builder, ai-log-recorder

平台/模型:
待补充

Prompt 摘要:
要求 AI 扮演 `cpp-no-stl-data-structure-builder`，先阅读项目规格、AI 上下文、架构文档以及当前 CMake 和 `common/tests` 目录；在 Phase 2 实现顶层 CMake、Debug/ASan 选项、`Status`、`DynamicArray<T>`、`HashTable<K,V>`、必要的 `LinkedList<T>`、返回自研数组的字符串切分工具、自研轻量测试框架和 common 单元测试；不得使用 GoogleTest/Catch2 或 forbidden STL 容器；完成后输出修改文件、核心接口、测试命令、ASan/UBSan 命令和 no-STL 扫描结果。

AI 产出:
- 新增顶层 `CMakeLists.txt` 和 `tests/CMakeLists.txt`，提供 `mini_dbms_common` 接口库、`MINI_DBMS_BUILD_TESTS`、`MINI_DBMS_ENABLE_ASAN` 和 `mini_dbms_common_tests`。
- 新增 common 头文件：`Status`、`DynamicArray<T>`、`HashTable<K,V>`、`LinkedList<T>`、`StringUtils`。
- 新增 `include/testing/test_framework.h` 自研测试框架：`TEST_CASE`、`EXPECT_TRUE`、`EXPECT_EQ`、`EXPECT_STATUS_OK` 和测试入口。
- 新增 common 单元测试，覆盖状态类型、动态数组扩容/删除/拷贝移动、哈希表插入/更新/删除/扩容碰撞、链表和字符串工具。

人工审查:
- 已检查项目文档中的 C++20、CMake、Linux、no-STL 容器和自研测试框架约束。
- 已发现 `docs/AI_USAGE_LOG.md` 存在既有未提交改动，追加记录时只在末尾增加本阶段条目。

验证结果:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j2 && ctest --test-dir build --output-on-failure`：通过，`common_tests` 1/1 passed。
- `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON && cmake --build build-asan -j2 && ctest --test-dir build-asan --output-on-failure`：通过，`common_tests` 1/1 passed。
- no-STL 扫描命令 `rg -n "std::(vector|map|set|unordered_map|unordered_set|list|deque|array|forward_list|span|stack|queue|priority_queue)\\b|#include\\s*<(vector|map|set|unordered_map|unordered_set|list|deque|array|forward_list|span|stack|queue)>" CMakeLists.txt include src tests docs --glob '!docs/PROJECT_SPEC.md' --glob '!docs/AI_PROJECT_CONTEXT.md' --glob '!docs/ARCHITECTURE.md'`：无匹配。

Git:
未提交，原因：当前任务未要求提交；工作区还包含此前已有的 `docs/AI_USAGE_LOG.md` 未提交状态。

遗留问题:
- 无。

## 2026-05-11 Phase 2 测试记录整理

Skill:
mini-dbms-test-report-recorder

平台/模型:
待补充

Prompt 摘要:
要求运行 `mini-dbms-test-report-recorder` skill，根据真实执行过的构建、测试、no-STL 扫描和 ASan/UBSan 结果，整理 Phase 2 测试报告记录。

AI 产出:
- 新增 `docs/TEST_LOG.md`，记录 Phase 2 测试范围、测试用例与结果、错误测试、性能测试状态、验证命令、AI 辅助测试说明和遗留测试问题。
- 重新执行并记录 CMake 构建、CTest、no-STL 扫描和 ASan/UBSan 验证。

人工审查:
- 已按 skill 规则区分真实通过项、未运行性能测试和待补充错误路径测试。
- 未修改生产代码。

验证结果:
- `cmake -S . -B build`：通过。
- `cmake --build build`：通过。
- `ctest --test-dir build --output-on-failure`：通过，`common_tests` 1/1 passed。
- no-STL 扫描 `rg "std::vector|std::map|std::set|std::unordered|std::list|std::deque|std::array|std::forward_list|std::span|std::stack|std::queue|std::priority_queue" include src tests`：无匹配。
- `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON`：通过。
- `cmake --build build-asan`：通过。
- `ctest --test-dir build-asan --output-on-failure`：通过，`common_tests` 1/1 passed。

Git:
未提交，原因：当前任务未要求提交；工作区包含 Phase 2 源码、测试和文档记录改动。

遗留问题:
- 需要补充显式错误路径测试和基础容器性能测试脚本。

## 2026-05-11 Phase 3 SQL Lexer / Parser

Skill:
sql-parser-builder

平台/模型:
待补充

Prompt 摘要:
要求 AI 扮演 `sql-parser-builder`，先阅读 `docs/PROJECT_SPEC.md`、`docs/AI_PROJECT_CONTEXT.md`、`docs/ARCHITECTURE.md`、`include/common`、`include/testing` 和 `tests/common`；在 Phase 3 实现 Token 类型、Lexer、Command/AST 结构、Parser，支持课程要求 SQL 子集；Parser 不访问文件、不访问网络、不执行 SQL，不扩展 join/order by/group by/复杂表达式；命令结构必须能被 Executor 直接消费；完成后输出公开头文件和核心 API、Command 字段说明、测试覆盖列表，以及 Storage 下阶段是否依赖 Parser schema 类型。

AI 产出:
- 新增 SQL 公开头文件：`include/sql/token.h`、`include/sql/lexer.h`、`include/sql/command.h`、`include/sql/parser.h`。
- 新增 SQL 实现文件：`src/sql/lexer.cpp`、`src/sql/parser.cpp`。
- 新增 `SqlCommand` tagged struct，包含数据库名、表名、字段定义、select 列表、insert 值、update assignment 和 where 条件。
- 新增 `tests/sql/parser_test.cpp`，覆盖课程要求 DDL/DML 解析、Lexer token 和常见非法 SQL。
- 更新 CMake，新增 `mini_dbms_sql` 库和 `mini_dbms_sql_tests` 测试目标。

人工审查:
- 已检查项目文档中的 SQL 子集、标识符规则、类型范围和 no-STL 容器约束。
- 已确认 Parser 只依赖 `common`，不访问 storage/index/network。
- 已在测试失败后修正 Lexer 测试中遗漏右括号 token 的预期。

验证结果:
- `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`：通过，`common_tests` 和 `sql_tests` 2/2 passed。
- no-STL 扫描 `rg -n "std::(vector|map|set|unordered_map|unordered_set|list|deque|array|forward_list|span|stack|queue|priority_queue)\\b" include/sql src/sql tests/sql CMakeLists.txt tests/CMakeLists.txt`：无匹配。
- `cmake -S . -B build-asan -DMINI_DBMS_ENABLE_ASAN=ON && cmake --build build-asan && ctest --test-dir build-asan --output-on-failure`：通过，`common_tests` 和 `sql_tests` 2/2 passed。

Git:
未提交，原因：当前任务未要求提交；工作区包含 Phase 3 源码、测试和日志记录改动。

遗留问题:
- Parser 批量解析性能测试未实现。
- Executor 阶段需要基于 `SqlCommand` 补充端到端执行集成测试。

