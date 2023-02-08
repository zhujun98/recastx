#ifndef SLICERECON_APPLICATION_H
#define SLICERECON_APPLICATION_H

#include <complex>
#include <condition_variable>
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

#include "buffer.hpp"
#include "slice_mediator.hpp"
#include "tomcat/tomcat.hpp"


namespace tomcat::recon {

class DaqClient;
class ZmqServer;

class Filter;
class Paganin;
class Reconstructor;


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

    SliceMediator slice_mediator_;

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

    std::optional<VolumeDataPacket> previewDataPacket(int timeout);

    std::vector<SliceDataPacket> sliceDataPackets(int timeout);

    std::vector<SliceDataPacket> onDemandSliceDataPackets(int timeout);

    size_t bufferSize() const;

    // for unittest

    const std::vector<RawDtype>& darks() const { return all_darks_; }
    const std::vector<RawDtype>& flats() const { return all_flats_; }
    const MemoryBuffer<float, 3>& rawBuffer() const { return raw_buffer_; }
    const TripleVectorBuffer<float, 3>& sinoBuffer() const { return sino_buffer_; }
};

} // tomcat::recon

#endif // SLICERECON_APPLICATION_H