#include "recon/filter.hpp"
#include <iostream>

namespace recastx::recon {

Filter::Filter(const std::string& filter_name, 
               bool gaussian_lowpass_filter,
               float* data,
               int num_cols,
               int num_rows, 
               int buffer_size) : num_cols_(num_cols), num_rows_(num_rows)  {
    initialize(filter_name, gaussian_lowpass_filter, data, buffer_size);
}

Filter::~Filter() = default;

void Filter::initialize(const std::string& filter_name, 
                        bool gaussian_lowpass_filter, 
                        float* data, 
                        int buffer_size) {
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

    if (filter_name == "shepp"){
        filter_ = Filter::shepp(num_cols_);
    } else if (filter_name == "ramlak") {
        filter_ = Filter::ramlak(num_cols_);
    } else {
        throw std::invalid_argument("Unsupported filter: " + filter_name);
    }

    if (gaussian_lowpass_filter) {
        auto filter_lowpass = gaussian(num_cols_, 0.06f);
        for (int i = 0; i < num_cols_; ++i) filter_[i] *= filter_lowpass[i];
    }
}

void Filter::apply(float *data, int buffer_index) {
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

std::vector<float> Filter::frequency(int n) {
    auto ret = std::vector<float>(n);
    int mid = (n + 1) / 2;
    for (int i = 0; i < mid; ++i) ret[i] = static_cast<float>(i) / n;
    for (int i = mid; i < n; ++i) ret[i] = static_cast<float>(i) / n - 1.f;
    return ret;  
}

std::vector<float> Filter::ramlak(int n) {
    auto ret = Filter::frequency(n);
    float c = 2.f / n; // compensate for unnormalized fft in fftw
    for (int i = 0; i < n; ++i) ret[i] = c * std::abs(ret[i]);
    return ret;
}

std::vector<float> Filter::shepp(int n) {
    auto ret = Filter::frequency(n);
    float c = 2.f / n; // compensate for unnormalized fft in fftw
    for (int i = 1; i < n; ++i) {
        float tmp = M_PI * ret[i];
        ret[i] = c * std::abs(ret[i] * std::sin(tmp) / tmp);
    }
    return ret;
}

std::vector<float> Filter::gaussian(int n, float sigma) {
    auto result = std::vector<float>(n);
    auto mid = (n + 1) / 2;

    auto filter_weight = [=](auto i) {
        auto norm_freq = (i / (float)mid);
        return std::exp(-(norm_freq * norm_freq) / (2.0f * sigma * sigma));
    };

    for (int i = 1; i < mid; ++i) result[i] = filter_weight(i);
    for (int j = mid; j < n; ++j) result[j] = filter_weight(2 * mid - j);
    return result;
}

} // namespace recastx::recon