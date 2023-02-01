#ifndef SLICERECON_APPLICATION_H
#define SLICERECON_APPLICATION_H

#include <complex>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <thread>
#include <unordered_set>
#include <vector>

#include <spdlog/spdlog.h>
#include <oneapi/tbb.h>

extern "C" {
#include <fftw3.h>
}

#include "buffer.hpp"
#include "phase.hpp"
#include "filter.hpp"
#include "reconstructor.hpp"
#include "tomcat/tomcat.hpp"


namespace tomcat::recon {

class DaqClient;
class ZmqServer;

class Reconstructor;

class SliceBufferBridge {

public:

    using DataType = typename SliceBuffer<float>::DataType;
    using ValueType = typename SliceBuffer<float>::ValueType;

protected:

    std::map<size_t, std::pair<size_t, Orientation>> params_;
    SliceBuffer<float> slices_;
    SliceBuffer<float> requested_slices_;
    std::unordered_set<size_t> updated_;

    std::mutex mtx_;

public:

    SliceBufferBridge() : slices_(NUM_SLICES), requested_slices_(NUM_SLICES, false) {}

    ~SliceBufferBridge() = default;

    void resize(const std::array<size_t, 2>& shape) {
        slices_.resize(shape);
        requested_slices_.resize(shape);
    }

    void insert(size_t timestamp, const Orientation& orientation) {
        std::lock_guard<std::mutex> lck(mtx_);
        size_t sid = timestamp % NUM_SLICES;
        params_[sid] = std::make_pair(timestamp, orientation);
        updated_.insert(sid);
    }

    void reconAll(Reconstructor* recon, int gpu_buffer_index) {
        {
            std::lock_guard<std::mutex> lck(mtx_);

            for (const auto& [sid, param] : params_) {
                auto& slice = slices_.back().second[sid];
                recon->reconstructSlice(param.second, gpu_buffer_index, slice.second);
                slice.first = param.first;
            }

            updated_.clear();
        }

        slices_.prepare();
    }

    void reconRequested(Reconstructor* recon, int gpu_buffer_index) {
        if (!updated_.empty()) {
            {
                std::lock_guard<std::mutex> lck(mtx_);

                for (auto sid : updated_) {
                    auto slice = requested_slices_.back().second[sid];
                    auto param = params_[sid];
                    recon->reconstructSlice(param.second, gpu_buffer_index, slice.second);
                    slice.first = param.first;
                    requested_slices_.back().first.emplace(sid);
                }

                updated_.clear();
            }

            requested_slices_.prepare();
        }
    }

    SliceBuffer<float>& slices() { return slices_; }

    SliceBuffer<float>& requestedSlices() { return requested_slices_; }

};

class Application {

    size_t num_cols_;
    size_t num_rows_;
    size_t num_pixels_;
    size_t num_angles_;

    ProjectionGeometry proj_geom_;
    VolumeGeometry vol_geom_;

    MemoryBuffer<float, 3> raw_buffer_;

    size_t num_darks_ = 1;
    size_t num_flats_ = 1;
    std::vector<RawDtype> all_darks_;
    std::vector<RawDtype> all_flats_;
    std::vector<float> dark_avg_;
    std::vector<float> reciprocal_;
    size_t received_darks_ = 0;
    size_t received_flats_ = 0;
    bool reciprocal_computed_ = false;

    TripleVectorBuffer<float, 3> sino_buffer_;

    SliceBufferBridge slice_buffer_;

    TripleVectorBuffer<float, 3> preview_buffer_;

    bool initialized_ = false;

    std::unique_ptr<Paganin> paganin_;
    std::unique_ptr<Filter> filter_;
    std::unique_ptr<Reconstructor> recon_;

    std::thread preproc_thread_;

    int gpu_buffer_index_ = 0;
    bool sino_uploaded_ = false;
    std::thread upload_thread_;
    std::thread recon_thread_;
    std::condition_variable gpu_cv_;
    std::mutex gpu_mutex_;

    int num_threads_;

    std::unique_ptr<DaqClient> daq_client_;
    std::unique_ptr<ZmqServer> zmq_server_;

    void processProjections(oneapi::tbb::task_arena& arena);

public:

    Application(size_t raw_buffer_size, int num_threads); 

    ~Application();

    void init(size_t num_cols, size_t num_rows, size_t num_angles, size_t num_darks, size_t num_flats);

    void initPaganin(const PaganinConfig& config, int num_cols, int num_rows);

    void initFilter(const FilterConfig& config, int num_cols, int num_rows);

    void initReconstructor(bool cone_beam,
                           const ProjectionGeometry& proj_geom,
                           const VolumeGeometry& slice_geom,
                           const VolumeGeometry& preview_geom);

    void initConnection(const DaqClientConfig& client_config, const ZmqServerConfig& server_config);

    void startPreprocessing();

    void startUploading();

    void startReconstructing();

    void runForEver();

    void pushProjection(ProjectionType k, 
                        size_t proj_idx, 
                        const std::array<size_t, 2>& shape, 
                        const char* data); 

    void setSlice(size_t timestamp, const Orientation& orientation);

    std::optional<VolumeDataPacket> previewDataPacket();

    std::vector<SliceDataPacket> sliceDataPackets();

    std::vector<SliceDataPacket> requestedSliceDataPackets();

    size_t bufferSize() const;

    // for unittest

    const std::vector<RawDtype>& darks() const { return all_darks_; }
    const std::vector<RawDtype>& flats() const { return all_flats_; }
    const MemoryBuffer<float, 3>& rawBuffer() const { return raw_buffer_; }
    const TripleVectorBuffer<float, 3>& sinoBuffer() const { return sino_buffer_; }
};

} // tomcat::recon

#endif // SLICERECON_APPLICATION_H