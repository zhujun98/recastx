#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "config.hpp"

namespace tomcat {

inline size_t sliceIdFromTimestamp(size_t ts) {
    return ts % MAX_NUM_SLICES; 
}

} // tomcat

#endif // COMMON_UTILS_H