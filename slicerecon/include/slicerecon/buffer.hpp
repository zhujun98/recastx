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

    template<typename D>
    void fill(char* data, int buffer_index, int index, int pixels) {
        std::vector<T>& bf = bf1_;
        if (buffer_index == 0) ;
        else if (buffer_index == 1) bf = bf2_;
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
};

} // slicerecon

#endif // BUFFER_H