#ifndef RECON_PHASE_H
#define RECON_PHASE_H

#include <complex>
#include <vector>

extern "C" {
#include <fftw3.h>
}

namespace tomcat::recon {

namespace details {
    std::vector<float> paganinFilter(
        float pixel_size, float lambda, float delta, float beta, float distance, int cols, int rows);
} // details

class Paganin {
    float pixel_size_;
    float lambda_;
    float delta_;
    float beta_;
    float distance_;

    int cols_;
    int rows_;

    fftwf_plan fft2d_plan_;
    fftwf_plan ffti2d_plan_;
    std::vector<std::vector<std::complex<float>>> freq_;
    std::vector<float> filter_;

  public:
    Paganin(float pixel_size, float lambda, float delta, float beta, float distance, float* data, int cols, int rows);

    void apply(float* data, int idx=1);

};

} // tomcat::recon

#endif // RECON_PHASE_H