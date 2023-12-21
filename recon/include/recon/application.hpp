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
#include "tensor.hpp"

#include "control.pb.h"
#include "imageproc.pb.h"
#include "projection.pb.h"
#include "reconstruction.pb.h"

#include "filter_interface.hpp"
#include "reconstructor_interface.hpp"
#include "projection.hpp"
#include "daq/daq_client_interface.hpp"

namespace recastx::recon {

class Monitor;
class Paganin;
class ProjectionMediator;
class RpcServer;
class SliceMediator;

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

  public:

    static uint32_t defaultImageprocConcurrency() {
        uint32_t n = std::thread::hardware_concurrency();
        return n > 2 ? n / 2 : 1;
    }

    static uint32_t defaultDaqConcurrency() {
        return 2;
    }

  private:

    size_t group_size_;
    // Why did I choose ProDtype over RawDtype?
    MemoryBuffer<ProDtype, 3> raw_buffer_;

    std::vector<RawImageData> darks_;
    std::vector<RawImageData> flats_;
    ProImageData dark_avg_;
    ProImageData reciprocal_;
    bool reciprocal_computed_ = false;

    TripleTensorBuffer<ProDtype, 3> sino_buffer_;

    std::unique_ptr<ProjectionMediator> proj_mediator_;
    std::unique_ptr<SliceMediator> slice_mediator_;

    TripleTensorBuffer<ProDtype, 3> volume_buffer_;

    std::optional<PaganinConfig> paganin_cfg_;
    std::unique_ptr<Paganin> paganin_;
    
    FilterFactory* ramp_filter_factory_;
    std::unique_ptr<Filter> ramp_filter_;

    ImageprocParams imgproc_params_;

    std::optional<size_t> slice_size_;
    std::optional<size_t> volume_size_;
    std::optional<float> min_x_;
    std::optional<float> max_x_;
    std::optional<float> min_y_;
    std::optional<float> max_y_;
    std::optional<float> min_z_;
    std::optional<float> max_z_;
    ProjectionGeometry proj_geom_;
    VolumeGeometry slice_geom_;
    VolumeGeometry volume_geom_;
    bool volume_required_ = true;
    ReconstructorFactory* recon_factory_;
    std::unique_ptr<Reconstructor> recon_;

    int gpu_buffer_index_ = 0;
    bool sino_initialized_ = false;
    bool sino_uploaded_ = false;
    std::condition_variable gpu_cv_;
    std::mutex gpu_mtx_;

    rpc::ServerState_State server_state_ = rpc::ServerState_State_UNKNOWN;
    rpc::ScanMode_Mode scan_mode_;
    uint32_t scan_update_interval_;

    // It's not a State because there might be race conditions.
    bool running_ = true;
    std::unique_ptr<Monitor> monitor_;

    DaqClientInterface* daq_client_;
    std::unique_ptr<RpcServer> rpc_server_;

    void init();

    void initParams();

    void initPaganin(size_t col_count, size_t row_count);

    void initFilter(size_t col_count, size_t row_count);

    void initReconstructor(size_t col_count, size_t row_count);

    void maybeInitFlatFieldBuffer(size_t row_count, size_t col_count);

    void maybeInitReconBuffer(size_t col_count, size_t row_count);

    void maybeResetDarkAndFlatAcquisition();

    void pushProjection(const Projection<>& proj);

    void tryComputeReciprocal();

    void preprocessProjections(oneapi::tbb::task_arena& arena);

    void onStartProcessing();
    void onStopProcessing();
    void onStartAcquiring();
    void onStopAcquiring();

    bool waitForProcessing() const {
        if (server_state_ != rpc::ServerState_State_PROCESSING) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return true;
        }
        return false;
    };

    bool waitForSinoInitialization() const {
        if (!sino_initialized_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return true;
        }
        return false;
    }

  public:

    Application(size_t raw_buffer_size,
                const ImageprocParams& imageproc_params,
                DaqClientInterface* daq_client,
                FilterFactory* ramp_filter_factory,
                ReconstructorFactory* recon_factory,
                const RpcServerConfig& rpc_config); 

    ~Application();

    template<typename ...Ts>
    void setPaganinParams(Ts... args) { paganin_cfg_ = {std::forward<Ts>(args)...}; }

    void setProjectionGeometry(BeamShape beam_shape, size_t col_count, size_t row_count,
                               float pixel_width, float pixel_height, 
                               float src2origin, float origin2det, size_t num_angles);

    void setReconGeometry(std::optional<size_t> slice_size, std::optional<size_t> volume_size,
                          std::optional<float> min_x, std::optional<float> max_x, 
                          std::optional<float> min_y, std::optional<float> max_y, 
                          std::optional<float> min_z, std::optional<float> max_z);

    bool consume();

    void startPreprocessing();

    void startUploading();

    void startReconstructing();

    void spin(rpc::ServerState_State state);

    void setDownsampling(uint32_t col, uint32_t row);

    void setRampFilter(std::string filter_name);

    void setProjectionReq(size_t id);

    void setSliceReq(size_t timestamp, const Orientation& orientation);

    void setVolumeReq(bool required);

    void setScanMode(rpc::ScanMode_Mode mode, uint32_t update_inverval);

    void onStateChanged(rpc::ServerState_State state);

    std::optional<rpc::ProjectionData> getProjectionData(int timeout);

    bool hasVolume() const { return volume_required_; }

    std::vector<rpc::ReconData> getVolumeData(int timeout);

    std::vector<rpc::ReconData> getSliceData(int timeout);

    std::vector<rpc::ReconData> getOnDemandSliceData(int timeout);

    size_t numAngles() const { return proj_geom_.angles.size(); };

    // for unittest

    const std::vector<RawImageData>& darks() const { return darks_; }
    const std::vector<RawImageData>& flats() const { return flats_; }
    const MemoryBuffer<float, 3>& rawBuffer() const { return raw_buffer_; }
    const TripleTensorBuffer<float, 3>& sinoBuffer() const { return sino_buffer_; }
    const Reconstructor* reconstructor() const { return recon_.get(); }
};

} // namespace recastx::recon

#endif // RECON_APPLICATION_H