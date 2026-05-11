#pragma once

#include <cstdint>
#include <filesystem>
#include <ostream>
#include <string>

#include "common/dynamic_array.h"
#include "common/status.h"
#include "sql/command.h"

namespace mini_dbms::storage {

constexpr std::size_t kMaxStringBytes = 256;

enum class ColumnType {
    kInt,
    kString
};

inline std::ostream& operator<<(std::ostream& os, ColumnType type) {
    os << static_cast<int>(type);
    return os;
}

struct RowId {
    std::uint64_t offset;

    RowId() : offset(kInvalidOffset) {}
    explicit RowId(std::uint64_t row_offset) : offset(row_offset) {}

    bool IsValid() const { return offset != kInvalidOffset; }

    static constexpr std::uint64_t kInvalidOffset = UINT64_MAX;
};

using RecordId = RowId;

inline bool operator==(const RowId& lhs, const RowId& rhs) {
    return lhs.offset == rhs.offset;
}

struct Column {
    std::string name;
    ColumnType type;
    bool primary;

    Column() : name(), type(ColumnType::kInt), primary(false) {}
};

struct Schema {
    common::DynamicArray<Column> columns;
    int primary_column_index;

    Schema() : columns(), primary_column_index(-1) {}

    const Column* FindColumn(const std::string& name) const;
    int FindColumnIndex(const std::string& name) const;
    bool HasPrimaryKey() const { return primary_column_index >= 0; }
};

struct Value {
    ColumnType type;
    int int_value;
    std::string string_value;

    Value() : type(ColumnType::kInt), int_value(0), string_value() {}

    static Value Int(int value);
    static Value String(const std::string& value);
};

struct Row {
    common::DynamicArray<Value> values;
};

struct StoredRow {
    RowId row_id;
    Row row;
};

struct TableRows {
    common::DynamicArray<StoredRow> rows;
};

struct InsertResult {
    common::Status status;
    RowId row_id;
};

struct ReadResult {
    common::Status status;
    Row row;
};

struct ScanResult {
    common::Status status;
    TableRows rows;
};

struct UpdateResult {
    common::Status status;
    RowId row_id;
    bool moved;
};

struct SchemaResult {
    common::Status status;
    Schema schema;
};

class StorageEngine {
public:
    explicit StorageEngine(std::filesystem::path data_root);

    common::Status Initialize();

    common::Status CreateDatabase(const std::string& database_name);
    common::Status DropDatabase(const std::string& database_name);
    common::Status UseDatabase(const std::string& database_name);

    common::Status CreateTable(const std::string& table_name, const Schema& schema);
    common::Status CreateTable(const std::string& table_name,
                               const common::DynamicArray<sql::ColumnDefinition>& columns);
    common::Status DropTable(const std::string& table_name);

    SchemaResult LoadSchema(const std::string& table_name) const;

    InsertResult InsertRow(const std::string& table_name, const Row& row);
    ScanResult ScanTable(const std::string& table_name) const;
    ReadResult ReadRow(const std::string& table_name, RowId row_id) const;
    UpdateResult UpdateRow(const std::string& table_name, RowId row_id, const Row& row);
    common::Status DeleteRow(const std::string& table_name, RowId row_id);

    const std::string& current_database() const { return current_database_; }
    const std::filesystem::path& data_root() const { return data_root_; }

private:
    std::filesystem::path DatabasePath(const std::string& database_name) const;
    std::filesystem::path CurrentDatabasePath() const;
    std::filesystem::path TableMetaPath(const std::string& table_name) const;
    std::filesystem::path TableDataPath(const std::string& table_name) const;

    common::Status RequireCurrentDatabase() const;
    common::Status ValidateIdentifier(const std::string& name, const std::string& kind) const;
    common::Status ValidateSchema(const Schema& schema) const;
    common::Status ValidateRowMatchesSchema(const Row& row, const Schema& schema) const;

    std::filesystem::path data_root_;
    std::string current_database_;
};

common::Status ConvertSqlColumnsToSchema(
    const common::DynamicArray<sql::ColumnDefinition>& columns,
    Schema* schema);

}  // namespace mini_dbms::storage
