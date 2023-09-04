#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "config.hpp"

namespace recastx {

inline size_t sliceIdFromTimestamp(size_t ts) {
    return ts % MAX_NUM_SLICES; 
}

inline size_t expandDataSizeForGpu(size_t s, size_t chunk_size) {
    return s % chunk_size == 0 ? s : (s / chunk_size + 1 ) * chunk_size;
}

} // namespace recastx

#endif // COMMON_UTILS_H