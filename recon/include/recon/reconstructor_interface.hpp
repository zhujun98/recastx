/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_RECONSTRUCTORINTERFACE_H
#define RECON_RECONSTRUCTORINTERFACE_H

#include <memory>

#include "common/config.hpp"
#include "tensor.hpp"

namespace recastx::recon {

class Reconstructor {

public:

    virtual ~Reconstructor() = default;

    virtual void reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) = 0;

    virtual void reconstructVolume(int buffer_idx, Tensor<float, 3>& buffer) = 0;

    virtual void uploadSinograms(int buffer_idx, const float* data, size_t count) = 0;
};

class ReconstructorFactory {

  public:

    virtual ~ReconstructorFactory() = default;

    virtual std::unique_ptr<Reconstructor> create(ProjectionGeometry proj_geom,
                                                  VolumeGeometry slice_geom, 
                                                  VolumeGeometry volume_geom,
                                                  bool double_buffering) = 0;

};

} // namespace recastx::recon

#endif // RECON_RECONSTRUCTORINTERFACE_H