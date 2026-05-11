#include "sql/lexer.h"
#include "sql/parser.h"
#include "testing/test_framework.h"

using mini_dbms::common::StatusCode;
using mini_dbms::sql::CommandType;
using mini_dbms::sql::ComparisonOp;
using mini_dbms::sql::DataType;
using mini_dbms::sql::Lexer;
using mini_dbms::sql::Parser;
using mini_dbms::sql::TokenType;
using mini_dbms::sql::ValueType;

namespace {

mini_dbms::sql::ParseResult ParseSql(const std::string& sql) {
    Parser parser;
    return parser.Parse(sql);
}

void ExpectParseError(const std::string& sql) {
    mini_dbms::sql::ParseResult result = ParseSql(sql);
    EXPECT_TRUE(!result.status.ok());
    EXPECT_EQ(StatusCode::kInvalidArgument, result.status.code());
}

}  // namespace

TEST_CASE("Lexer tokenizes SQL keywords, symbols, and literals") {
    Lexer lexer;
    mini_dbms::sql::LexResult result =
        lexer.Tokenize("insert users values (1, \"alice\")");

    EXPECT_STATUS_OK(result.status);
    EXPECT_EQ(static_cast<std::size_t>(9), result.tokens.size());
    EXPECT_EQ(TokenType::kInsert, result.tokens[0].type);
    EXPECT_EQ(TokenType::kIdentifier, result.tokens[1].type);
    EXPECT_EQ(std::string("users"), result.tokens[1].text);
    EXPECT_EQ(TokenType::kValues, result.tokens[2].type);
    EXPECT_EQ(TokenType::kLeftParen, result.tokens[3].type);
    EXPECT_EQ(TokenType::kIntLiteral, result.tokens[4].type);
    EXPECT_EQ(std::string("1"), result.tokens[4].text);
    EXPECT_EQ(TokenType::kStringLiteral, result.tokens[6].type);
    EXPECT_EQ(std::string("alice"), result.tokens[6].text);
    EXPECT_EQ(TokenType::kRightParen, result.tokens[7].type);
    EXPECT_EQ(TokenType::kEnd, result.tokens[8].type);
}

TEST_CASE("Parser parses create database") {
    mini_dbms::sql::ParseResult result = ParseSql("create database school");

    EXPECT_STATUS_OK(result.status);
    EXPECT_EQ(CommandType::kCreateDatabase, result.command.type);
    EXPECT_EQ(std::string("school"), result.command.database_name);
}

TEST_CASE("Parser parses drop database and use database") {
    mini_dbms::sql::ParseResult drop = ParseSql("drop database school");
    mini_dbms::sql::ParseResult use = ParseSql("use school");

    EXPECT_STATUS_OK(drop.status);
    EXPECT_EQ(CommandType::kDropDatabase, drop.command.type);
    EXPECT_EQ(std::string("school"), drop.command.database_name);

    EXPECT_STATUS_OK(use.status);
    EXPECT_EQ(CommandType::kUseDatabase, use.command.type);
    EXPECT_EQ(std::string("school"), use.command.database_name);
}

TEST_CASE("Parser parses create table with primary column") {
    mini_dbms::sql::ParseResult result =
        ParseSql("create table students (id int primary, name string, age int)");

    EXPECT_STATUS_OK(result.status);
    EXPECT_EQ(CommandType::kCreateTable, result.command.type);
    EXPECT_EQ(std::string("students"), result.command.table_name);
    EXPECT_EQ(static_cast<std::size_t>(3), result.command.column_definitions.size());
    EXPECT_EQ(std::string("id"), result.command.column_definitions[0].name);
    EXPECT_EQ(DataType::kInt, result.command.column_definitions[0].type);
    EXPECT_TRUE(result.command.column_definitions[0].primary);
    EXPECT_EQ(std::string("name"), result.command.column_definitions[1].name);
    EXPECT_EQ(DataType::kString, result.command.column_definitions[1].type);
    EXPECT_TRUE(!result.command.column_definitions[1].primary);
}

TEST_CASE("Parser parses drop table") {
    mini_dbms::sql::ParseResult result = ParseSql("drop table students");

    EXPECT_STATUS_OK(result.status);
    EXPECT_EQ(CommandType::kDropTable, result.command.type);
    EXPECT_EQ(std::string("students"), result.command.table_name);
}

TEST_CASE("Parser parses insert values") {
    mini_dbms::sql::ParseResult result =
        ParseSql("insert students values (1, \"alice\", -20)");

    EXPECT_STATUS_OK(result.status);
    EXPECT_EQ(CommandType::kInsert, result.command.type);
    EXPECT_EQ(std::string("students"), result.command.table_name);
    EXPECT_EQ(static_cast<std::size_t>(3), result.command.values.size());
    EXPECT_EQ(ValueType::kInt, result.command.values[0].type);
    EXPECT_EQ(1, result.command.values[0].int_value);
    EXPECT_EQ(ValueType::kString, result.command.values[1].type);
    EXPECT_EQ(std::string("alice"), result.command.values[1].string_value);
    EXPECT_EQ(-20, result.command.values[2].int_value);
}

TEST_CASE("Parser parses select star with where") {
    mini_dbms::sql::ParseResult result =
        ParseSql("select * from students where id = 1");

    EXPECT_STATUS_OK(result.status);
    EXPECT_EQ(CommandType::kSelect, result.command.type);
    EXPECT_TRUE(result.command.select_all);
    EXPECT_EQ(std::string("students"), result.command.table_name);
    EXPECT_TRUE(result.command.has_where);
    EXPECT_EQ(std::string("id"), result.command.where.column_name);
    EXPECT_EQ(ComparisonOp::kEqual, result.command.where.op);
    EXPECT_EQ(ValueType::kInt, result.command.where.value.type);
    EXPECT_EQ(1, result.command.where.value.int_value);
}

TEST_CASE("Parser parses select column list without where") {
    mini_dbms::sql::ParseResult result = ParseSql("select id, name from students");

    EXPECT_STATUS_OK(result.status);
    EXPECT_EQ(CommandType::kSelect, result.command.type);
    EXPECT_TRUE(!result.command.select_all);
    EXPECT_EQ(static_cast<std::size_t>(2), result.command.selected_columns.size());
    EXPECT_EQ(std::string("id"), result.command.selected_columns[0]);
    EXPECT_EQ(std::string("name"), result.command.selected_columns[1]);
    EXPECT_TRUE(!result.command.has_where);
}

TEST_CASE("Parser parses delete with and without where") {
    mini_dbms::sql::ParseResult all_rows = ParseSql("delete students");
    mini_dbms::sql::ParseResult filtered = ParseSql("delete students where age > 18");

    EXPECT_STATUS_OK(all_rows.status);
    EXPECT_EQ(CommandType::kDelete, all_rows.command.type);
    EXPECT_TRUE(!all_rows.command.has_where);

    EXPECT_STATUS_OK(filtered.status);
    EXPECT_EQ(CommandType::kDelete, filtered.command.type);
    EXPECT_TRUE(filtered.command.has_where);
    EXPECT_EQ(std::string("age"), filtered.command.where.column_name);
    EXPECT_EQ(ComparisonOp::kGreater, filtered.command.where.op);
    EXPECT_EQ(18, filtered.command.where.value.int_value);
}

TEST_CASE("Parser parses update with where") {
    mini_dbms::sql::ParseResult result =
        ParseSql("update students set name = \"bob\" where id < 10");

    EXPECT_STATUS_OK(result.status);
    EXPECT_EQ(CommandType::kUpdate, result.command.type);
    EXPECT_EQ(std::string("students"), result.command.table_name);
    EXPECT_EQ(std::string("name"), result.command.assignment_column);
    EXPECT_EQ(ValueType::kString, result.command.assignment_value.type);
    EXPECT_EQ(std::string("bob"), result.command.assignment_value.string_value);
    EXPECT_TRUE(result.command.has_where);
    EXPECT_EQ(std::string("id"), result.command.where.column_name);
    EXPECT_EQ(ComparisonOp::kLess, result.command.where.op);
    EXPECT_EQ(10, result.command.where.value.int_value);
}

TEST_CASE("Parser rejects unsupported or malformed SQL") {
    ExpectParseError("create table students ()");
    ExpectParseError("create table students (id float)");
    ExpectParseError("insert students values ()");
    ExpectParseError("select * from students order by id");
    ExpectParseError("delete from students");
    ExpectParseError("update students name = \"bob\"");
    ExpectParseError("select * from students where id != 1");
}

TEST_CASE("Parser rejects invalid identifiers and strings") {
    ExpectParseError("create database school_db");
    ExpectParseError("create database School");
    ExpectParseError("create table students (student_id int)");
    ExpectParseError("insert students values (\"unterminated)");
}
