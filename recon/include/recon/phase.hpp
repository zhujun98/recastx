/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_PHASE_H
#define RECON_PHASE_H

#include <complex>
#include <vector>

extern "C" {
#include <fftw3.h>
}

namespace recastx::recon {

namespace details {
    std::vector<float> paganinFilter(
        float pixel_size, float lambda, float delta, float beta, float distance, 
        int num_cols, int num_rows);
} // details

class Paganin {
    float pixel_size_;
    float lambda_;
    float delta_;
    float beta_;
    float distance_;

    int num_cols_;
    int num_rows_;

    fftwf_plan fft2d_plan_;
    fftwf_plan ffti2d_plan_;
    std::vector<std::vector<std::complex<float>>> freq_;
    std::vector<float> filter_;

  public:

    Paganin(float pixel_size, float lambda, float delta, float beta, float distance, 
            float* data, int num_cols, int num_rows);

    ~Paganin();

    void apply(float* data, int idx=1);

};

} // namespace recastx::recon

#endif // RECON_PHASE_H