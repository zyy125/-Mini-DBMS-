#include "common/linked_list.h"

#include <string>

#include "testing/test_framework.h"

using mini_dbms::common::LinkedList;

TEST_CASE("LinkedList supports queue-style operations") {
    LinkedList<std::string> list;
    EXPECT_TRUE(list.empty());

    EXPECT_STATUS_OK(list.PushBack("b"));
    EXPECT_STATUS_OK(list.PushFront("a"));
    EXPECT_STATUS_OK(list.PushBack("c"));

    EXPECT_EQ(static_cast<std::size_t>(3), list.size());
    EXPECT_EQ(std::string("a"), list.Front());
    EXPECT_EQ(std::string("c"), list.Back());

    EXPECT_STATUS_OK(list.PopFront());
    EXPECT_EQ(std::string("b"), list.Front());

    list.Clear();
    EXPECT_TRUE(list.empty());
}
