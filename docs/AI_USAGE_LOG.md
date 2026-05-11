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

## 2026-05-11 Phase 4 文件存储引擎

Skill:
storage-engine-designer, mini-dbms-test-report-recorder, ai-log-recorder

平台/模型:
待补充

Prompt 摘要:
要求 AI 扮演 `storage-engine-designer`，先阅读项目规格、AI 上下文、架构文档、`include/common` 和 SQL Command 定义；在 Phase 4 实现 `StorageEngine` 公开 API、数据库创建/删除/切换、表元数据创建/删除/加载、行插入/扫描/更新/删除、供 B+ 树定位记录的 `RowId` / `RecordId`、存储格式文档和持久化单元测试；序列化必须手动按字段写入，不得直接 `fwrite` 普通 struct。

AI 产出:
- 新增 `include/storage/storage_engine.h`，定义 `StorageEngine`、`Schema`、`Column`、`Value`、`Row`、`RowId` / `RecordId` 和结果结构。
- 新增 `src/storage/storage_engine.cpp`，实现数据库目录管理、`.meta` schema 文件、`.dat` 行文件、墓碑删除、按 RowId 读取和更新。
- 新增 `docs/STORAGE_FORMAT.md`，说明目录结构、metadata/data 文件格式、手动序列化策略、RowId 含义和 Phase 5 `.idx` 预留。
- 新增 `tests/storage/storage_engine_test.cpp`，覆盖数据库/表生命周期、持久化、insert/scan/read/update/delete 和错误路径。
- 更新 CMake，新增 `mini_dbms_storage` 和 `mini_dbms_storage_tests`。

人工审查:
- 已检查课程要求中的文件存储、`int`/`string`、string 256 字节限制、主键索引预留和 no-STL 容器约束。
- 已根据测试失败修正非法数据库名测试输入：`schooldb` 是合法小写英文名，改为 `schooldb1`。
- 已确认文件格式使用字段级序列化，没有直接持久化普通 C++ struct。

验证结果:
- `cmake --build build`：通过，storage 库和测试目标构建完成。
- `ctest --test-dir build --output-on-failure`：通过，`common_tests`、`sql_tests`、`storage_tests` 3/3 passed。
- no-STL 扫描 `rg "std::vector|std::map|std::set|std::unordered|std::list|std::deque|std::array|std::forward_list|std::span|std::stack|std::queue|std::priority_queue" include src tests`：无匹配。
- `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON && cmake --build build-asan && ctest --test-dir build-asan --output-on-failure`：通过，3/3 tests passed。

Git:
未提交，原因：当前任务未要求提交；工作区包含 Phase 4 源码、测试和文档记录改动。

遗留问题:
- 批量存储性能测试未实现。
- Phase 5 需要将主键唯一性和 `.idx` 持久化接入 `RowId`。
- Phase 6 需要通过 Executor 将 SQL 命令接入 Storage API。

## 2026-05-11 Phase 5 B+树主键索引

Skill:
bplustree-index-builder, ai-log-recorder

平台/模型:
待补充

Prompt 摘要:
要求 AI 扮演 `bplustree-index-builder`，先阅读项目规格、AI 上下文、架构文档、存储格式文档和 Storage 的 `RowId` / `RecordId` 定义；在 Phase 5 实现 `int primary key` 的 B+树索引、key 到 `RowId` 映射、插入、精确查询、`<` / `>` 范围查询、重复主键检测、`.idx` 持久化、索引设计文档和 index 单元测试；节点内部优先使用固定数组或项目自研结构，不使用 STL 容器；删除可先采用重建索引或懒删除策略并在文档中说明。

AI 产出:
- 新增 `include/index/bplus_tree_index.h`，定义 `BPlusTreeIndex`、`IndexLookupResult` 和 `IndexRangeResult`，公开 `Insert`、`FindEqual`、`FindLessThan`、`FindGreaterThan`、`SaveToFile`、`LoadFromFile` 等核心 API。
- 新增 `src/index/bplus_tree_index.cpp`，实现固定数组节点的 B+树插入、叶子/内部节点分裂、叶子链表范围扫描、重复主键拒绝和 `.idx` 字段级序列化/反序列化。
- 新增 `tests/index/bplus_tree_index_test.cpp`，覆盖空树查询、精确查询、重复主键、叶子和内部节点分裂、范围查询、保存和重载。
- 新增 `docs/INDEX_DESIGN.md`，说明公开 API、B+树阶数和节点结构、`.idx` 文件格式、删除限制以及 Phase 6 Executor 调用方式。
- 更新 CMake，新增 `mini_dbms_index` 库和 `mini_dbms_index_tests` 测试目标。

人工审查:
- 已检查课程要求中的 `int`/`string` 类型、主键必须建立 B+树索引、文件持久化、no-STL 容器和自研数据结构约束。
- 已确认 Phase 5 只支持 `int` key，没有扩展 string key。
- 已根据测试结果修正内部节点插入时父子指针定位错误；原问题表现为大量插入后返回 `Internal: B+ tree parent link is inconsistent`。
- 已修正范围持久化测试的越界风险，避免断言失败后继续访问空结果数组。

验证结果:
- `cmake --build build`：通过，新增 index 库和测试目标构建完成。
- `ctest --test-dir build --output-on-failure`：通过，`common_tests`、`sql_tests`、`storage_tests`、`index_tests` 4/4 passed。
- `cmake -S . -B build-asan -DMINI_DBMS_ENABLE_ASAN=ON && cmake --build build-asan --target mini_dbms_index_tests && ./build-asan/tests/mini_dbms_index_tests`：通过，index 单元测试 6/6 passed，未见 ASan/UBSan 报错。
- no-STL 扫描 `rg "std::(vector|map|set|unordered_map|unordered_set|list|deque|array|forward_list|span|stack|queue|priority_queue)" include src tests -n`：无匹配。

Git:
未提交，原因：当前任务未要求提交；当前工作区包含 Phase 5 源码、测试、文档和日志记录改动。记录时 `HEAD` 为 `1f009ab`，但该提交不包含本阶段改动。

遗留问题:
- B+树删除 API 暂未实现；Phase 6 delete/update 可先采用重建索引或延后 indexed delete/update 支持。
- 尚未实现 string primary key 索引；若后续课程验收要求 string 主键建索引，需要扩展 key 类型和 `.idx` 格式版本。

## 2026-05-11 Phase 6 Executor 集成

Skill:
executor-integrator, cpp-no-stl-reviewer, unit-test-designer, mini-dbms-test-report-recorder, ai-log-recorder

平台/模型:
待补充

Prompt 摘要:
要求 AI 使用 `executor-integrator`，先阅读项目规格、AI 上下文、Parser Command 定义、StorageEngine 公开 API、PrimaryIndex/B+Tree API、存储格式和索引设计文档；在 Phase 6 实现 Executor 公开 API、结构化 QueryResult、当前数据库上下文管理、课程 SQL 执行逻辑、insert 维护 primary index、select 主键条件优先走索引、非索引 where 走扫描，并补充 executor 单元测试和集成测试；Executor 不直接负责 socket 或 CLI 表格打印。

AI 产出:
- 新增 `include/executor/executor.h`，定义 `Executor`、`QueryResult`、`QueryRow`、`QueryValue`。
- 新增 `src/executor/executor.cpp`，实现 parse+execute 辅助入口、DDL/DML 执行、当前数据库上下文、schema/类型检查、主键唯一性、insert 索引维护、主键等值/范围索引查询、扫描查询，以及 update/delete 后主键索引重建。
- 新增 `tests/executor/executor_test.cpp`，覆盖 DDL、insert/select、主键索引等值和范围查询、非索引扫描、update/delete 后索引重建、主键冲突和错误路径。
- 更新 CMake，新增 `mini_dbms_executor` 库和 `mini_dbms_executor_tests` 测试目标。
- 追加 Phase 6 测试日志。

人工审查:
- 已检查 Executor 边界：不包含 socket 代码，不直接打印 CLI 表格，只返回结构化 `QueryResult`。
- 已检查课程 SQL 覆盖：create/drop/use/create table/drop table/insert/select/delete/update 均通过统一执行入口。
- 已根据测试失败修正空表 where 类型错误未提前校验的问题。
- 已补充主键 update 冲突的写入前校验，避免存储已改但索引重建失败导致部分提交。
- 已按 no-STL 约束扫描 `include src tests`，未发现 forbidden STL 容器。

验证结果:
- `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`：通过，5/5 tests passed。
- no-STL 扫描 `rg "std::vector|std::map|std::set|std::unordered|std::list|std::deque|std::array|std::forward_list|std::span|std::stack|std::queue|std::priority_queue|#include <vector>|#include <map>|#include <set>|#include <unordered_map>|#include <unordered_set>|#include <list>|#include <deque>|#include <array>|#include <span>|#include <stack>|#include <queue>" include src tests`：无匹配。
- `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON && cmake --build build-asan && ctest --test-dir build-asan --output-on-failure`：通过，5/5 tests passed，未见 ASan/UBSan 报错。

Git:
未提交，原因：当前任务未要求提交；工作区包含 Phase 6 源码、测试和日志记录改动。

遗留问题:
- Executor 批量性能测试未实现。
- Network/CLI 尚未接入 Executor；Phase 7 需要实现 TCP 请求处理和 MySQL 风格表格格式化。

## 2026-05-11 Phase 7 TCP 服务端和 CLI 客户端

Skill:
network-cli-builder, ai-log-recorder

平台/模型:
待补充

Prompt 摘要:
要求 AI 扮演 `network-cli-builder`，先阅读项目规格、AI 上下文、Executor 公开 API、QueryResult 定义和当前 CMake 配置；在 Phase 7 实现 `db_server` TCP 服务端、`db_client` CLI 客户端、length-prefixed text 请求/响应协议、select 结果 MySQL-like 表格输出、`exit` 退出客户端，以及网络 smoke test 或 `scripts/manual_demo.sh`；网络层不解析 SQL，CLI 不访问 storage 文件，Server 持有 Executor。

AI 产出:
- 新增 `include/network/protocol.h` 和 `src/network/protocol.cpp`，实现 4 字节 big-endian 长度前缀 + UTF-8 payload 的 TCP frame 收发。
- 新增 `src/server/db_server.cpp`，实现可指定端口和 data root 的 TCP 服务端，持有 `Executor` 并把 SQL 执行结果序列化为文本响应。
- 新增 `src/client/db_client.cpp`，实现交互式 CLI、`mini_dbms>` prompt、`exit` / `quit` 退出、SQL 发送、响应解析和 MySQL-like 表格输出。
- 新增 `scripts/manual_demo.sh`，构建服务端/客户端，启动本地服务端并通过客户端执行端到端 demo SQL。
- 更新 `CMakeLists.txt`，新增 `mini_dbms_network`、`db_server` 和 `db_client` 目标。

人工审查:
- 已检查网络边界：网络层只传输 SQL 文本和响应文本，不实现 SQL 解析或业务逻辑。
- 已确认 CLI 不访问 storage 文件，只通过 TCP 与服务端通信。
- 已确认服务端持有单个 `Executor` 实例，客户端连接上的多条 SQL 可以共享数据库上下文。
- 已检查新增代码没有使用 forbidden STL 容器。

验证结果:
- `cmake -S . -B build && cmake --build build --target db_server db_client`：通过，`db_server` 和 `db_client` 构建完成。
- `ctest --test-dir build --output-on-failure`：通过，`common_tests`、`sql_tests`、`storage_tests`、`index_tests`、`executor_tests` 5/5 passed。
- `bash scripts/manual_demo.sh`：通过，客户端成功执行 create/use/create table/insert/select/update/delete/select/exit；select 输出 MySQL-like 表格，主键查询显示 `(using index)`。
- no-STL 扫描 `rg -n "std::(vector|map|set|unordered_map|unordered_set|list|deque|array|forward_list|span|stack|queue|priority_queue)" include src tests CMakeLists.txt`：无匹配。
- ASan/UBSan：本阶段未单独运行，原因是本次变更主要为 Linux socket 集成和 CLI；已运行普通构建、全量测试和网络 demo。

Git:
未提交，原因：当前任务未要求提交；工作区包含 Phase 7 源码、脚本、CMake 和日志记录改动。记录时 `HEAD` 为 `5d6ef46`，但该提交不包含本阶段改动。

遗留问题:
- 服务端当前按连接串行处理请求，暂未实现多客户端并发。
- 响应格式为项目自定义文本协议，暂未提供独立网络单元测试；已有 `scripts/manual_demo.sh` 覆盖端到端 smoke test。
