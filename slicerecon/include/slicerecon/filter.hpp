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

  public:
    Filter(const std::string& filter,
           bool gaussian_pass,
           float* data, 
           int rows, 
           int cols);

    void apply(float* data);

    std::vector<float> ramlak(int n);
    std::vector<float> shepp(int n);
    std::vector<float> gaussian(int n, float sigma);
};

} // slicerecon


