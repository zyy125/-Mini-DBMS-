# Mini DBMS Architecture

当前阶段：Phase 1 架构与目录设计。

本阶段目标是确定项目的模块边界、核心对象、调用关系、MVP 顺序和后续接口契约。本文档只描述设计，不实现 Parser、Storage、Index 或其他业务逻辑。

## 1. 推荐目录结构

```text
mini_dbms/
├── CMakeLists.txt
├── README.md
├── data/
│   └── .gitkeep
├── docs/
│   ├── AI_PROJECT_CONTEXT.md
│   ├── ARCHITECTURE.md
│   ├── PROJECT_SPEC.md
│   └── ...
├── include/
│   ├── common/
│   │   └── .gitkeep
│   ├── sql/
│   │   └── .gitkeep
│   ├── storage/
│   │   └── .gitkeep
│   ├── index/
│   │   └── .gitkeep
│   ├── executor/
│   │   └── .gitkeep
│   ├── network/
│   │   └── .gitkeep
│   ├── client/
│   │   └── .gitkeep
│   └── server/
│       └── .gitkeep
├── src/
│   ├── common/
│   │   └── .gitkeep
│   ├── sql/
│   │   └── .gitkeep
│   ├── storage/
│   │   └── .gitkeep
│   ├── index/
│   │   └── .gitkeep
│   ├── executor/
│   │   └── .gitkeep
│   ├── network/
│   │   └── .gitkeep
│   ├── client/
│   │   └── .gitkeep
│   └── server/
│       └── .gitkeep
├── tests/
│   ├── common/
│   │   └── .gitkeep
│   ├── sql/
│   │   └── .gitkeep
│   ├── storage/
│   │   └── .gitkeep
│   ├── index/
│   │   └── .gitkeep
│   ├── executor/
│   │   └── .gitkeep
│   ├── network/
│   │   └── .gitkeep
│   └── integration/
│       └── .gitkeep
└── scripts/
    └── .gitkeep
```

目录约定：

| 路径 | 说明 |
| --- | --- |
| `include/common`, `src/common` | 通用状态码、自研容器、字符串工具、序列化基础工具。 |
| `include/sql`, `src/sql` | Lexer、Parser、SQL 命令结构，不访问文件和网络。 |
| `include/storage`, `src/storage` | 数据库目录、表元数据、行数据文件、`RowId` 管理。 |
| `include/index`, `src/index` | 主键 B+ 树索引和 `.idx` 文件持久化。 |
| `include/executor`, `src/executor` | SQL 执行协调层，连接 Parser、Storage、Index。 |
| `include/network`, `src/network` | TCP socket、请求响应协议、网络序列化。 |
| `include/client`, `src/client` | CLI 输入循环和 MySQL 风格结果显示。 |
| `include/server`, `src/server` | 服务端启动、连接接收、请求分发。 |
| `tests` | 自研轻量测试框架和各模块测试。 |
| `data` | 运行期数据库文件根目录，测试可使用独立临时数据目录。 |
| `scripts` | 构建、测试、ASan 运行、演示脚本。 |

## 2. 模块职责表

| 模块 | 主要职责 | 不应承担的职责 | 主要依赖 |
| --- | --- | --- | --- |
| `common` | `Status`、错误码、自研 `DynamicArray`、`HashTable`、基础字符串工具、字节序列化工具。 | SQL 语义、文件表结构、网络业务协议。 | 无项目内依赖。 |
| `sql` | 词法分析、语法分析、生成命令 AST/结构体、基础语法错误定位。 | 执行 SQL、访问存储、格式化查询结果。 | `common`。 |
| `storage` | 创建/删除数据库和表、读写元数据、插入/读取/更新/删除行、维护稳定 `RowId`。 | SQL 解析、索引算法、客户端显示。 | `common`。 |
| `index` | B+ 树插入、查找、范围查询、删除、索引文件保存/加载。 | 管理表数据文件、解析 where 表达式。 | `common`, `storage` 的 `RowId` 类型契约。 |
| `executor` | 接收 SQL 字符串或 parsed command，执行 DDL/DML，选择是否使用索引，生成 `QueryResult`。 | 直接处理 TCP socket、打印表格 UI。 | `common`, `sql`, `storage`, `index`。 |
| `network` | TCP 连接、请求响应帧、超时和连接关闭处理、网络层序列化。 | SQL 解析和执行。 | `common`。 |
| `server` | 初始化 `StorageEngine`/`Executor`，监听端口，处理客户端请求。 | CLI 展示、Parser 内部逻辑。 | `common`, `network`, `executor`。 |
| `client` | 读取用户输入、发送 SQL、接收响应、格式化输出为 MySQL 风格。 | 直接访问数据库文件或 B+ 树。 | `common`, `network`。 |
| `tests` | 单元测试、集成测试、网络 smoke test、ASan/UBSan 验证入口。 | 生产业务逻辑。 | 被测模块。 |

## 3. 核心类职责表

| 类/结构 | 所属模块 | 职责 | 关键接口契约 |
| --- | --- | --- | --- |
| `Status` | `common` | 表示成功/失败、错误码和错误消息。 | 所有跨模块调用返回 `Status` 或包含 `Status` 的结果对象。 |
| `DynamicArray<T>` | `common` | 替代 `std::vector`，用于 token、列定义、结果行等变长序列。 | 支持追加、按下标访问、大小查询、清空；负责自身内存释放。 |
| `HashTable<K, V>` | `common` | 替代 `std::unordered_map`，用于名字到元数据位置等映射。 | 不暴露 STL 容器；冲突处理和扩容必须可测试。 |
| `StringUtils` | `common` | SQL 辅助切分、大小写检查、标识符校验、字符串转 int。 | 返回自研数组或 `Status`，不返回 STL 容器。 |
| `ByteWriter` / `ByteReader` | `common` | 手动按字段序列化/反序列化整数、字符串、定长头。 | 不直接写入裸 struct padding。 |
| `Token` | `sql` | 表示关键字、标识符、字面量、运算符和符号。 | 保存 token 类型、文本、位置。 |
| `Lexer` | `sql` | 将 SQL 字符串转换为 token 序列。 | 输入 `std::string`，输出 `DynamicArray<Token>`。 |
| `SqlCommand` | `sql` | DDL/DML 命令的统一基类或 tagged union。 | 能区分 create/drop/use/insert/select/delete/update。 |
| `Parser` | `sql` | 将 token 序列转换为命令结构并做语法校验。 | 不检查表是否存在，不访问存储。 |
| `Column` | `storage` | 表字段定义：列名、类型、是否主键、字符串长度约束。 | 名称校验由 Parser 或 Storage 双重防护。 |
| `Schema` | `storage` | 表结构和主键信息。 | 能序列化到表元数据文件。 |
| `Value` | `storage` 或 `common` | 表示 `int` 或 `string` 单元值。 | 类型明确，字符串最长 256 字节或按需求定义的 UTF-8 长度限制。 |
| `Row` | `storage` | 一行记录，按 `Schema` 存放多个 `Value`。 | 使用 `DynamicArray<Value>`，可序列化。 |
| `RowId` | `storage` | 稳定定位一行记录，例如 `{page_or_file_offset, slot}`。 | Index 只保存 `RowId`，不保存整行。 |
| `Table` | `storage` | 表元数据和数据文件操作入口。 | 提供 insert/read/update/delete/scan。 |
| `Database` | `storage` | 单个数据库目录和表集合。 | 管理表创建、删除、加载。 |
| `StorageEngine` | `storage` | 管理 `data/` 根目录、数据库切换、表级操作。 | DDL/DML 存储接口的统一入口。 |
| `BPlusTree` | `index` | 主键索引的核心树结构。 | 支持唯一键插入、精确查找、范围查找、删除。 |
| `IndexManager` | `index` | 按表加载/保存 `.idx`，将主键映射到 `RowId`。 | 与 `StorageEngine` 通过表名和 `RowId` 交互。 |
| `QueryResult` | `executor` | 执行结果：状态、列名、行、受影响行数、消息。 | 服务端序列化响应，客户端只格式化展示。 |
| `Executor` | `executor` | 执行 parsed command，协调 Storage 和 Index。 | 是唯一知道 SQL 语义、存储、索引三者关系的模块。 |
| `Request` / `Response` | `network` | 网络传输对象，包含 SQL 文本、状态、结果载荷。 | 版本化字段，避免协议变动破坏兼容。 |
| `TcpServer` / `TcpClient` | `network` | TCP 收发、连接生命周期、帧边界处理。 | 不解析 SQL 内容。 |
| `CliApp` | `client` | 交互输入、多行 SQL 拼接、发送请求、打印结果。 | 不依赖 `storage` 或 `index`。 |
| `ServerApp` | `server` | 服务端主流程和模块初始化。 | 负责创建 `Executor` 并绑定网络处理。 |
| `TestRunner` | `tests` | 自研测试注册、断言、统计。 | 不使用第三方测试框架和 STL 容器。 |

## 4. 跨模块调用关系

推荐依赖方向：

```text
client  -> network -> common
server  -> network -> common
server  -> executor
executor -> sql
executor -> storage
executor -> index
sql      -> common
storage  -> common
index    -> common
index    -> storage(RowId contract only)
tests    -> target modules
```

核心约束：

| 规则 | 原因 |
| --- | --- |
| `client` 不依赖 `executor`、`storage`、`index`。 | 保持 C/S 架构，客户端不能绕过服务端访问数据。 |
| `network` 不依赖 `sql` 或 `executor`。 | 网络层只负责传输，便于独立测试协议。 |
| `sql` 不依赖 `storage`。 | Parser 只做语法和命令结构，不做语义执行。 |
| `executor` 是唯一协调 `sql`、`storage`、`index` 的模块。 | 避免 SQL 语义散落在多个模块。 |
| `index` 只依赖 `RowId` 契约，不读取表数据文件。 | B+ 树保持为主键到行位置的映射结构。 |
| `storage` 不反向调用 `executor`。 | 存储层只提供原子数据操作。 |

## 5. CLI 输入 SQL 到返回结果的数据流

```text
1. CliApp 读取用户输入
   - 支持 `exit` 退出。
   - 普通 SQL 作为文本保留，不在客户端解析。

2. CliApp 通过 TcpClient 发送 Request
   - Request 至少包含协议版本、请求类型、SQL 文本长度、SQL 文本。
   - TcpClient 负责帧边界和 TCP 发送。

3. TcpServer 接收 Request
   - 网络层只校验协议帧，不理解 SQL。
   - ServerApp 将 SQL 文本交给 Executor。

4. Executor 调用 Lexer 和 Parser
   - Lexer 生成 token。
   - Parser 生成 SqlCommand。
   - 语法错误直接生成失败 QueryResult。

5. Executor 根据 SqlCommand 执行
   - DDL：调用 StorageEngine 创建/删除数据库或表；主键表同时通知 IndexManager 创建/删除索引文件。
   - use：更新当前连接或当前会话的数据库上下文。
   - insert：校验 schema 和 value 类型；写入 StorageEngine；如果有主键，写入 B+ 树索引。
   - select：如果 where 命中主键且操作符为 `=`, `<`, `>`，优先通过 IndexManager 找 RowId；否则通过 StorageEngine 全表扫描。
   - delete：优先使用主键索引定位候选行；删除行后同步删除索引项。
   - update：定位候选行；更新数据文件；若主键值发生变化，需要维护唯一约束和索引项。

6. Executor 生成 QueryResult
   - 包含状态、列名、结果行、受影响行数、提示消息。
   - 不包含 CLI 表格边框格式。

7. ServerApp 通过 TcpServer 返回 Response
   - Response 序列化 QueryResult。
   - 网络层处理发送和错误。

8. CliApp 接收 Response 并格式化显示
   - 成功的 select 显示 MySQL 风格表格。
   - DDL/DML 显示明确状态和 affected rows。
   - 失败显示错误信息。
```

## 6. MVP 实现顺序

| 顺序 | 阶段 | 目标 | 验收方式 |
| --- | --- | --- | --- |
| 1 | 构建骨架 | CMake、目录、基础可执行目标、测试入口。 | `cmake --build` 成功，空测试可运行。 |
| 2 | `common` 基础设施 | `Status`、`DynamicArray`、基础字符串工具、轻量测试框架。 | common 单测通过，禁用 STL 容器扫描通过。 |
| 3 | SQL Lexer/Parser MVP | 支持课程规定 SQL 的命令结构，不执行。 | Parser 单测覆盖 DDL/DML 正常和错误输入。 |
| 4 | Storage MVP | 数据库/表目录、schema 文件、行序列化、insert/scan。 | 存储单测验证重启后可读。 |
| 5 | Executor 无索引 MVP | 串联 Parser 和 Storage，完成 create/use/create table/insert/select 全表扫描。 | 单进程 executor 集成测试通过。 |
| 6 | B+ 树索引 MVP | 主键唯一插入、精确查找、索引文件保存加载。 | index 单测和 insert/select by primary 集成测试通过。 |
| 7 | 完整 DML | delete/update，包含索引同步。 | DML 集成测试覆盖有 where 和无 where。 |
| 8 | Network MVP | TCP request/response，服务端单连接串行处理。 | 网络 smoke test 可发送 SQL 并收到结果。 |
| 9 | CLI MVP | MySQL 风格交互和结果表格。 | 手动 demo 脚本覆盖课程 SQL。 |
| 10 | 稳定化 | ASan/UBSan、错误处理、边界测试、报告材料。 | 全测试在 Debug/ASan 下通过或记录已知限制。 |

MVP 不建议一开始实现并发、复杂事务、BufferManager 或 SQL 扩展语法。若时间充足，可在基本功能稳定后再加入多连接串行队列或简单线程模型。

## 7. 后续阶段依赖的接口契约

### Phase 2: Common 与测试框架

| 契约 | 后续依赖 |
| --- | --- |
| `Status` 必须能表达成功、错误码、错误消息。 | 所有模块统一错误返回。 |
| `DynamicArray<T>` 的所有权、拷贝/移动策略必须明确。 | Parser token、schema 列、结果集、行值都依赖它。 |
| `StringUtils` 不返回 STL 容器。 | Parser 和 CLI 输入处理依赖它。 |
| 测试框架不使用禁用 STL 容器。 | 后续所有模块测试依赖它。 |

### Phase 3: SQL Parser

| 契约 | 后续依赖 |
| --- | --- |
| `Parser::Parse(sql)` 返回 `SqlCommand` 或错误 `Status`。 | Executor 依赖命令结构，不重新解析字符串。 |
| 标识符规则统一：小写英文、不含 `_` 和特殊字符。 | Storage 创建数据库/表时复核同一规则。 |
| `ValueLiteral` 区分 int 和 string。 | Storage 类型校验和序列化依赖它。 |
| where 条件结构统一为 `{column, op, const_value}`。 | Executor 判断是否可使用主键索引。 |

### Phase 4: Storage

| 契约 | 后续依赖 |
| --- | --- |
| `RowId` 在行未被物理重写前稳定。 | B+ 树索引保存 `RowId`。 |
| schema 文件格式版本化。 | 重启加载和报告说明依赖它。 |
| 行序列化按字段写入，不写裸 struct。 | 跨平台和长期文件稳定性依赖它。 |
| 删除策略明确：墓碑标记或空闲链表二选一。 | Index 删除、scan 过滤依赖它。 |

### Phase 5: Executor

| 契约 | 后续依赖 |
| --- | --- |
| `Executor::Execute(sql, session)` 返回 `QueryResult`。 | Server 网络处理直接调用。 |
| `QueryResult` 不包含 CLI 表格字符。 | Client 可独立决定显示样式。 |
| `SessionContext` 保存当前数据库。 | `use` 命令和多连接扩展依赖它。 |
| 主键唯一冲突在插入存储前或提交前被检测。 | Storage 和 Index 一致性依赖它。 |

### Phase 6: Index

| 契约 | 后续依赖 |
| --- | --- |
| `IndexManager::Insert(table, key, row_id)` 对重复主键返回错误。 | insert 主键约束依赖它。 |
| `FindEqual` 返回单个 `RowId` 或不存在。 | select/update/delete by primary 依赖它。 |
| `FindLessThan` / `FindGreaterThan` 返回 `DynamicArray<RowId>`。 | 范围 where 查询依赖它。 |
| `.idx` 文件和表 schema 的主键定义一致。 | drop table、重启加载和错误恢复依赖它。 |

### Phase 7: Network 与 CLI

| 契约 | 后续依赖 |
| --- | --- |
| Request/Response 有长度前缀或等价帧边界。 | TCP 粘包/拆包处理依赖它。 |
| Response 能承载 `QueryResult` 的状态、列、行、消息。 | CLI 格式化输出依赖它。 |
| CLI 不解析 SQL，只处理 `exit` 和输入拼接。 | 服务端保持唯一 SQL 语义来源。 |
| 网络错误和 SQL 错误分开表示。 | 用户反馈和测试定位依赖它。 |

## 8. 风险说明

| 风险 | 影响 | 规避策略 |
| --- | --- | --- |
| 禁止 STL 容器范围不清。 | 误用 `std::vector`、`std::map` 或测试框架内部容器，课程验收风险高。 | 代码评审和脚本扫描 forbidden STL；项目代码统一使用 `DynamicArray`、`HashTable` 等自研结构。 |
| 自研容器内存错误。 | 越界、悬垂指针、二次释放会影响 Parser、Storage、Index 全部模块。 | 先实现并测试 `common`；Debug/ASan/UBSan 构建作为阶段验收；明确拷贝/移动语义。 |
| 第三方测试框架可能使用 STL 容器。 | 即使业务代码合规，测试代码也可能不满足教师要求。 | 默认自研轻量测试框架；若教师明确允许，再考虑 GoogleTest/Catch2。 |
| B+ 树删除和持久化复杂度高。 | 容易延期并引入索引/数据不一致。 | MVP 先做唯一插入、精确查找、范围查找和保存加载；删除先保证正确，可接受简单重建索引作为早期策略。 |
| Storage 直接写裸 struct。 | padding、字节序、未初始化字段会导致文件格式不稳定。 | 使用 `ByteWriter`/`ByteReader` 按字段序列化；文件头写 magic、version、schema 信息。 |
| `string` 长度与 UTF-8 计数定义不清。 | 中文或多字节字符可能产生边界错误。 | MVP 明确按 UTF-8 字节数限制 256；如教师要求字符数，再单独实现 UTF-8 字符计数。 |
| 更新主键导致索引不一致。 | select/delete 可能找不到真实行或命中旧行。 | Executor 对主键更新采用先检查新键唯一性，再更新存储和索引；失败时保持旧数据。 |
| 网络层与执行层耦合。 | 难以单元测试，CLI 可能绕过协议。 | Network 只传输 Request/Response；Executor 单进程可独立测试。 |
| 多连接并发过早引入。 | 文件锁、索引同步、SessionContext 复杂度上升。 | MVP 使用单连接或串行请求；并发作为扩展，不作为第一阶段目标。 |
| 错误处理不统一。 | CLI、测试和报告难以解释失败原因。 | 所有模块统一返回 `Status`/`QueryResult`，错误码分层定义。 |

## 9. Phase 1 验收标准

| 检查项 | 期望 |
| --- | --- |
| 架构文档 | `docs/ARCHITECTURE.md` 覆盖目录、模块、类、调用流、MVP、接口契约和风险。 |
| 目录骨架 | `include/`、`src/`、`tests/` 按模块创建，占位文件不包含业务实现。 |
| 模块边界 | Parser、Storage、Index、Network、CLI、Executor 职责清晰且依赖方向单向。 |
| 课程约束 | C/S、TCP、文件持久化、B+ 树、禁用 STL 容器、单元测试、ASan 风险均被纳入设计。 |
