#pragma once

#include <complex>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>
#include <oneapi/tbb.h>

extern "C" {
#include <fftw3.h>
}

#include "buffer.hpp"
#include "data_types.hpp"
#include "phase.hpp"
#include "filter.hpp"
#include "solver.hpp"


namespace slicerecon {

class Reconstructor {

    int rows_;
    int cols_;
    int pixels_;

    std::vector<RawDtype> all_darks_;
    std::vector<RawDtype> all_flats_;
    std::vector<float> dark_avg_;
    std::vector<float> reciprocal_;
    Buffer2<float> buffer_;
    SimpleBuffer3<float> sino_buffer_;
    SimpleBuffer3<float> preview_buffer_;
    bool initialized_ = false;

    int buffer_size_;

    int num_darks_ = 1;
    int num_flats_ = 1;
    int num_projections_ = 1;
    int preview_size_ = 1; 

    int32_t received_darks_ = 0;
    int32_t received_flats_ = 0;
    bool reciprocal_computed_ = false;

    std::unique_ptr<Paganin> paganin_;
    std::unique_ptr<Filter> filter_;
    std::unique_ptr<Solver> solver_;

    int gpu_buffer_index_ = 0;
    bool sino_uploaded_ = false;
    std::thread gpu_upload_thread_;
    std::thread gpu_recon_thread_;
    std::condition_variable gpu_cv_;
    std::mutex gpu_mutex_;

    int num_threads_;
    oneapi::tbb::task_arena arena_;

    void processProjections();

public:

    Reconstructor(int rows, int cols, int num_threads); 

    ~Reconstructor();

    void initialize(int num_darks, 
                    int num_flats, 
                    int num_projections,
                    int preview_size);

    void initPaganin(float pixel_size, float lambda, float delta, float beta, float distance);

    void initFilter(const std::string& name, bool gaussian_pass);

    void setSolver(std::unique_ptr<Solver>&& solver);

    void start();

    void pushProjection(ProjectionType k, 
                        int32_t proj_idx, 
                        const std::array<int32_t, 2>& shape, 
                        char* data); 

    /**
     * @brief Copy projections (projection_id, rows, cols) to sinograms (rows, projection_id, cols).
     */
    void projectionToSino();

    slice_data reconstructSlice(orientation x); 

    const std::vector<float>& previewData();

    int previewSize() const;

    int bufferSize() const;

    // for unittest

    const std::vector<RawDtype>& darks() const;
    const std::vector<RawDtype>& flats() const;
    const Buffer2<float>& buffer() const;
    const SimpleBuffer3<float>& sinoBuffer() const;

};

} // namespace slicerecon
