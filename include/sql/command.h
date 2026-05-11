#pragma once

#include <ostream>
#include <string>

#include "common/dynamic_array.h"

namespace mini_dbms::sql {

enum class CommandType {
    kCreateDatabase,
    kDropDatabase,
    kUseDatabase,
    kCreateTable,
    kDropTable,
    kInsert,
    kSelect,
    kDelete,
    kUpdate
};

enum class DataType {
    kInt,
    kString
};

enum class ValueType {
    kInt,
    kString
};

enum class ComparisonOp {
    kEqual,
    kLess,
    kGreater
};

struct ValueLiteral {
    ValueType type;
    int int_value;
    std::string string_value;

    ValueLiteral() : type(ValueType::kInt), int_value(0), string_value() {}

    static ValueLiteral Int(int value) {
        ValueLiteral literal;
        literal.type = ValueType::kInt;
        literal.int_value = value;
        return literal;
    }

    static ValueLiteral String(const std::string& value) {
        ValueLiteral literal;
        literal.type = ValueType::kString;
        literal.string_value = value;
        return literal;
    }
};

struct ColumnDefinition {
    std::string name;
    DataType type;
    bool primary;

    ColumnDefinition() : name(), type(DataType::kInt), primary(false) {}
};

struct WhereClause {
    std::string column_name;
    ComparisonOp op;
    ValueLiteral value;

    WhereClause() : column_name(), op(ComparisonOp::kEqual), value() {}
};

struct SqlCommand {
    CommandType type;
    std::string database_name;
    std::string table_name;
    common::DynamicArray<ColumnDefinition> column_definitions;
    common::DynamicArray<std::string> selected_columns;
    bool select_all;
    common::DynamicArray<ValueLiteral> values;
    std::string assignment_column;
    ValueLiteral assignment_value;
    bool has_where;
    WhereClause where;

    SqlCommand()
        : type(CommandType::kUseDatabase),
          database_name(),
          table_name(),
          column_definitions(),
          selected_columns(),
          select_all(false),
          values(),
          assignment_column(),
          assignment_value(),
          has_where(false),
          where() {}
};

inline std::ostream& operator<<(std::ostream& os, CommandType type) {
    os << static_cast<int>(type);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, DataType type) {
    os << static_cast<int>(type);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, ValueType type) {
    os << static_cast<int>(type);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, ComparisonOp op) {
    os << static_cast<int>(op);
    return os;
}

}  // namespace mini_dbms::sql
