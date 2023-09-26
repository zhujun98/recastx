/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_DAQBUFFER_H
#define RECON_DAQBUFFER_H

#include <atomic>
#include <chrono>
#include <limits>
#include <memory>
#include <queue>
#include <thread>
#include <condition_variable>

namespace recastx::recon {

template<typename T>
class DaqBuffer {

    struct Item {
        T data;
        std::unique_ptr<Item> next;

        Item() = default;
        explicit Item(const T& data_) : data(data_) {}
        explicit Item(T&& data_) : data(std::move(data_)) {}
    };

    size_t max_len_;
    std::atomic<size_t> len_ {0};

    mutable std::mutex head_mtx_;
    std::unique_ptr<Item> head_;

    mutable std::mutex tail_mtx_;
    Item* tail_;

    std::condition_variable cv_;

    Item* tail() const {
        std::lock_guard lk(tail_mtx_);
        return tail_;
    }

    std::unique_ptr<Item> popHead() {
        std::unique_ptr<Item> old_head = std::move(head_);
        head_ = std::move(old_head->next);
        len_--;
        return old_head;
    }

    std::unique_ptr<Item> tryPopImpl(T& value) {
        std::lock_guard lk(head_mtx_);
        if (head_.get() == tail()) return nullptr;
        value = std::move(head_->data);
        return popHead();
    }

  public:

    explicit DaqBuffer(int max_len = 0)
     : max_len_(max_len <= 0 ? std::numeric_limits<unsigned long>::max() : max_len), 
       head_(new Item), 
       tail_(head_.get()) {}

    DaqBuffer(const DaqBuffer& other) = delete;
    DaqBuffer& operator=(const DaqBuffer& other) = delete;

    bool tryPush(T item) {
        if (len_ >= max_len_) return false;

        std::unique_ptr<Item> new_item(new Item);
        {
            std::lock_guard lk(tail_mtx_);
            tail_->data = std::move(item);
            Item* const new_tail = new_item.get();
            tail_->next = std::move(new_item);
            tail_ = new_tail;
            len_++;
        }
        cv_.notify_one();
        return true;
    }

    bool tryPop(T& value) {
        const std::unique_ptr<Item> old_head = tryPopImpl(value);
        return old_head != nullptr;
    }

    bool waitAndPop(T& value) {
        std::unique_lock<std::mutex> lk(head_mtx_);
        if (cv_.wait_for(lk, std::chrono::milliseconds(100), 
                         [&] { return head_.get() != tail(); })) {
            value = std::move(head_->data);
            popHead();
            return true;
        }
        return false;
    }

    bool empty() const {
        std::lock_guard lk(head_mtx_);
        return head_.get() == tail();
    }

    size_t size() const {
        return len_;
    }
};

} // namespace recastx::recon

#endif // RECON_DAQBUFFER_H