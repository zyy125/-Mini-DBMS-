# Phase 8 Requirement-To-Test Coverage

| Requirement | Existing / Added Coverage | Status |
| --- | --- | --- |
| CMake C++20 Linux build | `cmake -S . -B build`, `cmake --build build` | Covered |
| Self-owned test framework | `include/testing/test_framework.h` linked by all tests; no GoogleTest/Catch2 | Covered |
| No forbidden STL containers in source/tests | `rg` scan over `include src tests`; only docs mention forbidden names | Covered |
| ASan/UBSan build | `MINI_DBMS_ENABLE_ASAN=ON` CMake option and Phase 8 sanitizer run | Covered |
| Common data structures | `common_tests` for `DynamicArray`, `HashTable`, `LinkedList`, `Status`, `StringUtils` | Covered |
| SQL lexer/parser happy paths | `sql_tests` covers create/drop/use/create table/drop table/insert/select/delete/update | Covered |
| SQL parser invalid input | `Parser rejects unsupported or malformed SQL`, invalid identifiers, malformed list/trailing-token tests | Covered |
| Identifier rules | Parser and storage tests reject uppercase, underscore, duplicate columns | Covered |
| File-based database/table storage | `storage_tests` database/table lifecycle and file existence checks | Covered |
| Storage persistence | `StorageEngine creates table metadata and loads it after restart`; row persistence test | Covered |
| String max 256 bytes | `StorageEngine enforces string length boundary` | Covered |
| Primary key B+ tree index | `index_tests`; executor checks `.idx` creation and indexed select | Covered |
| Reject unsupported string primary | `StorageEngine rejects schema errors required by course constraints`; executor error test | Covered after Phase 8 fix |
| B+ tree split, duplicate, range, persistence | `BPlusTreeIndex split leaves and internal nodes`, duplicate, range, save/load tests | Covered |
| DDL executor behavior | `Executor runs database and table DDL`; e2e acceptance script | Covered |
| Insert/select/update/delete executor behavior | `executor_tests`; `scripts/e2e_acceptance.sh` | Covered |
| Indexed equality/range select | Executor index equality/range tests assert `used_index` | Covered |
| Error handling | Executor/storage/parser/protocol invalid input tests | Covered |
| TCP frame protocol | `network_tests` for send/receive/empty/null/oversized frame | Covered |
| Client/server end-to-end flow | CTest `e2e_acceptance` runs server + client and checks output | Covered |

## Must-Fix Findings From Phase 8

1. `string primary` was accepted but no B+ tree index could be built for string keys. Fixed by rejecting non-`int` primary keys in schema validation.
2. TCP protocol had no automated CTest coverage. Fixed with `tests/network/protocol_test.cpp`.
3. The demo script did not cover drop operations or assert semantic output. Fixed with `scripts/e2e_acceptance.sh` and CTest integration.
