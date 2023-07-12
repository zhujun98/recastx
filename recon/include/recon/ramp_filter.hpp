/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_FILTER_H
#define RECON_FILTER_H

#include <complex>
#include <vector>

extern "C" {
#include <fftw3.h>
}

namespace recastx::recon {

class RampFilter {
    std::vector<std::vector<std::complex<float>>> freq_;
    std::vector<float> filter_;
    fftwf_plan fft_plan_;
    fftwf_plan ffti_plan_;

    int num_cols_;
    int num_rows_;

    static std::vector<float> frequency(int n);

  public:
  
    RampFilter(const std::string& filter_name, 
               float* data,
               int num_cols,
               int num_rows, 
               int buffer_size);

    ~RampFilter();

    void initialize(const std::string& filter_name, 
                    float* data, 
                    int buffer_size);

    void apply(float* data, int buffer_index);

    static std::vector<float> ramlak(int n);
    static std::vector<float> shepp(int n);
    static std::vector<float> gaussian(int n, float sigma);
};

} // namespace recastx::recon

#endif // RECON_FILTER_H