#pragma once

#include <cstddef>
#include <vector>

namespace tomop::gui {

std::vector<uint32_t> pack(const std::vector<float>& data,
                           float min_value = 1.0f, float max_value = -1.0f);

} // tomop::gui
