#pragma once

#include <complex>
#include <condition_variable>
#include <cstdint>
#include <iostream>
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
#include "solver.hpp"


namespace tomop::slicerecon {

class Reconstructor {

    int rows_;
    int cols_;
    int pixels_;

    int num_darks_ = 1;
    int num_flats_ = 1;
    int preview_size_ = 0;
    int slice_size_ = 0;
    const int num_slices_ = 3;
    std::unordered_map<int, Orientation> slices_;
    std::mutex slice_mtx_;

    std::vector<RawDtype> all_darks_;
    std::vector<RawDtype> all_flats_;
    std::vector<float> dark_avg_;
    std::vector<float> reciprocal_;
    MemoryBuffer<float> buffer_;
    TripleBuffer<float> sino_buffer_;
    TripleBuffer<float> preview_buffer_;
    std::unordered_map<int, TripleBuffer<float>> slices_buffer_;
    bool initialized_ = false;

    size_t group_size_;
    size_t buffer_size_;

    int32_t received_darks_ = 0;
    int32_t received_flats_ = 0;
    bool reciprocal_computed_ = false;

    std::unique_ptr<Paganin> paganin_;
    std::unique_ptr<Filter> filter_;
    std::unique_ptr<Solver> solver_;

    std::thread processing_thread_;

    int gpu_buffer_index_ = 0;
    bool sino_uploaded_ = false;
    std::thread gpu_upload_thread_;
    std::thread gpu_recon_thread_;
    std::condition_variable gpu_cv_;
    std::mutex gpu_mutex_;

    int num_threads_;

    void processProjections(oneapi::tbb::task_arena& arena);

  public:

    Reconstructor(int rows, int cols, int num_threads); 

    ~Reconstructor();

    void initialize(int num_darks, 
                    int num_flats, 
                    int group_size,
                    int buffer_size,
                    int preview_size,
                    int slice_size);

    void initPaganin(float pixel_size, float lambda, float delta, float beta, float distance);

    void initFilter(const std::string& name, bool gaussian_pass);

    void setSolver(std::unique_ptr<Solver>&& solver);

    void startProcessing();

    void startReconstructing();

    void pushProjection(ProjectionType k, 
                        int32_t proj_idx, 
                        const std::array<int32_t, 2>& shape, 
                        const char* data); 

    void updateSlice(int slice_id, const Orientation& orientation);
    void removeSlice(int slice_id);

    tomop::VolumeDataPacket previewData();

    std::vector<tomop::SliceDataPacket> sliceData();

    size_t bufferSize() const;

    // for unittest

    const std::vector<RawDtype>& darks() const;
    const std::vector<RawDtype>& flats() const;
    const MemoryBuffer<float>& buffer() const;
    const TripleBuffer<float>& sinoBuffer() const;

};

} // tomop::slicerecon
