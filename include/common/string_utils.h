#pragma once

#include <cstddef>
#include <string>

#include "common/dynamic_array.h"

namespace mini_dbms::common::StringUtils {

inline DynamicArray<std::string> Split(const std::string& input, char delimiter) {
    DynamicArray<std::string> parts;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= input.size(); ++i) {
        if (i == input.size() || input[i] == delimiter) {
            parts.PushBack(input.substr(start, i - start));
            start = i + 1;
        }
    }
    return parts;
}

inline DynamicArray<std::string> SplitWhitespace(const std::string& input) {
    DynamicArray<std::string> parts;
    std::size_t i = 0;
    while (i < input.size()) {
        while (i < input.size() &&
               (input[i] == ' ' || input[i] == '\t' || input[i] == '\n' || input[i] == '\r')) {
            ++i;
        }
        std::size_t start = i;
        while (i < input.size() &&
               !(input[i] == ' ' || input[i] == '\t' || input[i] == '\n' || input[i] == '\r')) {
            ++i;
        }
        if (start < i) {
            parts.PushBack(input.substr(start, i - start));
        }
    }
    return parts;
}

inline bool IsLowerAlphaIdentifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] < 'a' || value[i] > 'z') {
            return false;
        }
    }
    return true;
}

inline std::string Trim(const std::string& input) {
    std::size_t begin = 0;
    while (begin < input.size() &&
           (input[begin] == ' ' || input[begin] == '\t' ||
            input[begin] == '\n' || input[begin] == '\r')) {
        ++begin;
    }
    std::size_t end = input.size();
    while (end > begin &&
           (input[end - 1] == ' ' || input[end - 1] == '\t' ||
            input[end - 1] == '\n' || input[end - 1] == '\r')) {
        --end;
    }
    return input.substr(begin, end - begin);
}

}  // namespace mini_dbms::common::StringUtils
