#ifndef BUFFER_H
#define BUFFER_H

namespace slicerecon {

template<typename T>
class Buffer {

    std::vector<T> bf1_;
    std::vector<T> bf2_;

    std::vector<int> indices1_;
    std::vector<int> indices2_;

    size_t capacity_ = 0;

public:

    Buffer() = default;

    ~Buffer() = default;

    void swap() {
        bf1_.swap(bf2_);
        indices1_.swap(indices2_);
        indices2_.clear();
    }

    void initialize(size_t capacity, int pixels) {
        bf1_.resize(capacity * pixels);
        bf2_.resize(capacity * pixels);
        capacity_ = capacity;
    }

    template<typename D>
    void fill(char* data, int buffer_index, int index, int pixels) {
        std::vector<T>* bf;
        if (buffer_index == 0) {
            bf = &bf1_;
            indices1_.push_back(index);
        } else if (buffer_index == 1) {
            bf = &bf2_;
            indices2_.push_back(index);
        } else
            throw std::runtime_error("Invalid buffer_index" + std::to_string(buffer_index));

        for (int i = index * pixels; i < (index + 1) * pixels; ++i) {
            D v;
            memcpy(&v, data, sizeof(D));
            data += sizeof(D);
            (*bf)[i] = static_cast<float>(v);
        }
    }

    std::vector<T>& front() { return bf1_; }
    std::vector<T>& back() { return bf2_; };

    const std::vector<T>& front() const { return bf1_; }
    const std::vector<T>& back() const { return bf2_; };

    bool full() const { return indices1_.size() == capacity_; }
    size_t size() const { return indices1_.size(); }
};

} // slicerecon

#endif // BUFFER_H