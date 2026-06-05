# Mini DBMS

一个基于 C++20 实现的微型关系型数据库管理系统课程项目，采用 TCP Client/Server 架构，支持文件持久化、基础 SQL 子集、主键 B+ 树索引以及单元测试与端到端验收。

## 项目概览

- 服务端 `db_server` 负责接收 SQL、解析、执行并返回结果。
- 客户端 `db_client` 通过 TCP 连接服务端，以接近 MySQL 的表格形式展示查询结果。
- 数据落盘到文件系统，表由 `.meta`、`.dat`、`.idx` 文件组成。
- 主键索引当前使用自研 B+ 树，仅支持 `int primary`。
- 项目按课程要求实现了自研基础数据结构与轻量测试框架。

## 已实现功能

### SQL 功能

当前支持以下 SQL 子集：

```sql
create database school;
drop database school;
use school;

create table students (id int primary, name string, age int);
drop table students;

insert students values (1, "alice", 18);
select * from students;
select name from students where id = 2;
select * from students where age = 20;

update students set age = 21 where id = 2;
delete students where id < 2;
```

说明：

- 支持数据类型：`int`、`string`
- `string` 最大长度为 256 字节
- `where` 目前只支持 `=`, `<`, `>`
- `select` 支持 `*` 和列名列表
- 主键等值/范围查询会优先使用 B+ 树索引

### 系统能力

- TCP Client/Server 通信
- 数据库、表、记录的文件持久化
- 主键唯一性校验
- 基于主键索引的精确查询与范围查询
- `update` / `delete` 后索引重建与同步
- 单元测试、集成测试、网络协议测试、端到端验收脚本

## 技术架构

项目主要模块如下：

- `common`：`Status`、`DynamicArray`、`HashTable`、`LinkedList`、字符串工具
- `sql`：Lexer、Parser、SQL 命令结构
- `storage`：数据库目录、schema、行数据读写、持久化
- `index`：主键 B+ 树索引及 `.idx` 文件保存/加载
- `executor`：串联 SQL、存储与索引，执行 DDL/DML
- `network`：TCP 帧协议收发
- `server`：数据库服务端入口
- `client`：命令行客户端与表格输出
- `tests`：自研测试框架与各模块测试

更详细的设计文档见：

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/STORAGE_FORMAT.md](docs/STORAGE_FORMAT.md)
- [docs/INDEX_DESIGN.md](docs/INDEX_DESIGN.md)

## 目录结构

```text
mini_dbms/
├── CMakeLists.txt
├── README.md
├── docs/
├── include/
├── src/
├── tests/
└── scripts/
```

常用目录说明：

- `include/`：头文件
- `src/`：实现代码
- `tests/`：单元测试与集成测试
- `scripts/`：演示与端到端脚本
- `docs/`：课程报告、架构、存储格式、测试记录等文档

## 构建环境

- Linux
- CMake 3.16+
- 支持 C++20 的编译器（GCC / Clang）

## 构建

```bash
cmake -S . -B build
cmake --build build
```

默认会构建：

- 库：`mini_dbms_sql`、`mini_dbms_storage`、`mini_dbms_index`、`mini_dbms_executor`、`mini_dbms_network`
- 可执行文件：`db_server`、`db_client`
- 测试目标：开启 `MINI_DBMS_BUILD_TESTS=ON` 时自动构建

如果只想构建主程序：

```bash
cmake -S . -B build -DMINI_DBMS_BUILD_TESTS=OFF
cmake --build build
```

## 运行方式

### 1. 启动服务端

```bash
./build/db_server 54321 ./build/demo_data
```

参数说明：

- 第一个参数：监听端口，默认 `54321`
- 第二个参数：数据根目录，默认 `data`

### 2. 启动客户端

```bash
./build/db_client 127.0.0.1 54321
```

进入交互后，每行输入一条 SQL，输入 `exit` 或 `quit` 退出。

### 3. 示例会话

```sql
create database school;
use school;
create table students (id int primary, name string, age int);
insert students values (1, "alice", 18);
insert students values (2, "bob", 20);
select * from students where id > 0;
select name from students where id = 2;
update students set age = 21 where id = 2;
delete students where id < 2;
drop table students;
drop database school;
exit
```

## 测试

### 运行全部测试

```bash
ctest --test-dir build --output-on-failure
```

当前测试覆盖：

- Common 数据结构与工具
- SQL Lexer / Parser
- Storage 持久化与 CRUD
- B+ 树索引
- Executor 集成执行
- Network 协议
- 端到端 C/S 验收

### AddressSanitizer / UBSanitizer

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

### 演示脚本

自动构建并跑一遍手工演示流程：

```bash
bash scripts/manual_demo.sh
```

端到端验收脚本：

```bash
scripts/e2e_acceptance.sh ./build/db_server ./build/db_client ./build/e2e_data
```

## 数据文件格式

每个数据库对应一个目录，每个表当前包含以下文件：

```text
data/
└── <database>/
    ├── <table>.meta
    ├── <table>.dat
    └── <table>.idx
```

- `.meta`：表结构定义
- `.dat`：行数据文件
- `.idx`：主键索引文件

当前采用按字段逐项序列化的方式写入文件，不直接写原生结构体，避免 padding 和平台布局差异问题。

## 当前约束与已知限制

- 标识符只能使用小写英文字母，不支持大写、下划线和特殊字符
- 主键索引当前只支持 `int primary`，`string primary` 会被显式拒绝
- `delete` 语法为 `delete <table> [where ...]`，当前不支持 `delete from <table>`
- 不支持 `join`、`order by`、`group by`、事务、权限管理
- 服务端当前按连接串行处理，不是高并发实现
- 客户端当前按“单行一条 SQL”交互，不支持多行 SQL 输入

## 文档索引

- [docs/PROJECT_SPEC.md](docs/PROJECT_SPEC.md)：课程需求整理
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)：总体架构设计
- [docs/STORAGE_FORMAT.md](docs/STORAGE_FORMAT.md)：文件存储格式
- [docs/INDEX_DESIGN.md](docs/INDEX_DESIGN.md)：B+ 树索引设计
- [docs/TEST_PLAN.md](docs/TEST_PLAN.md)：测试计划
- [docs/TEST_LOG.md](docs/TEST_LOG.md)：测试记录
- [docs/COURSE_REPORT_DRAFT.md](docs/COURSE_REPORT_DRAFT.md)：课程报告草稿

## 项目状态

当前仓库已经具备课程要求范围内的 MVP/验收版本能力：可构建、可运行、可通过网络执行基础 SQL、支持文件持久化，并具备测试与演示脚本。
