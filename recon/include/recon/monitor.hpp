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

#include <spdlog/spdlog.h>

namespace recastx::recon {

class Monitor {

    std::chrono::time_point<std::chrono::steady_clock> start_;

    size_t num_darks_ = 0;
    size_t num_flats_ = 0;
    size_t num_projections_= 0;
    size_t num_tomograms_ = 0;

    size_t scan_byte_size_;

    std::chrono::time_point<std::chrono::steady_clock> tomo_start_;
    size_t report_tomo_throughput_every_ = 10;

  public:

    explicit Monitor(size_t scan_byte_size = 0);

    void reset();

    void resetTimer();

    void addDark() { ++num_darks_; }

    void addFlat() { ++num_flats_; }

    void addProjection() { ++num_projections_; }

    void addTomogram();

    void summarize() const;
};

} // namespace recastx::recon

#endif // RECON_MONITOR_H