/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_DAQCLIENTINTERFACE_H
#define RECON_DAQCLIENTINTERFACE_H

#include <queue>

#include "recon/projection.hpp"

namespace recastx::recon {

class DaqClientInterface {

  protected:

    size_t concurrency_;

    bool acquiring_ = false;
    uint32_t num_rows_ {0};
    uint32_t num_cols_ {0};

    static ProjectionType parseProjectionType(int v) {
        if (v != static_cast<int>(ProjectionType::DARK) &&
            v != static_cast<int>(ProjectionType::FLAT) && 
            v != static_cast<int>(ProjectionType::PROJECTION)) {
                return ProjectionType::UNKNOWN;
            }
        return static_cast<ProjectionType>(v);
    }

    bool validate(size_t num_rows, size_t num_cols) {
        if (num_rows_ == 0 || num_cols_ == 0) return true;

        if (num_rows != num_rows_ || num_cols != num_cols_) {
            spdlog::warn("[DAQ client] Received image data with a different shape! Current: {} x {} / "
                         "Expected: {} x {}", num_rows, num_cols, num_rows_, num_cols_);
            return false;
        }
        return true;
    }

  public:

    explicit DaqClientInterface(size_t concurrency) : concurrency_(concurrency) {}

    virtual ~DaqClientInterface() = default;

    virtual void spin() = 0;

    virtual void setAcquiring(bool state) {
        acquiring_ = state;
        num_rows_ = 0;
        num_cols_ = 0;
    }

    virtual void startAcquiring(uint32_t num_rows, uint32_t num_cols) {
        acquiring_ = true;
        num_rows_ = num_rows;
        num_cols_ = num_cols;
    }

    [[nodiscard]] virtual bool next(Projection<>& proj) = 0;

    [[nodiscard]] size_t concurrency() const { return concurrency_; }
};


} // namespace recastx::recon

#endif // RECON_DAQCLIENTINTERFACE_H