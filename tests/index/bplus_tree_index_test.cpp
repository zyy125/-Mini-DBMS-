#include "index/bplus_tree_index.h"

#include <filesystem>
#include <string>

#include "common/status.h"
#include "testing/test_framework.h"

using mini_dbms::common::StatusCode;
using mini_dbms::index::BPlusTreeIndex;
using mini_dbms::storage::RowId;

namespace {

std::filesystem::path TestIndexPath(const std::string& name) {
    std::filesystem::path root = std::filesystem::temp_directory_path() / ("mini_dbms_" + name);
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    return root / "students.idx";
}

RowId Rid(std::uint64_t offset) {
    return RowId(offset);
}

}  // namespace

TEST_CASE("BPlusTreeIndex returns not found for empty tree") {
    BPlusTreeIndex index;

    EXPECT_EQ(StatusCode::kNotFound, index.FindEqual(1).status.code());
    EXPECT_STATUS_OK(index.FindLessThan(10).status);
    EXPECT_EQ(static_cast<std::size_t>(0), index.FindGreaterThan(10).row_ids.size());
}

TEST_CASE("BPlusTreeIndex inserts and finds exact keys") {
    BPlusTreeIndex index;

    EXPECT_STATUS_OK(index.Insert(3, Rid(30)));
    EXPECT_STATUS_OK(index.Insert(1, Rid(10)));
    EXPECT_STATUS_OK(index.Insert(5, Rid(50)));

    mini_dbms::index::IndexLookupResult found = index.FindEqual(1);
    EXPECT_STATUS_OK(found.status);
    EXPECT_EQ(static_cast<std::uint64_t>(10), found.row_id.offset);
    EXPECT_EQ(StatusCode::kNotFound, index.FindEqual(4).status.code());
}

TEST_CASE("BPlusTreeIndex rejects duplicate primary key") {
    BPlusTreeIndex index;

    EXPECT_STATUS_OK(index.Insert(7, Rid(70)));
    EXPECT_EQ(StatusCode::kAlreadyExists, index.Insert(7, Rid(700)).code());
    EXPECT_EQ(static_cast<std::size_t>(1), index.size());
    EXPECT_EQ(static_cast<std::uint64_t>(70), index.FindEqual(7).row_id.offset);
}

TEST_CASE("BPlusTreeIndex split leaves and internal nodes") {
    BPlusTreeIndex index;

    for (int i = 30; i >= 1; --i) {
        EXPECT_STATUS_OK(index.Insert(i, Rid(static_cast<std::uint64_t>(i * 100))));
    }

    EXPECT_EQ(static_cast<std::size_t>(30), index.size());
    for (int i = 1; i <= 30; ++i) {
        mini_dbms::index::IndexLookupResult found = index.FindEqual(i);
        EXPECT_STATUS_OK(found.status);
        EXPECT_EQ(static_cast<std::uint64_t>(i * 100), found.row_id.offset);
    }
}

TEST_CASE("BPlusTreeIndex supports less than and greater than range lookup") {
    BPlusTreeIndex index;

    EXPECT_STATUS_OK(index.Insert(10, Rid(100)));
    EXPECT_STATUS_OK(index.Insert(20, Rid(200)));
    EXPECT_STATUS_OK(index.Insert(30, Rid(300)));
    EXPECT_STATUS_OK(index.Insert(40, Rid(400)));
    EXPECT_STATUS_OK(index.Insert(50, Rid(500)));

    mini_dbms::index::IndexRangeResult less = index.FindLessThan(35);
    EXPECT_STATUS_OK(less.status);
    EXPECT_EQ(static_cast<std::size_t>(3), less.row_ids.size());
    EXPECT_EQ(static_cast<std::uint64_t>(100), less.row_ids[0].offset);
    EXPECT_EQ(static_cast<std::uint64_t>(200), less.row_ids[1].offset);
    EXPECT_EQ(static_cast<std::uint64_t>(300), less.row_ids[2].offset);

    mini_dbms::index::IndexRangeResult greater = index.FindGreaterThan(30);
    EXPECT_STATUS_OK(greater.status);
    EXPECT_EQ(static_cast<std::size_t>(2), greater.row_ids.size());
    EXPECT_EQ(static_cast<std::uint64_t>(400), greater.row_ids[0].offset);
    EXPECT_EQ(static_cast<std::uint64_t>(500), greater.row_ids[1].offset);
}

TEST_CASE("BPlusTreeIndex saves and loads idx file") {
    std::filesystem::path path = TestIndexPath("index_persistence");
    BPlusTreeIndex index;

    for (int i = 1; i <= 18; ++i) {
        EXPECT_STATUS_OK(index.Insert(i * 2, Rid(static_cast<std::uint64_t>(i + 1000))));
    }
    EXPECT_STATUS_OK(index.SaveToFile(path));
    EXPECT_TRUE(std::filesystem::exists(path));

    BPlusTreeIndex loaded;
    EXPECT_STATUS_OK(loaded.LoadFromFile(path));
    EXPECT_EQ(static_cast<std::size_t>(18), loaded.size());
    EXPECT_EQ(static_cast<std::uint64_t>(1005), loaded.FindEqual(10).row_id.offset);
    EXPECT_EQ(StatusCode::kNotFound, loaded.FindEqual(11).status.code());

    mini_dbms::index::IndexRangeResult greater = loaded.FindGreaterThan(30);
    EXPECT_STATUS_OK(greater.status);
    EXPECT_EQ(static_cast<std::size_t>(3), greater.row_ids.size());
    if (greater.row_ids.size() == 3) {
        EXPECT_EQ(static_cast<std::uint64_t>(1016), greater.row_ids[0].offset);
        EXPECT_EQ(static_cast<std::uint64_t>(1017), greater.row_ids[1].offset);
        EXPECT_EQ(static_cast<std::uint64_t>(1018), greater.row_ids[2].offset);
    }
}
