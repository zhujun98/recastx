/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "recon/phase.hpp"


namespace recastx::recon {

namespace details {

std::vector<float> paganinFilter(float pixel_size, float lambda, float delta, float beta, float distance, 
                                 int num_cols, int num_rows) {
    auto filter = std::vector<float>(num_rows * num_cols);

    auto dx = pixel_size / (2.0f * M_PI);
    auto dy = dx;
    auto mid_x = (num_cols + 1) / 2;
    auto mid_y = (num_rows + 1) / 2;

    for (int i = 0; i < num_rows; ++i) {
        for (int j = 0; j < num_cols; ++j) {
            // TODO Is this is FFTW convention compared to numpy?
            auto x = i < mid_x ? i : (2 * mid_x - i);
            auto y = j < mid_y ? j : (2 * mid_y - j);
            auto k_x = x * dx;
            auto k_y = y * dy;
            auto k_squared = k_x * k_x + k_y * k_y;
            filter[i * num_cols + j] = (4.0f * beta * M_PI) / 1.0f + distance * lambda * delta * k_squared;
        }
    }
    return filter;
}

}; // details

Paganin::Paganin(float pixel_size, float lambda, float delta, float beta, float distance, 
                 float* data, int num_cols, int num_rows): 
        pixel_size_(pixel_size), 
        lambda_(lambda),
        delta_(delta),
        beta_(beta),
        distance_(distance),
        num_cols_(num_cols),
        num_rows_(num_rows)  {

    freq_ = std::vector<std::vector<std::complex<float>>>(
        1, std::vector<std::complex<float>>(num_cols_ * num_rows_));

    fft2d_plan_ = fftwf_plan_dft_r2c_2d(
        num_cols_,
        num_rows_,
        data,
        reinterpret_cast<fftwf_complex*>(&freq_[0][0]),
        FFTW_ESTIMATE);

    ffti2d_plan_ = fftwf_plan_dft_c2r_2d(
        num_cols_,
        num_rows_,
        reinterpret_cast<fftwf_complex*>(&freq_[0][0]),
        data,
        FFTW_ESTIMATE);

    filter_ = details::paganinFilter(pixel_size_, lambda_, delta_, beta_, distance_, 
                                     num_cols_, num_rows_);
}

void Paganin::apply(float* data, int idx) {
    // take fft of proj
    // FIXME: cause segmentation fault in unittest
    fftwf_execute_dft_r2c(fft2d_plan_, 
                          data,
                          reinterpret_cast<fftwf_complex*>(&freq_[idx][0]));

    // filter the proj in 2D
    for (int i = 0; i < num_rows_ * num_cols_; ++i) freq_[idx][i] *= filter_[i];

    // ifft the proj
    fftwf_execute_dft_c2r(ffti2d_plan_,
                          reinterpret_cast<fftwf_complex*>(&freq_[idx][0]),
                          data);

    // log and scale
    for (int i = 0; i < num_rows_ * num_cols_; ++i) {
        data[i] = data[i] <= 0.0f ? 0.0f : -std::log(data[i]);
        data[i] *= lambda_ / (4.0 * M_PI * beta_);
    }
}

} // namespace recastx::recon