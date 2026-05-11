# Mini DBMS Test Log

本文档按阶段记录真实测试过程，最终用于汇总课程报告第五章“系统测试”。

## 2026-05-11 Phase 2 基础设施、测试框架和 CMake

### 测试范围

- 顶层 CMake 构建配置、`tests/CMakeLists.txt` 测试配置、`MINI_DBMS_ENABLE_ASAN` sanitizer 选项。
- `common` 基础设施：`Status`、`DynamicArray<T>`、`HashTable<K,V>`、`LinkedList<T>`、`StringUtils`。
- 自研轻量测试框架：`TEST_CASE`、`EXPECT_TRUE`、`EXPECT_EQ`、`EXPECT_STATUS_OK`。
- no-STL 容器约束扫描。

### 测试用例与结果

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| CMake 配置 | `cmake -S . -B build` | 生成 Debug/默认构建目录，发现 C++20 编译器，注册 common 测试目标 | 通过，CMake 配置和生成完成 |
| Common 测试构建 | `cmake --build build` | 成功编译 `mini_dbms_common_tests` | 通过，测试可执行文件构建完成 |
| Common 单元测试 | `ctest --test-dir build --output-on-failure` | `common_tests` 全部通过 | 通过，1/1 tests passed |
| `Status` 成功/失败状态 | `Status::OK()`、`Status::InvalidArgument("bad name")` | `ok()`、错误码、错误消息、`ToString()` 符合预期 | 通过，由 `status_test.cpp` 覆盖 |
| `DynamicArray<T>` 空数组和扩容 | 连续 `PushBack` 32 个整数 | size 正确，capacity 自动增长，元素可按下标读取 | 通过，由 `dynamic_array_test.cpp` 覆盖 |
| `DynamicArray<T>` 删除和清空 | 字符串数组删除中间元素并 `Clear()` | 删除后元素前移，清空后为空 | 通过，由 `dynamic_array_test.cpp` 覆盖 |
| `DynamicArray<T>` 拷贝和移动 | 拷贝、移动字符串数组 | 拷贝保留元素，移动后目标持有数据 | 通过，由 `dynamic_array_test.cpp` 覆盖 |
| `HashTable<K,V>` 插入、更新、查询 | 字符串 key 到 int value | `Put`、`Get`、`Contains` 正常，重复 key 更新 value | 通过，由 `hash_table_test.cpp` 覆盖 |
| `HashTable<K,V>` 扩容和碰撞 | 插入 80 个整数 key，使用相同低位触发线性探测 | 扩容后所有 key/value 仍可查询 | 通过，由 `hash_table_test.cpp` 覆盖 |
| `HashTable<K,V>` 删除和复用槽位 | 删除 key 后插入新 key | 删除 key 不可查询，新 key 可插入，size 正确 | 通过，由 `hash_table_test.cpp` 覆盖 |
| `LinkedList<T>` 队列式操作 | `PushFront`、`PushBack`、`PopFront` | 头尾元素和 size 正确，清空后为空 | 通过，由 `linked_list_test.cpp` 覆盖 |
| `StringUtils::Split` | `"a,,b,"` 按 `,` 切分 | 保留空字段，返回 `DynamicArray<std::string>` | 通过，由 `string_utils_test.cpp` 覆盖 |
| `StringUtils::SplitWhitespace` | `"  create\t database\nabc  "` | 合并连续空白并返回 3 个 token | 通过，由 `string_utils_test.cpp` 覆盖 |
| `StringUtils::IsLowerAlphaIdentifier` 和 `Trim` | `person`、`person1`、`Person`、`person_name`、`" \tselect\r\n"` | 仅小写英文字母标识符合法，trim 去除首尾空白 | 通过，由 `string_utils_test.cpp` 覆盖 |

### 性能测试

| 测试内容 | 输入规模 | 验证命令 | 预期结果 | 实际结果 |
| -------- | -------- | -------- | -------- | -------- |
| 基础容器性能基准 | 未定义 | 未运行 | 后续可增加批量插入/查询计时脚本 | 未运行；Phase 2 仅建立基础设施和正确性单测，尚无性能脚本 |

### 错误测试

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| `DynamicArray::RemoveAt` 越界 | 当前测试代码未显式调用越界删除 | 返回非 OK `Status`，不崩溃 | 未运行；建议后续补充断言错误码 |
| `HashTable::Remove` 删除不存在 key | 当前测试代码未显式删除不存在 key | 返回 `Status::NotFound`，不崩溃 | 未运行；建议后续补充断言错误码 |
| `LinkedList::PopFront` 空链表 | 当前测试代码未显式空链表弹出 | 返回 `Status::NotFound`，不崩溃 | 未运行；建议后续补充断言错误码 |
| 非法标识符校验 | `person1`、`Person`、`person_name` | 返回 false | 通过，由 `string_utils_test.cpp` 覆盖 |

### 验证命令

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
rg "std::vector|std::map|std::set|std::unordered|std::list|std::deque|std::array|std::forward_list|std::span|std::stack|std::queue|std::priority_queue" include src tests
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

结果摘要：
- `cmake -S . -B build`: 通过，配置和生成完成。
- `cmake --build build`: 通过，`mini_dbms_common_tests` 构建完成。
- `ctest --test-dir build --output-on-failure`: 通过，`common_tests` 1/1 passed。
- no-STL 扫描：通过，`include src tests` 下无 forbidden STL 容器匹配。
- `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON`: 通过，ASan/UBSan 构建目录生成完成。
- `cmake --build build-asan`: 通过，ASan/UBSan 版本 `mini_dbms_common_tests` 构建完成。
- `ctest --test-dir build-asan --output-on-failure`: 通过，`common_tests` 1/1 passed。

### AI 辅助测试说明

- AI 如何辅助测试：根据 Phase 2 基础设施范围设计 common 单元测试，执行构建、CTest、no-STL 扫描和 ASan/UBSan 验证，并整理本测试记录。
- AI 给出的建议：保留自研测试框架，不引入 GoogleTest/Catch2；后续补充显式错误路径测试和基础容器性能脚本。
- 最终选择：采纳自研测试框架与 common 单测；性能测试暂不下结论，只记录为后续计划项。

### 遗留测试问题

- 需要补充 `DynamicArray::RemoveAt` 越界、`HashTable::Remove` 删除不存在 key、`LinkedList::PopFront` 空链表等显式错误路径测试。
- 尚未设计基础容器性能测试脚本。

## 2026-05-11 Phase 3 SQL Lexer / Parser

### 测试范围

- `sql` 模块公开 API：`Token`、`Lexer`、`SqlCommand`、`Parser`。
- 课程要求 SQL 子集的 DDL/DML 语法解析。
- Parser 错误路径：非法标识符、非法类型、空列表、未支持 SQL 扩展和未闭合字符串。
- no-STL 容器约束扫描。
- ASan/UBSan 构建与测试。

### 测试用例与结果

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| SQL 模块默认构建 | `cmake -S . -B build && cmake --build build` | `mini_dbms_sql` 和 `mini_dbms_sql_tests` 构建成功 | 通过，SQL 库和测试目标构建完成 |
| SQL 单元测试 | `ctest --test-dir build --output-on-failure` | `common_tests`、`sql_tests` 全部通过 | 通过，2/2 tests passed |
| Lexer 基础 token | `insert users values (1, "alice")` | 识别 keyword、identifier、括号、逗号、int literal、string literal 和 end token | 通过，由 `tests/sql/parser_test.cpp` 覆盖 |
| `create database` | `create database school` | 返回 `CommandType::kCreateDatabase`，数据库名为 `school` | 通过 |
| `drop database` 和 `use` | `drop database school`、`use school` | 返回对应 command type 和数据库名 | 通过 |
| `create table` | `create table students (id int primary, name string, age int)` | 返回表名、3 个字段、字段类型和 primary 标记 | 通过 |
| `drop table` | `drop table students` | 返回 `CommandType::kDropTable` 和表名 | 通过 |
| `insert` | `insert students values (1, "alice", -20)` | 返回表名和 3 个 `ValueLiteral`，区分 int/string | 通过 |
| `select * where` | `select * from students where id = 1` | `select_all=true`，where 为 `{id, =, 1}` | 通过 |
| `select` 列表 | `select id, name from students` | 返回 selected columns，`has_where=false` | 通过 |
| `delete` | `delete students`、`delete students where age > 18` | 支持无 where 和带 where 两种形式 | 通过 |
| `update` | `update students set name = "bob" where id < 10` | 返回 set 字段和值，并解析 where 条件 | 通过 |

### 性能测试

| 测试内容 | 输入规模 | 验证命令 | 预期结果 | 实际结果 |
| -------- | -------- | -------- | -------- | -------- |
| Parser 批量解析性能 | 未定义 | 未运行 | 后续可增加批量 SQL 解析计时脚本 | 未运行；Phase 3 只建立正确性单测，尚无性能脚本 |

### 错误测试

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| 空字段建表 | `create table students ()` | 返回非 OK `Status`，不崩溃 | 通过 |
| 非法字段类型 | `create table students (id float)` | 返回非 OK `Status`，不扩展 `float` | 通过 |
| 空 insert values | `insert students values ()` | 返回非 OK `Status` | 通过 |
| 不支持 order by | `select * from students order by id` | 返回非 OK `Status`，不扩展 order by | 通过 |
| 不支持 `delete from` | `delete from students` | 返回非 OK `Status`，保持课程语法 `delete <table>` | 通过 |
| update 缺少 set | `update students name = "bob"` | 返回非 OK `Status` | 通过 |
| 不支持 `!=` | `select * from students where id != 1` | 返回非 OK `Status`，只支持 `= < >` | 通过 |
| 非法标识符 | `create database school_db`、`create database School`、`create table students (student_id int)` | 返回非 OK `Status`，只允许小写英文字母 | 通过 |
| 未闭合字符串 | `insert students values ("unterminated)` | 返回非 OK `Status`，不崩溃 | 通过 |

### 验证命令

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
rg -n "std::(vector|map|set|unordered_map|unordered_set|list|deque|array|forward_list|span|stack|queue|priority_queue)\b" include/sql src/sql tests/sql CMakeLists.txt tests/CMakeLists.txt
cmake -S . -B build-asan -DMINI_DBMS_ENABLE_ASAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

结果摘要：
- `cmake -S . -B build`: 通过，配置和生成完成。
- `cmake --build build`: 通过，`mini_dbms_sql`、`mini_dbms_common_tests`、`mini_dbms_sql_tests` 构建完成。
- `ctest --test-dir build --output-on-failure`: 通过，2/2 tests passed。
- no-STL 扫描：通过，新增 SQL 模块和相关 CMake 文件无 forbidden STL 容器匹配。
- `cmake -S . -B build-asan -DMINI_DBMS_ENABLE_ASAN=ON`: 通过，ASan/UBSan 构建目录生成完成。
- `cmake --build build-asan`: 通过，ASan/UBSan 版本测试目标构建完成。
- `ctest --test-dir build-asan --output-on-failure`: 通过，2/2 tests passed。

### AI 辅助测试说明

- AI 如何辅助测试：根据 Phase 3 SQL 子集设计 Lexer/Parser 单元测试，覆盖 DDL、DML、where 条件和值字面量，并在失败时修正测试预期。
- AI 给出的建议：Parser 只返回结构化 `SqlCommand`，不访问 storage/index/network；错误路径保持非 OK `Status`，不扩展 join/order by/group by/复杂表达式。
- 最终选择：采纳针对课程 SQL 子集的单元测试；性能测试暂不下结论，只记录为后续计划项。

### 遗留测试问题

- 尚未设计 Parser 批量解析性能测试脚本。
- 后续 Executor 集成阶段需要补充 Parser 到 Executor 的端到端 SQL 行为测试。
