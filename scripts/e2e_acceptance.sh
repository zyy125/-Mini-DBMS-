#!/usr/bin/env bash
set -euo pipefail

SERVER_BIN="${1:?server binary required}"
CLIENT_BIN="${2:?client binary required}"
DATA_ROOT="${3:?data root required}"
PORT="${PORT:-55432}"
OUTPUT_FILE="$DATA_ROOT/client.out"

rm -rf "$DATA_ROOT"
mkdir -p "$DATA_ROOT"

"$SERVER_BIN" "$PORT" "$DATA_ROOT" >"$DATA_ROOT/server.out" 2>"$DATA_ROOT/server.err" &
SERVER_PID=$!
trap 'kill "$SERVER_PID" 2>/dev/null || true' EXIT

for _ in 1 2 3 4 5 6 7 8 9 10; do
    if "$CLIENT_BIN" 127.0.0.1 "$PORT" <<<'exit' >/dev/null 2>&1; then
        break
    fi
    sleep 0.2
done

"$CLIENT_BIN" 127.0.0.1 "$PORT" >"$OUTPUT_FILE" <<'SQL'
create database school;
use school;
create table students (id int primary, name string, age int);
insert students values (1, "alice", 18);
insert students values (2, "bob", 20);
insert students values (3, "cora", 20);
select * from students where age = 20;
select name from students where id = 2;
update students set age = 21 where id = 2;
delete students where id < 2;
select * from students where id > 0;
drop table students;
drop database school;
exit
SQL

grep -q "OK: Database created" "$OUTPUT_FILE"
grep -q "OK: Table created" "$OUTPUT_FILE"
grep -q "2 rows in set" "$OUTPUT_FILE"
grep -q "1 rows in set (using index)" "$OUTPUT_FILE"
grep -q "OK: 1 rows updated (1 affected)" "$OUTPUT_FILE"
grep -q "OK: 1 rows deleted (1 affected)" "$OUTPUT_FILE"
grep -q "OK: Table dropped" "$OUTPUT_FILE"
grep -q "OK: Database dropped" "$OUTPUT_FILE"
