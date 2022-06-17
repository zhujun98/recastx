#pragma once

#include <complex>
#include <vector>

extern "C" {
#include <fftw3.h>
}

#include "data_types.hpp"

namespace slicerecon {

class Filter {
    std::vector<std::vector<std::complex<float>>> freq_;
    std::vector<float> filter_;
    fftwf_plan fft_plan_;
    fftwf_plan ffti_plan_;

    int rows_;
    int cols_;

    static std::vector<float> frequency(int n);

  public:
    Filter(const std::string& filter_name, 
           bool gaussian_pass, 
           float* data,
           int rows, 
           int cols,
           int buffer_size);

    ~Filter();

    void initialize(const std::string& filter_name, 
                    bool gaussian_pass, 
                    float* data, 
                    int buffer_size);

    void apply(float* data, int buffer_index);

    static std::vector<float> ramlak(int n);
    static std::vector<float> shepp(int n);
    static std::vector<float> gaussian(int n, float sigma);
};

} // slicerecon


