/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_PROJECTION_H
#define RECON_PROJECTION_H

#include "common/config.hpp"

namespace recastx::recon {

enum class ProjectionType : int { DARK = 0, FLAT = 1, PROJECTION = 2, UNKNOWN = 99 };

namespace daq { class Message; }

struct Projection {
    ProjectionType type;
    size_t index;
    size_t col_count;
    size_t row_count;
    std::vector<RawDtype> data;

    Projection(const daq::Message& msg);
};

} // namespace recastx::recon

#endif // RECON_PROJECTION_H