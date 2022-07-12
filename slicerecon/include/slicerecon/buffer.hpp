#ifndef BUFFER_H
#define BUFFER_H

namespace slicerecon {

template<typename T>
class DoubleBufferInterface {

  protected:

    std::vector<T> back_;
    std::vector<T> front_;

  public:

    virtual void swap() = 0;

    std::vector<T>& front() { return front_; }
    std::vector<T>& back() { return back_; };

    const std::vector<T>& front() const { return front_; }
    const std::vector<T>& back() const { return back_; };
};

template<typename T>
class SimpleBuffer : public DoubleBufferInterface<T> {

  public:

    SimpleBuffer() = default;
    ~SimpleBuffer() = default;

    void swap() override { this->front_.swap(this->back_); }

    void initialize(size_t capacity) {
        this->back_.resize(capacity);
        this->front_.resize(capacity);
    }

};

template<typename T>
class Buffer : public DoubleBufferInterface<T> {

    std::vector<int> indices_back_;
    std::vector<int> indices_front_;

    int buffer_index_ = -2; // current buffer index

    size_t capacity_ = 0;

  public:

    Buffer() = default;
    ~Buffer() = default;

    void swap() override {
        this->front_.swap(this->back_);
        indices_front_.swap(indices_back_);
        indices_front_.clear();
        buffer_index_ += 1;
    }

    void initialize(size_t capacity, int size) {
        this->back_.resize(capacity * size);
        this->front_.resize(capacity * size);
        capacity_ = capacity;
    }

    template<typename D>
    void fill(char* raw, int buffer_index, int index, int size) {
        std::vector<T>* data;
        if (buffer_index == buffer_index_) {
            data = &(this->back_);
            indices_back_.push_back(index);
        } else if (buffer_index - buffer_index_ == 1) {
            data = &(this->front_);
            indices_front_.push_back(index);
        } else if (buffer_index - buffer_index_ > 1) {
            buffer_index_ = buffer_index; // initialization
            data = &(this->back_);
            indices_back_.push_back(index);
        } else {
            throw std::runtime_error(
                "Received old buffer index" + std::to_string(buffer_index));
        }

        for (int i = index * size; i < (index + 1) * size; ++i) {
            D v;
            memcpy(&v, raw, sizeof(D));
            raw += sizeof(D);
            (*data)[i] = static_cast<float>(v);
        }
    }

    bool full() const { return indices_back_.size() == capacity_; }
    size_t size() const { return indices_back_.size(); }
};

} // slicerecon

#endif // BUFFER_H