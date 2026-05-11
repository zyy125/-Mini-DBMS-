#include "common/string_utils.h"

#include "testing/test_framework.h"

namespace StringUtils = mini_dbms::common::StringUtils;

TEST_CASE("StringUtils Split keeps empty fields") {
    mini_dbms::common::DynamicArray<std::string> parts = StringUtils::Split("a,,b,", ',');
    EXPECT_EQ(static_cast<std::size_t>(4), parts.size());
    EXPECT_EQ(std::string("a"), parts[0]);
    EXPECT_EQ(std::string(""), parts[1]);
    EXPECT_EQ(std::string("b"), parts[2]);
    EXPECT_EQ(std::string(""), parts[3]);
}

TEST_CASE("StringUtils SplitWhitespace drops repeated whitespace") {
    mini_dbms::common::DynamicArray<std::string> parts =
        StringUtils::SplitWhitespace("  create\t database\nabc  ");
    EXPECT_EQ(static_cast<std::size_t>(3), parts.size());
    EXPECT_EQ(std::string("create"), parts[0]);
    EXPECT_EQ(std::string("database"), parts[1]);
    EXPECT_EQ(std::string("abc"), parts[2]);
}

TEST_CASE("StringUtils validates simple identifiers and trims") {
    EXPECT_TRUE(StringUtils::IsLowerAlphaIdentifier("person"));
    EXPECT_TRUE(!StringUtils::IsLowerAlphaIdentifier("person1"));
    EXPECT_TRUE(!StringUtils::IsLowerAlphaIdentifier("Person"));
    EXPECT_TRUE(!StringUtils::IsLowerAlphaIdentifier("person_name"));
    EXPECT_EQ(std::string("select"), StringUtils::Trim(" \tselect\r\n"));
}
