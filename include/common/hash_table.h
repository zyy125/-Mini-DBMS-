#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "common/status.h"

namespace mini_dbms::common {

inline std::size_t HashKey(int key) {
    return static_cast<std::size_t>(static_cast<std::uint32_t>(key) * 2654435761u);
}

inline std::size_t HashKey(std::size_t key) {
    return key * static_cast<std::size_t>(11400714819323198485ull);
}

inline std::size_t HashKey(const std::string& key) {
    std::size_t hash = 1469598103934665603ull;
    for (std::size_t i = 0; i < key.size(); ++i) {
        hash ^= static_cast<unsigned char>(key[i]);
        hash *= 1099511628211ull;
    }
    return hash;
}

template <typename K, typename V>
class HashTable {
public:
    HashTable() : entries_(nullptr), capacity_(0), size_(0), tombstones_(0) {
        Initialize(8);
    }

    explicit HashTable(std::size_t initial_capacity)
        : entries_(nullptr), capacity_(0), size_(0), tombstones_(0) {
        Initialize(NormalizeCapacity(initial_capacity));
    }

    HashTable(const HashTable& other)
        : entries_(nullptr), capacity_(0), size_(0), tombstones_(0) {
        Initialize(other.capacity_ == 0 ? 8 : other.capacity_);
        for (std::size_t i = 0; i < other.capacity_; ++i) {
            if (other.entries_[i].state == EntryState::kOccupied) {
                Put(other.entries_[i].Key(), other.entries_[i].Value());
            }
        }
    }

    HashTable(HashTable&& other) noexcept
        : entries_(other.entries_),
          capacity_(other.capacity_),
          size_(other.size_),
          tombstones_(other.tombstones_) {
        other.entries_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
        other.tombstones_ = 0;
    }

    HashTable& operator=(const HashTable& other) {
        if (this == &other) {
            return *this;
        }
        HashTable copy(other);
        Swap(copy);
        return *this;
    }

    HashTable& operator=(HashTable&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Deallocate();
        entries_ = other.entries_;
        capacity_ = other.capacity_;
        size_ = other.size_;
        tombstones_ = other.tombstones_;
        other.entries_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
        other.tombstones_ = 0;
        return *this;
    }

    ~HashTable() { Deallocate(); }

    Status Put(const K& key, const V& value) {
        Status status = EnsureLoadForInsert();
        if (!status.ok()) {
            return status;
        }
        return PutWithoutResize(key, value);
    }

    Status Put(const K& key, V&& value) {
        Status status = EnsureLoadForInsert();
        if (!status.ok()) {
            return status;
        }
        return PutWithoutResize(key, std::move(value));
    }

    V* Get(const K& key) {
        std::size_t index = FindIndex(key);
        return index == kNotFound ? nullptr : &entries_[index].Value();
    }

    const V* Get(const K& key) const {
        std::size_t index = FindIndex(key);
        return index == kNotFound ? nullptr : &entries_[index].Value();
    }

    bool Contains(const K& key) const { return FindIndex(key) != kNotFound; }

    Status Remove(const K& key) {
        std::size_t index = FindIndex(key);
        if (index == kNotFound) {
            return Status::NotFound("HashTable key not found");
        }
        entries_[index].Key().~K();
        entries_[index].Value().~V();
        entries_[index].state = EntryState::kDeleted;
        --size_;
        ++tombstones_;
        return Status::OK();
    }

    void Clear() {
        for (std::size_t i = 0; i < capacity_; ++i) {
            if (entries_[i].state == EntryState::kOccupied) {
                entries_[i].Key().~K();
                entries_[i].Value().~V();
            }
            entries_[i].state = EntryState::kEmpty;
        }
        size_ = 0;
        tombstones_ = 0;
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    void Swap(HashTable& other) noexcept {
        Entry* other_entries = other.entries_;
        std::size_t other_capacity = other.capacity_;
        std::size_t other_size = other.size_;
        std::size_t other_tombstones = other.tombstones_;
        other.entries_ = entries_;
        other.capacity_ = capacity_;
        other.size_ = size_;
        other.tombstones_ = tombstones_;
        entries_ = other_entries;
        capacity_ = other_capacity;
        size_ = other_size;
        tombstones_ = other_tombstones;
    }

private:
    enum class EntryState { kEmpty, kOccupied, kDeleted };

    struct Entry {
        EntryState state;
        alignas(K) unsigned char key_storage[sizeof(K)];
        alignas(V) unsigned char value_storage[sizeof(V)];

        K& Key() { return *reinterpret_cast<K*>(key_storage); }
        const K& Key() const { return *reinterpret_cast<const K*>(key_storage); }
        V& Value() { return *reinterpret_cast<V*>(value_storage); }
        const V& Value() const { return *reinterpret_cast<const V*>(value_storage); }
    };

    static constexpr std::size_t kNotFound = static_cast<std::size_t>(-1);

    static std::size_t NormalizeCapacity(std::size_t requested) {
        std::size_t capacity = 8;
        while (capacity < requested) {
            capacity *= 2;
        }
        return capacity;
    }

    Status Initialize(std::size_t capacity) {
        entries_ = static_cast<Entry*>(::operator new(sizeof(Entry) * capacity, std::nothrow));
        if (entries_ == nullptr) {
            capacity_ = 0;
            return Status::OutOfMemory("HashTable allocation failed");
        }
        capacity_ = capacity;
        for (std::size_t i = 0; i < capacity_; ++i) {
            new (entries_ + i) Entry();
            entries_[i].state = EntryState::kEmpty;
        }
        return Status::OK();
    }

    Status EnsureLoadForInsert() {
        if (capacity_ == 0) {
            return Initialize(8);
        }
        if ((size_ + tombstones_ + 1) * 10 < capacity_ * 7) {
            return Status::OK();
        }
        return Rehash(capacity_ * 2);
    }

    Status Rehash(std::size_t new_capacity) {
        HashTable replacement(new_capacity);
        if (replacement.capacity_ == 0) {
            return Status::OutOfMemory("HashTable rehash allocation failed");
        }
        for (std::size_t i = 0; i < capacity_; ++i) {
            if (entries_[i].state == EntryState::kOccupied) {
                Status status = replacement.PutWithoutResize(
                    std::move(entries_[i].Key()), std::move(entries_[i].Value()));
                if (!status.ok()) {
                    return status;
                }
            }
        }
        Swap(replacement);
        return Status::OK();
    }

    template <typename ValueArg>
    Status PutWithoutResize(const K& key, ValueArg&& value) {
        std::size_t first_deleted = kNotFound;
        std::size_t index = HashKey(key) & (capacity_ - 1);
        for (std::size_t probe = 0; probe < capacity_; ++probe) {
            Entry& entry = entries_[index];
            if (entry.state == EntryState::kOccupied) {
                if (entry.Key() == key) {
                    entry.Value() = std::forward<ValueArg>(value);
                    return Status::OK();
                }
            } else if (entry.state == EntryState::kDeleted) {
                if (first_deleted == kNotFound) {
                    first_deleted = index;
                }
            } else {
                std::size_t target = first_deleted == kNotFound ? index : first_deleted;
                InsertAt(target, key, std::forward<ValueArg>(value));
                if (first_deleted != kNotFound) {
                    --tombstones_;
                }
                ++size_;
                return Status::OK();
            }
            index = (index + 1) & (capacity_ - 1);
        }
        if (first_deleted != kNotFound) {
            InsertAt(first_deleted, key, std::forward<ValueArg>(value));
            --tombstones_;
            ++size_;
            return Status::OK();
        }
        return Status::Internal("HashTable insert probe exhausted");
    }

    template <typename ValueArg>
    Status PutWithoutResize(K&& key, ValueArg&& value) {
        std::size_t first_deleted = kNotFound;
        std::size_t index = HashKey(key) & (capacity_ - 1);
        for (std::size_t probe = 0; probe < capacity_; ++probe) {
            Entry& entry = entries_[index];
            if (entry.state == EntryState::kOccupied) {
                if (entry.Key() == key) {
                    entry.Value() = std::forward<ValueArg>(value);
                    return Status::OK();
                }
            } else if (entry.state == EntryState::kDeleted) {
                if (first_deleted == kNotFound) {
                    first_deleted = index;
                }
            } else {
                std::size_t target = first_deleted == kNotFound ? index : first_deleted;
                InsertAt(target, std::move(key), std::forward<ValueArg>(value));
                if (first_deleted != kNotFound) {
                    --tombstones_;
                }
                ++size_;
                return Status::OK();
            }
            index = (index + 1) & (capacity_ - 1);
        }
        return Status::Internal("HashTable insert probe exhausted");
    }

    template <typename KeyArg, typename ValueArg>
    void InsertAt(std::size_t index, KeyArg&& key, ValueArg&& value) {
        new (entries_[index].key_storage) K(std::forward<KeyArg>(key));
        new (entries_[index].value_storage) V(std::forward<ValueArg>(value));
        entries_[index].state = EntryState::kOccupied;
    }

    std::size_t FindIndex(const K& key) const {
        if (capacity_ == 0) {
            return kNotFound;
        }
        std::size_t index = HashKey(key) & (capacity_ - 1);
        for (std::size_t probe = 0; probe < capacity_; ++probe) {
            const Entry& entry = entries_[index];
            if (entry.state == EntryState::kEmpty) {
                return kNotFound;
            }
            if (entry.state == EntryState::kOccupied && entry.Key() == key) {
                return index;
            }
            index = (index + 1) & (capacity_ - 1);
        }
        return kNotFound;
    }

    void Deallocate() {
        if (entries_ == nullptr) {
            return;
        }
        Clear();
        for (std::size_t i = 0; i < capacity_; ++i) {
            entries_[i].~Entry();
        }
        ::operator delete(entries_);
        entries_ = nullptr;
        capacity_ = 0;
    }

    Entry* entries_;
    std::size_t capacity_;
    std::size_t size_;
    std::size_t tombstones_;
};

}  // namespace mini_dbms::common
