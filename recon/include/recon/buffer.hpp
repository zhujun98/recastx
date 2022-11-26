#ifndef SLICERECON_BUFFER_H
#define SLICERECON_BUFFER_H

#include <chrono>
#include <condition_variable>
#include <queue>
#include <thread>
#include <unordered_map>
using namespace std::chrono_literals;

#include <spdlog/spdlog.h>

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

template<typename T>
class TrippleBufferInterface {

  public:

    virtual bool fetch(int timeout = -1) = 0;
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

    TripleBuffer(const TripleBuffer&) = delete;
    TripleBuffer& operator=(const TripleBuffer&) = delete;

    TripleBuffer(TripleBuffer&& other) noexcept {
        std::lock_guard lk(other.mtx_);
        back_ = std::move(other.back_);
        ready_ = std::move(other.ready_);
        front_ = std::move(other.front_);
        is_ready_ = other.is_ready_;
    }

    TripleBuffer& operator=(TripleBuffer&& other) noexcept {
        std::lock_guard lk(other.mtx_);
        back_ = std::move(other.back_);
        ready_ = std::move(other.ready_);
        front_ = std::move(other.front_);
        is_ready_ = other.is_ready_;
        return *this;
    }

    void initialize(size_t capacity) {
        this->back_.resize(capacity);
        this->ready_.resize(capacity);
        this->front_.resize(capacity);
    }

    bool fetch(int timeout = -1) override {
        std::unique_lock lk(this->mtx_);
        if (timeout < 0) {
            cv_.wait(lk, [this] { return this->is_ready_; });
        } else {
            if(!(cv_.wait_for(lk, timeout * 1ms, 
                              [this] { return this->is_ready_; }))) {
                return false;
            }
        }
        this->front_.swap(this->ready_); 
        this->is_ready_ = false;
        return true;
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

    std::queue<int> group_indices_;
    std::unordered_map<int, size_t> map_;

    std::vector<T> front_;
    std::deque<std::vector<T>> buffer_;
    std::queue<size_t> unoccupied_;
    std::vector<size_t> counter_;

    size_t chunk_size_ = 0;
    size_t group_size_ = 0;

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

    MemoryBuffer(const MemoryBuffer&) = delete;
    MemoryBuffer& operator=(const MemoryBuffer&) = delete;

    MemoryBuffer(MemoryBuffer&&) = delete;
    MemoryBuffer& operator=(MemoryBuffer&&) = delete;

    void initialize(size_t capacity, size_t group_size, int chunk_size) {
        chunk_size_ = chunk_size;
        group_size_ = group_size;
        for (size_t i = 0; i < capacity; ++i) {
            std::vector<T> group_buffer(chunk_size * group_size, 0);
            buffer_.emplace_back(std::move(group_buffer));
            counter_.push_back(0);
            unoccupied_.push(i);
        }
        front_.resize(chunk_size * group_size);
    }

    void reset() {
        std::queue<int>().swap(group_indices_);
        std::queue<size_t>().swap(unoccupied_);
        map_.clear();
#if (VERBOSITY >= 2)
        data_received_ = 0;
#endif
        for (size_t i = 0; i < buffer_.size(); ++i) {
            counter_[i] = 0;
            unoccupied_.push(i);
        }
    }

    template<typename D>
    void fill(const char* raw, int group_idx, int chunk_idx) {
        std::lock_guard lk(mtx_);

        if (group_indices_.empty()) {
            group_indices_.push(group_idx);
            size_t buffer_idx = unoccupied_.front();
            map_[group_idx] = buffer_idx;
            unoccupied_.pop();
        } else if (group_idx > group_indices_.back()) {
            for (int i = group_indices_.back() + 1; i <= group_idx; ++i) {
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

        float* data = buffer_[buffer_idx].data();
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
            int idx = group_indices_.front();
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
                         occupied(), buffer_.size());
        }
#endif

    }

    bool fetch(int timeout = -1) override {
        std::unique_lock lk(mtx_);
        if (timeout < 0) {
            cv_.wait(lk, [this] { return this->is_ready_; });
        } else {
            if (!(cv_.wait_for(lk, timeout * 1ms, 
                               [this] { return this->is_ready_; }))) {
                return false;
            }
        }
        this->front_.swap(buffer_[map_.at(group_indices_.front())]);
        pop();
        is_ready_ = false;
        
        return true;
    }

    void prepare() override {}

    std::vector<T>& front() override {
        return this->front_;
    }
    std::vector<T>& back() override {
        return buffer_[map_.at(group_indices_.front())];
    }

    const std::vector<T>& front() const override {
        return this->front_;
    }
    const std::vector<T>& ready() const override {
        return buffer_[map_.at(group_indices_.front())]; 
    }
    const std::vector<T>& back() const override {
        return buffer_[map_.at(group_indices_.front())]; 
    }

    size_t capacity() const { return buffer_.size(); }

    size_t occupied() const { return buffer_.size() - unoccupied_.size(); }
};

} // tomcat::recon

#endif // SLICERECON_BUFFER_H