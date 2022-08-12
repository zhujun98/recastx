#include <complex>
#include <vector>

extern "C" {
#include <fftw3.h>
}

namespace tomcat::recon {

namespace details {
    std::vector<float> paganinFilter(
        float pixel_size, float lambda, float delta, float beta, float distance, int rows, int cols);
} // details

class Paganin {
    float pixel_size_;
    float lambda_;
    float delta_;
    float beta_;
    float distance_;

    int rows_;
    int cols_;

    fftwf_plan fft2d_plan_;
    fftwf_plan ffti2d_plan_;
    std::vector<std::vector<std::complex<float>>> freq_;
    std::vector<float> filter_;

  public:
    Paganin(float pixel_size, float lambda, float delta, float beta, float distance, float* data, int rows, int cols);

    void apply(float* data, int idx=1);

};

} // tomcat::recon