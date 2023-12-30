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

#include <atomic>
#include <chrono>

namespace recastx::recon {

class Monitor {

    size_t num_darks_ = 0;
    size_t num_flats_ = 0;
    std::atomic<size_t> num_projections_= 0;
    size_t num_tomograms_ = 0;

    size_t image_byte_size_;

    std::chrono::time_point<std::chrono::steady_clock> start_;
    std::chrono::time_point<std::chrono::steady_clock> tomo_start_;
    std::chrono::time_point<std::chrono::steady_clock> end_;

    size_t report_tomo_throughput_every_;
    const size_t report_darks_every_ = 10;
    const size_t report_flats_every_ = 10;

  public:

    Monitor(size_t image_byte_size = 0,
            size_t report_tomo_throughput_every = 10);

    void reset();

    void resetTimer();

    void countDark();
    
    void countFlat();

    void countProjection();

    void countTomogram();

    void summarize() const;
  
    size_t numDarks() const { return num_darks_; }
    size_t numFlats() const { return num_flats_; }
    size_t numProjections() const { return num_projections_; }
};

} // namespace recastx::recon

#endif // RECON_MONITOR_H