#include "common/hash_table.h"

#include <string>

#include "testing/test_framework.h"

using mini_dbms::common::HashTable;

TEST_CASE("HashTable stores and updates string keys") {
    HashTable<std::string, int> table;
    EXPECT_TRUE(table.empty());

    EXPECT_STATUS_OK(table.Put("alpha", 1));
    EXPECT_STATUS_OK(table.Put("beta", 2));
    EXPECT_TRUE(table.Contains("alpha"));
    EXPECT_TRUE(table.Contains("beta"));
    EXPECT_EQ(1, *table.Get("alpha"));

    EXPECT_STATUS_OK(table.Put("alpha", 10));
    EXPECT_EQ(static_cast<std::size_t>(2), table.size());
    EXPECT_EQ(10, *table.Get("alpha"));
}

TEST_CASE("HashTable grows and keeps colliding integer keys") {
    HashTable<int, std::string> table(8);

    for (int i = 0; i < 80; ++i) {
        EXPECT_STATUS_OK(table.Put(i * 8, std::string("v") + std::to_string(i)));
    }

    EXPECT_EQ(static_cast<std::size_t>(80), table.size());
    for (int i = 0; i < 80; ++i) {
        std::string* value = table.Get(i * 8);
        EXPECT_TRUE(value != nullptr);
        EXPECT_EQ(std::string("v") + std::to_string(i), *value);
    }
}

TEST_CASE("HashTable removes and reuses slots") {
    HashTable<std::string, std::string> table;
    EXPECT_STATUS_OK(table.Put("first", "1"));
    EXPECT_STATUS_OK(table.Put("second", "2"));
    EXPECT_STATUS_OK(table.Put("third", "3"));

    EXPECT_STATUS_OK(table.Remove("second"));
    EXPECT_TRUE(!table.Contains("second"));
    EXPECT_EQ(static_cast<std::size_t>(2), table.size());

    EXPECT_STATUS_OK(table.Put("fourth", "4"));
    EXPECT_EQ(std::string("4"), *table.Get("fourth"));
    EXPECT_EQ(static_cast<std::size_t>(3), table.size());

    table.Clear();
    EXPECT_TRUE(table.empty());
}
