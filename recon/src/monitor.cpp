/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "recon/monitor.hpp"

namespace recastx::recon {

Monitor::Monitor(size_t scan_byte_size) : 
    start_(std::chrono::steady_clock::now()),
    scan_byte_size_(scan_byte_size),
    tomo_start_(start_) {
}

void Monitor::reset() {
    num_darks_ = 0;
    num_flats_ = 0;
    num_projections_ = 0;
    num_tomograms_ = 0;

    std::queue<Projection>().swap(projections_);

    resetTimer();
}

void Monitor::resetTimer() {
    start_ = std::chrono::steady_clock::now();
    tomo_start_ = start_;
}

void Monitor::countProjection(Projection msg) {
    ++num_projections_;
    if (num_projections_ % monitor_projection_every_ == 0) {
        projections_.emplace(std::move(msg));
    }
}

std::optional<Projection> Monitor::popProjection() {
    if (projections_.empty()) return std::nullopt;
    auto proj = std::move(projections_.front());
    projections_.pop();
    return proj;
}

void Monitor::addTomogram() {
    ++num_tomograms_;

    if (num_tomograms_ % report_tomo_throughput_every_ == 0) {
        // The number for the first <report_tomo_throughput_every_> tomograms 
        // underestimates the throughput! 
        auto end = std::chrono::steady_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::microseconds>(
            end -  tomo_start_).count();
        float throughput = scan_byte_size_ * report_tomo_throughput_every_ / dt;
        spdlog::info("[Bench] Reconstruction throughput: {:.1f} (MB/s)", throughput);
        tomo_start_ = end;
    }
}

void Monitor::summarize() const {
    float dt = std::chrono::duration_cast<std::chrono::microseconds>(
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