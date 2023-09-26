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

Monitor::Monitor(size_t scan_byte_size, 
                 size_t report_tomo_throughput_every) : 
    scan_byte_size_(scan_byte_size),
    start_(std::chrono::steady_clock::now()),
    tomo_start_(start_),
    report_tomo_throughput_every_(report_tomo_throughput_every) {
}

void Monitor::reset() {
    num_darks_ = 0;
    num_flats_ = 0;
    num_projections_ = 0;
    num_tomograms_ = 0;

    resetTimer();
}

void Monitor::resetTimer() {
    start_ = std::chrono::steady_clock::now();
    tomo_start_ = start_;
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

void Monitor::countTomogram() {
    ++num_tomograms_;

#if (VERBOSITY >= 1)
    spdlog::info("{} tomograms reconstructed", num_tomograms_);
#endif

#if (VERBOSITY >= 2)
    if (num_tomograms_ % report_tomo_throughput_every_ == 0) {
        // The number for the first <report_tomo_throughput_every_> tomograms 
        // underestimates the throughput! 
        auto end = std::chrono::steady_clock::now();
        size_t dt = std::chrono::duration_cast<std::chrono::microseconds>(
            end -  tomo_start_).count();
        double throughput = scan_byte_size_ * report_tomo_throughput_every_ / dt;
        double throughput_per_tomo = 1000000. * report_tomo_throughput_every_ / dt;
        spdlog::info("Throughput (averaged over the last {} tomograms): {:.1f} (MB/s) / {:.1f} (tomo/s)", 
                     report_tomo_throughput_every_, throughput, throughput_per_tomo);
        tomo_start_ = end;
    }
#endif    
}

void Monitor::summarize() const {
    size_t dt = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() -  start_).count();
    float throughput = scan_byte_size_ * num_tomograms_ / dt;

    spdlog::info("Summarise of run:");
    spdlog::info("- Number of darks processed: {}", num_darks_);
    spdlog::info("- Number of flats processed: {}", num_flats_);
    spdlog::info("- Number of projections processed: {}", num_projections_);
    spdlog::info("- Tomograms reconstructed: {}, average throughput: {:.1f} (MB/s)", 
                    num_tomograms_, throughput);
}

} // namespace recastx::recon