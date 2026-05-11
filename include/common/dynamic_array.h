#pragma once

#include <cstddef>
#include <new>
#include <utility>

#include "common/status.h"

namespace mini_dbms::common {

template <typename T>
class DynamicArray {
public:
    DynamicArray() : data_(nullptr), size_(0), capacity_(0) {}

    explicit DynamicArray(std::size_t initial_capacity)
        : data_(nullptr), size_(0), capacity_(0) {
        if (initial_capacity > 0) {
            Reserve(initial_capacity);
        }
    }

    DynamicArray(const DynamicArray& other)
        : data_(nullptr), size_(0), capacity_(0) {
        if (!Reserve(other.size_).ok()) {
            return;
        }
        for (std::size_t i = 0; i < other.size_; ++i) {
            new (data_ + i) T(other.data_[i]);
        }
        size_ = other.size_;
    }

    DynamicArray(DynamicArray&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this == &other) {
            return *this;
        }
        DynamicArray copy(other);
        Swap(copy);
        return *this;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        DestroyAndDeallocate();
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        return *this;
    }

    ~DynamicArray() { DestroyAndDeallocate(); }

    Status PushBack(const T& value) {
        Status status = EnsureCapacityForOneMore();
        if (!status.ok()) {
            return status;
        }
        new (data_ + size_) T(value);
        ++size_;
        return Status::OK();
    }

    Status PushBack(T&& value) {
        Status status = EnsureCapacityForOneMore();
        if (!status.ok()) {
            return status;
        }
        new (data_ + size_) T(std::move(value));
        ++size_;
        return Status::OK();
    }

    template <typename... Args>
    Status EmplaceBack(Args&&... args) {
        Status status = EnsureCapacityForOneMore();
        if (!status.ok()) {
            return status;
        }
        new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
        return Status::OK();
    }

    Status Reserve(std::size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return Status::OK();
        }

        T* new_data = static_cast<T*>(::operator new(sizeof(T) * new_capacity, std::nothrow));
        if (new_data == nullptr) {
            return Status::OutOfMemory("DynamicArray allocation failed");
        }

        std::size_t constructed = 0;
        for (; constructed < size_; ++constructed) {
            new (new_data + constructed) T(std::move(data_[constructed]));
        }
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i].~T();
        }
        ::operator delete(data_);
        data_ = new_data;
        capacity_ = new_capacity;
        return Status::OK();
    }

    Status RemoveAt(std::size_t index) {
        if (index >= size_) {
            return Status::InvalidArgument("DynamicArray remove index out of range");
        }
        data_[index].~T();
        for (std::size_t i = index; i + 1 < size_; ++i) {
            new (data_ + i) T(std::move(data_[i + 1]));
            data_[i + 1].~T();
        }
        --size_;
        return Status::OK();
    }

    void Clear() {
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i].~T();
        }
        size_ = 0;
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    T& operator[](std::size_t index) { return data_[index]; }
    const T& operator[](std::size_t index) const { return data_[index]; }

    T* data() { return data_; }
    const T* data() const { return data_; }

    void Swap(DynamicArray& other) noexcept {
        T* other_data = other.data_;
        std::size_t other_size = other.size_;
        std::size_t other_capacity = other.capacity_;
        other.data_ = data_;
        other.size_ = size_;
        other.capacity_ = capacity_;
        data_ = other_data;
        size_ = other_size;
        capacity_ = other_capacity;
    }

private:
    Status EnsureCapacityForOneMore() {
        if (size_ < capacity_) {
            return Status::OK();
        }
        std::size_t new_capacity = capacity_ == 0 ? 4 : capacity_ * 2;
        return Reserve(new_capacity);
    }

    void DestroyAndDeallocate() {
        Clear();
        ::operator delete(data_);
        data_ = nullptr;
        capacity_ = 0;
    }

    T* data_;
    std::size_t size_;
    std::size_t capacity_;
};

}  // namespace mini_dbms::common
