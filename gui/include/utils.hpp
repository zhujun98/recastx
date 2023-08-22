/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <cstddef>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace recastx::gui {

std::vector<uint32_t> pack(const std::vector<float>& data,
                           float min_value = 1.0f, float max_value = -1.0f);

inline bool checkRayPlaneIntersection(const glm::vec3& origin,
                                      const glm::vec3& direction,
                                      const glm::vec3& base,
                                      const glm::vec3& normal,
                                      float& distance) {
    auto alpha = glm::dot(normal, direction);
    if (glm::abs(alpha) > 0.001f) {
        distance = glm::dot((base - origin), normal) / alpha;
        if (distance >= 0.001f) return true;
    }
    return false;
}

std::tuple<bool, float, glm::vec3> intersectionPoint(const glm::mat4& inv_matrix,
                                                     const glm::mat4& orientation,
                                                     const glm::vec2& point);

class FpsCounter {

    int frames_ = 0;
    double prev_;
    double threshold_;
    double fps_ = 0.;

    static constexpr double reset_interval_ = 5.;

public:

    FpsCounter();
    ~FpsCounter();

    void update();

    [[nodiscard]] double frameRate();
};

inline std::ostream& operator<<(std::ostream& stream, const glm::vec3& v) {
    stream << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
    return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const glm::vec4& v) {
    stream << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", "<< v[3] << ")";
    return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const glm::mat4& m) {
    stream << "[" << m[0] << ", " << m[1] << ", " << m[2] << ", "<< m[3] << "]";
    return stream;
}

} // namespace recastx::gui

#endif //GUI_UTILS_H