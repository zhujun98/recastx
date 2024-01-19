/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <iostream>

#include "recon/ramp_filter.hpp"

namespace recastx::recon {

RampFilter::RampFilter(float* data, int num_cols, int num_rows, int buffer_size)
         : num_cols_(num_cols), num_rows_(num_rows)  {
    freq_ = std::vector<std::vector<std::complex<float>>>(
        buffer_size, std::vector<std::complex<float>>(num_cols_));

    fft_plan_ = fftwf_plan_dft_r2c_1d(
        num_cols_, 
        data,
        reinterpret_cast<fftwf_complex*>(&freq_[0][0]),
        FFTW_ESTIMATE);

    ffti_plan_ = fftwf_plan_dft_c2r_1d(
        num_cols_,
        reinterpret_cast<fftwf_complex*>(&freq_[0][0]),
        data, 
        FFTW_ESTIMATE);
}

void RampFilter::apply(float *data, int buffer_index) {
    for (int r = 0; r < num_rows_; ++r) {
        auto idx = r * num_cols_;

        fftwf_execute_dft_r2c(fft_plan_, 
                              &data[idx],
                              reinterpret_cast<fftwf_complex*>(&freq_[buffer_index][0]));

        for (int c = 0; c < num_cols_; ++c) freq_[buffer_index][c] *= filter_[c];

        fftwf_execute_dft_c2r(ffti_plan_,
                              reinterpret_cast<fftwf_complex*>(&freq_[buffer_index][0]),
                              &data[idx]);
    }
}

RampFilter::DataType RampFilter::frequency(int n) {
    auto ret = std::vector<float>(n);
    int mid = (n + 1) / 2;
    for (int i = 0; i < mid; ++i) ret[i] = static_cast<float>(i) / n;
    for (int i = mid; i < n; ++i) ret[i] = static_cast<float>(i) / n - 1.f;
    return ret;  
}


RamlakFilter::RamlakFilter(float* data, int num_cols, int num_rows, int buffer_size)
        : RampFilter(data, num_cols, num_rows, buffer_size)  {
    initFilter();
}

void RamlakFilter::initFilter() {
    filter_ = RamlakFilter::generate(num_cols_);
}

RampFilter::DataType RamlakFilter::generate(int n) {
    auto ret = RampFilter::frequency(n);
    float c = 2.f / n; // compensate for unnormalized fft in fftw
    for (int i = 0; i < n; ++i) ret[i] = c * std::abs(ret[i]);
    return ret;
}


SheppFilter::SheppFilter(float* data, int num_cols, int num_rows, int buffer_size)
        : RampFilter(data, num_cols, num_rows, buffer_size)  {
    initFilter();
}

void SheppFilter::initFilter() {
    filter_ = SheppFilter::generate(num_cols_);
}

RampFilter::DataType SheppFilter::generate(int n) {
    auto ret = RampFilter::frequency(n);
    float c = 2.f / n; // compensate for unnormalized fft in fftw
    for (int i = 1; i < n; ++i) {
        float tmp = M_PI * ret[i];
        ret[i] = c * std::abs(ret[i] * std::sin(tmp) / tmp);
    }
    return ret;
}


// std::vector<float> RampFilter::gaussian(int n, float sigma) {
//     auto result = std::vector<float>(n);
//     auto mid = (n + 1) / 2;

//     auto filter_weight = [=](auto i) {
//         auto norm_freq = (i / (float)mid);
//         return std::exp(-(norm_freq * norm_freq) / (2.0f * sigma * sigma));
//     };

//     for (int i = 1; i < mid; ++i) result[i] = filter_weight(i);
//     for (int j = mid; j < n; ++j) result[j] = filter_weight(2 * mid - j);
//     return result;
// }


std::unique_ptr<Filter> 
RampFilterFactory::create(const std::string& name, 
                          float* data, int num_cols, int num_rows, int buffer_size) {
    
    if (name == "shepp") return std::make_unique<SheppFilter>(data, num_cols, num_rows, buffer_size);

    if (name == "ramlak") return std::make_unique<RamlakFilter>(data, num_cols, num_rows, buffer_size);
    
    throw std::invalid_argument("Unknown ramp filter: " + name);
}

} // namespace recastx::recon