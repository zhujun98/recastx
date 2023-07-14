/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_RAMPFILTER_H
#define RECON_RAMPFILTER_H

#include <complex>
#include <vector>

extern "C" {
#include <fftw3.h>
}

#include "filter_interface.hpp"

namespace recastx::recon {

class RampFilter : public Filter {

  public:

    using DataType = std::vector<float>;

  protected:

    std::vector<std::vector<std::complex<float>>> freq_;
    DataType filter_;
    fftwf_plan fft_plan_;
    fftwf_plan ffti_plan_;

    int num_cols_;
    int num_rows_;

    static DataType frequency(int n);

    virtual void initFilter() = 0;
  
  public:
  
    RampFilter(float* data, int num_cols, int num_rows, int buffer_size);

    ~RampFilter();

    void apply(float* data, int buffer_index) override;

    static DataType generate(int n);
};


class RamlakFilter : public RampFilter {

  protected:

    void initFilter() override;

  public:

    RamlakFilter(float* data, int num_cols, int num_rows, int buffer_size);

    static DataType generate(int n);
};


class SheppFilter : public RampFilter {

  protected:

    void initFilter() override;

  public:

    SheppFilter(float* data, int num_cols, int num_rows, int buffer_size);

    static DataType generate(int n);
};


class RampFilterFactory: public FilterFactory {

  public:

    std::unique_ptr<Filter> create(const std::string& name, 
                                   float* data, int num_cols, int num_rows, int buffer_size) override;

};

} // namespace recastx::recon

#endif // RECON_RAMPFILTER_H