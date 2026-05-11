#include "common/dynamic_array.h"

#include <string>

#include "testing/test_framework.h"

using mini_dbms::common::DynamicArray;

TEST_CASE("DynamicArray starts empty and grows") {
    DynamicArray<int> values;
    EXPECT_EQ(static_cast<std::size_t>(0), values.size());
    EXPECT_TRUE(values.empty());

    for (int i = 0; i < 32; ++i) {
        EXPECT_STATUS_OK(values.PushBack(i * 2));
    }

    EXPECT_EQ(static_cast<std::size_t>(32), values.size());
    EXPECT_TRUE(values.capacity() >= values.size());
    EXPECT_EQ(0, values[0]);
    EXPECT_EQ(62, values[31]);
}

TEST_CASE("DynamicArray removes and clears values") {
    DynamicArray<std::string> values;
    EXPECT_STATUS_OK(values.PushBack("a"));
    EXPECT_STATUS_OK(values.PushBack("b"));
    EXPECT_STATUS_OK(values.PushBack("c"));

    EXPECT_STATUS_OK(values.RemoveAt(1));
    EXPECT_EQ(static_cast<std::size_t>(2), values.size());
    EXPECT_EQ(std::string("a"), values[0]);
    EXPECT_EQ(std::string("c"), values[1]);

    values.Clear();
    EXPECT_TRUE(values.empty());
}

TEST_CASE("DynamicArray supports copy and move") {
    DynamicArray<std::string> values;
    EXPECT_STATUS_OK(values.PushBack("one"));
    EXPECT_STATUS_OK(values.PushBack("two"));

    DynamicArray<std::string> copy(values);
    EXPECT_EQ(static_cast<std::size_t>(2), copy.size());
    EXPECT_EQ(std::string("one"), copy[0]);

    DynamicArray<std::string> moved(std::move(copy));
    EXPECT_EQ(static_cast<std::size_t>(2), moved.size());
    EXPECT_EQ(std::string("two"), moved[1]);
    EXPECT_EQ(static_cast<std::size_t>(0), copy.size());
}
