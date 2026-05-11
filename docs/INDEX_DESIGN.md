# B+ Tree Primary Index Design

Phase 5 implements a primary-key B+ tree for `int` keys only. The index maps one unique primary-key value to one `storage::RowId`.

## Public API

Header: `include/index/bplus_tree_index.h`

```cpp
class BPlusTreeIndex {
public:
    common::Status Insert(int key, storage::RowId row_id);
    IndexLookupResult FindEqual(int key) const;
    IndexRangeResult FindLessThan(int key) const;
    IndexRangeResult FindGreaterThan(int key) const;
    common::Status SaveToFile(const std::filesystem::path& path) const;
    common::Status LoadFromFile(const std::filesystem::path& path);
    void Clear();
    bool empty() const;
    std::size_t size() const;
};
```

`IndexLookupResult` contains `Status status` and one `RowId`. Missing keys return `StatusCode::kNotFound`. `IndexRangeResult` contains `Status status` and `DynamicArray<RowId>`.

Duplicate primary-key insertions return `StatusCode::kAlreadyExists`. Invalid `RowId` values return `StatusCode::kInvalidArgument`.

## Tree Order and Node Layout

The current implementation uses:

```text
kMaxKeys = 4
kMaxChildren = 6
```

Each node stores fixed-size arrays:

```text
bool leaf
int key_count
int keys[kMaxKeys + 1]
RowId row_ids[kMaxKeys + 1]      # leaf nodes only
Node* children[kMaxChildren]     # internal nodes only
Node* parent
Node* next                       # leaf linked list
```

The extra key slot is used temporarily during insertion before splitting. Leaf nodes are linked from left to right so `<` and `>` range queries can scan sorted keys without traversing the tree again.

Internal separator keys use the first key of the right child. Exact lookup descends by choosing the first child whose separator is greater than the search key.

## File Format

`.idx` files are serialized field by field in little-endian order. Ordinary C++ structs are not written directly.

```text
bytes[8]  magic = "MDBIDX1\0"
uint32    format_version = 1
uint8     key_type = 1          # int primary key
uint32    max_keys = 4          # protects against loading with a different order
uint64    entry_count

repeated entry_count times in ascending key order:
  int32   key
  uint64  row_id.offset
```

`LoadFromFile` rebuilds the in-memory tree by inserting the sorted entries. This keeps the persistent format simple and independent of pointer addresses or native struct padding.

## Delete Limitation

Phase 5 does not expose an in-place B+ tree delete API. The storage engine already tombstones deleted rows. Until a proper delete operation is added, Phase 6 should handle delete/update index maintenance by rebuilding the table index from live rows after the data operation, or by delaying delete support in indexed execution. This limitation is intentional for the course MVP and avoids under-tested merge/redistribution code.

## Phase 6 Executor Usage

For a table with an `int primary` column:

1. On `create table`, create an empty `<table>.idx` with `BPlusTreeIndex::SaveToFile`.
2. On `insert`, load the index before writing the row or check uniqueness first:
   - `FindEqual(primary_key)` detects an existing key.
   - If not found, insert the row through `StorageEngine::InsertRow`.
   - `Insert(primary_key, row_id)` and `SaveToFile` persist the mapping.
3. On `select ... where primary = const`, call `FindEqual` and then `StorageEngine::ReadRow(table, row_id)`.
4. On `select ... where primary < const`, call `FindLessThan` and read each returned `RowId`.
5. On `select ... where primary > const`, call `FindGreaterThan` and read each returned `RowId`.
6. On `drop table`, storage already removes `<table>.idx`.

If the primary column is not `int`, the executor must fall back to full table scans until string-key indexes are explicitly added.
