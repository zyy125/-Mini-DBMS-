#pragma once

#include <filesystem>
#include <string>

#include "common/dynamic_array.h"
#include "common/status.h"
#include "sql/command.h"
#include "storage/storage_engine.h"

namespace mini_dbms::executor {

struct QueryValue {
    storage::ColumnType type;
    int int_value;
    std::string string_value;

    QueryValue() : type(storage::ColumnType::kInt), int_value(0), string_value() {}
};

struct QueryRow {
    common::DynamicArray<QueryValue> values;
};

struct QueryResult {
    common::Status status;
    std::string message;
    common::DynamicArray<std::string> columns;
    common::DynamicArray<QueryRow> rows;
    std::size_t affected_rows;
    bool used_index;

    QueryResult()
        : status(common::Status::OK()),
          message(),
          columns(),
          rows(),
          affected_rows(0),
          used_index(false) {}
};

class Executor {
public:
    explicit Executor(std::filesystem::path data_root);

    common::Status Initialize();

    QueryResult Execute(const sql::SqlCommand& command);
    QueryResult ExecuteSql(const std::string& sql);

    const std::string& current_database() const;
    const std::filesystem::path& data_root() const;

private:
    storage::StorageEngine storage_;
};

}  // namespace mini_dbms::executor
