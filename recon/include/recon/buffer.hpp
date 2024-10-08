/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_BUFFER_H
#define RECON_BUFFER_H

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <shared_mutex>
#include <thread>
#include <type_traits>

#include <spdlog/spdlog.h>

#include "common/config.hpp"
#include "tensor.hpp"

namespace recastx::recon {

using namespace std::chrono_literals;

template<typename T>
class BufferInterface {

  protected:

    T front_;

    std::mutex mtx_;
    std::condition_variable cv_;

    // Something beyond the normal swap might be needed.
    virtual void swap(T& v1, T& v2) noexcept { v1.swap(v2); };

  public:

    BufferInterface() = default;
    virtual ~BufferInterface() = default;

    BufferInterface(const BufferInterface&) = default;
    BufferInterface& operator=(const BufferInterface&) = default;

    BufferInterface(BufferInterface&&) = default;
    BufferInterface& operator=(BufferInterface&&) = default;

    T& front() { return front_; }
    const T& front() const { return front_; }

    virtual bool fetch(int timeout) = 0;
};

template<typename T>
class TripleBuffer : public BufferInterface<T> {

    bool is_ready_ = false;
    std::condition_variable cv2_;

protected:

    T back_;
    T ready_;

public:

    TripleBuffer() = default;

    TripleBuffer(const TripleBuffer&) = delete;
    TripleBuffer& operator=(const TripleBuffer&) = delete;

    TripleBuffer(TripleBuffer&&) = delete;
    TripleBuffer& operator=(TripleBuffer&&) = delete;
    
    bool fetch(int timeout) override;

    virtual bool prepare();

    virtual bool tryPrepare(int timeout);

    void reset() {
        std::lock_guard lk(this->mtx_);
        is_ready_ = false;
    }

    const T& ready() const { return ready_; }

    T& back() { return back_; };
    const T& back() const { return back_; };
};

template<typename T>
bool TripleBuffer<T>::fetch(int timeout) {
    {
        std::unique_lock lk(this->mtx_);
        if (timeout < 0) {
            this->cv_.wait(lk, [this] { return is_ready_; });
        } else {
            if (!this->cv_.wait_for(lk, timeout * 1ms, [this] { return is_ready_; })) {
                return false;
            }
        }
        this->swap(this->front_, ready_);
        is_ready_ = false;
    }
    this->cv2_.notify_one();
    return true;
}

template<typename T>
bool TripleBuffer<T>::prepare() {
    bool dropped;
    {
        std::lock_guard lk(this->mtx_);
        dropped = is_ready_;
        this->swap(ready_, back_);
        is_ready_ = true;
    }
    this->cv_.notify_one();
    return dropped;
}

template<typename T>
bool TripleBuffer<T>::tryPrepare(int timeout) {
    {
        std::unique_lock lk(this->mtx_);
        if (timeout < 0) {
            this->cv2_.wait(lk, [this] { return !is_ready_; });
        } else {
            if (!this->cv2_.wait_for(lk, timeout * 1ms, [this] { return !is_ready_; })) {
                return false;
            }
        }
        this->swap(ready_, back_);
        is_ready_ = true;
    }
    this->cv_.notify_one();
    return true;
}

template<typename T, size_t N>
class TripleTensorBuffer : public TripleBuffer<Tensor<T, N>> {

public:

    using BufferType = Tensor<T, N>;
    using ValueType = typename BufferType::value_type;
    using ShapeType = typename BufferType::ShapeType;

public:

    TripleTensorBuffer() = default;
    ~TripleTensorBuffer() override = default;

    void resize(const ShapeType& shape);

    const ShapeType& shape() const { return this->front_.shape(); }

    [[nodiscard]] size_t size() const { return this->front_.size(); }
};

template<typename T, size_t N>
void TripleTensorBuffer<T, N>::resize(const ShapeType& shape) {
    std::lock_guard lk(this->mtx_);
    this->back_.resize(shape);
    this->ready_.resize(shape);
    this->front_.resize(shape);
}

template<typename T, bool OD = false>
class SliceBuffer : public TripleBuffer<std::map<size_t, std::tuple<bool, size_t, Tensor<T, 2>>>> {

public:

    using BufferType = std::map<size_t, std::tuple<bool, size_t, Tensor<T, 2>>>;
    using ValueType = typename BufferType::value_type;
    using SliceType = Tensor<T, 2>;
    using ShapeType = typename SliceType::ShapeType;

private:

    ShapeType shape_;

protected:

    void swap(BufferType& v1, BufferType& v2) noexcept override {
        v1.swap(v2);
        if constexpr (OD) {
            for ([[maybe_unused]] auto& [k, v] : v2) std::get<0>(v) = false;
        }
    }

public:

    SliceBuffer() : shape_{0, 0} {}
    
    ~SliceBuffer() override = default;

    bool insert(size_t index);

    void resize(const ShapeType& shape);

    const ShapeType& shape() const { return shape_; }

    [[nodiscard]] size_t size() const { return this->back_.size(); }

    [[nodiscard]] bool onDemand() const { return OD; }
};

template<typename T, bool OD>
bool SliceBuffer<T, OD>::insert(size_t index) {
    std::lock_guard lk(this->mtx_);
    auto [it1, success1] = this->back_.insert({index, {!OD, 0, SliceType(shape_)}});
    [[maybe_unused]] auto [it2, success2] = this->ready_.insert({index, {!OD, 0, SliceType(shape_)}});
    [[maybe_unused]] auto [it3, success3] = this->front_.insert({index, {!OD, 0, SliceType(shape_)}});
    assert(success1 == success2);
    assert(success1 == success3);
    return success1;
}

template<typename T, bool OD>
void SliceBuffer<T, OD>::resize(const ShapeType& shape) {
    std::lock_guard lk(this->mtx_);
    for (auto& [k, v] : this->back_) std::get<2>(v).resize(shape);
    for (auto& [k, v] : this->ready_) std::get<2>(v).resize(shape);
    for (auto& [k, v] : this->front_) std::get<2>(v).resize(shape);
    shape_ = shape;
}


namespace details {

template<typename T, typename D>
inline void copyToBuffer(T *dst, const char *src, const std::array<size_t, 2> &shape) {
    D v;
    for (size_t size = sizeof(D), i = 0; i < shape[0] * shape[1]; ++i) {
        memcpy(&v, src, size);
        src += size;
        *(dst++) = static_cast<T>(v);
    }
}

template<typename T, typename D>
inline void copyToBuffer(T *dst,
                         const std::array<size_t, 2> &dst_shape,
                         const char *src,
                         const std::array<size_t, 2> &src_shape,
                         const std::array<size_t, 2> &downsampling) {
    if (src_shape == dst_shape) {
        assert(downsampling == (std::array < size_t, 2 > {1, 1}));
        copyToBuffer<T, D>(dst, src, dst_shape);
        return;
    }

    auto [ds_r, ds_c] = downsampling;

    auto rows_ds = src_shape[0] / ds_r;
    auto cols_ds = src_shape[1] / ds_c;

    assert(dst_shape[0] >= rows_ds);
    assert(dst_shape[1] >= cols_ds);

    D v;
    size_t padding_r = (dst_shape[0] - rows_ds) / 2;
    size_t padding_c = (dst_shape[1] - cols_ds) / 2;
    for (size_t size = sizeof(D), i = 0; i < rows_ds; ++i) {
        char* ptr_src = const_cast<char *>(src) + i * ds_r * size * src_shape[1];
        T *ptr_dst = dst + dst_shape[1] * (i + padding_r) + padding_c;
        for (size_t j = 0; j < cols_ds; ++j) {
            memcpy(&v, ptr_src, size);
            ptr_src += ds_c * size;
            *(ptr_dst++) = static_cast<T>(v);
        }
    }
}

} // details

template<typename T, size_t N>
class MemoryBuffer {

  public:

    using BufferType = Tensor<T, N>;
    using ValueType = typename BufferType::ValueType;
    using ShapeType = typename BufferType::ShapeType;

  private:

    std::shared_mutex data_mtx_;
    BufferType front_;
    std::deque<BufferType> buffer_;

    std::mutex index_mtx_;
    std::condition_variable cv_;
    bool is_ready_ = false;
    std::queue<size_t> chunk_indices_;
    std::unordered_map<size_t, size_t> map_;
    std::queue<size_t> unoccupied_;
    std::vector<size_t> counter_;
#if (VERBOSITY >= 2)
    size_t data_received_ = 0;
#endif

    size_t capacity_;

    void popChunk();

    template<typename D>
    void fillImp(size_t buffer_idx, 
                 size_t data_idx,
                 const char* src,
                 const std::array<size_t, N-1>& src_shape,
                 const std::array<size_t, N-1>& downsampling);

    void registerChunk(size_t idx);

    void update(size_t buffer_idx, size_t chunk_idx);

  public:

    explicit MemoryBuffer(int capacity);

    void resize(const std::array<size_t, N>& shape);

    void reset();

    template<typename D>
    void fill(size_t index,
              const char* src,
              const std::array<size_t, N-1>& shape,
              const std::array<size_t, N-1>& downsampling);

    bool fetch(int timeout);

    [[nodiscard]] bool isReady() const { return is_ready_; }

    BufferType& front() { return front_; }
    const BufferType& front() const { return front_; }

    BufferType& ready() { return buffer_[map_.at(chunk_indices_.front())]; }
    const BufferType& ready() const { return buffer_[map_.at(chunk_indices_.front())]; }

    [[nodiscard]] size_t capacity() const {
        assert(capacity_ == buffer_.size());
        return capacity_;
    }

    [[nodiscard]] size_t occupied() const {
        assert(capacity_ >= unoccupied_.size());
        return capacity_ - unoccupied_.size();
    }

    const ShapeType& shape() const { return front_.shape(); }

    [[nodiscard]] size_t size() const { return front_.size(); }
};

template<typename T, size_t N>
void MemoryBuffer<T, N>::popChunk() {
    size_t chunk_idx = chunk_indices_.front();
    chunk_indices_.pop();

    size_t buffer_idx = map_[chunk_idx];
    counter_[buffer_idx] = 0;
    unoccupied_.push(buffer_idx);
    map_.erase(chunk_idx);

    is_ready_ = !chunk_indices_.empty() && counter_[map_.at(chunk_indices_.front())] == front_.shape()[0];
}

template<typename T, size_t N>
MemoryBuffer<T, N>::MemoryBuffer(int capacity) {
    if (capacity <= 0) {
        throw std::runtime_error(fmt::format("[Image buffer] 'capacity' must be positive. Actual: {}", capacity));
    } else {
        capacity_ = static_cast<size_t>(capacity);
    }

    for (size_t i = 0; i < capacity_; ++i) {
        buffer_.push_back(BufferType {});
        counter_.push_back(0);
        unoccupied_.push(i);
    }
}

template<typename T, size_t N>
void MemoryBuffer<T, N>::resize(const std::array<size_t, N>& shape) {
    reset();

    std::lock_guard lck(data_mtx_);
    for (size_t i = 0; i < capacity_; ++i) buffer_[i].resize(shape, 0);
    front_.resize(shape, 0);
}

template<typename T, size_t N>
void MemoryBuffer<T, N>::reset() {
    std::lock_guard lk(index_mtx_);

    std::queue<size_t>().swap(chunk_indices_);
    std::queue<size_t>().swap(unoccupied_);
    map_.clear();

    for (size_t i = 0; i < capacity_; ++i) {
        counter_[i] = 0;
        unoccupied_.push(i);
    }

    is_ready_ = false;

#if (VERBOSITY >= 2)
    data_received_ = 0;
#endif

    spdlog::debug("[Memory buffer] Reset");
}

template<typename T, size_t N>
template<typename D>
void MemoryBuffer<T, N>::fill(size_t index,
                              const char* src,
                              const std::array<size_t, N-1>& shape,
                              const std::array<size_t, N-1>& downsampling) {
    size_t chunk_size = front_.shape()[0];
    size_t chunk_idx = index / chunk_size;
    size_t data_idx = index % chunk_size;

    std::unique_lock lk(index_mtx_);

    if (chunk_indices_.empty()) {
        registerChunk(chunk_idx);
    } else if (chunk_idx > chunk_indices_.back()) {
        for (size_t i = chunk_indices_.back() + 1; i <= chunk_idx; ++i) {
            if (unoccupied_.empty()) {
                int idx = chunk_indices_.front();
                popChunk();
                spdlog::warn("[Image buffer] Memory buffer is full! Chunk {} dropped!", idx);
            }
            registerChunk(i);
        }
    } else if (chunk_idx < chunk_indices_.front()) {
        spdlog::warn("[Image buffer] Received projection with outdated chunk index: {}, data ignored!",
                     chunk_idx);
        return;
    }

    size_t buffer_idx = map_[chunk_idx];
    lk.unlock();

    {
        std::shared_lock data_lk(data_mtx_);
        fillImp<D>(buffer_idx, data_idx, src, shape, downsampling);
    }

    lk.lock();
    update(buffer_idx, chunk_idx);

#if (VERBOSITY >= 2)
    // Outdated data are excluded.
    ++data_received_;
    if (data_received_ % chunk_size == 0) {
        spdlog::info("[Image buffer] {}/{} chunks are occupied. {} of images received in total.",
                     occupied(), buffer_.size(), data_received_);
    }
#endif

}

template<typename T, size_t N>
bool MemoryBuffer<T, N>::fetch(int timeout) {
    std::unique_lock lk(index_mtx_);
    if (timeout < 0) {
        cv_.wait(lk, [this] { return isReady(); });
    } else {
        if (!(cv_.wait_for(lk, timeout * 1ms, [this] { return isReady(); }))) {
            return false;
        }
    }

    assert(!map_.empty());
    {
        std::lock_guard data_lk(data_mtx_);
        front_.swap(ready());
    }
    popChunk();

    return true;
}

template<typename T, size_t N>
template<typename D>
void MemoryBuffer<T, N>::fillImp(size_t buffer_idx, 
                                 size_t data_idx,
                                 const char* src,
                                 const std::array<size_t, N-1>& src_shape,
                                 const std::array<size_t, N-1>& downsampling) {
    float* data = buffer_[buffer_idx].data();
    std::array<size_t, N-1> dst_shape;
    auto& buffer_shape = front_.shape();
    std::copy(buffer_shape.begin() + 1, buffer_shape.end(), dst_shape.begin());
    size_t data_size = std::accumulate(dst_shape.begin(), dst_shape.end(), 1, std::multiplies<>());
    details::copyToBuffer<T, D>(&data[data_idx * data_size], dst_shape, src, src_shape, downsampling);
}

template<typename T, size_t N>
void MemoryBuffer<T, N>::registerChunk(size_t idx) {
    chunk_indices_.push(idx);
    map_[idx] = unoccupied_.front();
    unoccupied_.pop();

    spdlog::debug("[Image buffer] Registered chunk: {}", idx);
}

template<typename T, size_t N>
void MemoryBuffer<T, N>::update(size_t buffer_idx, size_t chunk_idx) {
    ++counter_[buffer_idx];

    // Caveat: We do not track the data indices. Therefore, if data with the same
    //         frame idx arrives repeatedly, it will still fill the chunk.
    if (counter_[buffer_idx] == front_.shape()[0]) {
        // Remove earlier chunks, no matter they are ready or not.
        size_t idx = chunk_indices_.front();
        while (chunk_idx != idx) {
            popChunk();
            spdlog::warn("[Image buffer] Chunk {} is ready! Earlier chunks {} ({}/{}) dropped!",
                         chunk_idx, idx, counter_[map_[idx]], front_.shape()[0]);
            idx = chunk_indices_.front();
        }
        is_ready_ = true;
        cv_.notify_one();
    }
}

} // namespace recastx::recon

#endif // RECON_BUFFER_H