/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_FILTERINTERFACE_H
#define RECON_FILTERINTERFACE_H

#include <memory>

namespace recastx::recon {

class Filter {

  public:

    virtual ~Filter() = default;

    virtual void apply(float* data, int buffer_index) = 0;
};

class FilterFactory {

  public:

    virtual ~FilterFactory() = default;

    virtual std::unique_ptr<Filter> create(const std::string& name, 
                                           float* data, int num_cols, int num_rows, int buffer_size) = 0;

};

} // namespace recastx::recon


#endif // RECON_FILTERINTERFACE_H