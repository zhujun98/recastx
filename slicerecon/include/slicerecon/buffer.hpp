#ifndef BUFFER_H
#define BUFFER_H

namespace slicerecon {

class BufferInterface {
public:
    virtual void swap() = 0;
};

template<typename T>
class SimpleBuffer : BufferInterface {

    std::vector<T> data1_;
    std::vector<T> data2_;

public:

    SimpleBuffer() = default;
    ~SimpleBuffer() = default;

    void swap() override { data1_.swap(data2_); }

    void initialize(size_t capacity) {
        data1_.resize(capacity);
        data2_.resize(capacity);
    }

    std::vector<T>& front() { return data1_; }
    std::vector<T>& back() { return data2_; };

    const std::vector<T>& front() const { return data1_; }
    const std::vector<T>& back() const { return data2_; };

};

template<typename T>
class Buffer : BufferInterface {

    std::vector<T> data1_;
    std::vector<T> data2_;

    std::vector<int> indices1_;
    std::vector<int> indices2_;

    int buffer_index_ = -2; // current buffer index

    size_t capacity_ = 0;

public:

    Buffer() = default;
    ~Buffer() = default;

    void swap() override {
        data1_.swap(data2_);
        indices1_.swap(indices2_);
        indices2_.clear();
        buffer_index_ += 1;
    }

    void initialize(size_t capacity, int pixels) {
        data1_.resize(capacity * pixels);
        data2_.resize(capacity * pixels);
        capacity_ = capacity;
    }

    template<typename D>
    void fill(char* raw, int buffer_index, int index, int pixels) {
        std::vector<T>* data;
        if (buffer_index == buffer_index_) {
            data = &data1_;
            indices1_.push_back(index);
        } else if (buffer_index - buffer_index_ == 1) {
            data = &data2_;
            indices2_.push_back(index);
        } else if (buffer_index - buffer_index_ > 1) {
            buffer_index_ = buffer_index; // initialization
            data = &data1_;
            indices1_.push_back(index);
        } else {
            throw std::runtime_error("Received old buffer index" + std::to_string(buffer_index));
        }

        for (int i = index * pixels; i < (index + 1) * pixels; ++i) {
            D v;
            memcpy(&v, raw, sizeof(D));
            raw += sizeof(D);
            (*data)[i] = static_cast<float>(v);
        }
    }

    std::vector<T>& front() { return data1_; }
    std::vector<T>& back() { return data2_; };

    const std::vector<T>& front() const { return data1_; }
    const std::vector<T>& back() const { return data2_; };

    bool full() const { return indices1_.size() == capacity_; }
    size_t size() const { return indices1_.size(); }
};

} // slicerecon

#endif // BUFFER_H