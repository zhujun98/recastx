#ifndef SLICERECON_BUFFER_H
#define SLICERECON_BUFFER_H

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <numeric>
#include <queue>
#include <thread>
#include <type_traits>
#include <unordered_map>
using namespace std::chrono_literals;

#include <spdlog/spdlog.h>

#include <tomcat/tomcat.hpp>

namespace tomcat::recon {

template<typename T>
class DoubleBufferInterface {

  public:

    virtual void swap() = 0;

    virtual T& front() = 0;
    virtual T& back() = 0;

    virtual const T& front() const = 0;
    virtual const T& back() const = 0;
};


template<typename T>
class DoubleBuffer : public DoubleBufferInterface<std::vector<T>> {
    
    std::vector<T> front_;
    std::vector<T> back_;

  public:

    DoubleBuffer() = default;
    ~DoubleBuffer() = default;

    void initialize(size_t capacity) {
        this->back_.resize(capacity);
        this->front_.resize(capacity);
    }

    void swap() override { this->front_.swap(this->back_); }

    std::vector<T>& front() override { return front_; }
    std::vector<T>& back() override { return back_; }

    const std::vector<T>& front() const override { return front_; }
    const std::vector<T>& back() const override { return back_; }
};


template<typename T, size_t N>
class TripleBufferInterface {

protected:

    T back_;
    T ready_;
    T front_;

    size_t capacity_ = 0;
    size_t chunk_size_ = 0;
    std::array<size_t, N> shape_;

    bool is_ready_ = false;
    std::mutex mtx_;
    std::condition_variable cv_;

public:

    TripleBufferInterface() = default;
    virtual ~TripleBufferInterface() = default;

    TripleBufferInterface(const TripleBufferInterface&) = delete;
    TripleBufferInterface& operator=(const TripleBufferInterface&) = delete;

    TripleBufferInterface(TripleBufferInterface&&) = delete;
    TripleBufferInterface& operator=(TripleBufferInterface&&) = delete;
    
    virtual void resize(size_t capacity, const std::array<size_t, N>& shape) = 0;

    bool fetch(int timeout = -1) {
        std::unique_lock lk(mtx_);
        if (timeout < 0) {
            cv_.wait(lk, [this] { return is_ready_; });
        } else {
            if(!(cv_.wait_for(lk, timeout * 1ms, 
                              [this] { return is_ready_; }))) {
                return false;
            }
        }
        this->front_.swap(ready_); 
        this->is_ready_ = false;
        return true;
    }

    void prepare() {
        {
            std::lock_guard lk(mtx_);
            ready_.swap(back_);
            is_ready_ = true;    
        }
        cv_.notify_one();
    }

    T& front() { return front_; }
    T& back() { return back_; };

    const T& front() const { return front_; }
    const T& ready() const { return ready_; }
    const T& back() const { return back_; };

    size_t capacity() const { return capacity_; }

    size_t chunkSize() const { return chunk_size_; }

    const std::array<size_t, N>& shape() const { return shape_; }
};

template<template <typename> typename Container, typename T, size_t N, 
         std::enable_if_t<std::is_arithmetic_v<T>, bool> = false>
class TripleBuffer : public TripleBufferInterface<Container<T>, N> {

  public:

    TripleBuffer() = default;
    ~TripleBuffer() override = default;

    void resize(size_t capacity, const std::array<size_t, N>& shape) override {
        size_t chunk_size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

        this->back_.resize(capacity * chunk_size);
        this->ready_.resize(capacity * chunk_size);
        this->front_.resize(capacity * chunk_size);
        this->capacity_ = capacity;
        this->chunk_size_ = chunk_size;
        this->shape_ = shape;
    }
};


template<typename T, size_t N>
using TripleVectorBuffer = TripleBuffer<std::vector, T, N>;


template<typename T, size_t N>
class MultiTripleBuffer : public TripleBufferInterface<std::vector<T>, N> {

public:
    MultiTripleBuffer() = default;
    ~MultiTripleBuffer() override = default;

    void resize(size_t capacity, const std::array<size_t, 2>& shape) {
        size_t chunk_size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

        for (size_t i = 0; i < capacity; ++i) {
            this->back_.emplace_back(T(chunk_size));
            this->ready_.emplace_back(T(chunk_size));
            this->front_.emplace_back(T(chunk_size));
            
        }
        this->capacity_ = capacity;
        this->chunk_size_ = chunk_size;
        this->shape_ = shape;
    }
};


using SliceBuffer = MultiTripleBuffer<std::vector<float>, 2>;


template<typename T, size_t N>
class MemoryBuffer {

    std::queue<size_t> group_indices_;
    std::unordered_map<int, size_t> map_;

    std::vector<T> front_;
    std::deque<std::vector<T>> raw_buffer_;
    std::queue<size_t> unoccupied_;
    std::vector<size_t> counter_;

    size_t capacity_ = 0;
    size_t chunk_size_ = 0;
    size_t group_size_ = 0;
    std::array<size_t, N> shape_;

#if (VERBOSITY >= 2)
    size_t data_received_ = 0;
#endif

    std::mutex mtx_;
    std::condition_variable cv_;
    bool is_ready_ = false;

    void pop() {
        int idx = group_indices_.front();
        group_indices_.pop();

        size_t buffer_idx = map_[idx];
        counter_[buffer_idx] = 0;
        unoccupied_.push(buffer_idx);
        is_ready_ = false;

        map_.erase(idx);
    }

  public:

    MemoryBuffer() = default;
    ~MemoryBuffer() = default;

    void resize(size_t capacity, size_t group_size, const std::array<size_t, N>& shape) {
        size_t chunk_size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());
        for (size_t i = 0; i < capacity; ++i) {
            std::vector<T> group_buffer(group_size * chunk_size, 0);
            raw_buffer_.emplace_back(std::move(group_buffer));
            counter_.push_back(0);
            unoccupied_.push(i);
        }
        front_.resize(group_size * chunk_size);

        capacity_ = capacity;
        chunk_size_ = chunk_size;
        group_size_ = group_size;
    }

    void reset() {
        std::queue<size_t>().swap(group_indices_);
        std::queue<size_t>().swap(unoccupied_);
        map_.clear();
#if (VERBOSITY >= 2)
        data_received_ = 0;
#endif
        for (size_t i = 0; i < raw_buffer_.size(); ++i) {
            counter_[i] = 0;
            unoccupied_.push(i);
        }
    }

    template<typename D>
    void fill(const char* raw, size_t group_idx, size_t chunk_idx) {
        std::lock_guard lk(mtx_);

        if (group_indices_.empty()) {
            group_indices_.push(group_idx);
            size_t buffer_idx = unoccupied_.front();
            map_[group_idx] = buffer_idx;
            unoccupied_.pop();
        } else if (group_idx > group_indices_.back()) {
            for (size_t i = group_indices_.back() + 1; i <= group_idx; ++i) {
                if (unoccupied_.empty()) {
                    int idx = group_indices_.front();
                    this->pop();
#if (VERBOSITY >= 1)
                    spdlog::warn("Memory buffer is full! Group {} dropped!", idx);
#endif
                }
                group_indices_.push(i);
                size_t buffer_idx = unoccupied_.front();
                map_[i] = buffer_idx;
                unoccupied_.pop();
            }
        } else if (group_idx < group_indices_.front()) {

#if (VERBOSITY >= 1)
            spdlog::warn("Received projection with outdated group index: {}, data ignored!", 
                         group_idx);
#endif

            return;
        }

        size_t buffer_idx = map_[group_idx];

        float* data = raw_buffer_[buffer_idx].data();
        for (size_t i = chunk_idx * chunk_size_; i < (chunk_idx + 1) * chunk_size_; ++i) {
            D v;
            memcpy(&v, raw, sizeof(D));
            raw += sizeof(D);
            data[i] = static_cast<float>(v);
        }

        ++counter_[buffer_idx];
        // Caveat: We do not track the chunk indices. Therefore, if data with the same
        //         frame idx arrives repeatedly, it will still fill the group.
        if (counter_[buffer_idx] == group_size_) {
            // Remove earlier groups, no matter they are ready or not.
            size_t idx = group_indices_.front();
            while (group_idx != idx) {
                pop();
#if (VERBOSITY >= 1)
                spdlog::warn("Group {} is ready! Earlier group {} dropped!", group_idx, idx);
#endif
                idx = group_indices_.front();
            }
            is_ready_ = true;
            cv_.notify_one();
        }

#if (VERBOSITY >= 2)
        // Outdated data are excluded.
        ++data_received_;
        if (data_received_ % group_size_ == 0) {
            spdlog::info("{}/{} groups in the memory buffer are occupied", 
                         occupied(), raw_buffer_.size());
        }
#endif

    }

    bool fetch(int timeout = -1) {
        std::unique_lock lk(mtx_);
        if (timeout < 0) {
            cv_.wait(lk, [this] { return is_ready_; });
        } else {
            if (!(cv_.wait_for(lk, timeout * 1ms, 
                               [this] { return is_ready_; }))) {
                return false;
            }
        }
        front_.swap(raw_buffer_[map_.at(group_indices_.front())]);
        pop();
        is_ready_ = false;
        
        return true;
    }

    std::vector<T>& front() {
        return front_;
    }
    std::vector<T>& back() {
        return raw_buffer_[map_.at(group_indices_.front())];
    }

    const std::vector<T>& front() const {
        return front_;
    }
    const std::vector<T>& ready() const {
        return raw_buffer_[map_.at(group_indices_.front())]; 
    }
    const std::vector<T>& back() const {
        return raw_buffer_[map_.at(group_indices_.front())]; 
    }

    size_t capacity() const {
        assert(this->capacity_ == raw_buffer_.size());
        return raw_buffer_.size(); 
    }

    size_t occupied() const { return this->capacity_ - unoccupied_.size(); }

    size_t groupSize() const { return group_size_; }

    size_t chunkSize() const { return chunk_size_; }

    const std::array<size_t, N>& shape() const { return shape_; }
};

} // tomcat::recon

#endif // SLICERECON_BUFFER_H