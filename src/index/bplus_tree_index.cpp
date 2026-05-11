#include "index/bplus_tree_index.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <utility>

namespace mini_dbms::index {
namespace {

using common::Status;
using storage::RowId;

constexpr char kIndexMagic[8] = {'M', 'D', 'B', 'I', 'D', 'X', '1', '\0'};
constexpr std::uint32_t kIndexFormatVersion = 1;
constexpr std::uint8_t kIntKeyType = 1;

Status IoError(const std::string& action, const std::filesystem::path& path) {
    return Status::IOError(action + ": " + path.string());
}

Status WriteBytes(std::ostream& output, const char* data, std::size_t size) {
    output.write(data, static_cast<std::streamsize>(size));
    if (!output.good()) {
        return Status::IOError("Failed to write index file");
    }
    return Status::OK();
}

bool ReadExact(std::istream& input, char* data, std::size_t size) {
    input.read(data, static_cast<std::streamsize>(size));
    return input.good() || (size == 0 && !input.bad());
}

Status WriteUInt8(std::ostream& output, std::uint8_t value) {
    char byte = static_cast<char>(value);
    return WriteBytes(output, &byte, 1);
}

Status ReadUInt8(std::istream& input, std::uint8_t* value) {
    char byte = 0;
    if (!ReadExact(input, &byte, 1)) {
        return Status::IOError("Failed to read uint8 from index");
    }
    *value = static_cast<std::uint8_t>(byte);
    return Status::OK();
}

Status WriteUInt32(std::ostream& output, std::uint32_t value) {
    char bytes[4];
    for (int i = 0; i < 4; ++i) {
        bytes[i] = static_cast<char>((value >> (i * 8)) & 0xffu);
    }
    return WriteBytes(output, bytes, 4);
}

Status ReadUInt32(std::istream& input, std::uint32_t* value) {
    char bytes[4];
    if (!ReadExact(input, bytes, 4)) {
        return Status::IOError("Failed to read uint32 from index");
    }
    std::uint32_t result = 0;
    for (int i = 0; i < 4; ++i) {
        result |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[i])) << (i * 8);
    }
    *value = result;
    return Status::OK();
}

Status WriteUInt64(std::ostream& output, std::uint64_t value) {
    char bytes[8];
    for (int i = 0; i < 8; ++i) {
        bytes[i] = static_cast<char>((value >> (i * 8)) & 0xffu);
    }
    return WriteBytes(output, bytes, 8);
}

Status ReadUInt64(std::istream& input, std::uint64_t* value) {
    char bytes[8];
    if (!ReadExact(input, bytes, 8)) {
        return Status::IOError("Failed to read uint64 from index");
    }
    std::uint64_t result = 0;
    for (int i = 0; i < 8; ++i) {
        result |= static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[i])) << (i * 8);
    }
    *value = result;
    return Status::OK();
}

Status WriteInt32(std::ostream& output, int value) {
    return WriteUInt32(output, static_cast<std::uint32_t>(value));
}

Status ReadInt32(std::istream& input, int* value) {
    std::uint32_t raw = 0;
    Status status = ReadUInt32(input, &raw);
    if (!status.ok()) {
        return status;
    }
    *value = static_cast<int>(raw);
    return Status::OK();
}

}  // namespace

BPlusTreeIndex::Node::Node(bool is_leaf)
    : leaf(is_leaf), key_count(0), parent(nullptr), next(nullptr) {
    for (int i = 0; i < kMaxKeys + 1; ++i) {
        keys[i] = 0;
        row_ids[i] = RowId();
    }
    for (int i = 0; i < kMaxChildren; ++i) {
        children[i] = nullptr;
    }
}

BPlusTreeIndex::BPlusTreeIndex() : root_(nullptr), size_(0) {}

BPlusTreeIndex::~BPlusTreeIndex() {
    Clear();
}

Status BPlusTreeIndex::Insert(int key, RowId row_id) {
    if (!row_id.IsValid()) {
        return Status::InvalidArgument("Cannot index invalid RowId");
    }
    if (root_ == nullptr) {
        root_ = new (std::nothrow) Node(true);
        if (root_ == nullptr) {
            return Status::OutOfMemory("Failed to allocate B+ tree root");
        }
        root_->keys[0] = key;
        root_->row_ids[0] = row_id;
        root_->key_count = 1;
        ++size_;
        return Status::OK();
    }
    Node* leaf = FindLeaf(key);
    return InsertIntoLeaf(leaf, key, row_id);
}

IndexLookupResult BPlusTreeIndex::FindEqual(int key) const {
    IndexLookupResult result;
    Node* leaf = FindLeaf(key);
    if (leaf == nullptr) {
        result.status = Status::NotFound("Index key not found");
        return result;
    }
    for (int i = 0; i < leaf->key_count; ++i) {
        if (leaf->keys[i] == key) {
            result.row_id = leaf->row_ids[i];
            result.status = Status::OK();
            return result;
        }
    }
    result.status = Status::NotFound("Index key not found");
    return result;
}

IndexRangeResult BPlusTreeIndex::FindLessThan(int key) const {
    IndexRangeResult result;
    for (Node* leaf = LeftmostLeaf(); leaf != nullptr; leaf = leaf->next) {
        for (int i = 0; i < leaf->key_count; ++i) {
            if (leaf->keys[i] >= key) {
                result.status = Status::OK();
                return result;
            }
            result.status = result.row_ids.PushBack(leaf->row_ids[i]);
            if (!result.status.ok()) {
                return result;
            }
        }
    }
    result.status = Status::OK();
    return result;
}

IndexRangeResult BPlusTreeIndex::FindGreaterThan(int key) const {
    IndexRangeResult result;
    Node* leaf = FindLeaf(key);
    if (leaf == nullptr) {
        result.status = Status::OK();
        return result;
    }
    for (; leaf != nullptr; leaf = leaf->next) {
        for (int i = 0; i < leaf->key_count; ++i) {
            if (leaf->keys[i] > key) {
                result.status = result.row_ids.PushBack(leaf->row_ids[i]);
                if (!result.status.ok()) {
                    return result;
                }
            }
        }
    }
    result.status = Status::OK();
    return result;
}

Status BPlusTreeIndex::SaveToFile(const std::filesystem::path& path) const {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return IoError("Failed to open index file for write", path);
    }
    Status status = WriteBytes(output, kIndexMagic, 8);
    if (!status.ok()) {
        return status;
    }
    status = WriteUInt32(output, kIndexFormatVersion);
    if (!status.ok()) {
        return status;
    }
    status = WriteUInt8(output, kIntKeyType);
    if (!status.ok()) {
        return status;
    }
    status = WriteUInt32(output, static_cast<std::uint32_t>(kMaxKeys));
    if (!status.ok()) {
        return status;
    }
    status = WriteUInt64(output, static_cast<std::uint64_t>(EntryCount()));
    if (!status.ok()) {
        return status;
    }
    for (Node* leaf = LeftmostLeaf(); leaf != nullptr; leaf = leaf->next) {
        for (int i = 0; i < leaf->key_count; ++i) {
            status = WriteInt32(output, leaf->keys[i]);
            if (!status.ok()) {
                return status;
            }
            status = WriteUInt64(output, leaf->row_ids[i].offset);
            if (!status.ok()) {
                return status;
            }
        }
    }
    return output.good() ? Status::OK() : IoError("Failed to write index file", path);
}

Status BPlusTreeIndex::LoadFromFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return IoError("Failed to open index file for read", path);
    }
    char magic[8];
    if (!ReadExact(input, magic, 8)) {
        return Status::IOError("Failed to read index magic");
    }
    for (int i = 0; i < 8; ++i) {
        if (magic[i] != kIndexMagic[i]) {
            return Status::InvalidArgument("Invalid index file magic");
        }
    }
    std::uint32_t version = 0;
    Status status = ReadUInt32(input, &version);
    if (!status.ok()) {
        return status;
    }
    if (version != kIndexFormatVersion) {
        return Status::InvalidArgument("Unsupported index format version");
    }
    std::uint8_t key_type = 0;
    status = ReadUInt8(input, &key_type);
    if (!status.ok()) {
        return status;
    }
    if (key_type != kIntKeyType) {
        return Status::InvalidArgument("Unsupported index key type");
    }
    std::uint32_t saved_order = 0;
    status = ReadUInt32(input, &saved_order);
    if (!status.ok()) {
        return status;
    }
    if (saved_order != static_cast<std::uint32_t>(kMaxKeys)) {
        return Status::InvalidArgument("Index order does not match this build");
    }
    std::uint64_t entry_count = 0;
    status = ReadUInt64(input, &entry_count);
    if (!status.ok()) {
        return status;
    }

    BPlusTreeIndex loaded;
    for (std::uint64_t i = 0; i < entry_count; ++i) {
        int key = 0;
        std::uint64_t offset = RowId::kInvalidOffset;
        status = ReadInt32(input, &key);
        if (!status.ok()) {
            return status;
        }
        status = ReadUInt64(input, &offset);
        if (!status.ok()) {
            return status;
        }
        status = loaded.Insert(key, RowId(offset));
        if (!status.ok()) {
            return status;
        }
    }
    Clear();
    root_ = loaded.root_;
    size_ = loaded.size_;
    loaded.root_ = nullptr;
    loaded.size_ = 0;
    return Status::OK();
}

void BPlusTreeIndex::Clear() {
    Destroy(root_);
    root_ = nullptr;
    size_ = 0;
}

BPlusTreeIndex::Node* BPlusTreeIndex::FindLeaf(int key) const {
    Node* node = root_;
    while (node != nullptr && !node->leaf) {
        int child_index = 0;
        while (child_index < node->key_count && key >= node->keys[child_index]) {
            ++child_index;
        }
        node = node->children[child_index];
    }
    return node;
}

BPlusTreeIndex::Node* BPlusTreeIndex::LeftmostLeaf() const {
    Node* node = root_;
    while (node != nullptr && !node->leaf) {
        node = node->children[0];
    }
    return node;
}

Status BPlusTreeIndex::InsertIntoLeaf(Node* leaf, int key, RowId row_id) {
    int pos = 0;
    while (pos < leaf->key_count && leaf->keys[pos] < key) {
        ++pos;
    }
    if (pos < leaf->key_count && leaf->keys[pos] == key) {
        return Status::AlreadyExists("Duplicate primary key");
    }
    for (int i = leaf->key_count; i > pos; --i) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->row_ids[i] = leaf->row_ids[i - 1];
    }
    leaf->keys[pos] = key;
    leaf->row_ids[pos] = row_id;
    ++leaf->key_count;
    ++size_;
    if (leaf->key_count <= kMaxKeys) {
        return Status::OK();
    }
    return SplitLeaf(leaf);
}

Status BPlusTreeIndex::SplitLeaf(Node* leaf) {
    Node* right = new (std::nothrow) Node(true);
    if (right == nullptr) {
        return Status::OutOfMemory("Failed to allocate B+ tree leaf");
    }
    right->parent = leaf->parent;
    int split = (leaf->key_count + 1) / 2;
    int right_count = leaf->key_count - split;
    for (int i = 0; i < right_count; ++i) {
        right->keys[i] = leaf->keys[split + i];
        right->row_ids[i] = leaf->row_ids[split + i];
    }
    right->key_count = right_count;
    leaf->key_count = split;
    right->next = leaf->next;
    leaf->next = right;
    return InsertIntoParent(leaf, right->keys[0], right);
}

Status BPlusTreeIndex::InsertIntoParent(Node* left, int key, Node* right) {
    if (left->parent == nullptr) {
        Node* new_root = new (std::nothrow) Node(false);
        if (new_root == nullptr) {
            return Status::OutOfMemory("Failed to allocate B+ tree root");
        }
        new_root->keys[0] = key;
        new_root->children[0] = left;
        new_root->children[1] = right;
        new_root->key_count = 1;
        left->parent = new_root;
        right->parent = new_root;
        root_ = new_root;
        return Status::OK();
    }
    return InsertIntoInternal(left->parent, left, key, right);
}

Status BPlusTreeIndex::InsertIntoInternal(Node* parent, Node* left, int key, Node* right) {
    int child_pos = 0;
    while (child_pos <= parent->key_count && parent->children[child_pos] != left) {
        ++child_pos;
    }
    if (child_pos > parent->key_count) {
        return Status::Internal("B+ tree parent link is inconsistent");
    }
    int key_pos = child_pos;
    for (int i = parent->key_count; i > key_pos; --i) {
        parent->keys[i] = parent->keys[i - 1];
    }
    for (int i = parent->key_count + 1; i > key_pos + 1; --i) {
        parent->children[i] = parent->children[i - 1];
    }
    parent->keys[key_pos] = key;
    parent->children[key_pos + 1] = right;
    right->parent = parent;
    ++parent->key_count;
    if (parent->key_count <= kMaxKeys) {
        return Status::OK();
    }
    return SplitInternal(parent);
}

Status BPlusTreeIndex::SplitInternal(Node* node) {
    Node* right = new (std::nothrow) Node(false);
    if (right == nullptr) {
        return Status::OutOfMemory("Failed to allocate B+ tree internal node");
    }
    right->parent = node->parent;
    int mid = node->key_count / 2;
    int promote_key = node->keys[mid];
    int right_key_count = node->key_count - mid - 1;
    for (int i = 0; i < right_key_count; ++i) {
        right->keys[i] = node->keys[mid + 1 + i];
    }
    for (int i = 0; i < right_key_count + 1; ++i) {
        right->children[i] = node->children[mid + 1 + i];
        if (right->children[i] != nullptr) {
            right->children[i]->parent = right;
        }
    }
    right->key_count = right_key_count;
    node->key_count = mid;
    for (int i = node->key_count + 1; i < kMaxChildren; ++i) {
        node->children[i] = nullptr;
    }
    return InsertIntoParent(node, promote_key, right);
}

void BPlusTreeIndex::Destroy(Node* node) {
    if (node == nullptr) {
        return;
    }
    if (!node->leaf) {
        for (int i = 0; i <= node->key_count; ++i) {
            Destroy(node->children[i]);
        }
    }
    delete node;
}

std::size_t BPlusTreeIndex::EntryCount() const {
    std::size_t count = 0;
    for (Node* leaf = LeftmostLeaf(); leaf != nullptr; leaf = leaf->next) {
        count += static_cast<std::size_t>(leaf->key_count);
    }
    return count;
}

}  // namespace mini_dbms::index
