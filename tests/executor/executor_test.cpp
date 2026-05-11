#include "executor/executor.h"

#include <filesystem>
#include <string>

#include "common/status.h"
#include "testing/test_framework.h"

using mini_dbms::common::StatusCode;
using mini_dbms::executor::Executor;
using mini_dbms::executor::QueryResult;
using mini_dbms::storage::ColumnType;

namespace {

std::filesystem::path TestRoot(const std::string& name) {
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("mini_dbms_" + name);
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    return path;
}

void CreateStudents(Executor* executor) {
    EXPECT_STATUS_OK(executor->ExecuteSql("create database school").status);
    EXPECT_STATUS_OK(executor->ExecuteSql("use school").status);
    EXPECT_STATUS_OK(
        executor->ExecuteSql("create table students (id int primary, name string, age int)")
            .status);
}

void InsertStudent(Executor* executor, int id, const std::string& name, int age) {
    QueryResult result = executor->ExecuteSql("insert students values (" + std::to_string(id) +
                                              ", \"" + name + "\", " + std::to_string(age) + ")");
    EXPECT_STATUS_OK(result.status);
    EXPECT_EQ(static_cast<std::size_t>(1), result.affected_rows);
}

}  // namespace

TEST_CASE("Executor runs database and table DDL") {
    Executor executor(TestRoot("executor_ddl"));
    EXPECT_STATUS_OK(executor.Initialize());

    QueryResult created = executor.ExecuteSql("create database school");
    EXPECT_STATUS_OK(created.status);
    EXPECT_EQ(std::string("Database created"), created.message);

    QueryResult used = executor.ExecuteSql("use school");
    EXPECT_STATUS_OK(used.status);
    EXPECT_EQ(std::string("school"), executor.current_database());

    QueryResult table =
        executor.ExecuteSql("create table students (id int primary, name string, age int)");
    EXPECT_STATUS_OK(table.status);
    EXPECT_TRUE(std::filesystem::exists(executor.data_root() / "school" / "students.idx"));

    EXPECT_STATUS_OK(executor.ExecuteSql("drop table students").status);
    EXPECT_TRUE(!std::filesystem::exists(executor.data_root() / "school" / "students.idx"));
    EXPECT_STATUS_OK(executor.ExecuteSql("drop database school").status);
    EXPECT_EQ(std::string(""), executor.current_database());
}

TEST_CASE("Executor inserts and selects by primary index equality") {
    Executor executor(TestRoot("executor_index_equal"));
    EXPECT_STATUS_OK(executor.Initialize());
    CreateStudents(&executor);
    InsertStudent(&executor, 1, "alice", 18);
    InsertStudent(&executor, 2, "bob", 20);

    QueryResult selected = executor.ExecuteSql("select name from students where id = 2");
    EXPECT_STATUS_OK(selected.status);
    EXPECT_TRUE(selected.used_index);
    EXPECT_EQ(static_cast<std::size_t>(1), selected.columns.size());
    EXPECT_EQ(std::string("name"), selected.columns[0]);
    EXPECT_EQ(static_cast<std::size_t>(1), selected.rows.size());
    EXPECT_EQ(ColumnType::kString, selected.rows[0].values[0].type);
    EXPECT_EQ(std::string("bob"), selected.rows[0].values[0].string_value);

    QueryResult duplicate = executor.ExecuteSql("insert students values (2, \"bobby\", 22)");
    EXPECT_EQ(StatusCode::kAlreadyExists, duplicate.status.code());
}

TEST_CASE("Executor uses primary index for range selects") {
    Executor executor(TestRoot("executor_index_range"));
    EXPECT_STATUS_OK(executor.Initialize());
    CreateStudents(&executor);
    InsertStudent(&executor, 10, "ann", 18);
    InsertStudent(&executor, 20, "ben", 21);
    InsertStudent(&executor, 30, "cam", 22);
    InsertStudent(&executor, 40, "dan", 23);

    QueryResult less = executor.ExecuteSql("select id from students where id < 30");
    EXPECT_STATUS_OK(less.status);
    EXPECT_TRUE(less.used_index);
    EXPECT_EQ(static_cast<std::size_t>(2), less.rows.size());
    EXPECT_EQ(10, less.rows[0].values[0].int_value);
    EXPECT_EQ(20, less.rows[1].values[0].int_value);

    QueryResult greater = executor.ExecuteSql("select id from students where id > 20");
    EXPECT_STATUS_OK(greater.status);
    EXPECT_TRUE(greater.used_index);
    EXPECT_EQ(static_cast<std::size_t>(2), greater.rows.size());
    EXPECT_EQ(30, greater.rows[0].values[0].int_value);
    EXPECT_EQ(40, greater.rows[1].values[0].int_value);
}

TEST_CASE("Executor scans for non-index where and select all") {
    Executor executor(TestRoot("executor_scan"));
    EXPECT_STATUS_OK(executor.Initialize());
    CreateStudents(&executor);
    InsertStudent(&executor, 1, "alice", 18);
    InsertStudent(&executor, 2, "bob", 20);
    InsertStudent(&executor, 3, "cora", 20);

    QueryResult selected = executor.ExecuteSql("select * from students where age = 20");
    EXPECT_STATUS_OK(selected.status);
    EXPECT_TRUE(!selected.used_index);
    EXPECT_EQ(static_cast<std::size_t>(3), selected.columns.size());
    EXPECT_EQ(static_cast<std::size_t>(2), selected.rows.size());
    EXPECT_EQ(2, selected.rows[0].values[0].int_value);
    EXPECT_EQ(std::string("bob"), selected.rows[0].values[1].string_value);
    EXPECT_EQ(3, selected.rows[1].values[0].int_value);
}

TEST_CASE("Executor updates deletes and rebuilds primary index") {
    Executor executor(TestRoot("executor_update_delete"));
    EXPECT_STATUS_OK(executor.Initialize());
    CreateStudents(&executor);
    InsertStudent(&executor, 1, "alice", 18);
    InsertStudent(&executor, 2, "bob", 20);
    InsertStudent(&executor, 3, "cora", 21);

    QueryResult updated = executor.ExecuteSql("update students set id = 5 where id = 2");
    EXPECT_STATUS_OK(updated.status);
    EXPECT_EQ(static_cast<std::size_t>(1), updated.affected_rows);

    QueryResult old_key = executor.ExecuteSql("select name from students where id = 2");
    EXPECT_STATUS_OK(old_key.status);
    EXPECT_TRUE(old_key.used_index);
    EXPECT_EQ(static_cast<std::size_t>(0), old_key.rows.size());

    QueryResult new_key = executor.ExecuteSql("select name from students where id = 5");
    EXPECT_STATUS_OK(new_key.status);
    EXPECT_TRUE(new_key.used_index);
    EXPECT_EQ(static_cast<std::size_t>(1), new_key.rows.size());
    EXPECT_EQ(std::string("bob"), new_key.rows[0].values[0].string_value);

    QueryResult deleted = executor.ExecuteSql("delete students where id < 3");
    EXPECT_STATUS_OK(deleted.status);
    EXPECT_EQ(static_cast<std::size_t>(1), deleted.affected_rows);

    QueryResult remaining = executor.ExecuteSql("select id from students where id > 0");
    EXPECT_STATUS_OK(remaining.status);
    EXPECT_TRUE(remaining.used_index);
    EXPECT_EQ(static_cast<std::size_t>(2), remaining.rows.size());
    EXPECT_EQ(3, remaining.rows[0].values[0].int_value);
    EXPECT_EQ(5, remaining.rows[1].values[0].int_value);
}

TEST_CASE("Executor rejects primary key update conflicts before writing") {
    Executor executor(TestRoot("executor_update_conflict"));
    EXPECT_STATUS_OK(executor.Initialize());
    CreateStudents(&executor);
    InsertStudent(&executor, 1, "alice", 18);
    InsertStudent(&executor, 2, "bob", 20);

    QueryResult conflict = executor.ExecuteSql("update students set id = 1 where id = 2");
    EXPECT_EQ(StatusCode::kAlreadyExists, conflict.status.code());

    QueryResult selected = executor.ExecuteSql("select name from students where id = 2");
    EXPECT_STATUS_OK(selected.status);
    EXPECT_TRUE(selected.used_index);
    EXPECT_EQ(static_cast<std::size_t>(1), selected.rows.size());
    EXPECT_EQ(std::string("bob"), selected.rows[0].values[0].string_value);
}

TEST_CASE("Executor rejects unknown columns and wrong value types") {
    Executor executor(TestRoot("executor_errors"));
    EXPECT_STATUS_OK(executor.Initialize());
    CreateStudents(&executor);

    EXPECT_EQ(StatusCode::kInvalidArgument,
              executor.ExecuteSql("insert students values (\"bad\", \"alice\", 18)").status.code());
    EXPECT_EQ(StatusCode::kInvalidArgument,
              executor.ExecuteSql("select missing from students").status.code());
    EXPECT_EQ(StatusCode::kInvalidArgument,
              executor.ExecuteSql("select id from students where age = \"old\"").status.code());
    EXPECT_EQ(StatusCode::kInvalidArgument,
              executor.ExecuteSql("update students set missing = 1").status.code());
}
