#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
PORT="${PORT:-54321}"
DATA_ROOT="${DATA_ROOT:-$BUILD_DIR/demo_data}"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target db_server db_client

rm -rf "$DATA_ROOT"
"$BUILD_DIR/db_server" "$PORT" "$DATA_ROOT" &
SERVER_PID=$!
trap 'kill "$SERVER_PID" 2>/dev/null || true' EXIT

sleep 1

"$BUILD_DIR/db_client" 127.0.0.1 "$PORT" <<'SQL'
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
exit
SQL
