/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_RAY_CASTER_H
#define GUI_RAY_CASTER_H

#include <optional>

#include <glm/glm.hpp>

namespace recastx::gui {

inline std::optional<glm::vec3> castRayPlane(const glm::vec3& src,
                                             const glm::vec3& dir,
                                             const glm::vec3& normal,
                                             float d) {
    float alpha = glm::dot(normal, dir);
    if (std::abs(alpha) > 0.001f) {
        return src - dir * (glm::dot(normal, src) + d) / alpha;
    }
    return std::nullopt;
}

inline std::optional<glm::vec3> castRayRectangle(const glm::vec3& src,
                                                 const glm::vec3& dir,
                                                 const std::array<glm::vec3, 3>& rectangle) {
    const auto& axis1 = rectangle[0];
    const auto& axis2 = rectangle[1];
    const auto& center  = rectangle[2] + 0.5f * (axis1 + axis2);

    auto normal = glm::normalize(glm::cross(axis1, axis2));
    auto point = castRayPlane(src, dir, normal, -glm::dot(normal, center));
    if (point) {
        auto vec = point.value() - center;
        if (std::abs(glm::dot(vec, glm::normalize(axis1))) > 0.5f * glm::length(axis1)
            || std::abs(glm::dot(vec, glm::normalize(axis2))) > 0.5f * glm::length(axis2)) {
            return std::nullopt;
        }
        return point.value();
    }
    return std::nullopt;
}

} // recastx::gui

#endif // GUI_RAY_CASTER_H
