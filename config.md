# Mini DBMS 运行环境配置

本文件用于记录 Mini DBMS 项目的运行环境、编译环境和基本构建方式，满足课程关于“运行环境配置”说明的要求。

## 1. 项目位置

- 项目名称：Mini DBMS
- 源代码目录：`/home/zhuyin/mini_dbms`

## 2. 操作系统环境

- 操作系统：Linux
- 运行内核：`6.18.33.1-microsoft-standard-WSL2`
- 系统架构：`x86_64`
- 运行方式：WSL2 Linux 环境

说明：本项目在 Linux 环境下开发与运行，符合课程要求。

## 3. 编译环境

- 编译器：`g++`
- 编译器版本：`g++ 13.3.0`
- 构建工具：`CMake 3.28.3`
- C++ 标准：`C++20`

## 4. 项目构建配置

项目顶层构建文件为：`CMakeLists.txt`

主要配置如下：

- 最低 CMake 版本：`3.16`
- 项目语言：`CXX`
- C++ 标准：`20`
- 关闭编译器扩展：`CMAKE_CXX_EXTENSIONS OFF`
- 默认警告选项：`-Wall -Wextra -Wpedantic`

可选构建开关：

- `MINI_DBMS_BUILD_TESTS`
  说明：是否构建单元测试，默认值为 `ON`
- `MINI_DBMS_ENABLE_ASAN`
  说明：是否启用 AddressSanitizer 和 UndefinedBehaviorSanitizer，默认值为 `OFF`

## 5. 生成目标

项目主要生成以下目标：

- 静态库：`mini_dbms_sql`
- 静态库：`mini_dbms_storage`
- 静态库：`mini_dbms_index`
- 静态库：`mini_dbms_executor`
- 静态库：`mini_dbms_network`
- 可执行程序：`db_server`
- 可执行程序：`db_client`

## 6. 构建命令

### 6.1 普通构建

```bash
cmake -S . -B build
cmake --build build
```

### 6.2 不构建测试的主程序构建

```bash
cmake -S . -B build -DMINI_DBMS_BUILD_TESTS=OFF
cmake --build build
```

### 6.3 启用 ASan/UBSan 的调试构建

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINI_DBMS_ENABLE_ASAN=ON
cmake --build build-asan
```

## 7. 运行方式

### 7.1 启动服务端

```bash
./build/db_server 54321 ./build/report_data
```

参数说明：

- `54321`：服务端监听端口
- `./build/report_data`：数据库文件存储根目录

### 7.2 启动客户端

```bash
./build/db_client 127.0.0.1 54321
```

参数说明：

- `127.0.0.1`：服务端地址
- `54321`：服务端端口

## 8. 依赖说明

本项目主要依赖以下环境能力：

- Linux 文件系统
- C++20 编译器
- CMake 构建系统
- POSIX Socket/TCP 网络接口

项目未依赖大型第三方数据库框架或第三方测试框架，核心逻辑由课程项目代码自行实现。

## 9. 备注

- 本项目采用 Client/Server 架构。
- 客户端通过 TCP 与服务端通信。
- 数据以文件形式持久化存储在指定目录中。
- 项目按照课程要求实现自研数据结构，避免使用被禁止的 STL 容器。
