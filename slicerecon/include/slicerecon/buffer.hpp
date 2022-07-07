#ifndef BUFFER_H
#define BUFFER_H

namespace slicerecon {

template<typename T>
class Buffer {

    std::vector<T> bf1_;
    std::vector<T> bf2_;

    std::vector<int> indices1_;
    std::vector<int> indices2_;

    size_t size_ = 0;

public:

    Buffer() = default;

    ~Buffer() = default;

    void swap() {
        bf1_.swap(bf2_);
        indices1_.swap(indices2_);
        indices2_.clear();
    }

    void initialize(size_t s, int pixels) {
        bf1_.resize(s * pixels);
        bf2_.resize(s * pixels);
        size_ = s;
    }

    template<typename D>
    void fill(char* data, int buffer_index, int index, int pixels) {
        std::vector<T>& bf = bf1_;
        if (buffer_index == 0) {
            indices1_.push_back(index);
        } else if (buffer_index == 1) {
            bf = bf2_;
            indices2_.push_back(index);
        }
        else
            throw std::runtime_error("Invalid buffer_index" + std::to_string(buffer_index));

        for (int i = index * pixels; i < (index + 1) * pixels; ++i) {
            D v;
            memcpy(&v, data, sizeof(D));
            data += sizeof(D);
            bf[i] = static_cast<float>(v);
        }
    }

    std::vector<T>& front() { return bf1_; }
    std::vector<T>& back() { return bf2_; };

    const std::vector<T>& front() const { return bf1_; }
    const std::vector<T>& back() const { return bf2_; };

    bool full() { return indices1_.size() == size_; }
};

} // slicerecon

#endif // BUFFER_H