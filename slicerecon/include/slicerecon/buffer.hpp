#ifndef BUFFER_H
#define BUFFER_H

namespace slicerecon {

template<typename T>
class DoubleBufferInterface {

  protected:

    std::vector<T> data1_;
    std::vector<T> data2_;

  public:

    virtual void swap() = 0;

    std::vector<T>& front() { return data1_; }
    std::vector<T>& back() { return data2_; };

    const std::vector<T>& front() const { return data1_; }
    const std::vector<T>& back() const { return data2_; };
};

template<typename T>
class SimpleBuffer : public DoubleBufferInterface<T> {

  public:

    SimpleBuffer() = default;
    ~SimpleBuffer() = default;

    void swap() override { this->data1_.swap(this->data2_); }

    void initialize(size_t capacity) {
        this->data1_.resize(capacity);
        this->data2_.resize(capacity);
    }

};

template<typename T>
class Buffer : public DoubleBufferInterface<T> {

    std::vector<int> indices1_;
    std::vector<int> indices2_;

    int buffer_index_ = -2; // current buffer index

    size_t capacity_ = 0;

  public:

    Buffer() = default;
    ~Buffer() = default;

    void swap() override {
        this->data1_.swap(this->data2_);
        indices1_.swap(indices2_);
        indices2_.clear();
        buffer_index_ += 1;
    }

    void initialize(size_t capacity, int size) {
        this->data1_.resize(capacity * size);
        this->data2_.resize(capacity * size);
        capacity_ = capacity;
    }

    template<typename D>
    void fill(char* raw, int buffer_index, int index, int size) {
        std::vector<T>* data;
        if (buffer_index == buffer_index_) {
            data = &(this->data1_);
            indices1_.push_back(index);
        } else if (buffer_index - buffer_index_ == 1) {
            data = &(this->data2_);
            indices2_.push_back(index);
        } else if (buffer_index - buffer_index_ > 1) {
            buffer_index_ = buffer_index; // initialization
            data = &(this->data1_);
            indices1_.push_back(index);
        } else {
            throw std::runtime_error("Received old buffer index" + std::to_string(buffer_index));
        }

        for (int i = index * size; i < (index + 1) * size; ++i) {
            D v;
            memcpy(&v, raw, sizeof(D));
            raw += sizeof(D);
            (*data)[i] = static_cast<float>(v);
        }
    }

    bool full() const { return indices1_.size() == capacity_; }
    size_t size() const { return indices1_.size(); }
};

} // slicerecon

#endif // BUFFER_H