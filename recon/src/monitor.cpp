/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <spdlog/spdlog.h>

#include "recon/monitor.hpp"

namespace recastx::recon {

Monitor::Monitor(size_t image_byte_size,
                 size_t report_tomo_throughput_every) : 
    image_byte_size_(image_byte_size),
    start_(std::chrono::steady_clock::now()),
    tomo_start_(start_),
    end_(start_),
    report_tomo_throughput_every_(report_tomo_throughput_every) {
}

void Monitor::reset() {
    num_darks_ = 0;
    num_flats_ = 0;
    num_projections_ = 0;

    resetTimer();
}

void Monitor::resetTimer() {
    start_ = std::chrono::steady_clock::now();
    tomo_start_ = start_;
    end_ = start_;
}

void Monitor::countDark() {
    if (++num_darks_ % report_darks_every_ == 0) {
        spdlog::info("# of darks received: {}", num_darks_); 
    }
}

void Monitor::countFlat() {
    if (++num_flats_ % report_flats_every_ == 0) {
        spdlog::info("# of flats received: {}", num_flats_); 
    }
}

void Monitor::countProjection() {
    ++num_projections_;
    end_ = std::chrono::steady_clock::now();
}

void Monitor::countTomogram() {
    ++num_tomograms_;

#if (VERBOSITY >= 1)
    spdlog::info("{} tomograms reconstructed", num_tomograms_);
#endif

#if (VERBOSITY >= 2)
    if (num_tomograms_ % report_tomo_throughput_every_ == 0) {
        auto end = std::chrono::steady_clock::now();
        // The number for the first <report_tomo_throughput_every_> tomograms 
        // underestimates the throughput! 
        size_t dt = std::chrono::duration_cast<std::chrono::microseconds>(end -  tomo_start_).count();
        double throughput_per_tomo = dt == 0 ? 0. : 1000000. * report_tomo_throughput_every_ / dt;
        spdlog::info("Throughput (averaged over the last {} tomograms): {:.1f} (tomo/s)",
                     report_tomo_throughput_every_, throughput_per_tomo);
        tomo_start_ = end_;
    }
#endif    
}

void Monitor::summarize() const {
    size_t dt = std::chrono::duration_cast<std::chrono::microseconds>(end_ -  start_).count();
    float throughput = dt == 0 ? 0. : image_byte_size_ * num_projections_ / dt;
    float throughput_per_tomo = dt == 0 ? 0. : 1000000. * num_tomograms_ / dt;

    spdlog::info("--------------------------------------------------------------------------------");
    spdlog::info("Summarise of run:");
    spdlog::info("- Duration: {:.1f} s", static_cast<double>(dt) * 0.000001);
    spdlog::info("- Number of darks processed: {}", num_darks_);
    spdlog::info("- Number of flats processed: {}", num_flats_);
    spdlog::info("- Number of projections processed: {}", num_projections_);
    spdlog::info("- Tomograms reconstructed: {}", num_tomograms_);
    spdlog::info("- Average throughput: {:.1f} (MB/s) / {:.1f} (tomo/s)", throughput, throughput_per_tomo);
    spdlog::info("--------------------------------------------------------------------------------");
}

} // namespace recastx::recon