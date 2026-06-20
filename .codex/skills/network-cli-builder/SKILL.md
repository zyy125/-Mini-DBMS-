---
name: network-cli-builder
description: Use when implementing or revising the Mini DBMS TCP server, TCP client, request/response protocol, CLI loop, MySQL-style output, and network smoke tests.
---

# Network CLI Builder

中文说明：这是“网络和命令行客户端”skill。用于实现 TCP 服务端、TCP 客户端、请求响应协议和类 MySQL 交互。

## Mission

Expose the working Executor through a simple Linux TCP Client/Server interface and a usable CLI.

中文任务：把已经可用的 Executor 包装成 C/S 架构，让用户通过 CLI 输入 SQL。

## Responsibilities

- `db_server` accepts TCP connections and executes SQL.
- `db_client` reads user input and sends SQL to server.
- Response protocol carries success/error and result rows.
- CLI formats output in a MySQL-like style.

中文职责：

- `db_server` 监听 TCP 连接，接收 SQL，调用 Executor。
- `db_client` 读取用户输入，发送给服务端。
- 协议能表达成功、失败和查询结果。
- CLI 输出尽量接近 MySQL 风格。

## Boundaries

- Network layer does not implement SQL logic.
- CLI does not access storage files directly.
- Server owns the Executor instance.

中文边界：

- 网络层不实现 SQL 业务逻辑。
- CLI 不直接访问数据文件。
- 服务端持有 Executor。

## Protocol Guidance

Prefer a simple length-prefixed text protocol:

- 4-byte payload length.
- UTF-8 SQL request or response body.

Use JSON-style text only if it does not introduce heavy dependencies or STL container pressure.

中文协议建议：

- 优先用简单长度前缀文本协议。
- 例如 4 字节长度 + UTF-8 内容。
- 如果 JSON 会引入复杂依赖或 STL 容器压力，就不要用 JSON。

## CLI Requirements

- Prompt similar to `mini_dbms>`.
- `exit` exits client.
- Clear messages for success and errors.
- Select results display as simple tables.

中文 CLI 要求：

- 有类似 `mini_dbms>` 的提示符。
- 输入 `exit` 退出。
- 成功和错误信息清晰。
- `select` 结果用简单表格展示。

## Acceptance

- Server starts on a configurable port.
- Client can send multiple SQL statements.
- Client can exit cleanly.
- Manual demo script is documented.
- Network tests or smoke scripts exist.

中文验收标准：

- 服务端可以指定端口启动。
- 客户端能连续发送多条 SQL。
- 客户端能正常退出。
- 有手动演示命令或脚本。
- 有网络 smoke test 或说明。
