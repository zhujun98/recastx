#pragma once

#include <complex>
#include <cstdint>
#include <iostream>
#include <vector>

extern "C" {
#include <fftw3.h>
}

#include <spdlog/spdlog.h>

#include "../data_types.hpp"
#include "helpers.hpp"
#include "solver.hpp"
#include "listener.hpp"
#include "projection_processor.hpp"

namespace slicerecon {

class Reconstructor {

    std::vector<raw_dtype> all_darks_;
    std::vector<raw_dtype> all_flats_;
    std::vector<float> dark_avg_;
    std::vector<float> reciprocal_;
    std::vector<float> buffer_;

    int active_gpu_buffer_index_ = 0;
    int buffer_size_;

    int32_t pixels_ = -1;
    int32_t received_darks_ = 0;
    int32_t received_flats_ = 0;
    bool reciprocal_computed_ = false;

    std::unique_ptr<Solver> solver_;

    Settings param_;
    Geometry geom_;

    int update_count_ = 0;
    std::vector<float> small_volume_buffer_;
    std::vector<float> sino_buffer_;

    std::vector<Listener*> listeners_;
    bool initialized_ = false;

    std::unique_ptr<ProjectionProcessor> projection_processor_;

    std::mutex gpu_mutex_;

    // list of parameters that can be changed from the visualization UI
    // NOTE: the enum parameters are hard coded into the handler
    std::map<std::string, float*> float_param_;
    std::map<std::string, bool*> bool_param_;

    void initialize();

    std::vector<float> averageProjections(std::vector<raw_dtype> all); 

    void computeReciprocal();

    /**
     * In-memory processing the projections [proj_id_begin, ..., proj_id_end].
     *
     * @param proj_id_begin
     * @param proj_id_end
     */
    void processProjections(int proj_id_begin, int proj_id_end);

    /**
     * Upload the CPU sinogram buffer to ASTRA on the GPU
     *
     * @param proj_id_begin: The starting position of the data in the GPU.
     * @param proj_id_end: The last position of the data in the GPU.
     * @param buffer_begin: Position of the to-be-uploaded data in the CPU buffer.
     * @param buffer_idx: Index of the GPU buffer the sinogram data should be uploaded to.
     * @param lock_gpu: Whether or not to use gpu_mutex_ to block access to the GPU.
     */
    void uploadSinoBuffer(int proj_id_begin, int proj_id_end, int buffer_idx, bool lock_gpu = false);

    /**
     * Copy from a data buffer to a sino buffer, while transposing the data.
     *
     * If an offset is given, it transposes projections [offset, offset+1, ...,
     * proj_end] to the *front* of the sino_buffer_, leaving the remainder of the
     * buffer unused.
     *
     * @param projection_group: Reference to buffered data.
     * @param sino_buffer: Reference to the CPU buffer of the projections.
     * @param group_size: Number of projections in the group.
     *
     */
    void transposeIntoSino(int proj_offset, int proj_end);

    void refreshData();

public:

    Reconstructor(Settings parameters, Geometry geom);

    void addListener(Listener* l);

    void pushProjection(ProjectionType k, 
                        int32_t proj_idx, 
                        std::array<int32_t, 2> shape, 
                        char* data); 

    slice_data reconstructSlice(orientation x); 

    std::vector<float>& previewData();

    Settings parameters() const;

    bool initialized() const;

    void parameterChanged(std::string name, std::variant<float, std::string, bool> value);

    static std::vector<float> defaultAngles(int n);

};

} // namespace slicerecon
