/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_QUEUE_H
#define RECON_QUEUE_H

#include <atomic>
#include <cassert>
#include <chrono>
#include <limits>
#include <memory>
#include <queue>
#include <thread>
#include <condition_variable>

namespace recastx::recon {

template<typename T>
class ThreadSafeQueue {

    struct Item {
        T data;
        std::unique_ptr<Item> next;

        Item() = default;
        explicit Item(const T& data_) : data(data_) {}
        explicit Item(T&& data_) : data(std::move(data_)) {}
    };

    const size_t max_len_;
    // Note: It could happen that len_ > max_len_. But len_ will not grow infinitely if not 0.
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

    std::unique_ptr<Item> tryPopImpl(T& data) {
        std::lock_guard lk(head_mtx_);
        if (head_.get() == tail()) return nullptr;
        data = std::move(head_->data);
        return popHead();
    }

    void pushImpl(T&& data) {
        std::unique_ptr<Item> new_item(new Item);
        {
            std::lock_guard lk(tail_mtx_);
            tail_->data = std::move(data);
            Item* const new_tail = new_item.get();
            tail_->next = std::move(new_item);
            tail_ = new_tail;
            len_++;
        }
        cv_.notify_one();
    }

  public:

    explicit ThreadSafeQueue(int max_len = 0)
     : max_len_(max_len <= 0 ? 0 : max_len), 
       head_(new Item), 
       tail_(head_.get()) {}

    ThreadSafeQueue(const ThreadSafeQueue& other) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue& other) = delete;

    bool tryPush(T data) {
        if (max_len_ > 0 && len_ >= max_len_) return false;

        pushImpl(std::move(data));
        return true;
    }

    void push(T data) {
        if (max_len_ > 0) {
            std::lock_guard lk(head_mtx_);
            while (len_ >= max_len_) {
                if (head_.get() != tail()) {
                    head_ = std::move(head_->next);
                    len_--;
                } 
            }
        }

        pushImpl(std::move(data));
    }

    bool tryPop(T& data) {
        const std::unique_ptr<Item> old_head = tryPopImpl(data);
        return old_head != nullptr;
    }

    bool waitAndPop(T& data, int timeout = -1) {
        std::unique_lock<std::mutex> lk(head_mtx_);
        if (timeout < 0) {
            cv_.wait(lk, [&] { return head_.get() != tail(); });
        } else {
            if (!cv_.wait_for(lk, std::chrono::milliseconds(timeout), 
                              [&] { return head_.get() != tail(); })) {
                return false;
            } 
        }
        data = std::move(head_->data);
        popHead();
        return true;
    }

    bool empty() const {
        std::lock_guard lk(head_mtx_);
        return head_.get() == tail();
    }

    void reset() {
        std::lock_guard lk(head_mtx_);
        while (len_ > 0) {
            if (head_.get() != tail()) {
                head_ = std::move(head_->next);
                len_--;
            } 
        }
        assert(head_.get() == tail_);
        assert(len_ == 0);
    }

    size_t size() const { return len_; }
};

} // namespace recastx::recon

#endif // RECON_QUEUE_H