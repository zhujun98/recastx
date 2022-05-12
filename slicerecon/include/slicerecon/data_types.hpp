#pragma once

#include <array>
#include <utility>
#include <variant>
#include <vector>

namespace slicerecon {

/**
 * The mode that the reconstructor is in. Continuous corresponds to a sliding-
 * window. Alternating is the default.
 * @see https://github.com/cicwi/SliceRecon/issues/4
 */
enum class ReconstructMode { alternating, continuous };

struct PaganinSettings {
    float pixel_size;
    float lambda;
    float delta;
    float beta;
    float distance;
};

struct Settings {
    int32_t slice_size;
    int32_t preview_size;
    int32_t group_size;
    int32_t filter_cores;
    int32_t darks;
    int32_t flats;
    int32_t projections;
    ReconstructMode recon_mode;
    bool already_linear;
    bool retrieve_phase;
    bool tilt_axis;
    PaganinSettings paganin;
    bool gaussian_pass;
    std::string filter;
};

/**
 * Parameters that define a simple single-axis circular Geometry.
 */
struct Geometry {
    int32_t rows = 0;
    int32_t cols = 0;
    int32_t projections = 0;
    std::vector<float> angles = {};
    bool parallel = false;
    bool vec_geometry = false;
    std::array<float, 2> detector_size = {0.0f, 0.0f};
    std::array<float, 3> volume_min_point = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> volume_max_point = {1.0f, 1.0f, 1.0f};
    // for cone beam
    float source_origin = 0.0f;
    float origin_det = 0.0f;
};

/**
 * The orientation is an array of 9 floating point numbers. This corresponds to
 * the way tomopackets defines an orientation.
 */
using orientation = std::array<float, 9>;

/**
 * Slice data is a pair of a size in pixels, and the packed data as tomopackets
 * expects it.
 */
using slice_data = std::pair<std::array<int32_t, 2>, std::vector<float>>;

/**
 * An enum with the different projection types, which are dark, flat, and standard.
 */
enum class ProjectionType : int32_t { dark = 0, flat = 1, standard = 2 };

using raw_dtype = uint16_t;

} // namespace slicerecon
