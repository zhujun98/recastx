/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_MONITOR_H
#define RECON_MONITOR_H

#include <atomic>
#include <queue>
#include <chrono>

namespace recastx::recon {

class Monitor {

    std::chrono::time_point<std::chrono::steady_clock> start_;

    size_t num_darks_ = 0;
    const size_t report_darks_every_ = 10;

    size_t num_flats_ = 0;
    const size_t report_flats_every_ = 10;

    std::atomic<size_t> num_projections_= 0;
    const size_t report_projections_every_;
    size_t tomo_byte_size_;

    size_t num_tomograms_ = 0;
    const size_t report_tomo_throughput_every_ = 10;
    std::queue<std::chrono::time_point<std::chrono::steady_clock>> perf_tomo_;

  public:

    explicit Monitor(size_t tomo_byte_size = 0, size_t report_projections_every_ = 100);

    void reset();

    void resetPerf();

    void countDark();
    
    void countFlat();

    void countProjection();

    void countTomogram();

    void summarize() const;
  
    [[nodiscard]] size_t numDarks() const { return num_darks_; }
    [[nodiscard]] size_t numFlats() const { return num_flats_; }
    [[nodiscard]] size_t numProjections() const { return num_projections_; }
    [[nodiscard]] size_t numTomograms() const { return num_tomograms_; }
};

} // namespace recastx::recon

#endif // RECON_MONITOR_H