#ifndef RECON_BUFFER_H
#define RECON_BUFFER_H

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <map>
#include <numeric>
#include <queue>
#include <thread>
#include <type_traits>
using namespace std::chrono_literals;

#include <spdlog/spdlog.h>

#include "common/config.hpp"
#include "tensor.hpp"

namespace tomcat::recon {

template<typename T>
class TripleBufferInterface {

public:

    using BufferType = T;

private:

    bool is_ready_ = false;
    std::condition_variable cv_;

protected:

    std::mutex mtx_;

    T back_;
    T ready_;
    T front_;

    // Something beyond the normal swap might be needed.
    virtual void swap(BufferType& v1, BufferType& v2) noexcept = 0;

public:

    TripleBufferInterface() = default;
    virtual ~TripleBufferInterface() = default;

    TripleBufferInterface(const TripleBufferInterface&) = delete;
    TripleBufferInterface& operator=(const TripleBufferInterface&) = delete;

    TripleBufferInterface(TripleBufferInterface&&) = delete;
    TripleBufferInterface& operator=(TripleBufferInterface&&) = delete;
    
    bool fetch(int timeout = -1);

    virtual void prepare();

    T& front() { return front_; }
    T& back() { return back_; };

    const T& front() const { return front_; }
    const T& ready() const { return ready_; }
    const T& back() const { return back_; };
};

template<typename T>
bool TripleBufferInterface<T>::fetch(int timeout) {
    std::unique_lock lk(mtx_);
    if (timeout < 0) {
        cv_.wait(lk, [this] { return is_ready_; });
    } else {
        if (!(cv_.wait_for(lk, timeout * 1ms, [this] { return is_ready_; }))) {
            return false;
        }
    }
    swap(front_, ready_);
    is_ready_ = false;
    return true;
}

template<typename T>
void TripleBufferInterface<T>::prepare() {
    {
        std::lock_guard lk(mtx_);
        swap(ready_, back_);
        is_ready_ = true;    
    }
    cv_.notify_one();
}


template<typename T, size_t N>
class TripleTensorBuffer : public TripleBufferInterface<Tensor<T, N>> {

public:

    using BufferType = Tensor<T, N>;
    using ValueType = typename BufferType::ValueType;
    using ShapeType = typename BufferType::ShapeType;

protected:

    void swap(BufferType& c1, BufferType& c2) noexcept override { c1.swap(c2); }

public:

    TripleTensorBuffer() = default;
    ~TripleTensorBuffer() override = default;

    virtual void reshape(const ShapeType& shape);
};

template<typename T, size_t N>
void TripleTensorBuffer<T, N>::reshape(const ShapeType& shape) {
    std::lock_guard(this->mtx_);
    this->back_.reshape(shape);
    this->ready_.reshape(shape);
    this->front_.reshape(shape);
}


template<typename T>
class SliceBuffer : public TripleBufferInterface<std::map<size_t, std::tuple<bool, size_t, Tensor<T, 2>>>> {

public:

    using BufferType = std::map<size_t, std::tuple<bool, size_t, Tensor<T, 2>>>;
    using ValueType = typename BufferType::value_type;
    using SliceType = Tensor<T, 2>;
    using ShapeType = typename SliceType::ShapeType;

private:

    bool on_demand_;

    ShapeType shape_;

protected:

    void swap(BufferType& v1, BufferType& v2) noexcept override {
        v1.swap(v2);
        if (on_demand_) {
            for ([[maybe_unused]] auto& [k, v] : v2) std::get<0>(v) = false;
        }
    }

public:

    SliceBuffer(bool on_demand = false) : on_demand_(on_demand), shape_{0, 0} {}
    
    ~SliceBuffer() override = default;

    bool insert(size_t index);

    void reshape(const ShapeType& shape);

    const ShapeType& shape() const { return shape_; }

    size_t size() const { return this->back_.size(); }

    bool onDemand() const { return on_demand_; }
};

template<typename T>
bool SliceBuffer<T>::insert(size_t index) {
    std::lock_guard lk(this->mtx_);
    auto [it1, success1] = this->back_.insert({index, {!on_demand_, 0, SliceType(shape_)}});
    [[maybe_unused]] auto [it2, success2] = this->ready_.insert({index, {!on_demand_, 0, SliceType(shape_)}});
    [[maybe_unused]] auto [it3, success3] = this->front_.insert({index, {!on_demand_, 0, SliceType(shape_)}});
    assert(success1 == success2);
    assert(success1 == success3);
    return success1;
}

template<typename T>
void SliceBuffer<T>::reshape(const ShapeType& shape) {
    std::lock_guard lk(this->mtx_);
    for (auto& [k, v] : this->back_) std::get<2>(v).reshape(shape);
    for (auto& [k, v] : this->ready_) std::get<2>(v).reshape(shape);
    for (auto& [k, v] : this->front_) std::get<2>(v).reshape(shape);
    shape_ = shape;
}

namespace details {

template<typename T, typename D>
inline void copyBuffer(T* dst, const char* src, size_t n) {
    D v;
    for (size_t size = sizeof(D), i = 0; i < n; ++i) {
        memcpy(&v, src, size);
        src += size;
        *(dst++) = static_cast<T>(v);
    }
}

template<typename T, typename D>
inline void copyBuffer(T* dst, const char* src,
                       const std::array<size_t, 2>& dst_shape,
                       const std::array<size_t, 2>& src_shape,
                       const std::array<size_t, 2>& downsampling) {
    D v;
    for (size_t size = sizeof(D), 
             rstep = downsampling[0] * size * src_shape[1], 
             cstep = downsampling[1] * size, i = 0; i < dst_shape[0]; ++i) {
        char* ptr = const_cast<char*>(src) + i * rstep;
        for (size_t j = 0; j < dst_shape[1]; ++j) {
            memcpy(&v, ptr, size);
            ptr += cstep;
            *(dst++) = static_cast<T>(v);
        }
    }
}

} // details

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

    void pop();

  public:

    explicit MemoryBuffer(size_t capacity);
    ~MemoryBuffer() = default;

    void resize(const std::array<size_t, N>& shape);

    void reset();

    template<typename D>
    void fill(const char* raw, size_t chunk_idx, size_t data_idx);

    template<typename D>
    void fill(const char* raw, size_t chunk_idx, size_t data_idx, 
              const std::array<size_t, N-1>& shape, 
              const std::array<size_t, N-1>& downsampling);

    bool fetch(int timeout = -1);

    std::vector<T>& front() { return front_; }
    std::vector<T>& back() { return buffer_[map_.at(chunk_indices_.front())]; }

    const std::vector<T>& front() const { return front_; }
    const std::vector<T>& ready() const { return buffer_[map_.at(chunk_indices_.front())]; }
    const std::vector<T>& back() const { return buffer_[map_.at(chunk_indices_.front())]; }

    size_t capacity() const { assert(this->capacity_ == buffer_.size()); return this->capacity_; }

    size_t occupied() const { return this->capacity_ - unoccupied_.size(); }

    size_t chunkSize() const { return chunk_size_; }

    const std::array<size_t, N>& shape() const { return shape_; }
};

template<typename T, size_t N>
void MemoryBuffer<T, N>::pop() {
    size_t idx = chunk_indices_.front();
    chunk_indices_.pop();

    size_t buffer_idx = map_[idx];
    counter_[buffer_idx] = 0;
    unoccupied_.push(buffer_idx);
    is_ready_ = false;

    map_.erase(idx);
}

template<typename T, size_t N>
MemoryBuffer<T, N>::MemoryBuffer(size_t capacity) : capacity_(capacity) {}

template<typename T, size_t N>
void MemoryBuffer<T, N>::resize(const std::array<size_t, N>& shape) {
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

template<typename T, size_t N>
void MemoryBuffer<T, N>::reset() {
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


template<typename T, size_t N>
template<typename D>
void MemoryBuffer<T, N>::fill(const char* raw, size_t chunk_idx, size_t data_idx) {
    fill<D>(raw, chunk_idx, data_idx, {shape_[1], shape_[2]}, {1, 1});
}


template<typename T, size_t N>
template<typename D>
void MemoryBuffer<T, N>::fill(const char* raw, size_t chunk_idx, size_t data_idx, 
                              const std::array<size_t, N-1>& shape, 
                              const std::array<size_t, N-1>& downsampling) {
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
    size_t data_size = std::accumulate(shape_.begin() + 1, shape_.end(), 1, std::multiplies<>());
    if (std::all_of(downsampling.cbegin(), downsampling.cend(), [](size_t v) { return v == 1; })) {
        details::copyBuffer<T, D>(
            &data[data_idx * data_size], raw, data_size);
    } else {
        std::array<size_t, N-1> dst_shape;
        std::copy(shape_.begin() + 1, shape_.end(), dst_shape.begin());
        details::copyBuffer<T, D>(
            &data[data_idx * data_size], raw, dst_shape, shape, downsampling);
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

template<typename T, size_t N>
bool MemoryBuffer<T, N>::fetch(int timeout) {
    std::unique_lock lk(mtx_);
    if (timeout < 0) {
        cv_.wait(lk, [this] { return is_ready_; });
    } else {
        if (!(cv_.wait_for(lk, timeout * 1ms, [this] { return is_ready_; }))) {
            return false;
        }
    }
    front_.swap(buffer_[map_.at(chunk_indices_.front())]);
    pop();
    is_ready_ = false;
    
    return true;
}

} // tomcat::recon

#endif // RECON_BUFFER_H