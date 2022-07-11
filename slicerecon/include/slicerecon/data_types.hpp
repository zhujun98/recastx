#pragma once

#include <array>
#include <utility>
#include <variant>
#include <vector>

namespace slicerecon {

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

enum class ProjectionType : int32_t { dark = 0, flat = 1, projection = 2 };

using RawDtype = uint16_t;

using RawImageData = std::vector<RawDtype>;
using ImageData = std::vector<float>;

} // namespace slicerecon
