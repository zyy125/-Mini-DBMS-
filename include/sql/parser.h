#pragma once

#include <string>

#include "common/status.h"
#include "sql/command.h"
#include "sql/lexer.h"

namespace mini_dbms::sql {

struct ParseResult {
    common::Status status;
    SqlCommand command;
};

class Parser {
public:
    ParseResult Parse(const std::string& sql) const;
    ParseResult ParseTokens(const common::DynamicArray<Token>& tokens) const;
};

}  // namespace mini_dbms::sql
