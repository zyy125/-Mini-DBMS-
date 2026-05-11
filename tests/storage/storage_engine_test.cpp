#include "storage/storage_engine.h"

#include <filesystem>
#include <string>

#include "common/status.h"
#include "testing/test_framework.h"

using mini_dbms::common::StatusCode;
using mini_dbms::storage::Column;
using mini_dbms::storage::ColumnType;
using mini_dbms::storage::Row;
using mini_dbms::storage::Schema;
using mini_dbms::storage::StorageEngine;
using mini_dbms::storage::Value;

namespace {

std::filesystem::path TestRoot(const std::string& name) {
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("mini_dbms_" + name);
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    return path;
}

Schema StudentSchema() {
    Schema schema;
    Column id;
    id.name = "id";
    id.type = ColumnType::kInt;
    id.primary = true;
    EXPECT_STATUS_OK(schema.columns.PushBack(id));

    Column name;
    name.name = "name";
    name.type = ColumnType::kString;
    name.primary = false;
    EXPECT_STATUS_OK(schema.columns.PushBack(name));

    Column age;
    age.name = "age";
    age.type = ColumnType::kInt;
    age.primary = false;
    EXPECT_STATUS_OK(schema.columns.PushBack(age));

    schema.primary_column_index = 0;
    return schema;
}

Row StudentRow(int id, const std::string& name, int age) {
    Row row;
    EXPECT_STATUS_OK(row.values.PushBack(Value::Int(id)));
    EXPECT_STATUS_OK(row.values.PushBack(Value::String(name)));
    EXPECT_STATUS_OK(row.values.PushBack(Value::Int(age)));
    return row;
}

}  // namespace

TEST_CASE("StorageEngine creates uses and drops databases") {
    std::filesystem::path root = TestRoot("database_lifecycle");
    StorageEngine engine(root);

    EXPECT_STATUS_OK(engine.Initialize());
    EXPECT_STATUS_OK(engine.CreateDatabase("school"));
    EXPECT_EQ(StatusCode::kAlreadyExists, engine.CreateDatabase("school").code());
    EXPECT_STATUS_OK(engine.UseDatabase("school"));
    EXPECT_EQ(std::string("school"), engine.current_database());
    EXPECT_EQ(StatusCode::kInvalidArgument, engine.CreateDatabase("schooldb1").code());
    EXPECT_STATUS_OK(engine.DropDatabase("school"));
    EXPECT_TRUE(!std::filesystem::exists(root / "school"));
}

TEST_CASE("StorageEngine creates table metadata and loads it after restart") {
    std::filesystem::path root = TestRoot("table_metadata");
    StorageEngine engine(root);

    EXPECT_STATUS_OK(engine.Initialize());
    EXPECT_STATUS_OK(engine.CreateDatabase("school"));
    EXPECT_STATUS_OK(engine.UseDatabase("school"));
    EXPECT_STATUS_OK(engine.CreateTable("students", StudentSchema()));

    StorageEngine restarted(root);
    EXPECT_STATUS_OK(restarted.Initialize());
    EXPECT_STATUS_OK(restarted.UseDatabase("school"));
    mini_dbms::storage::SchemaResult schema = restarted.LoadSchema("students");

    EXPECT_STATUS_OK(schema.status);
    EXPECT_EQ(static_cast<std::size_t>(3), schema.schema.columns.size());
    EXPECT_EQ(std::string("id"), schema.schema.columns[0].name);
    EXPECT_EQ(ColumnType::kInt, schema.schema.columns[0].type);
    EXPECT_TRUE(schema.schema.columns[0].primary);
    EXPECT_EQ(std::string("name"), schema.schema.columns[1].name);
    EXPECT_EQ(ColumnType::kString, schema.schema.columns[1].type);
    EXPECT_EQ(0, schema.schema.primary_column_index);
}

TEST_CASE("StorageEngine inserts scans and reads rows by RowId") {
    std::filesystem::path root = TestRoot("row_crud");
    StorageEngine engine(root);

    EXPECT_STATUS_OK(engine.Initialize());
    EXPECT_STATUS_OK(engine.CreateDatabase("school"));
    EXPECT_STATUS_OK(engine.UseDatabase("school"));
    EXPECT_STATUS_OK(engine.CreateTable("students", StudentSchema()));

    mini_dbms::storage::InsertResult first =
        engine.InsertRow("students", StudentRow(1, "alice", 18));
    mini_dbms::storage::InsertResult second =
        engine.InsertRow("students", StudentRow(2, "bob", 20));

    EXPECT_STATUS_OK(first.status);
    EXPECT_STATUS_OK(second.status);
    EXPECT_TRUE(first.row_id.IsValid());
    EXPECT_TRUE(second.row_id.IsValid());
    EXPECT_TRUE(first.row_id.offset != second.row_id.offset);

    mini_dbms::storage::ReadResult read = engine.ReadRow("students", second.row_id);
    EXPECT_STATUS_OK(read.status);
    EXPECT_EQ(2, read.row.values[0].int_value);
    EXPECT_EQ(std::string("bob"), read.row.values[1].string_value);

    mini_dbms::storage::ScanResult scan = engine.ScanTable("students");
    EXPECT_STATUS_OK(scan.status);
    EXPECT_EQ(static_cast<std::size_t>(2), scan.rows.rows.size());
    EXPECT_EQ(1, scan.rows.rows[0].row.values[0].int_value);
    EXPECT_EQ(2, scan.rows.rows[1].row.values[0].int_value);
}

TEST_CASE("StorageEngine update preserves RowId when payload fits and moves when needed") {
    std::filesystem::path root = TestRoot("row_update");
    StorageEngine engine(root);

    EXPECT_STATUS_OK(engine.Initialize());
    EXPECT_STATUS_OK(engine.CreateDatabase("school"));
    EXPECT_STATUS_OK(engine.UseDatabase("school"));
    EXPECT_STATUS_OK(engine.CreateTable("students", StudentSchema()));

    mini_dbms::storage::InsertResult inserted =
        engine.InsertRow("students", StudentRow(1, "alice", 18));
    EXPECT_STATUS_OK(inserted.status);

    mini_dbms::storage::UpdateResult same_slot =
        engine.UpdateRow("students", inserted.row_id, StudentRow(1, "amy", 19));
    EXPECT_STATUS_OK(same_slot.status);
    EXPECT_TRUE(!same_slot.moved);
    EXPECT_EQ(inserted.row_id.offset, same_slot.row_id.offset);

    mini_dbms::storage::UpdateResult moved =
        engine.UpdateRow("students", same_slot.row_id, StudentRow(1, "averylongername", 21));
    EXPECT_STATUS_OK(moved.status);
    EXPECT_TRUE(moved.moved);
    EXPECT_TRUE(moved.row_id.offset != same_slot.row_id.offset);

    EXPECT_EQ(StatusCode::kNotFound, engine.ReadRow("students", same_slot.row_id).status.code());
    mini_dbms::storage::ReadResult read = engine.ReadRow("students", moved.row_id);
    EXPECT_STATUS_OK(read.status);
    EXPECT_EQ(std::string("averylongername"), read.row.values[1].string_value);
}

TEST_CASE("StorageEngine delete tombstones rows and persistence keeps live rows") {
    std::filesystem::path root = TestRoot("persistence");
    StorageEngine engine(root);

    EXPECT_STATUS_OK(engine.Initialize());
    EXPECT_STATUS_OK(engine.CreateDatabase("school"));
    EXPECT_STATUS_OK(engine.UseDatabase("school"));
    EXPECT_STATUS_OK(engine.CreateTable("students", StudentSchema()));

    mini_dbms::storage::InsertResult first =
        engine.InsertRow("students", StudentRow(1, "alice", 18));
    mini_dbms::storage::InsertResult second =
        engine.InsertRow("students", StudentRow(2, "bob", 20));
    EXPECT_STATUS_OK(first.status);
    EXPECT_STATUS_OK(second.status);
    EXPECT_STATUS_OK(engine.DeleteRow("students", first.row_id));
    EXPECT_EQ(StatusCode::kNotFound, engine.ReadRow("students", first.row_id).status.code());

    StorageEngine restarted(root);
    EXPECT_STATUS_OK(restarted.Initialize());
    EXPECT_STATUS_OK(restarted.UseDatabase("school"));
    mini_dbms::storage::ScanResult scan = restarted.ScanTable("students");
    EXPECT_STATUS_OK(scan.status);
    EXPECT_EQ(static_cast<std::size_t>(1), scan.rows.rows.size());
    EXPECT_EQ(2, scan.rows.rows[0].row.values[0].int_value);
    EXPECT_EQ(std::string("bob"), scan.rows.rows[0].row.values[1].string_value);
}

TEST_CASE("StorageEngine validates row shape and drops table files") {
    std::filesystem::path root = TestRoot("errors_drop_table");
    StorageEngine engine(root);

    EXPECT_STATUS_OK(engine.Initialize());
    EXPECT_STATUS_OK(engine.CreateDatabase("school"));
    EXPECT_STATUS_OK(engine.UseDatabase("school"));
    EXPECT_STATUS_OK(engine.CreateTable("students", StudentSchema()));

    Row bad;
    EXPECT_STATUS_OK(bad.values.PushBack(Value::String("wrong")));
    EXPECT_EQ(StatusCode::kInvalidArgument, engine.InsertRow("students", bad).status.code());

    EXPECT_STATUS_OK(engine.DropTable("students"));
    EXPECT_TRUE(!std::filesystem::exists(root / "school" / "students.meta"));
    EXPECT_TRUE(!std::filesystem::exists(root / "school" / "students.dat"));
    EXPECT_EQ(StatusCode::kNotFound, engine.LoadSchema("students").status.code());
}
