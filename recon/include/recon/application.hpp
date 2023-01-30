#ifndef SLICERECON_APPLICATION_H
#define SLICERECON_APPLICATION_H

#include <complex>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <thread>
#include <unordered_map>
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

class Application {

    int num_cols_;
    int num_rows_;
    int num_pixels_;
    int num_angles_;
    int preview_size_;
    int slice_size_;

    ProjectionGeometry proj_geom_;
    VolumeGeometry vol_geom_;

    int num_darks_ = 1;
    int num_flats_ = 1;
    std::unordered_map<int, Orientation> slices_;
    std::set<int> updated_slices_;
    std::set<int> requested_slices_;
    std::condition_variable slice_cv_;
    std::mutex slice_mtx_;

    std::vector<RawDtype> all_darks_;
    std::vector<RawDtype> all_flats_;
    std::vector<float> dark_avg_;
    std::vector<float> reciprocal_;
    MemoryBuffer<float> buffer_;
    TripleBuffer<float> sino_buffer_;
    TripleBuffer<float> preview_buffer_;
    std::unordered_map<int, std::vector<float>> slices_buffer_;
    bool initialized_ = false;

    int32_t received_darks_ = 0;
    int32_t received_flats_ = 0;
    bool reciprocal_computed_ = false;

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

    explicit Application(int num_threads); 

    ~Application();

    void init(int num_cols, int num_rows, int num_angles,
              int num_darks, int num_flats, 
              int slice_size, int preview_size, 
              int buffer_size);

    void initPaganin(float pixel_size, float lambda, float delta, float beta, float distance, int num_cols, int num_rows);

    void initFilter(const std::string& name, int num_cols, int num_rows, bool gaussian_lowpass_filter);

    void initReconstructor(
        bool cone_beam, int num_cols, int num_rows, int num_angles, 
        std::array<float, 2> pixel_size, float source2origin, float origin2det,
        int slice_size, int preview_size, 
        std::array<float, 2> xrange, std::array<float, 2> yrange, std::array<float, 2> zrange);

    void setReconstructor(std::unique_ptr<Reconstructor>&& recon);

    void initConnection(const DaqClientConfig& client_config, const ZmqServerConfig& server_config);

    void startPreprocessing();

    void startUploading();

    void startReconstructing();

    void runForEver();

    void pushProjection(ProjectionType k, 
                        int32_t proj_idx, 
                        const std::array<int32_t, 2>& shape, 
                        const char* data); 

    void setSlice(int slice_id, const Orientation& orientation);
    
    void removeSlice(int slice_id);
 
    void removeAllSlices();

    std::optional<VolumeDataPacket> previewDataPacket(int timeout=-1);

    std::vector<SliceDataPacket> sliceDataPackets();

    std::optional<std::vector<SliceDataPacket>> requestedSliceDataPackets(int timeout=-1);

    size_t bufferSize() const;

    // for unittest

    const std::vector<RawDtype>& darks() const;
    const std::vector<RawDtype>& flats() const;
    const MemoryBuffer<float>& buffer() const;
    const TripleBuffer<float>& sinoBuffer() const;

};

} // tomcat::recon

#endif // SLICERECON_APPLICATION_H