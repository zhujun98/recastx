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

#include <type_traits>

#include "common/config.hpp"
#include "recon/buffer.hpp"
#include "recon/daq_client_interface.hpp"

namespace recastx::recon {

template<typename T = RawDtype>
struct Projection {
    ProjectionType type;
    size_t index;
    Tensor<T, 2> data;

    Projection(ProjectionType type, size_t index, size_t col_count, size_t row_count, std::vector<T> data)
         : type(type), index(index), data(std::array<size_t, 2> {row_count, col_count}, std::move(data)) {
    }

    Projection(ProjectionType type, size_t index, Tensor<T, 2> data)
         : type(type), index(index), data(std::move(data)) {
    }

    template<std::enable_if_t<std::is_same<T, RawDtype>::value, bool> = true>
    Projection(const daq::Message& msg) :
            type(msg.type), 
            index(msg.index),
            data({msg.row_count, msg.col_count}) {
        std::memcpy(data.data(), msg.data.data(), msg.data.size());
    }
};

} // namespace recastx::recon

#endif // RECON_PROJECTION_H