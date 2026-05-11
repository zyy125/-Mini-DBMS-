#include "executor/executor.h"

#include <utility>

#include "index/bplus_tree_index.h"
#include "sql/parser.h"

namespace mini_dbms::executor {
namespace {

using common::DynamicArray;
using common::Status;
using common::StatusCode;
using index::BPlusTreeIndex;
using sql::CommandType;
using sql::ComparisonOp;
using sql::ValueLiteral;
using storage::ColumnType;
using storage::Row;
using storage::RowId;
using storage::Schema;
using storage::StoredRow;
using storage::Value;

std::filesystem::path IndexPath(const storage::StorageEngine& storage,
                                const std::string& table_name) {
    return storage.data_root() / storage.current_database() / (table_name + ".idx");
}

QueryResult ErrorResult(const Status& status) {
    QueryResult result;
    result.status = status;
    result.message = status.ToString();
    return result;
}

Status LiteralToValue(const ValueLiteral& literal, ColumnType expected_type, Value* value) {
    if (expected_type == ColumnType::kInt) {
        if (literal.type != sql::ValueType::kInt) {
            return Status::InvalidArgument("Expected int value");
        }
        *value = Value::Int(literal.int_value);
        return Status::OK();
    }
    if (literal.type != sql::ValueType::kString) {
        return Status::InvalidArgument("Expected string value");
    }
    *value = Value::String(literal.string_value);
    return Status::OK();
}

Status BuildRow(const DynamicArray<ValueLiteral>& literals, const Schema& schema, Row* row) {
    row->values.Clear();
    if (literals.size() != schema.columns.size()) {
        return Status::InvalidArgument("Inserted value count does not match table schema");
    }
    for (std::size_t i = 0; i < literals.size(); ++i) {
        Value value;
        Status status = LiteralToValue(literals[i], schema.columns[i].type, &value);
        if (!status.ok()) {
            return status;
        }
        status = row->values.PushBack(std::move(value));
        if (!status.ok()) {
            return status;
        }
    }
    return Status::OK();
}

bool ValueMatches(const Value& left, ComparisonOp op, const Value& right) {
    if (left.type != right.type) {
        return false;
    }
    if (left.type == ColumnType::kInt) {
        if (op == ComparisonOp::kEqual) {
            return left.int_value == right.int_value;
        }
        if (op == ComparisonOp::kLess) {
            return left.int_value < right.int_value;
        }
        return left.int_value > right.int_value;
    }
    int cmp = left.string_value.compare(right.string_value);
    if (op == ComparisonOp::kEqual) {
        return cmp == 0;
    }
    if (op == ComparisonOp::kLess) {
        return cmp < 0;
    }
    return cmp > 0;
}

bool ValuesEqual(const Value& left, const Value& right) {
    return ValueMatches(left, ComparisonOp::kEqual, right);
}

Status RowMatchesWhere(const Row& row,
                       const Schema& schema,
                       const sql::SqlCommand& command,
                       bool* matches) {
    *matches = true;
    if (!command.has_where) {
        return Status::OK();
    }
    int column_index = schema.FindColumnIndex(command.where.column_name);
    if (column_index < 0) {
        return Status::InvalidArgument("Unknown where column: " + command.where.column_name);
    }
    Value compare_value;
    Status status = LiteralToValue(command.where.value,
                                   schema.columns[static_cast<std::size_t>(column_index)].type,
                                   &compare_value);
    if (!status.ok()) {
        return status;
    }
    *matches = ValueMatches(row.values[static_cast<std::size_t>(column_index)],
                            command.where.op,
                            compare_value);
    return Status::OK();
}

Status ValidateWhereClause(const Schema& schema, const sql::SqlCommand& command) {
    if (!command.has_where) {
        return Status::OK();
    }
    int column_index = schema.FindColumnIndex(command.where.column_name);
    if (column_index < 0) {
        return Status::InvalidArgument("Unknown where column: " + command.where.column_name);
    }
    Value compare_value;
    return LiteralToValue(command.where.value,
                          schema.columns[static_cast<std::size_t>(column_index)].type,
                          &compare_value);
}

bool HasUsablePrimaryIndex(const Schema& schema, const sql::SqlCommand& command) {
    if (!command.has_where || !schema.HasPrimaryKey()) {
        return false;
    }
    const storage::Column& primary =
        schema.columns[static_cast<std::size_t>(schema.primary_column_index)];
    return primary.type == ColumnType::kInt && primary.name == command.where.column_name &&
           command.where.value.type == sql::ValueType::kInt;
}

Status LoadIndex(const storage::StorageEngine& storage,
                 const std::string& table_name,
                 BPlusTreeIndex* tree) {
    std::filesystem::path path = IndexPath(storage, table_name);
    if (!std::filesystem::exists(path)) {
        return Status::NotFound("Index file not found: " + path.string());
    }
    return tree->LoadFromFile(path);
}

Status SaveEmptyIndexIfNeeded(const storage::StorageEngine& storage,
                              const std::string& table_name,
                              const Schema& schema) {
    if (!schema.HasPrimaryKey()) {
        return Status::OK();
    }
    const storage::Column& primary =
        schema.columns[static_cast<std::size_t>(schema.primary_column_index)];
    if (primary.type != ColumnType::kInt) {
        return Status::OK();
    }
    BPlusTreeIndex tree;
    return tree.SaveToFile(IndexPath(storage, table_name));
}

Status RebuildPrimaryIndex(const storage::StorageEngine& storage,
                           const std::string& table_name,
                           const Schema& schema) {
    if (!schema.HasPrimaryKey()) {
        return Status::OK();
    }
    const storage::Column& primary =
        schema.columns[static_cast<std::size_t>(schema.primary_column_index)];
    if (primary.type != ColumnType::kInt) {
        return Status::OK();
    }

    storage::ScanResult scan = storage.ScanTable(table_name);
    if (!scan.status.ok()) {
        return scan.status;
    }
    BPlusTreeIndex tree;
    std::size_t primary_index = static_cast<std::size_t>(schema.primary_column_index);
    for (std::size_t i = 0; i < scan.rows.rows.size(); ++i) {
        const StoredRow& stored = scan.rows.rows[i];
        Status status = tree.Insert(stored.row.values[primary_index].int_value, stored.row_id);
        if (!status.ok()) {
            return status;
        }
    }
    return tree.SaveToFile(IndexPath(storage, table_name));
}

Status CheckPrimaryUniqueByScan(const storage::StorageEngine& storage,
                                const std::string& table_name,
                                const Schema& schema,
                                const Value& key,
                                RowId ignored_row_id) {
    if (!schema.HasPrimaryKey()) {
        return Status::OK();
    }
    storage::ScanResult scan = storage.ScanTable(table_name);
    if (!scan.status.ok()) {
        return scan.status;
    }
    std::size_t primary_index = static_cast<std::size_t>(schema.primary_column_index);
    for (std::size_t i = 0; i < scan.rows.rows.size(); ++i) {
        if (scan.rows.rows[i].row_id == ignored_row_id) {
            continue;
        }
        if (ValuesEqual(scan.rows.rows[i].row.values[primary_index], key)) {
            return Status::AlreadyExists("Duplicate primary key");
        }
    }
    return Status::OK();
}

Status ValidatePrimaryUpdate(const storage::StorageEngine& storage,
                             const std::string& table_name,
                             const Schema& schema,
                             const sql::SqlCommand& command,
                             const Value& assignment_value) {
    if (!schema.HasPrimaryKey()) {
        return Status::OK();
    }
    int primary_index = schema.primary_column_index;
    if (command.assignment_column != schema.columns[static_cast<std::size_t>(primary_index)].name) {
        return Status::OK();
    }

    storage::ScanResult scan = storage.ScanTable(table_name);
    if (!scan.status.ok()) {
        return scan.status;
    }
    std::size_t matched_count = 0;
    RowId matched_row_id;
    for (std::size_t i = 0; i < scan.rows.rows.size(); ++i) {
        bool matches = false;
        Status status = RowMatchesWhere(scan.rows.rows[i].row, schema, command, &matches);
        if (!status.ok()) {
            return status;
        }
        if (matches) {
            ++matched_count;
            matched_row_id = scan.rows.rows[i].row_id;
        }
    }
    if (matched_count == 0) {
        return Status::OK();
    }
    if (matched_count > 1) {
        return Status::AlreadyExists("Primary key update would create duplicates");
    }
    return CheckPrimaryUniqueByScan(storage, table_name, schema, assignment_value, matched_row_id);
}

Status AppendProjectedRow(const Row& row,
                          const DynamicArray<int>& selected_indexes,
                          QueryResult* result) {
    QueryRow query_row;
    for (std::size_t i = 0; i < selected_indexes.size(); ++i) {
        const Value& value = row.values[static_cast<std::size_t>(selected_indexes[i])];
        QueryValue query_value;
        query_value.type = value.type;
        query_value.int_value = value.int_value;
        query_value.string_value = value.string_value;
        Status status = query_row.values.PushBack(std::move(query_value));
        if (!status.ok()) {
            return status;
        }
    }
    return result->rows.PushBack(std::move(query_row));
}

Status BuildProjection(const Schema& schema,
                       const sql::SqlCommand& command,
                       DynamicArray<int>* selected_indexes,
                       QueryResult* result) {
    selected_indexes->Clear();
    result->columns.Clear();
    if (command.select_all) {
        for (std::size_t i = 0; i < schema.columns.size(); ++i) {
            Status status = selected_indexes->PushBack(static_cast<int>(i));
            if (!status.ok()) {
                return status;
            }
            status = result->columns.PushBack(schema.columns[i].name);
            if (!status.ok()) {
                return status;
            }
        }
        return Status::OK();
    }
    for (std::size_t i = 0; i < command.selected_columns.size(); ++i) {
        int index = schema.FindColumnIndex(command.selected_columns[i]);
        if (index < 0) {
            return Status::InvalidArgument("Unknown selected column: " +
                                           command.selected_columns[i]);
        }
        Status status = selected_indexes->PushBack(index);
        if (!status.ok()) {
            return status;
        }
        status = result->columns.PushBack(command.selected_columns[i]);
        if (!status.ok()) {
            return status;
        }
    }
    return Status::OK();
}

QueryResult ExecuteSelectWithScan(storage::StorageEngine* storage,
                                  const sql::SqlCommand& command,
                                  const Schema& schema,
                                  const DynamicArray<int>& selected_indexes,
                                  QueryResult result) {
    storage::ScanResult scan = storage->ScanTable(command.table_name);
    if (!scan.status.ok()) {
        return ErrorResult(scan.status);
    }
    for (std::size_t i = 0; i < scan.rows.rows.size(); ++i) {
        bool matches = false;
        Status status = RowMatchesWhere(scan.rows.rows[i].row, schema, command, &matches);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        if (matches) {
            status = AppendProjectedRow(scan.rows.rows[i].row, selected_indexes, &result);
            if (!status.ok()) {
                return ErrorResult(status);
            }
        }
    }
    result.affected_rows = result.rows.size();
    result.message = std::to_string(result.rows.size()) + " rows selected";
    return result;
}

QueryResult ExecuteSelectWithIndex(storage::StorageEngine* storage,
                                   const sql::SqlCommand& command,
                                   const DynamicArray<int>& selected_indexes,
                                   QueryResult result) {
    BPlusTreeIndex tree;
    Status status = LoadIndex(*storage, command.table_name, &tree);
    if (!status.ok()) {
        return ErrorResult(status);
    }
    result.used_index = true;
    DynamicArray<RowId> row_ids;
    if (command.where.op == ComparisonOp::kEqual) {
        index::IndexLookupResult lookup = tree.FindEqual(command.where.value.int_value);
        if (lookup.status.ok()) {
            status = row_ids.PushBack(lookup.row_id);
            if (!status.ok()) {
                return ErrorResult(status);
            }
        } else if (lookup.status.code() != StatusCode::kNotFound) {
            return ErrorResult(lookup.status);
        }
    } else if (command.where.op == ComparisonOp::kLess) {
        index::IndexRangeResult range = tree.FindLessThan(command.where.value.int_value);
        if (!range.status.ok()) {
            return ErrorResult(range.status);
        }
        row_ids = std::move(range.row_ids);
    } else {
        index::IndexRangeResult range = tree.FindGreaterThan(command.where.value.int_value);
        if (!range.status.ok()) {
            return ErrorResult(range.status);
        }
        row_ids = std::move(range.row_ids);
    }

    for (std::size_t i = 0; i < row_ids.size(); ++i) {
        storage::ReadResult read = storage->ReadRow(command.table_name, row_ids[i]);
        if (!read.status.ok()) {
            if (read.status.code() == StatusCode::kNotFound) {
                continue;
            }
            return ErrorResult(read.status);
        }
        status = AppendProjectedRow(read.row, selected_indexes, &result);
        if (!status.ok()) {
            return ErrorResult(status);
        }
    }
    result.affected_rows = result.rows.size();
    result.message = std::to_string(result.rows.size()) + " rows selected";
    return result;
}

QueryResult SuccessMessage(const std::string& message, std::size_t affected_rows) {
    QueryResult result;
    result.message = message;
    result.affected_rows = affected_rows;
    return result;
}

}  // namespace

Executor::Executor(std::filesystem::path data_root) : storage_(std::move(data_root)) {}

Status Executor::Initialize() {
    return storage_.Initialize();
}

QueryResult Executor::Execute(const sql::SqlCommand& command) {
    if (command.type == CommandType::kCreateDatabase) {
        Status status = storage_.CreateDatabase(command.database_name);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        return SuccessMessage("Database created", 0);
    }
    if (command.type == CommandType::kDropDatabase) {
        Status status = storage_.DropDatabase(command.database_name);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        return SuccessMessage("Database dropped", 0);
    }
    if (command.type == CommandType::kUseDatabase) {
        Status status = storage_.UseDatabase(command.database_name);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        return SuccessMessage("Database changed", 0);
    }
    if (command.type == CommandType::kCreateTable) {
        storage::Schema schema;
        Status status = storage::ConvertSqlColumnsToSchema(command.column_definitions, &schema);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        status = storage_.CreateTable(command.table_name, schema);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        status = SaveEmptyIndexIfNeeded(storage_, command.table_name, schema);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        return SuccessMessage("Table created", 0);
    }
    if (command.type == CommandType::kDropTable) {
        Status status = storage_.DropTable(command.table_name);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        return SuccessMessage("Table dropped", 0);
    }

    storage::SchemaResult schema_result = storage_.LoadSchema(command.table_name);
    if (!schema_result.status.ok()) {
        return ErrorResult(schema_result.status);
    }
    const Schema& schema = schema_result.schema;

    if (command.type == CommandType::kInsert) {
        Row row;
        Status status = BuildRow(command.values, schema, &row);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        if (schema.HasPrimaryKey() &&
            schema.columns[static_cast<std::size_t>(schema.primary_column_index)].type ==
                ColumnType::kInt) {
            BPlusTreeIndex tree;
            status = LoadIndex(storage_, command.table_name, &tree);
            if (!status.ok()) {
                return ErrorResult(status);
            }
            int key = row.values[static_cast<std::size_t>(schema.primary_column_index)].int_value;
            index::IndexLookupResult existing = tree.FindEqual(key);
            if (existing.status.ok()) {
                return ErrorResult(Status::AlreadyExists("Duplicate primary key"));
            }
            if (existing.status.code() != StatusCode::kNotFound) {
                return ErrorResult(existing.status);
            }
            storage::InsertResult insert = storage_.InsertRow(command.table_name, row);
            if (!insert.status.ok()) {
                return ErrorResult(insert.status);
            }
            status = tree.Insert(key, insert.row_id);
            if (!status.ok()) {
                return ErrorResult(status);
            }
            status = tree.SaveToFile(IndexPath(storage_, command.table_name));
            if (!status.ok()) {
                return ErrorResult(status);
            }
            return SuccessMessage("1 row inserted", 1);
        }
        if (schema.HasPrimaryKey()) {
            const Value& key = row.values[static_cast<std::size_t>(schema.primary_column_index)];
            Status status = CheckPrimaryUniqueByScan(storage_, command.table_name, schema, key, RowId());
            if (!status.ok()) {
                return ErrorResult(status);
            }
        }
        storage::InsertResult insert = storage_.InsertRow(command.table_name, row);
        if (!insert.status.ok()) {
            return ErrorResult(insert.status);
        }
        return SuccessMessage("1 row inserted", 1);
    }

    if (command.type == CommandType::kSelect) {
        Status status = ValidateWhereClause(schema, command);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        DynamicArray<int> selected_indexes;
        QueryResult result;
        status = BuildProjection(schema, command, &selected_indexes, &result);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        if (HasUsablePrimaryIndex(schema, command)) {
            return ExecuteSelectWithIndex(&storage_, command, selected_indexes, std::move(result));
        }
        return ExecuteSelectWithScan(&storage_, command, schema, selected_indexes, std::move(result));
    }

    if (command.type == CommandType::kDelete) {
        Status status = ValidateWhereClause(schema, command);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        storage::ScanResult scan = storage_.ScanTable(command.table_name);
        if (!scan.status.ok()) {
            return ErrorResult(scan.status);
        }
        std::size_t deleted = 0;
        for (std::size_t i = 0; i < scan.rows.rows.size(); ++i) {
            bool matches = false;
            status = RowMatchesWhere(scan.rows.rows[i].row, schema, command, &matches);
            if (!status.ok()) {
                return ErrorResult(status);
            }
            if (matches) {
                status = storage_.DeleteRow(command.table_name, scan.rows.rows[i].row_id);
                if (!status.ok()) {
                    return ErrorResult(status);
                }
                ++deleted;
            }
        }
        status = RebuildPrimaryIndex(storage_, command.table_name, schema);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        return SuccessMessage(std::to_string(deleted) + " rows deleted", deleted);
    }

    if (command.type == CommandType::kUpdate) {
        Status status = ValidateWhereClause(schema, command);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        int assignment_index = schema.FindColumnIndex(command.assignment_column);
        if (assignment_index < 0) {
            return ErrorResult(
                Status::InvalidArgument("Unknown assignment column: " + command.assignment_column));
        }
        Value assignment_value;
        status = LiteralToValue(command.assignment_value,
                                schema.columns[static_cast<std::size_t>(assignment_index)].type,
                                &assignment_value);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        status =
            ValidatePrimaryUpdate(storage_, command.table_name, schema, command, assignment_value);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        storage::ScanResult scan = storage_.ScanTable(command.table_name);
        if (!scan.status.ok()) {
            return ErrorResult(scan.status);
        }
        std::size_t updated = 0;
        for (std::size_t i = 0; i < scan.rows.rows.size(); ++i) {
            bool matches = false;
            status = RowMatchesWhere(scan.rows.rows[i].row, schema, command, &matches);
            if (!status.ok()) {
                return ErrorResult(status);
            }
            if (matches) {
                Row new_row = scan.rows.rows[i].row;
                new_row.values[static_cast<std::size_t>(assignment_index)] = assignment_value;
                storage::UpdateResult update =
                    storage_.UpdateRow(command.table_name, scan.rows.rows[i].row_id, new_row);
                if (!update.status.ok()) {
                    return ErrorResult(update.status);
                }
                ++updated;
            }
        }
        status = RebuildPrimaryIndex(storage_, command.table_name, schema);
        if (!status.ok()) {
            return ErrorResult(status);
        }
        return SuccessMessage(std::to_string(updated) + " rows updated", updated);
    }

    return ErrorResult(Status::InvalidArgument("Unsupported command"));
}

QueryResult Executor::ExecuteSql(const std::string& sql_text) {
    sql::Parser parser;
    sql::ParseResult parsed = parser.Parse(sql_text);
    if (!parsed.status.ok()) {
        return ErrorResult(parsed.status);
    }
    return Execute(parsed.command);
}

const std::string& Executor::current_database() const {
    return storage_.current_database();
}

const std::filesystem::path& Executor::data_root() const {
    return storage_.data_root();
}

}  // namespace mini_dbms::executor
