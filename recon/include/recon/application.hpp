/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_APPLICATION_H
#define RECON_APPLICATION_H

#include <complex>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>
#include <oneapi/tbb.h>

extern "C" {
#include <fftw3.h>
}

#include "common/config.hpp"
#include "buffer.hpp"
#include "daq_client.hpp"
#include "slice_mediator.hpp"
#include "tensor.hpp"

#include "imageproc.pb.h"
#include "reconstruction.pb.h"
#include "control.pb.h"


namespace recastx::recon {

class Filter;
class Paganin;
class Reconstructor;
class DaqClient;
class RpcServer;

namespace details {

inline std::pair<float, float> parseReconstructedVolumeBoundary(
        const std::optional<float>& min_val, const std::optional<float>& max_val, int size) {
    float min_v = min_val.value_or(- size / 2.f);
    float max_v = max_val.value_or(size / 2.f);
    if (min_v >= max_v) throw std::invalid_argument(
        "Minimum of volume coordinate must be smaller than maximum of volume coordinate");
    return {min_v, max_v};
}

} // details

class Application {

    // Why did I choose ProDtype over RawDtype?
    MemoryBuffer<ProDtype, 3> raw_buffer_;

    RawImageGroup darks_;
    RawImageGroup flats_;
    ProImageData dark_avg_;
    ProImageData reciprocal_;
    bool reciprocal_computed_ = false;

    TripleTensorBuffer<ProDtype, 3> sino_buffer_;

    SliceMediator slice_mediator_;

    TripleTensorBuffer<ProDtype, 3> preview_buffer_;

    std::optional<PaganinConfig> paganin_cfg_;
    std::unique_ptr<Paganin> paganin_;
    
    FilterConfig filter_cfg_;
    std::unique_ptr<Filter> filter_;

    ImageprocParams imgproc_params_;
    FlatFieldCorrectionParams flatfield_params_;

    std::optional<size_t> slice_size_;
    std::optional<size_t> preview_size_;
    std::optional<float> min_x_;
    std::optional<float> max_x_;
    std::optional<float> min_y_;
    std::optional<float> max_y_;
    std::optional<float> min_z_;
    std::optional<float> max_z_;
    ProjectionGeometry proj_geom_;
    VolumeGeometry slice_geom_;
    VolumeGeometry preview_geom_;
    std::unique_ptr<Reconstructor> recon_;

    int gpu_buffer_index_ = 0;
    bool sino_uploaded_ = false;
    std::condition_variable gpu_cv_;
    std::mutex gpu_mtx_;

    ServerState_State server_state_;
    ScanMode_Mode scan_mode_;
    uint32_t scan_update_interval_;

    // It's not a State because there might be race conditions.
    bool running_ = true;

    std::unique_ptr<DaqClient> daq_client_;
    std::unique_ptr<RpcServer> rpc_server_;

    void initPaganin(size_t col_count, size_t row_count);

    void initFilter(size_t col_count, size_t row_count);

    void initReconstructor(size_t col_count, size_t row_count);

    void maybeInitFlatFieldBuffer(size_t row_count, size_t col_count);

    void maybeInitReconBuffer(size_t col_count, size_t row_count);

    void maybeResetDarkAndFlatAcquisition() {
        if (reciprocal_computed_) {
            raw_buffer_.reset();
            darks_.reset();
            flats_.reset();
            reciprocal_computed_ = false;
        }
    }

    void pushDark(const Projection& proj);
    void pushFlat(const Projection& proj);
    void pushProjection(const Projection& proj);

    void processProjections(oneapi::tbb::task_arena& arena);

    bool waitForAcquiring() const {
        if (server_state_ != ServerState_State::ServerState_State_ACQUIRING 
                && server_state_ != ServerState_State_PROCESSING) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return true;
        }
        return false;
    }

    bool waitForProcessing() const {
        if (server_state_ != ServerState_State::ServerState_State_PROCESSING) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return true;
        }
        return false;
    };

public:

    Application(size_t raw_buffer_size,
                const ImageprocParams& imageproc_params, 
                const DaqClientConfig& daq_config, 
                const RpcServerConfig& rpc_config); 

    ~Application();

    void init();

    template<typename ...Ts>
    void setFlatFieldCorrectionParams(Ts... args) {
        flatfield_params_ = {std::forward<Ts>(args)...};
    }

    template<typename ...Ts>
    void setPaganinParams(Ts... args) { paganin_cfg_ = {std::forward<Ts>(args)...}; }

    template<typename ...Ts>
    void setFilterParams(Ts... args) { filter_cfg_ = {std::forward<Ts>(args)...}; }

    void setProjectionGeometry(BeamShape beam_shape, size_t col_count, size_t row_count,
                               float pixel_width, float pixel_height, 
                               float src2origin, float origin2det, size_t num_angles);

    void setReconGeometry(std::optional<size_t> slice_size, std::optional<size_t> preview_size, 
                          std::optional<float> min_x, std::optional<float> max_x, 
                          std::optional<float> min_y, std::optional<float> max_y, 
                          std::optional<float> min_z, std::optional<float> max_z);

    void startAcquiring();
    void stopAcquiring();

    void startPreprocessing();

    void startUploading();

    void startReconstructing();

    void runForEver();

    void pushProjection(
        ProjectionType k, size_t proj_idx, size_t num_rows, size_t num_cols, const char* data); 

    void setDownsamplingParams(uint32_t col, uint32_t row);

    void setSlice(size_t timestamp, const Orientation& orientation);

    void setScanMode(ScanMode_Mode mode, uint32_t update_inverval);

    void onStateChanged(ServerState_State state);

    std::optional<ReconData> previewData(int timeout);

    std::vector<ReconData> sliceData(int timeout);

    std::vector<ReconData> onDemandSliceData(int timeout);

    size_t numAngles() const { return proj_geom_.angles.size(); };

    // for unittest

    const RawImageGroup& darks() const { return darks_; }
    const RawImageGroup& flats() const { return flats_; }
    const MemoryBuffer<float, 3>& rawBuffer() const { return raw_buffer_; }
    const TripleTensorBuffer<float, 3>& sinoBuffer() const { return sino_buffer_; }
};

} // namespace recastx::recon

#endif // RECON_APPLICATION_H