#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <utility>

namespace mini_dbms::sql {

enum class TokenType {
    kEnd,
    kIdentifier,
    kIntLiteral,
    kStringLiteral,
    kCreate,
    kDatabase,
    kDrop,
    kUse,
    kTable,
    kPrimary,
    kInsert,
    kValues,
    kSelect,
    kFrom,
    kWhere,
    kDelete,
    kUpdate,
    kSet,
    kTypeInt,
    kTypeString,
    kStar,
    kComma,
    kLeftParen,
    kRightParen,
    kEqual,
    kLess,
    kGreater
};

struct Token {
    TokenType type;
    std::string text;
    std::size_t position;

    Token() : type(TokenType::kEnd), text(), position(0) {}
    Token(TokenType token_type, std::string token_text, std::size_t token_position)
        : type(token_type), text(std::move(token_text)), position(token_position) {}
};

inline std::ostream& operator<<(std::ostream& os, TokenType type) {
    os << static_cast<int>(type);
    return os;
}

}  // namespace mini_dbms::sql
