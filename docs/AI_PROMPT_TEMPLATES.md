# Mini DBMS AI Prompt Templates

本文档保存可写入课程报告的 Prompt 模板。所有模板只引用公开项目上下文，不引用个人自用操作手册。

## 1. 通用阶段 Prompt

```text
请使用 <skill-name> skill。

请先阅读：
- docs/PROJECT_SPEC.md
- docs/AI_PROJECT_CONTEXT.md
- 当前相关源码

当前阶段：
<填写阶段名称>

请先输出：
1. 你会检查哪些文件。
2. 你计划修改或新增哪些文件。
3. 本阶段的验收标准。

然后开始实现。每完成一部分，请运行相关测试，或说明无法运行的原因。
如果本阶段涉及手写内存管理，请优先运行 ASan/UBSan 构建，或说明未运行原因。
```

## 2. 架构设计 Prompt

```text
你现在扮演 mini-dbms-architect。

请先阅读：
- docs/PROJECT_SPEC.md
- docs/AI_PROJECT_CONTEXT.md

请为 C++ Mini DBMS 设计一个可实现的系统架构。

请输出：
1. 推荐目录结构。
2. 核心模块划分。
3. 每个模块的职责。
4. 核心类列表和职责。
5. 模块之间的数据流。
6. 最小可行版本 MVP 的实现顺序。
7. 哪些地方最容易违反禁止 STL 容器要求。
8. 可以直接写入课程报告的系统设计说明。

不要写完整代码，只输出设计。
```

## 3. 阶段审查 Prompt

```text
请使用 cpp-no-stl-reviewer 和 unit-test-designer 的思路审查当前阶段。

请检查：
1. 是否违反禁止 STL 容器要求。
2. 是否满足 docs/PROJECT_SPEC.md 和 docs/AI_PROJECT_CONTEXT.md 中对应功能。
3. 是否有必要测试。
4. 是否存在跨模块接口风险。
5. 哪些问题必须现在修，哪些可以留到后续阶段。

请优先列出问题，给出文件路径和修复建议。
```

## 4. Git 收尾 Prompt

```text
请使用 git-workflow-manager skill。

请先阅读：
- docs/AI_PROJECT_CONTEXT.md

当前阶段：
<填写阶段名称>

请执行 Git 收尾：
1. 查看 git status。
2. 查看本阶段相关 diff。
3. 判断哪些文件应该进入同一个 commit。
4. 给出建议 commit message。
5. 如果工作区只有本阶段相关改动，请完成 git add 和 git commit。
6. 提交后输出 commit hash、改动摘要、验证命令和遗留问题。

不要 push，除非我明确要求。
不要回退任何未确认的改动。
```

## 5. 基础数据结构 Prompt

```text
你现在扮演 cpp-no-stl-data-structure-builder。

请先阅读：
- docs/PROJECT_SPEC.md
- docs/AI_PROJECT_CONTEXT.md
- 现有 common、tests、CMake 配置，如果存在

请实现本项目后续模块可复用的基础设施：
1. DynamicArray<T>
2. HashTable<K, V>
3. 必要的 LinkedList<T>
4. Status 或等价错误类型
5. StringUtils::Split 或等价字符串切分工具，返回项目自研数组结构
6. 不依赖 forbidden STL 容器的轻量测试框架，例如 TEST_CASE、EXPECT_EQ、EXPECT_TRUE
7. CMake Debug/ASan 选项，用于 -fsanitize=address,undefined -g

不要使用 GoogleTest/Catch2，除非课程教师明确允许测试目录使用第三方测试库。

请添加测试并说明：
1. 如何运行普通测试。
2. 如何运行 ASan/UBSan 测试。
3. 如何扫描 forbidden STL 容器。
```

## 6. 存储引擎 Prompt 补充

```text
实现 StorageEngine 时，请不要直接把普通 C++ struct fwrite 到文件作为长期持久化格式。

请优先手动按字段序列化；如果使用 packed struct，必须使用 #pragma pack(push, 1) / #pragma pack(pop)，并在 docs/STORAGE_FORMAT.md 中解释如何避免 struct padding 和未初始化内存问题。
```
