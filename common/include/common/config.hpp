/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include <array>
#include <string>
#include <vector>


namespace recastx {

    inline constexpr size_t MAX_NUM_SLICES = 3;

    inline constexpr uint32_t K_SCAN_UPDATE_INTERVAL_STEP_SIZE = 16;
    inline constexpr uint32_t K_MIN_SCAN_UPDATE_INTERVAL = 16;
    inline constexpr uint32_t K_MAX_SCAN_UPDATE_INTERVAL = 128;

    inline constexpr size_t k_MAX_NUM_DARKS = 1000;
    inline constexpr size_t k_MAX_NUM_FLATS = 1000;

    inline constexpr size_t k_DAQ_BUFFER_SIZE = 1000;
    inline constexpr size_t k_DAQ_MONITOR_EVERY = 1000;
    inline constexpr size_t k_PROJECTION_MEDIATOR_BUFFER_SIZE = 10;

    using Orientation = std::array<float, 9>;

    enum class ProjectionType : int { DARK = 0, FLAT = 1, PROJECTION = 2, UNKNOWN = 99 };
    enum class BeamShape { PARALELL, CONE };

    using RawDtype = uint16_t;
    using ProDtype = float;

    inline constexpr int K_MAX_RPC_CLIENT_RECV_MESSAGE_SIZE = (16 * 4 + 1) * 1024 * 1024; // 16 MPixel
    inline constexpr int K_MAX_RPC_SERVER_SEND_MESSAGE_SIZE = (16 * 4 + 1) * 1024 * 1024; // 16 MPixel

    struct RpcServerConfig {
        int port;
    };

    struct ImageprocParams {
        struct RampFilter {
            std::string name;
        };

        uint32_t num_threads;
        uint32_t downsampling_col;
        uint32_t downsampling_row;
        int32_t offset_col;
        int32_t offset_row;
        bool disable_negative_log;
        RampFilter ramp_filter;
    };

    struct PaganinParams {
        float pixel_size;
        float lambda;
        float delta;
        float beta;
        float distance;
    };

    struct ProjectionGeometry {
        BeamShape beam_shape;
        size_t col_count; // number of detector columns
        size_t row_count; // number of detector rows
        float pixel_width; // width of each detector
        float pixel_height; // height of each detector
        float source2origin = 0.f;
        float origin2detector = 0.f;
        std::vector<float> angles; // array of projection angles
    };

    struct VolumeGeometry {
        size_t col_count; // number of columns
        size_t row_count; // number of rows
        size_t slice_count; // number of slices
        float min_x; // minimum x coordinates
        float max_x; // maximum x coordinates
        float min_y; // minimum y coordinates
        float max_y; // maximum y coordinates
        float min_z; // minimum z coordinates
        float max_z; // maximum z coordinates
    };

} // namespace recastx

#endif // COMMON_CONFIG_H