#ifndef BUFFER_H
#define BUFFER_H

namespace slicerecon {

template<typename T>
class Buffer {

    std::vector<T> bf1_;
    std::vector<T> bf2_;

public:

    Buffer() = default;

    ~Buffer() = default;

    void swap() {
        bf1_.swap(bf2_);
    }

    void initialize(size_t s) {
        bf1_.resize(s);
        bf2_.resize(s);
    }

    std::vector<T>& front() { return bf1_; }
    std::vector<T>& back() { return bf2_; };

    const std::vector<T>& front() const { return bf1_; }
    const std::vector<T>& back() const { return bf2_; };
};

} // slicerecon

#endif // BUFFER_H