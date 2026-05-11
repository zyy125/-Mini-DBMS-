#include "sql/parser.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>

#include "common/string_utils.h"

namespace mini_dbms::sql {
namespace {

using common::DynamicArray;
using common::Status;

bool IsIdentifierToken(TokenType type) {
    return type == TokenType::kIdentifier;
}

ComparisonOp ToComparisonOp(TokenType type) {
    if (type == TokenType::kLess) {
        return ComparisonOp::kLess;
    }
    if (type == TokenType::kGreater) {
        return ComparisonOp::kGreater;
    }
    return ComparisonOp::kEqual;
}

class ParserImpl {
public:
    explicit ParserImpl(const DynamicArray<Token>& tokens) : tokens_(tokens), position_(0) {}

    ParseResult Parse() {
        ParseResult result;
        if (tokens_.empty()) {
            result.status = Status::InvalidArgument("Empty token stream");
            return result;
        }

        TokenType first = Current().type;
        if (first == TokenType::kCreate) {
            result.status = ParseCreate(&result.command);
        } else if (first == TokenType::kDrop) {
            result.status = ParseDrop(&result.command);
        } else if (first == TokenType::kUse) {
            result.status = ParseUse(&result.command);
        } else if (first == TokenType::kInsert) {
            result.status = ParseInsert(&result.command);
        } else if (first == TokenType::kSelect) {
            result.status = ParseSelect(&result.command);
        } else if (first == TokenType::kDelete) {
            result.status = ParseDelete(&result.command);
        } else if (first == TokenType::kUpdate) {
            result.status = ParseUpdate(&result.command);
        } else {
            result.status = Error("Expected SQL command");
        }

        if (result.status.ok()) {
            result.status = Expect(TokenType::kEnd, "Expected end of SQL statement");
        }
        return result;
    }

private:
    const Token& Current() const { return tokens_[position_]; }

    const Token& Previous() const { return tokens_[position_ - 1]; }

    bool Match(TokenType type) {
        if (Current().type != type) {
            return false;
        }
        ++position_;
        return true;
    }

    Status Expect(TokenType type, const std::string& message) {
        if (!Match(type)) {
            return Error(message);
        }
        return Status::OK();
    }

    Status Error(const std::string& message) const {
        return Status::InvalidArgument(message + " at position " +
                                       std::to_string(Current().position));
    }

    Status ParseIdentifier(std::string* out, const std::string& what) {
        if (!IsIdentifierToken(Current().type)) {
            return Error("Expected " + what);
        }
        if (!common::StringUtils::IsLowerAlphaIdentifier(Current().text)) {
            return Error("Invalid " + what);
        }
        *out = Current().text;
        ++position_;
        return Status::OK();
    }

    Status ParseDataType(DataType* type) {
        if (Match(TokenType::kTypeInt)) {
            *type = DataType::kInt;
            return Status::OK();
        }
        if (Match(TokenType::kTypeString)) {
            *type = DataType::kString;
            return Status::OK();
        }
        return Error("Expected column type");
    }

    Status ParseValue(ValueLiteral* value) {
        if (Match(TokenType::kStringLiteral)) {
            *value = ValueLiteral::String(Previous().text);
            return Status::OK();
        }
        if (!Match(TokenType::kIntLiteral)) {
            return Error("Expected constant value");
        }

        errno = 0;
        char* end = nullptr;
        long parsed = std::strtol(Previous().text.c_str(), &end, 10);
        if (errno != 0 || end == nullptr || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
            return Status::InvalidArgument("Invalid integer literal at position " +
                                           std::to_string(Previous().position));
        }
        *value = ValueLiteral::Int(static_cast<int>(parsed));
        return Status::OK();
    }

    Status ParseComparisonOp(ComparisonOp* op) {
        if (Current().type == TokenType::kEqual || Current().type == TokenType::kLess ||
            Current().type == TokenType::kGreater) {
            *op = ToComparisonOp(Current().type);
            ++position_;
            return Status::OK();
        }
        return Error("Expected comparison operator");
    }

    Status ParseWhere(SqlCommand* command) {
        if (!Match(TokenType::kWhere)) {
            command->has_where = false;
            return Status::OK();
        }
        command->has_where = true;
        Status status = ParseIdentifier(&command->where.column_name, "where column name");
        if (!status.ok()) {
            return status;
        }
        status = ParseComparisonOp(&command->where.op);
        if (!status.ok()) {
            return status;
        }
        return ParseValue(&command->where.value);
    }

    Status ParseCreate(SqlCommand* command) {
        Expect(TokenType::kCreate, "Expected create");
        if (Match(TokenType::kDatabase)) {
            command->type = CommandType::kCreateDatabase;
            return ParseIdentifier(&command->database_name, "database name");
        }
        if (Match(TokenType::kTable)) {
            return ParseCreateTable(command);
        }
        return Error("Expected database or table after create");
    }

    Status ParseCreateTable(SqlCommand* command) {
        command->type = CommandType::kCreateTable;
        Status status = ParseIdentifier(&command->table_name, "table name");
        if (!status.ok()) {
            return status;
        }
        status = Expect(TokenType::kLeftParen, "Expected '(' after table name");
        if (!status.ok()) {
            return status;
        }
        if (Current().type == TokenType::kRightParen) {
            return Error("Expected at least one column definition");
        }

        while (true) {
            ColumnDefinition column;
            status = ParseIdentifier(&column.name, "column name");
            if (!status.ok()) {
                return status;
            }
            status = ParseDataType(&column.type);
            if (!status.ok()) {
                return status;
            }
            column.primary = Match(TokenType::kPrimary);
            status = command->column_definitions.PushBack(column);
            if (!status.ok()) {
                return status;
            }
            if (!Match(TokenType::kComma)) {
                break;
            }
        }
        return Expect(TokenType::kRightParen, "Expected ')' after column definitions");
    }

    Status ParseDrop(SqlCommand* command) {
        Expect(TokenType::kDrop, "Expected drop");
        if (Match(TokenType::kDatabase)) {
            command->type = CommandType::kDropDatabase;
            return ParseIdentifier(&command->database_name, "database name");
        }
        if (Match(TokenType::kTable)) {
            command->type = CommandType::kDropTable;
            return ParseIdentifier(&command->table_name, "table name");
        }
        return Error("Expected database or table after drop");
    }

    Status ParseUse(SqlCommand* command) {
        Expect(TokenType::kUse, "Expected use");
        command->type = CommandType::kUseDatabase;
        return ParseIdentifier(&command->database_name, "database name");
    }

    Status ParseInsert(SqlCommand* command) {
        Expect(TokenType::kInsert, "Expected insert");
        command->type = CommandType::kInsert;
        Status status = ParseIdentifier(&command->table_name, "table name");
        if (!status.ok()) {
            return status;
        }
        status = Expect(TokenType::kValues, "Expected values");
        if (!status.ok()) {
            return status;
        }
        status = Expect(TokenType::kLeftParen, "Expected '(' before values");
        if (!status.ok()) {
            return status;
        }
        if (Current().type == TokenType::kRightParen) {
            return Error("Expected at least one value");
        }
        while (true) {
            ValueLiteral value;
            status = ParseValue(&value);
            if (!status.ok()) {
                return status;
            }
            status = command->values.PushBack(value);
            if (!status.ok()) {
                return status;
            }
            if (!Match(TokenType::kComma)) {
                break;
            }
        }
        return Expect(TokenType::kRightParen, "Expected ')' after values");
    }

    Status ParseSelect(SqlCommand* command) {
        Expect(TokenType::kSelect, "Expected select");
        command->type = CommandType::kSelect;
        if (Match(TokenType::kStar)) {
            command->select_all = true;
        } else {
            command->select_all = false;
            Status status = ParseIdentifierList(&command->selected_columns, "selected column name");
            if (!status.ok()) {
                return status;
            }
        }

        Status status = Expect(TokenType::kFrom, "Expected from");
        if (!status.ok()) {
            return status;
        }
        status = ParseIdentifier(&command->table_name, "table name");
        if (!status.ok()) {
            return status;
        }
        return ParseWhere(command);
    }

    Status ParseIdentifierList(DynamicArray<std::string>* values, const std::string& what) {
        while (true) {
            std::string name;
            Status status = ParseIdentifier(&name, what);
            if (!status.ok()) {
                return status;
            }
            status = values->PushBack(name);
            if (!status.ok()) {
                return status;
            }
            if (!Match(TokenType::kComma)) {
                return Status::OK();
            }
        }
    }

    Status ParseDelete(SqlCommand* command) {
        Expect(TokenType::kDelete, "Expected delete");
        command->type = CommandType::kDelete;
        Status status = ParseIdentifier(&command->table_name, "table name");
        if (!status.ok()) {
            return status;
        }
        return ParseWhere(command);
    }

    Status ParseUpdate(SqlCommand* command) {
        Expect(TokenType::kUpdate, "Expected update");
        command->type = CommandType::kUpdate;
        Status status = ParseIdentifier(&command->table_name, "table name");
        if (!status.ok()) {
            return status;
        }
        status = Expect(TokenType::kSet, "Expected set");
        if (!status.ok()) {
            return status;
        }
        status = ParseIdentifier(&command->assignment_column, "assignment column name");
        if (!status.ok()) {
            return status;
        }
        status = Expect(TokenType::kEqual, "Expected '=' in update assignment");
        if (!status.ok()) {
            return status;
        }
        status = ParseValue(&command->assignment_value);
        if (!status.ok()) {
            return status;
        }
        return ParseWhere(command);
    }

    const DynamicArray<Token>& tokens_;
    std::size_t position_;
};

}  // namespace

ParseResult Parser::Parse(const std::string& sql) const {
    Lexer lexer;
    LexResult lex_result = lexer.Tokenize(sql);
    ParseResult result;
    if (!lex_result.status.ok()) {
        result.status = lex_result.status;
        return result;
    }
    return ParseTokens(lex_result.tokens);
}

ParseResult Parser::ParseTokens(const DynamicArray<Token>& tokens) const {
    ParserImpl impl(tokens);
    return impl.Parse();
}

}  // namespace mini_dbms::sql
