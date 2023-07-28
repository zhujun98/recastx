/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_MONITOR_H
#define RECON_MONITOR_H

#include <chrono>
#include <optional>
#include <queue>

#include <spdlog/spdlog.h>

#include "daq_client_interface.hpp"

namespace recastx::recon {

class Projection;

class Monitor {

    size_t num_darks_ = 0;
    size_t num_flats_ = 0;
    size_t num_projections_= 0;
    size_t num_tomograms_ = 0;

    size_t scan_byte_size_;

    size_t monitor_projection_every_;
    std::queue<Projection> projections_;

    std::chrono::time_point<std::chrono::steady_clock> start_;
    std::chrono::time_point<std::chrono::steady_clock> tomo_start_;
    size_t report_tomo_throughput_every_;

  public:

    Monitor(size_t scan_byte_size = 0, 
            size_t monitor_projection_every = 100,
            size_t report_tomo_throughput_every = 10);

    void reset();

    void resetTimer();

    void countDark() { ++num_darks_; }

    void countFlat() { ++num_flats_; }

    void countProjection(Projection msg);

    std::optional<Projection> popProjection();

    size_t numProjections() const { return num_projections_; }

    void addTomogram();

    void summarize() const;
};

} // namespace recastx::recon

#endif // RECON_MONITOR_H