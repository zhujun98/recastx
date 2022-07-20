#ifndef BUFFER_H
#define BUFFER_H

#include <condition_variable>
#include <queue>
#include <thread>
#include <unordered_map>

#include <spdlog/spdlog.h>

namespace slicerecon {

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

template<typename T>
class TrippleBufferInterface {

  public:

    virtual void fetch() = 0;
    virtual void prepare() = 0;

    virtual T& front() = 0;
    virtual T& back() = 0;

    virtual const T& front() const  = 0;
    virtual const T& ready() const = 0;
    virtual const T& back() const = 0;
};

template<typename T>
class TripleBuffer : public TrippleBufferInterface<std::vector<T>> {

    std::vector<T> back_;
    std::vector<T> ready_;
    std::vector<T> front_;

    bool is_ready_ = false;
    std::mutex mtx_;
    std::condition_variable cv_;

  public:

    TripleBuffer() = default;
    ~TripleBuffer() = default;

    void initialize(size_t capacity) {
        this->back_.resize(capacity);
        this->ready_.resize(capacity);
        this->front_.resize(capacity);
    }

    void fetch() override {
        std::unique_lock lk(this->mtx_);
        this->cv_.wait(lk, [this] { return this->is_ready_; });
        this->front_.swap(this->ready_); 
        this->is_ready_ = false;
    }

    void prepare() override {
        {
            std::lock_guard lk(this->mtx_);
            this->ready_.swap(this->back_);
            this->is_ready_ = true;    
        }
        this->cv_.notify_one();
    }

    std::vector<T>& front() override { return front_; }
    std::vector<T>& back() override { return back_; };

    const std::vector<T>& front() const override { return front_; }
    const std::vector<T>& ready() const override { return ready_; }
    const std::vector<T>& back() const override { return back_; };
};

template<typename T>
class MemoryBuffer: TrippleBufferInterface<std::vector<T>> {

    std::queue<int> indices_;
    std::unordered_map<int, size_t> map_;

    std::vector<T> front_;
    std::deque<std::vector<T>> buffer_;
    std::queue<size_t> unoccupied_;
    std::unordered_map<size_t, size_t> counter_;

    size_t chunk_size_ = 0;
    size_t group_size_ = 0;

    bool is_ready_ = false;

    void pop() {
        int idx = indices_.front();
        indices_.pop();

        size_t buffer_idx = map_[idx];
        counter_[buffer_idx] = 0;
        unoccupied_.push(buffer_idx);
        is_ready_ = false;

        map_.erase(idx);
    }

  public:

    MemoryBuffer() = default;
    ~MemoryBuffer() = default;

    void initialize(size_t capacity, size_t group_size, int chunk_size) {
        chunk_size_ = chunk_size;
        group_size_ = group_size;
        for (size_t i = 0; i < capacity; ++i) {
            std::vector<T> group_buffer(chunk_size * group_size, 0);
            buffer_.emplace_back(std::move(group_buffer));
            counter_[i] = 0;
            unoccupied_.push(i);
        }
        front_.resize(chunk_size * group_size);
    }

    template<typename D>
    void fill(const char* raw, int group_idx, int chunk_idx) {
        if (indices_.empty()) {
            indices_.push(group_idx);
            size_t buffer_idx = unoccupied_.front();
            map_[group_idx] = buffer_idx;
            unoccupied_.pop();
        } else if (group_idx > indices_.back()) {
            for (int i = indices_.back() + 1; i <= group_idx; ++i) {
                if (unoccupied_.empty()) this->pop();
                indices_.push(i);
                size_t buffer_idx = unoccupied_.front();
                map_[i] = buffer_idx;
                unoccupied_.pop();
            }
        } else if (group_idx < indices_.front()) {
            spdlog::warn("Received projection with old group index: {}, data ignored!", 
                         group_idx);
        }

        size_t buffer_idx = map_[group_idx];

        float* data = buffer_[buffer_idx].data();
        for (size_t i = chunk_idx * chunk_size_; i < (chunk_idx + 1) * chunk_size_; ++i) {
            D v;
            memcpy(&v, raw, sizeof(D));
            raw += sizeof(D);
            data[i] = static_cast<float>(v);
        }

        ++counter_[group_idx];
        if (counter_[group_idx] == group_size_) {
            // Remove earlier groups, no matter they are ready or not.
            while (group_idx != indices_.front()) pop();
            is_ready_ = true;
        }
    }

    bool isReady() const { return is_ready_; }

    void fetch() override {
        this->front_.swap(buffer_[map_.at(indices_.front())]);
        pop();
        is_ready_ = false;
    }

    void prepare() override {}

    std::vector<T>& front() override {
        return this->front_;
    }
    std::vector<T>& back() override {
        return buffer_[map_.at(indices_.front())];
    }

    const std::vector<T>& front() const override {
        return this->front_;
    }
    const std::vector<T>& ready() const override {
        return buffer_[map_.at(indices_.front())]; 
    }
    const std::vector<T>& back() const override {
        return buffer_[map_.at(indices_.front())]; 
    }

    size_t capacity() const { return buffer_.size(); }

    size_t occupied() const { return buffer_.size() - unoccupied_.size(); }
};

} // slicerecon

#endif // BUFFER_H