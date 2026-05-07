# Mini DBMS AI Project Context

本文档是所有 AI 对话共享的项目上下文。开启新对话时，不需要反复粘贴完整课程要求，只需要要求 AI 阅读本文档和 `docs/PROJECT_SPEC.md`。

## 1. 项目定位

本项目是《C++现代程序设计》课程大作业，目标是实现一个 Mini DBMS。项目允许使用 AI/Vibe Coding，但最终代码、测试和报告必须与课程要求一致，并能够解释 AI 参与方式和人工审查过程。

## 2. 必须满足的硬性要求

- 使用 C++20 或 C++23。
- 使用 CMake 构建。
- 运行环境为 Linux。
- 采用 Client/Server 架构。
- CLI 客户端通过 TCP/IP 与 DB 服务端通信。
- 数据必须基于文件系统持久化。
- 支持基本 SQL：
  - `create database`
  - `drop database`
  - `use`
  - `create table`
  - `drop table`
  - `insert`
  - `select`
  - `delete`
  - `update`
- 支持 `int` 和 `string` 类型。
- `string` 最长 256 字符，UTF-8 编码。
- `primary` 字段必须建立 B+树索引。
- 必须自行实现核心数据结构。
- 必须编写单元测试。
- 代码结构要便于课程报告说明。

## 3. STL 容器限制

课程要求禁止使用 STL 容器。代码中不得使用：

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
- 容器适配器，例如 `std::stack`、`std::queue`、`std::priority_queue`

通常可以使用：

- `std::string`
- `std::iostream`
- `std::fstream`
- `std::filesystem`
- `std::thread`
- `std::mutex`
- 智能指针

如果课程教师后续给出更严格要求，以教师要求为准。

## 4. 自研数据结构要求

项目应提供自己的基础数据结构，例如：

- `DynamicArray<T>` 替代 `std::vector`
- `HashTable<K, V>` 替代 `std::unordered_map`
- 必要时实现链表
- B+树索引
- 基于 `DynamicArray<std::string>` 或等价自研结构的字符串切分工具，例如 `StringUtils::Split`

所有模块应优先使用项目内自研结构。

## 5. 测试与调试约束

单元测试也要注意 STL 容器限制。除非课程教师明确允许测试目录使用 GoogleTest、Catch2 等第三方框架，否则默认实现一个轻量级自研测试框架，例如：

- `TEST_CASE(name)`
- `EXPECT_TRUE(expr)`
- `EXPECT_EQ(expected, actual)`
- `EXPECT_STATUS_OK(status)`

测试框架本身不得依赖 forbidden STL 容器。

CMake 应提供 Debug/ASan 构建方式，便于发现手写数据结构中的内存问题。建议在 Debug 或专门选项中启用：

```text
-fsanitize=address,undefined -g
```

阶段验收时，尽量保证相关测试在 ASan/UBSan 下无报错。

## 6. 持久化与序列化原则

存储引擎不得随意把普通 C++ struct 直接 `fwrite` 到文件中作为长期格式，因为 struct padding、字节对齐和未初始化内存会导致持久化格式不稳定。

优先使用手动按字段序列化；如必须使用 packed struct，必须明确处理 `#pragma pack(push, 1)` / `#pragma pack(pop)`，并在存储格式文档中说明。

## 7. AI 协作原则

AI 可以参与：

- 架构设计
- 模块实现
- 代码审查
- 测试设计
- 报告整理

但每个阶段都需要人工审查，并记录：

- 使用的模型/平台
- 使用的 Agent/Skill
- 关键 Prompt
- AI 产出内容
- 人工检查和修改
- 测试结果

## 8. 推荐给 AI 的默认工作方式

AI 在执行任务前应：

1. 阅读相关文档和现有代码。
2. 明确当前阶段目标。
3. 列出计划修改的文件。
4. 明确验收标准。
5. 实现后运行相关构建或测试。
6. 检查是否违反 STL 容器限制。
7. 对涉及内存管理的代码，优先运行 ASan/UBSan 构建或说明未运行原因。
