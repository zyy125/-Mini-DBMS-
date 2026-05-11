#include "sql/lexer.h"

#include <cctype>
#include <string>

namespace mini_dbms::sql {
namespace {

bool IsWhitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

bool IsDigit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool IsLowerAlpha(char ch) {
    return ch >= 'a' && ch <= 'z';
}

TokenType KeywordOrIdentifier(const std::string& text) {
    if (text == "create") {
        return TokenType::kCreate;
    }
    if (text == "database") {
        return TokenType::kDatabase;
    }
    if (text == "drop") {
        return TokenType::kDrop;
    }
    if (text == "use") {
        return TokenType::kUse;
    }
    if (text == "table") {
        return TokenType::kTable;
    }
    if (text == "primary") {
        return TokenType::kPrimary;
    }
    if (text == "insert") {
        return TokenType::kInsert;
    }
    if (text == "values") {
        return TokenType::kValues;
    }
    if (text == "select") {
        return TokenType::kSelect;
    }
    if (text == "from") {
        return TokenType::kFrom;
    }
    if (text == "where") {
        return TokenType::kWhere;
    }
    if (text == "delete") {
        return TokenType::kDelete;
    }
    if (text == "update") {
        return TokenType::kUpdate;
    }
    if (text == "set") {
        return TokenType::kSet;
    }
    if (text == "int") {
        return TokenType::kTypeInt;
    }
    if (text == "string") {
        return TokenType::kTypeString;
    }
    return TokenType::kIdentifier;
}

common::Status PushToken(common::DynamicArray<Token>* tokens,
                         TokenType type,
                         const std::string& text,
                         std::size_t position) {
    return tokens->EmplaceBack(type, text, position);
}

}  // namespace

LexResult Lexer::Tokenize(const std::string& sql) const {
    LexResult result;
    std::size_t i = 0;

    while (i < sql.size()) {
        if (IsWhitespace(sql[i])) {
            ++i;
            continue;
        }

        const std::size_t position = i;
        const char ch = sql[i];

        if (IsLowerAlpha(ch)) {
            while (i < sql.size() && IsLowerAlpha(sql[i])) {
                ++i;
            }
            std::string text = sql.substr(position, i - position);
            result.status = PushToken(&result.tokens, KeywordOrIdentifier(text), text, position);
            if (!result.status.ok()) {
                return result;
            }
            continue;
        }

        if (IsDigit(ch) || (ch == '-' && i + 1 < sql.size() && IsDigit(sql[i + 1]))) {
            if (ch == '-') {
                ++i;
            }
            while (i < sql.size() && IsDigit(sql[i])) {
                ++i;
            }
            std::string text = sql.substr(position, i - position);
            result.status = PushToken(&result.tokens, TokenType::kIntLiteral, text, position);
            if (!result.status.ok()) {
                return result;
            }
            continue;
        }

        if (ch == '"') {
            ++i;
            std::string value;
            while (i < sql.size() && sql[i] != '"') {
                value.push_back(sql[i]);
                ++i;
            }
            if (i >= sql.size()) {
                result.status =
                    common::Status::InvalidArgument("Unterminated string literal at position " +
                                                    std::to_string(position));
                return result;
            }
            ++i;
            result.status = PushToken(&result.tokens, TokenType::kStringLiteral, value, position);
            if (!result.status.ok()) {
                return result;
            }
            continue;
        }

        TokenType symbol_type = TokenType::kEnd;
        switch (ch) {
            case '*':
                symbol_type = TokenType::kStar;
                break;
            case ',':
                symbol_type = TokenType::kComma;
                break;
            case '(':
                symbol_type = TokenType::kLeftParen;
                break;
            case ')':
                symbol_type = TokenType::kRightParen;
                break;
            case '=':
                symbol_type = TokenType::kEqual;
                break;
            case '<':
                symbol_type = TokenType::kLess;
                break;
            case '>':
                symbol_type = TokenType::kGreater;
                break;
            default:
                result.status = common::Status::InvalidArgument(
                    "Unexpected character at position " + std::to_string(position));
                return result;
        }

        result.status = PushToken(&result.tokens, symbol_type, sql.substr(position, 1), position);
        if (!result.status.ok()) {
            return result;
        }
        ++i;
    }

    result.status = PushToken(&result.tokens, TokenType::kEnd, "", sql.size());
    return result;
}

}  // namespace mini_dbms::sql
