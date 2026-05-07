# 《C++现代程序设计》课程大作业需求整理



---

# 第一部分：项目任务具体内容

## 一、项目目标

使用 C++ 开发一个“微型关系型数据库管理系统（Mini DBMS）”，整体功能对标 MySQL。

项目要求：

- 使用团队协作开发
- 必须自行实现核心数据结构与逻辑
- 需要编写单元测试
- 系统需要支持基本 SQL
- 必须采用 C/S（客户端 / 服务端）架构
- 建议使用 Vibe Coding / AI 辅助开发

---

# 二、系统整体要求

## 1. 系统架构要求

系统必须采用：

- Client / Server 架构

推荐方案：

```text
CLI客户端  <----TCP/IP---->  DB服务器
```

基本流程：

1. 用户在客户端输入 SQL
2. 客户端通过网络发送 SQL 到服务端
3. 服务端解析 SQL
4. 服务端执行 SQL
5. 服务端返回结果集
6. 客户端格式化显示结果（类似 MySQL）

---

## 2. 网络要求

推荐：

- TCP/IP 通信

允许：

- 第三方网络库

数据交换格式：

- 自定义
- 推荐 JSON

因此系统需要：

- 序列化
- 反序列化

---

# 三、存储系统要求

## 1. 文件存储

数据库必须基于文件系统存储。

示例：

```text
data/
├── person/
│   ├── person.dat
│   └── person.idx
└── other/
```

说明：

- `.dat`：表数据文件
- `.idx`：索引文件

目录结构与文件格式允许自行设计。

---

# 四、SQL 功能要求

---

## 1. DDL（数据库定义语言）

### （1）create database

功能：

- 创建数据库

语法：

```sql
create database <dbname>
```

---

### （2）drop database

功能：

- 删除数据库

语法：

```sql
drop database <dbname>
```

---

### （3）use

功能：

- 切换数据库

语法：

```sql
use <dbname>
```

---

### （4）create table

功能：

- 创建数据表

语法：

```sql
create table <table-name> (
    <column> <type> [primary],
    ...
)
```

要求：

### 表名与列名：

- 全英文小写
- 不允许 `_`
- 不允许特殊字符

### 数据类型：

至少支持：

```text
int
string
```

说明：

- int：使用 C++ 默认 int
- string：最长 256 字符
- UTF-8 编码

### 主键要求：

如果字段为：

```text
primary
```

则必须建立索引。

索引结构：

```text
B+树
```

---

### （5）drop table

功能：

- 删除表

语法：

```sql
drop table <table-name>
```

要求：

- 删除表时同时删除索引

---

# 五、DML（数据库操作语言）

---

## 1. select

功能：

- 查询数据

语法：

```sql
select <column> from <table> [where <cond>]
```

条件格式：

```sql
<column> <op> <const-value>
```

操作符：

```text
=
<
>
```

要求：

- 支持 `*`
- where 可选
- 如果存在索引，需要优先使用索引

---

## 2. delete

功能：

- 删除记录

语法：

```sql
delete <table> [where <cond>]
```

---

## 3. insert

功能：

- 插入记录

语法：

```sql
insert <table> values (...)
```

要求：

- 字符串必须使用双引号

示例：

```sql
insert person values(1001, "peter")
```

---

## 4. update

功能：

- 更新记录

语法：

```sql
update <table>
set <column> = <const-value>
[where <cond>]
```

要求：

- where 可选
- 字符串使用双引号

---

# 六、交互界面要求

需要模仿 MySQL CLI。

示例：

```text
> create table person (id int primary, name string)

> use person

> insert person values(1001, "peter")

> select name from person where id = 1001

> exit
```

要求：

- 反馈信息清晰
- 类似 MySQL 风格

---

# 七、核心类设计（建议）

建议至少包含：

| 类名     | 作用       |
| -------- | ---------- |
| column   | 表字段     |
| row      | 表中的一行 |
| table    | 数据表     |
| index    | B+树索引   |
| database | 数据库     |

建议额外设计：

- SQLParser
- Lexer
- Executor
- StorageEngine
- NetworkEngine
- BufferManager
- UnitTest

---

# 八、编码要求（非常重要）

## 1. 禁止使用 STL 容器

禁止：

```text
vector
map
set
unordered_map
unordered_set
list
deque
array
forward_list
span
```

以及所有容器适配器。

说明：

必须自己实现数据结构。

例如：

- 动态数组
- 链表
- 哈希表
- B+树

都需要自己实现。

---

## 2. C++ 标准

最低：

```text
C++20
```

推荐：

```text
C++23
```

---

## 3. 构建要求

必须：

```text
CMake
```

---

## 4. 开发环境

必须：

```text
Linux
```

---

## 5. Git 要求

推荐：

- Git
- Gitee

---

# 九、测试要求

必须：

- 编写单元测试
- 验证功能正确性

建议测试：

- SQL解析
- 文件存储
- 索引
- 网络通信
- 并发
- 错误处理

---

# 十、AI/Vibe Coding 要求

允许使用 AI。

如果使用 AI：

报告中必须说明：

- 使用的平台
- 使用的模型
- 使用的 Agent
- Prompt / Skills
- AI 在哪些部分参与开发

---

# 第二部分：项目报告模板（建议喂给 AI 用于生成报告）

# 报告结构

## 第一章 引言

需要内容：

### 1. 项目背景

说明：

- 为什么要做 Mini DBMS
- 数据库的重要性
- MySQL 的意义
- 本项目的学习价值

建议：

不少于 100 字。

---

### 2. 术语与缩写解释

例如：

| 术语 | 解释           |
| ---- | -------------- |
| DBMS | 数据库管理系统 |
| SQL  | 结构化查询语言 |
| B+树 | 数据库索引结构 |

---

# 第二章 系统需求

需要内容：

- 系统功能分析
- 功能结构图
- 用户需求
- 系统目标

要求：

- 使用 OOA（面向对象分析）

建议包含：

- 用例图
- 功能模块图

---

# 第三章 系统设计

需要内容：

- 系统架构设计
- 模块划分
- 类设计
- 类图
- 网络设计
- 存储设计
- 索引设计
- SQL解析设计

要求：

- 使用 OOD（面向对象设计）

如果使用 AI：

需要说明：

- 使用的 AI 平台
- Agent
- Prompt
- 开发流程

---

# 第四章 系统实现

需要内容：

- 核心代码
- 核心功能流程
- 关键算法
- 截图

建议重点：

- SQL解析器
- B+树
- 文件存储
- 网络模块
- 查询执行器

要求：

- 代码不能太长
- 每段代码最好不超过 1 页

如果使用 AI：

每个核心功能前说明：

- 模型名
- Agent名
- Prompt

---

# 第五章 系统测试

需要内容：

- 测试用例
- 测试结果
- 性能测试
- 错误测试

建议：

使用表格：

| 测试内容 | 输入 | 预期结果 | 实际结果 |
| -------- | ---- | -------- | -------- |

如果使用 AI：

说明：

- AI 如何辅助测试
- AI 给出的建议
- 你最终如何选择

---

# 第六章 结语

需要内容：

- 项目总结
- 项目不足
- 后续优化方向
- 成员心得

---

# 参考文献要求

至少：

```text
5篇
```

建议包含：

- 数据库原理
- C++20
- B+树
- 网络编程
- MySQL

