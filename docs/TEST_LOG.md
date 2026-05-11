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
