#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include <array>
#include <string>
#include <vector>


namespace recastx {

    constexpr size_t MAX_NUM_SLICES = 3;

    using Orientation = std::array<float, 9>;

    enum class ProjectionType : int { dark = 0, flat = 1, projection = 2, unknown = 99 };

    enum class BeamShape { PARALELL, CONE };

    using RawDtype = uint16_t;
    using ProDtype = float;

    struct DaqClientConfig {
        int port;
        std::string hostname;
        std::string socket_type;
    };

    struct RpcServerConfig {
        int port;
    };

    struct FlatFieldCorrectionParams {
        size_t num_darks;
        size_t num_flats;
    };

    struct ImageProcParams {
        uint32_t downsampling_col;
        uint32_t downsampling_row;
    };

    struct FilterConfig {
        bool gaussian_lowpass_filter;
        std::string name;
    };

    struct PaganinConfig {
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