/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <spdlog/spdlog.h>

#include "recon/monitor.hpp"

namespace recastx::recon {

Monitor::Monitor(size_t tomo_byte_size, size_t report_projections_every) :
        start_(std::chrono::steady_clock::now()),
        report_projections_every_(report_projections_every),
        tomo_byte_size_(tomo_byte_size),
        perf_start_(start_) {
    perf_tomo_.push(start_);
}

void Monitor::reset() {
    start_ = std::chrono::steady_clock::now();
    num_darks_ = 0;
    num_flats_ = 0;
    num_projections_ = 0;

    resetPerf();
}

void Monitor::resetPerf() {
    num_tomograms_ = 0;

    perf_start_ = std::chrono::steady_clock::now();
    std::queue<std::chrono::time_point<std::chrono::steady_clock>>().swap(perf_tomo_);
    perf_tomo_.push(perf_start_);
}

void Monitor::countDark() {
    if (++num_darks_ % report_darks_every_ == 0) {
        spdlog::info("[Monitor] # of darks received: {}", num_darks_);
    }
}

void Monitor::countFlat() {
    if (++num_flats_ % report_flats_every_ == 0) {
        spdlog::info("[Monitor] # of flats received: {}", num_flats_);
    }
}

void Monitor::countProjection() {
    if (++num_projections_ % report_projections_every_ == 0) {
        spdlog::info("[Monitor] # of projections received: {}", num_projections_);
    }
}

void Monitor::countTomogram() {
    ++num_tomograms_;
    perf_tomo_.push(std::chrono::steady_clock::now());
    if (perf_tomo_.size() > report_tomo_throughput_every_) perf_tomo_.pop();

#if (VERBOSITY >= 1)
    size_t dt = std::chrono::duration_cast<std::chrono::microseconds>(
            perf_tomo_.back() - perf_tomo_.front()).count();
    double throughput = 1000000. * perf_tomo_.size() / static_cast<double>(dt);

    spdlog::info("[Monitor] {} tomograms reconstructed. Throughput: : {:.1f} (tomo/s)",
                 num_tomograms_, throughput);
#endif

}

void Monitor::summarize() const {
    size_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() -  start_).count();

    size_t dt = std::chrono::duration_cast<std::chrono::milliseconds>(
            perf_tomo_.back() -  perf_start_).count();
    double throughput_per_tomo = dt == 0 ? 0. : 1000. * static_cast<double>(num_tomograms_) / dt;
    double throughput = dt == 0 ? 0. : 0.001 * static_cast<double>(tomo_byte_size_) * num_tomograms_ / dt;

    spdlog::info("[Monitor] ------------------------------------------------------------");
    spdlog::info("[Monitor] Summarise of run:");
    spdlog::info("[Monitor] - Duration: {:.1f} s", static_cast<double>(duration) * 0.001);
    spdlog::info("[Monitor] - Number of darks processed: {}", num_darks_);
    spdlog::info("[Monitor] - Number of flats processed: {}", num_flats_);
    spdlog::info("[Monitor] - Number of projections processed: {}", num_projections_);
    spdlog::info("[Monitor] - Tomograms reconstructed: {}", num_tomograms_);
    spdlog::info("[Monitor] - Average throughput: {:.1f} (MB/s) / {:.1f} (tomo/s)", throughput, throughput_per_tomo);
    spdlog::info("[Monitor] ------------------------------------------------------------");
}

} // namespace recastx::recon