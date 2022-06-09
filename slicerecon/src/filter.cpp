#include "slicerecon/filter.hpp"


namespace slicerecon {

Filter::Filter(const std::string& filter, 
               bool gaussian_pass,
               float* data, 
               int rows, 
               int cols) : rows_(rows), cols_(cols) {

    freq_ = std::vector<std::vector<std::complex<float>>>(
        rows, std::vector<std::complex<float>>(cols));

    fft_plan_ = fftwf_plan_dft_r2c_1d(
        cols, 
        data,
        reinterpret_cast<fftwf_complex*>(&freq_[0][0]),
        FFTW_ESTIMATE);

    ffti_plan_ = fftwf_plan_dft_c2r_1d(
        cols,
        reinterpret_cast<fftwf_complex*>(&freq_[0][0]),
        data, 
        FFTW_ESTIMATE);

    if (filter == "shepp"){
        filter_ = Filter::shepp(cols);
    } else if (filter == "ramlak") {
        filter_ = Filter::ramlak(cols);
    } else {
        throw std::invalid_argument("Unsupported filter: " + filter);
    }

    if (gaussian_pass) {
        auto filter_lowpass = gaussian(cols, 0.06f);
        for (int i = 0; i < cols; ++i) filter_[i] *= filter_lowpass[i];
    }
}

void Filter::apply(float *data) {
    for (int r = 0; r < rows_; ++r) {
        fftwf_execute_dft_r2c(fft_plan_, 
                              &data[r * cols_],
                              reinterpret_cast<fftwf_complex*>(&freq_[r][0]));

        for (int c = 0; c < cols_; ++c) freq_[r][c] *= filter_[c];

        fftwf_execute_dft_c2r(ffti_plan_,
                              reinterpret_cast<fftwf_complex*>(&freq_[r][0]),
                              &data[r * cols_]);
    }
}

std::vector<float> Filter::ramlak(int cols) {
    auto result = std::vector<float>(cols);
    auto mid = (cols + 1) / 2;
    for (int i = 0; i < mid; ++i) result[i] = i;
    for (int j = mid; j < cols; ++j) result[j] = 2 * mid - j;
    return result;
}

std::vector<float> Filter::shepp(int cols) {
    auto result = std::vector<float>(cols);
    auto mid = (cols + 1) / 2;

    auto filter_weight = [=](auto i) {
        auto norm_freq = (i / (float)mid);
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