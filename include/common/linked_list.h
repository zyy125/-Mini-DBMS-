#pragma once

#include <cstddef>
#include <utility>

#include "common/status.h"

namespace mini_dbms::common {

template <typename T>
class LinkedList {
public:
    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

    LinkedList(const LinkedList& other) : LinkedList() {
        Node* current = other.head_;
        while (current != nullptr) {
            PushBack(current->value);
            current = current->next;
        }
    }

    LinkedList(LinkedList&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this == &other) {
            return *this;
        }
        LinkedList copy(other);
        Swap(copy);
        return *this;
    }

    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Clear();
        head_ = other.head_;
        tail_ = other.tail_;
        size_ = other.size_;
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    ~LinkedList() { Clear(); }

    Status PushBack(const T& value) {
        Node* node = new (std::nothrow) Node(value);
        if (node == nullptr) {
            return Status::OutOfMemory("LinkedList allocation failed");
        }
        AppendNode(node);
        return Status::OK();
    }

    Status PushBack(T&& value) {
        Node* node = new (std::nothrow) Node(std::move(value));
        if (node == nullptr) {
            return Status::OutOfMemory("LinkedList allocation failed");
        }
        AppendNode(node);
        return Status::OK();
    }

    Status PushFront(const T& value) {
        Node* node = new (std::nothrow) Node(value);
        if (node == nullptr) {
            return Status::OutOfMemory("LinkedList allocation failed");
        }
        PrependNode(node);
        return Status::OK();
    }

    Status PopFront() {
        if (head_ == nullptr) {
            return Status::NotFound("LinkedList is empty");
        }
        Node* old_head = head_;
        head_ = head_->next;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        delete old_head;
        --size_;
        return Status::OK();
    }

    void Clear() {
        Node* current = head_;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

    T& Front() { return head_->value; }
    const T& Front() const { return head_->value; }
    T& Back() { return tail_->value; }
    const T& Back() const { return tail_->value; }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void Swap(LinkedList& other) noexcept {
        Node* other_head = other.head_;
        Node* other_tail = other.tail_;
        std::size_t other_size = other.size_;
        other.head_ = head_;
        other.tail_ = tail_;
        other.size_ = size_;
        head_ = other_head;
        tail_ = other_tail;
        size_ = other_size;
    }

private:
    struct Node {
        explicit Node(const T& item) : value(item), next(nullptr) {}
        explicit Node(T&& item) : value(std::move(item)), next(nullptr) {}

        T value;
        Node* next;
    };

    void AppendNode(Node* node) {
        if (tail_ == nullptr) {
            head_ = node;
            tail_ = node;
        } else {
            tail_->next = node;
            tail_ = node;
        }
        ++size_;
    }

    void PrependNode(Node* node) {
        node->next = head_;
        head_ = node;
        if (tail_ == nullptr) {
            tail_ = node;
        }
        ++size_;
    }

    Node* head_;
    Node* tail_;
    std::size_t size_;
};

}  // namespace mini_dbms::common
