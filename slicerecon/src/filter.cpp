#include "slicerecon/filter.hpp"


namespace slicerecon {

Filter::Filter(const std::string& filter_name, 
               bool gaussian_pass,
               float* data,
               int rows, 
               int cols,
               int buffer_size) : rows_(rows), cols_(cols) {
    initialize(filter_name, gaussian_pass, data, buffer_size);
}

Filter::~Filter() = default;

void Filter::initialize(const std::string& filter_name, 
                        bool gaussian_pass, 
                        float* data, 
                        int buffer_size) {
    freq_ = std::vector<std::vector<std::complex<float>>>(
        buffer_size, std::vector<std::complex<float>>(cols_));

    fft_plan_ = fftwf_plan_dft_r2c_1d(
        cols_, 
        data,
        reinterpret_cast<fftwf_complex*>(&freq_[0][0]),
        FFTW_ESTIMATE);

    ffti_plan_ = fftwf_plan_dft_c2r_1d(
        cols_,
        reinterpret_cast<fftwf_complex*>(&freq_[0][0]),
        data, 
        FFTW_ESTIMATE);

    if (filter_name == "shepp"){
        filter_ = Filter::shepp(cols_);
    } else if (filter_name == "ramlak") {
        filter_ = Filter::ramlak(cols_);
    } else {
        throw std::invalid_argument("Unsupported filter: " + filter_name);
    }

    if (gaussian_pass) {
        auto filter_lowpass = gaussian(cols_, 0.06f);
        for (int i = 0; i < cols_; ++i) filter_[i] *= filter_lowpass[i];
    }
}

void Filter::apply(float *data, int buffer_index) {
    for (int r = 0; r < rows_; ++r) {
        fftwf_execute_dft_r2c(fft_plan_, 
                              &data[r * cols_],
                              reinterpret_cast<fftwf_complex*>(&freq_[buffer_index][0]));

        for (int c = 0; c < cols_; ++c) freq_[buffer_index][c] *= filter_[c];

        fftwf_execute_dft_c2r(ffti_plan_,
                              reinterpret_cast<fftwf_complex*>(&freq_[buffer_index][0]),
                              &data[r * cols_]);
    }
}

std::vector<float> Filter::ramlak(int cols) {
    auto result = std::vector<float>(cols);
    int mid = (cols + 1) / 2;
    for (int i = 0; i < mid; ++i) result[i] = i;
    for (int j = mid; j < cols; ++j) result[j] = 2 * mid - j;
    return result;
}

std::vector<float> Filter::shepp(int cols) {
    auto result = std::vector<float>(cols);
    int mid = (cols + 1) / 2;

    auto filter_weight = [=](int i) {
        auto norm_freq = i / static_cast<float>(mid);
        return norm_freq * std::sin(M_PI * norm_freq) / (M_PI * norm_freq);
    };

    for (int i = 1; i < mid; ++i) result[i] = filter_weight(i);
    for (int j = mid; j < cols; ++j) result[j] = filter_weight(2 * mid - j);
    return result;
}

std::vector<float> Filter::gaussian(int n, float sigma)
{
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

} // slicerecon