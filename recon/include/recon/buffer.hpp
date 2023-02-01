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
#include <unordered_set>
using namespace std::chrono_literals;

#include <spdlog/spdlog.h>

#include <tomcat/tomcat.hpp>

namespace tomcat::recon {

template<typename T, size_t N>
class TripleBufferInterface {

public:

    using ValueType = T;

private:

    bool is_ready_ = false;
    std::mutex mtx_;
    std::condition_variable cv_;

protected:

    size_t chunk_size_ = 0;
    std::array<size_t, N> shape_;

    T back_;
    T ready_;
    T front_;

public:

    TripleBufferInterface() = default;
    virtual ~TripleBufferInterface() = default;

    TripleBufferInterface(const TripleBufferInterface&) = delete;
    TripleBufferInterface& operator=(const TripleBufferInterface&) = delete;

    TripleBufferInterface(TripleBufferInterface&&) = delete;
    TripleBufferInterface& operator=(TripleBufferInterface&&) = delete;
    
    virtual void resize(const std::array<size_t, N>& shape) = 0;

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

    size_t chunkSize() const { return chunk_size_; }

    const std::array<size_t, N>& shape() const { return shape_; }
};

template<typename T, size_t N>
class TripleBuffer : public TripleBufferInterface<T, N> {

  public:

    TripleBuffer() = default;
    ~TripleBuffer() override = default;

    void resize(const std::array<size_t, N>& shape) override {
        size_t chunk_size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

        this->back_.resize(chunk_size);
        this->ready_.resize(chunk_size);
        this->front_.resize(chunk_size);
        this->shape_ = shape;
        this->chunk_size_ = chunk_size;
    }
};


template<typename T, size_t N>
using TripleVectorBuffer = TripleBuffer<std::vector<T>, N>;


namespace details {

    template<typename T>
    using SliceBufferDataType = std::pair<size_t, std::vector<T>>;

    template<typename T>
    using SliceBufferValueType = std::pair<std::unordered_set<size_t>, 
                                          std::vector<SliceBufferDataType<T>>>;
}

template<typename T>
class SliceBuffer : public TripleBufferInterface<details::SliceBufferValueType<T>, 2> {

public:

    using DataType = typename details::SliceBufferDataType<T>;
    using ValueType = typename details::SliceBufferValueType<T>;

private:

    size_t capacity_;
    bool full_;

public:

    SliceBuffer(size_t capacity, bool full = true) : 
        TripleBufferInterface<ValueType, 2>(), capacity_(capacity), full_(full) {}
    
    ~SliceBuffer() override = default;

    void resize(const std::array<size_t, 2>& shape) {
        size_t chunk_size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

        for (size_t i = 0; i < capacity_; ++i) {
            this->back_.second.emplace_back(DataType());
            this->ready_.second.emplace_back(DataType());
            this->front_.second.emplace_back(DataType());

            this->back_.second.back().second.resize(chunk_size);
            this->ready_.second.back().second.resize(chunk_size);
            this->front_.second.back().second.resize(chunk_size);

            if (full_) {
                this->back_.first.emplace(i);
                this->ready_.first.emplace(i);
                this->front_.first.emplace(i);
            }
        }
        this->chunk_size_ = chunk_size;
        this->shape_ = shape;
    }

    size_t capacity() const { return capacity_; }
};


template<typename T, size_t N>
class MemoryBuffer {

    std::queue<size_t> chunk_indices_;
    std::unordered_map<size_t, size_t> map_;

    std::vector<T> front_;
    std::deque<std::vector<T>> buffer_;
    std::queue<size_t> unoccupied_;
    std::vector<size_t> counter_;

    size_t capacity_ = 0;
    size_t chunk_size_ = 0;
    std::array<size_t, N> shape_;

#if (VERBOSITY >= 2)
    size_t data_received_ = 0;
#endif

    std::mutex mtx_;
    std::condition_variable cv_;
    bool is_ready_ = false;

    void pop() {
        size_t idx = chunk_indices_.front();
        chunk_indices_.pop();

        size_t buffer_idx = map_[idx];
        counter_[buffer_idx] = 0;
        unoccupied_.push(buffer_idx);
        is_ready_ = false;

        map_.erase(idx);
    }

  public:

    explicit MemoryBuffer(size_t capacity) : capacity_(capacity) {}
    ~MemoryBuffer() = default;

    void resize(const std::array<size_t, N>& shape) {
        size_t chunk_size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());
        for (size_t i = 0; i < capacity_; ++i) {
            buffer_.emplace_back(std::vector<T>(chunk_size, 0));
            counter_.push_back(0);
            unoccupied_.push(i);
        }
        front_.resize(chunk_size);

        chunk_size_ = chunk_size;
        shape_ = shape;
    }

    void reset() {
        std::queue<size_t>().swap(chunk_indices_);
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
    void fill(const char* raw, size_t chunk_idx, size_t data_idx) {
        std::lock_guard lk(mtx_);

        if (chunk_indices_.empty()) {
            chunk_indices_.push(chunk_idx);
            size_t buffer_idx = unoccupied_.front();
            map_[chunk_idx] = buffer_idx;
            unoccupied_.pop();
        } else if (chunk_idx > chunk_indices_.back()) {
            for (size_t i = chunk_indices_.back() + 1; i <= chunk_idx; ++i) {
                if (unoccupied_.empty()) {
                    int idx = chunk_indices_.front();
                    this->pop();
#if (VERBOSITY >= 1)
                    spdlog::warn("Memory buffer is full! Group {} dropped!", idx);
#endif
                }
                chunk_indices_.push(i);
                size_t buffer_idx = unoccupied_.front();
                map_[i] = buffer_idx;
                unoccupied_.pop();
            }
        } else if (chunk_idx < chunk_indices_.front()) {

#if (VERBOSITY >= 1)
            spdlog::warn("Received projection with outdated chunk index: {}, data ignored!", 
                         chunk_idx);
#endif

            return;
        }

        size_t buffer_idx = map_[chunk_idx];

        float* data = buffer_[buffer_idx].data();
        for (size_t data_size = chunk_size_ / shape_[0], 
                i = data_idx * data_size; i < (data_idx + 1) * data_size; ++i) {
            D v;
            memcpy(&v, raw, sizeof(D));
            raw += sizeof(D);
            data[i] = static_cast<float>(v);
        }

        ++counter_[buffer_idx];
        // Caveat: We do not track the data indices. Therefore, if data with the same
        //         frame idx arrives repeatedly, it will still fill the chunk.
        if (counter_[buffer_idx] == shape_[0]) {
            // Remove earlier chunks, no matter they are ready or not.
            size_t idx = chunk_indices_.front();
            while (chunk_idx != idx) {
                pop();
#if (VERBOSITY >= 1)
                spdlog::warn("Chunk {} is ready! Earlier chunks {} dropped!", chunk_idx, idx);
#endif
                idx = chunk_indices_.front();
            }
            is_ready_ = true;
            cv_.notify_one();
        }

#if (VERBOSITY >= 2)
        // Outdated data are excluded.
        ++data_received_;
        if (data_received_ % shape_[0] == 0) {
            spdlog::info("{}/{} groups in the memory buffer are occupied", 
                         occupied(), buffer_.size());
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
        front_.swap(buffer_[map_.at(chunk_indices_.front())]);
        pop();
        is_ready_ = false;
        
        return true;
    }

    std::vector<T>& front() {
        return front_;
    }
    std::vector<T>& back() {
        return buffer_[map_.at(chunk_indices_.front())];
    }

    const std::vector<T>& front() const {
        return front_;
    }
    const std::vector<T>& ready() const {
        return buffer_[map_.at(chunk_indices_.front())]; 
    }
    const std::vector<T>& back() const {
        return buffer_[map_.at(chunk_indices_.front())]; 
    }

    size_t capacity() const {
        assert(this->capacity_ == buffer_.size());
        return this->capacity_; 
    }

    size_t occupied() const { return this->capacity_ - unoccupied_.size(); }

    size_t chunkSize() const { return chunk_size_; }

    const std::array<size_t, N>& shape() const { return shape_; }
};

} // tomcat::recon

#endif // SLICERECON_BUFFER_H