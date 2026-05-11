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

## 2026-05-11 Phase 4 文件存储引擎

### 测试范围

- `storage` 模块公开 API：`StorageEngine`、`Schema`、`Row`、`Value`、`RowId` / `RecordId`。
- 数据库目录创建、删除、切换。
- 表元数据创建、删除、重启后加载。
- 行插入、扫描、按 `RowId` 读取、更新、删除墓碑和持久化。
- no-STL 容器约束扫描。
- ASan/UBSan 构建与测试。

### 测试用例与结果

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| Storage 默认构建 | `cmake --build build` | `mini_dbms_storage` 和 `mini_dbms_storage_tests` 构建成功 | 通过 |
| Storage 单元测试 | `ctest --test-dir build --output-on-failure` | `common_tests`、`sql_tests`、`storage_tests` 全部通过 | 通过，3/3 tests passed |
| 数据库生命周期 | `CreateDatabase("school")`、重复创建、`UseDatabase("school")`、`DropDatabase("school")` | 首次创建成功，重复创建 `AlreadyExists`，切换成功，删除目录 | 通过 |
| 表元数据持久化 | 创建 `students(id int primary, name string, age int)` 后重建 `StorageEngine` 并 `LoadSchema` | schema、字段类型和 primary index 可恢复 | 通过 |
| 插入、扫描、按 RowId 读取 | 插入两行学生数据，扫描表，读取第二行 RowId | scan 返回 live rows，RowId 可定位指定行 | 通过 |
| 更新行为 | 短字符串更新为短字符串；再更新为更长字符串 | 可容纳时 RowId 不变；变长超出旧 payload 时返回新 RowId 且 `moved=true` | 通过 |
| 删除和重启持久化 | 删除第一行后重建 `StorageEngine` 并扫描 | 删除行返回 `NotFound`，scan 只返回未删除行 | 通过 |
| Drop table | `DropTable("students")` | 删除 `.meta`、`.dat`，并尝试删除保留的 `.idx` | 通过 |

### 性能测试

| 测试内容 | 输入规模 | 验证命令 | 预期结果 | 实际结果 |
| -------- | -------- | -------- | -------- | -------- |
| 批量插入和全表扫描性能 | 未定义 | 未运行 | 后续可增加固定行数插入/扫描计时脚本 | 未运行；Phase 4 只建立正确性和持久化单测 |

### 错误测试

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| 重复创建数据库 | `CreateDatabase("school")` 调用两次 | 第二次返回 `StatusCode::kAlreadyExists` | 通过 |
| 非法数据库名 | `CreateDatabase("schooldb1")` | 返回 `StatusCode::kInvalidArgument` | 通过 |
| Row 与 schema 不匹配 | 对三列表插入一列且类型错误的 row | 返回 `StatusCode::kInvalidArgument` | 通过 |
| 删除后读取旧 RowId | `DeleteRow` 后 `ReadRow` | 返回 `StatusCode::kNotFound` | 通过 |
| Drop table 后加载 schema | `DropTable("students")` 后 `LoadSchema` | 返回 `StatusCode::kNotFound` | 通过 |

### 验证命令

```bash
cmake --build build
ctest --test-dir build --output-on-failure
rg "std::vector|std::map|std::set|std::unordered|std::list|std::deque|std::array|std::forward_list|std::span|std::stack|std::queue|std::priority_queue" include src tests
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

结果摘要：
- `cmake --build build`: 通过，storage 库和测试目标构建完成。
- `ctest --test-dir build --output-on-failure`: 通过，3/3 tests passed。
- no-STL 扫描：通过，`include src tests` 下无 forbidden STL 容器匹配。
- `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON`: 通过，ASan/UBSan 构建目录生成完成。
- `cmake --build build-asan`: 通过，ASan/UBSan 版本测试目标构建完成。
- `ctest --test-dir build-asan --output-on-failure`: 通过，3/3 tests passed。

### AI 辅助测试说明

- AI 如何辅助测试：根据 Phase 4 存储需求设计数据库、表元数据、行 CRUD、持久化和错误路径测试，并执行构建、CTest、no-STL 扫描和 ASan/UBSan 验证。
- AI 给出的建议：使用手动字段序列化和墓碑删除；以 `.dat` 文件偏移作为稳定 `RowId`；更新变长超出旧 payload 时返回新 RowId，供后续索引同步。
- 最终选择：采纳上述测试与实现策略；性能测试暂不下结论，只记录为后续计划项。

### 遗留测试问题

- 尚未实现批量插入/扫描性能测试。
- Phase 5 Index 需要补充索引文件与 `RowId` 的一致性测试。
- Phase 6 Executor 需要补充 SQL 到 Storage 的端到端 DDL/DML 集成测试。

## 2026-05-11 Phase 5 B+树主键索引

### 测试范围

- `index` 模块公开 API：`BPlusTreeIndex`、`IndexLookupResult`、`IndexRangeResult`。
- `int primary key` 到 `storage::RowId` 的唯一映射。
- B+树插入、叶子节点分裂、内部节点分裂、精确查询和范围查询。
- `.idx` 文件保存和重新加载。
- 重复主键、空树查询和未命中查询等错误路径。
- no-STL 容器约束扫描。
- ASan/UBSan 构建与 index 单元测试。

### 测试用例与结果

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| Index 默认构建 | `cmake --build build` | `mini_dbms_index` 和 `mini_dbms_index_tests` 构建成功 | 通过 |
| Index 单元测试 | `ctest --test-dir build --output-on-failure` | `common_tests`、`sql_tests`、`storage_tests`、`index_tests` 全部通过 | 通过，4/4 tests passed |
| 空树查询 | 新建空 `BPlusTreeIndex` 后 `FindEqual(1)`、`FindLessThan(10)`、`FindGreaterThan(10)` | 精确查询返回 `NotFound`；范围查询成功且结果为空 | 通过 |
| 精确查询 | 插入 `(3, RowId(30))`、`(1, RowId(10))`、`(5, RowId(50))` 后查询 key 1 和 key 4 | key 1 返回 `RowId(10)`；key 4 返回 `NotFound` | 通过 |
| 重复主键检测 | 插入 key 7 后再次插入 key 7 | 第二次插入返回 `AlreadyExists`，原 `RowId` 保持不变 | 通过 |
| 节点分裂 | 逆序插入 key 30 到 1，共 30 条 | 触发叶子和内部节点分裂；全部 key 可精确查回正确 `RowId` | 通过 |
| 范围查询 `<` / `>` | 插入 key 10、20、30、40、50 后查询 `< 35` 和 `> 30` | `< 35` 返回 100、200、300；`> 30` 返回 400、500 | 通过 |
| `.idx` 持久化 | 插入 18 条偶数 key，`SaveToFile` 后用新索引 `LoadFromFile` | 文件存在；重载后 size 为 18；精确查询和范围查询结果保持一致 | 通过 |

### 性能测试

| 测试内容 | 输入规模 | 验证命令 | 预期结果 | 实际结果 |
| -------- | -------- | -------- | -------- | -------- |
| B+树批量插入/查询性能 | 未定义 | 未运行 | 后续可增加固定规模插入、精确查询和范围查询计时脚本 | 未运行；Phase 5 只建立正确性、持久化和 sanitizer 单测 |

### 错误测试

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| 空树精确查询 | `FindEqual(1)` | 返回 `StatusCode::kNotFound`，不崩溃 | 通过 |
| 查询不存在 key | 已插入 key 1、3、5 后 `FindEqual(4)` | 返回 `StatusCode::kNotFound` | 通过 |
| 重复主键插入 | `Insert(7, RowId(70))` 后 `Insert(7, RowId(700))` | 返回 `StatusCode::kAlreadyExists`，size 仍为 1 | 通过 |
| 持久化后查询不存在 key | 重载 `.idx` 后 `FindEqual(11)` | 返回 `StatusCode::kNotFound` | 通过 |
| 无效 RowId 插入 | `Insert(key, RowId())` | 返回 `StatusCode::kInvalidArgument` | 待验证；代码已实现校验，但当前单测未覆盖 |
| 损坏 `.idx` 文件 | 非法 magic、版本或 key type | 返回非 OK `Status`，不崩溃 | 待验证；代码已实现校验，但当前单测未覆盖 |

### 验证命令

```bash
cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build-asan -DMINI_DBMS_ENABLE_ASAN=ON
cmake --build build-asan --target mini_dbms_index_tests
./build-asan/tests/mini_dbms_index_tests
rg "std::(vector|map|set|unordered_map|unordered_set|list|deque|array|forward_list|span|stack|queue|priority_queue)" include src tests -n
```

结果摘要：
- `cmake --build build`: 通过，index 库和测试目标构建完成。
- `ctest --test-dir build --output-on-failure`: 通过，4/4 tests passed。
- `cmake -S . -B build-asan -DMINI_DBMS_ENABLE_ASAN=ON`: 通过，ASan/UBSan 构建目录生成完成。
- `cmake --build build-asan --target mini_dbms_index_tests`: 通过，ASan/UBSan 版本 index 测试目标构建完成。
- `./build-asan/tests/mini_dbms_index_tests`: 通过，index 单元测试 6/6 passed，未见 ASan/UBSan 报错。
- no-STL 扫描：通过，`include src tests` 下无 forbidden STL 容器匹配。

### AI 辅助测试说明

- AI 如何辅助测试：根据 Phase 5 B+树索引需求设计 index 单元测试，覆盖空树、精确查询、重复主键、节点分裂、范围查询和 `.idx` 持久化，并整理本测试记录。
- AI 给出的建议：先用固定数组节点和小阶数触发更多分裂路径；范围查询通过叶子链表验证；持久化测试使用保存后重新加载的方式验证文件格式。
- 最终选择：采纳上述单元测试和 ASan/UBSan 验证；性能测试和损坏文件错误测试暂不下结论，列为后续补充项。

### 遗留测试问题

- 尚未实现 B+树批量性能测试。
- 尚未补充 `Insert(key, RowId())` 的无效 RowId 单测。
- 尚未补充损坏 `.idx` 文件的错误路径单测。
- Phase 6 Executor 需要补充 SQL insert/select 通过主键索引访问 Storage 的集成测试。

## 2026-05-11 Phase 6 Executor 集成

### 测试范围

- `executor` 模块公开 API：`Executor`、`QueryResult`、`QueryRow`、`QueryValue`。
- SQL Parser 到 StorageEngine 和 B+树 PrimaryIndex 的端到端执行。
- 当前数据库上下文管理：`create database`、`use`、`drop database`。
- DDL/DML：`create table`、`drop table`、`insert`、`select`、`update`、`delete`。
- `insert` 主键唯一性和 `.idx` 维护。
- `select where primary =`、`<`、`>` 的索引路径。
- 非索引 where 的全表扫描路径。
- update/delete 后重建主键索引。
- 错误输入、no-STL 容器约束、ASan/UBSan。

### 测试用例与结果

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| Executor 构建 | `cmake --build build` | `mini_dbms_executor` 和 `mini_dbms_executor_tests` 构建成功 | 通过 |
| 全量 CTest | `ctest --test-dir build --output-on-failure` | common/sql/storage/index/executor 全部通过 | 通过，5/5 tests passed |
| 数据库和表 DDL | `create database school`、`use school`、`create table students (...)`、`drop table`、`drop database` | 数据库上下文正确切换；建表创建 `.idx`；删表删除 `.idx` | 通过 |
| 主键等值索引查询 | 插入 id 1、2 后 `select name from students where id = 2` | 返回 bob，`used_index = true` | 通过 |
| 主键范围索引查询 | `select id ... where id < 30`、`select id ... where id > 20` | 返回有序 RowId 对应行，`used_index = true` | 通过 |
| 非索引扫描查询 | `select * from students where age = 20` | 走全表扫描，返回 bob 和 cora，`used_index = false` | 通过 |
| update 后索引重建 | `update students set id = 5 where id = 2` 后分别查询 id 2 和 id 5 | id 2 查空；id 5 查到 bob；索引可用 | 通过 |
| delete 后索引重建 | `delete students where id < 3` 后 `select id where id > 0` | 已删除行不再返回；剩余 id 3 和 5 | 通过 |
| 主键更新冲突 | `update students set id = 1 where id = 2` | 写入前返回 `AlreadyExists`，原 id 2 行仍可查 | 通过 |

### 性能测试

| 测试内容 | 输入规模 | 验证命令 | 预期结果 | 实际结果 |
| -------- | -------- | -------- | -------- | -------- |
| Executor 批量 insert/select 性能 | 未定义 | 未运行 | 后续可增加固定规模 SQL 脚本和计时 | 未运行；Phase 6 只做正确性、错误路径和 sanitizer 验证 |

### 错误测试

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |
| insert 类型错误 | `insert students values ("bad", "alice", 18)` | 返回 `StatusCode::kInvalidArgument` | 通过 |
| select 未知列 | `select missing from students` | 返回 `StatusCode::kInvalidArgument` | 通过 |
| where 类型错误 | `select id from students where age = "old"` | 返回 `StatusCode::kInvalidArgument`，即使表为空也先校验 where | 通过 |
| update 未知列 | `update students set missing = 1` | 返回 `StatusCode::kInvalidArgument` | 通过 |
| 重复主键 insert | 已有 id 2 后再次插入 id 2 | 返回 `StatusCode::kAlreadyExists` | 通过 |

### 验证命令

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
rg "std::vector|std::map|std::set|std::unordered|std::list|std::deque|std::array|std::forward_list|std::span|std::stack|std::queue|std::priority_queue|#include <vector>|#include <map>|#include <set>|#include <unordered_map>|#include <unordered_set>|#include <list>|#include <deque>|#include <array>|#include <span>|#include <stack>|#include <queue>" include src tests
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

结果摘要：
- `cmake -S . -B build`: 通过，普通构建目录配置完成。
- `cmake --build build`: 通过，executor 库和测试目标构建完成。
- `ctest --test-dir build --output-on-failure`: 通过，5/5 tests passed。
- no-STL 扫描：通过，`include src tests` 下无 forbidden STL 容器匹配。
- `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON`: 通过，ASan/UBSan 构建目录生成完成。
- `cmake --build build-asan`: 通过，ASan/UBSan 版本全部目标构建完成。
- `ctest --test-dir build-asan --output-on-failure`: 通过，5/5 tests passed，未见 ASan/UBSan 报错。

### AI 辅助测试说明

- AI 如何辅助测试：根据 Phase 6 Executor 集成需求设计 executor 单元/集成测试，覆盖 DDL、DML、索引路径、扫描路径、update/delete 后索引重建和错误输入。
- AI 给出的建议：Executor 返回结构化 `QueryResult`，CLI/Network 后续只负责格式化；B+树尚无删除 API，因此 update/delete 后按 live rows 重建主键索引。
- 最终选择：采纳上述测试；批量性能脚本和网络/CLI smoke 测试留到后续阶段。

### 遗留测试问题

- 尚未实现 Executor 批量性能测试。
- 尚未实现 Phase 7 Network/CLI 端到端 socket smoke 测试。
