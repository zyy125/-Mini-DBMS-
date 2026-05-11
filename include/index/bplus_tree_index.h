#pragma once

#include <cstddef>
#include <filesystem>

#include "common/dynamic_array.h"
#include "common/status.h"
#include "storage/storage_engine.h"

namespace mini_dbms::index {

struct IndexLookupResult {
    common::Status status;
    storage::RowId row_id;
};

struct IndexRangeResult {
    common::Status status;
    common::DynamicArray<storage::RowId> row_ids;
};

class BPlusTreeIndex {
public:
    static constexpr int kMaxKeys = 4;
    static constexpr int kMaxChildren = kMaxKeys + 2;

    BPlusTreeIndex();
    ~BPlusTreeIndex();

    BPlusTreeIndex(const BPlusTreeIndex&) = delete;
    BPlusTreeIndex& operator=(const BPlusTreeIndex&) = delete;

    common::Status Insert(int key, storage::RowId row_id);
    IndexLookupResult FindEqual(int key) const;
    IndexRangeResult FindLessThan(int key) const;
    IndexRangeResult FindGreaterThan(int key) const;

    common::Status SaveToFile(const std::filesystem::path& path) const;
    common::Status LoadFromFile(const std::filesystem::path& path);

    void Clear();
    bool empty() const { return root_ == nullptr; }
    std::size_t size() const { return size_; }

private:
    struct Node {
        bool leaf;
        int key_count;
        int keys[kMaxKeys + 1];
        storage::RowId row_ids[kMaxKeys + 1];
        Node* children[kMaxChildren];
        Node* parent;
        Node* next;

        explicit Node(bool is_leaf);
    };

    Node* FindLeaf(int key) const;
    Node* LeftmostLeaf() const;
    common::Status InsertIntoLeaf(Node* leaf, int key, storage::RowId row_id);
    common::Status SplitLeaf(Node* leaf);
    common::Status InsertIntoParent(Node* left, int key, Node* right);
    common::Status InsertIntoInternal(Node* parent, Node* left, int key, Node* right);
    common::Status SplitInternal(Node* node);
    void Destroy(Node* node);
    std::size_t EntryCount() const;

    Node* root_;
    std::size_t size_;
};

}  // namespace mini_dbms::index
