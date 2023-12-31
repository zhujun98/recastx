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

Monitor::Monitor(size_t image_byte_size, size_t report_projections_every) :
        start_(std::chrono::steady_clock::now()),
        report_projections_every_(report_projections_every),
        image_byte_size_(image_byte_size),
        perf_start_(start_),
        perf_end_(start_) {
    perf_tomo_.push_back(start_);
}

void Monitor::reset() {
    num_darks_ = 0;
    num_flats_ = 0;

    start_ = std::chrono::steady_clock::now();

    resetPerf();
}

void Monitor::resetPerf() {
    auto now = std::chrono::steady_clock::now();

    num_projections_ = 0;
    perf_start_ = now;
    perf_end_ = now;

    num_tomograms_ = 0;
    perf_tomo_.clear();
    perf_tomo_.push_back(now);
}

void Monitor::countDark() {
    if (num_projections_ != 0) reset();

    if (++num_darks_ % report_darks_every_ == 0) {
        spdlog::info("[Monitor] # of darks received: {}", num_darks_);
    }
}

void Monitor::countFlat() {
    if (num_projections_ != 0) reset();

    if (++num_flats_ % report_flats_every_ == 0) {
        spdlog::info("[Monitor] # of flats received: {}", num_flats_);
    }
    resetPerf();
}

void Monitor::countProjection() {
    if (++num_projections_ % report_projections_every_ == 0) {
        spdlog::info("[Monitor] # of projections received: {}", num_projections_);
    }
    perf_end_ = std::chrono::steady_clock::now();
}

void Monitor::countTomogram() {
    ++num_tomograms_;
    perf_tomo_.push_back(std::chrono::steady_clock::now());

#if (VERBOSITY >= 1)
    double throughput;
    const size_t window = 10;
    if (perf_tomo_.size() < window) {
        size_t dt = std::chrono::duration_cast<std::chrono::microseconds>(
                perf_tomo_.back() - perf_tomo_.front()).count();
        throughput = 1000000. * perf_tomo_.size() / dt;
    } else {
        size_t dt = std::chrono::duration_cast<std::chrono::microseconds>(
                perf_tomo_.back() - perf_tomo_[perf_tomo_.size() - window]).count();
        throughput = 1000000. * window / dt;
    }
    spdlog::info("[Monitor] {} tomograms reconstructed. Throughput: : {:.1f} (tomo/s)",
                 num_tomograms_, throughput);
#endif

}

void Monitor::summarize() const {
    size_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() -  start_).count();

    size_t dt = std::chrono::duration_cast<std::chrono::microseconds>(perf_end_ - perf_start_).count();
    float throughput = dt == 0 ? 0. : image_byte_size_ * num_projections_ / dt;
    float throughput_per_tomo = dt == 0 ? 0. : 1000000. * num_tomograms_ / dt;

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