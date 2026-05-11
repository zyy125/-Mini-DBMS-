#pragma once

#include <string>

#include "common/dynamic_array.h"
#include "common/status.h"
#include "sql/token.h"

namespace mini_dbms::sql {

struct LexResult {
    common::Status status;
    common::DynamicArray<Token> tokens;
};

class Lexer {
public:
    LexResult Tokenize(const std::string& sql) const;
};

}  // namespace mini_dbms::sql
