#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>
#include <iostream>
#include "utils.hpp"

namespace recastx::gui {

std::vector<uint32_t> pack(const std::vector<float> &data,
                           float min_value, float max_value) {
  std::vector<uint32_t> data_buffer(data.size());

  // convert data to uint32_t, then set
  auto min = min_value;
  auto max = max_value;
  if (max < min) {
    min = *std::min_element(data.begin(), data.end());
    max = *std::max_element(data.begin(), data.end());
  }

  auto max_uint = std::numeric_limits<uint32_t>::max() - 128;

  for (auto i = 0u; i < data.size(); ++i) {
    data_buffer[i] = (uint32_t)(max_uint * ((data[i] - min) / (max - min)));
  }

  return data_buffer;
}

std::tuple<bool, float, glm::vec3> intersectionPoint(const glm::mat4& inv_matrix,
                                                     const glm::mat4& orientation,
                                                     const glm::vec2& point) {
    // how do we want to do this
    // end points of plane/line?
    // first see where the end
    // points of the square end up
    // within the box.
    // in world space:
    auto axis1 = glm::vec3(orientation[0][0], orientation[0][1], orientation[0][2]);
    auto axis2 = glm::vec3(orientation[1][0], orientation[1][1], orientation[1][2]);
    auto base  = glm::vec3(orientation[2][0], orientation[2][1], orientation[2][2]);
    base += 0.5f * (axis1 + axis2);
    auto normal = glm::normalize(glm::cross(axis1, axis2));
    float distance = -1.0f;

    auto from = inv_matrix * glm::vec4(point.x, point.y, -1.0f, 1.0f);
    from /= from[3];
    auto to = inv_matrix * glm::vec4(point.x, point.y, 1.0f, 1.0f);
    to /= to[3];
    auto direction = glm::normalize(glm::vec3(to) - glm::vec3(from));

    bool does_intersect = checkRayPlaneIntersection(glm::vec3(from), direction, base, normal, distance);

    // now check if the actual point is inside the plane
    auto intersection = glm::vec3(from) + direction * distance;
    intersection -= base;
    auto along_1 = glm::dot(intersection, glm::normalize(axis1));
    auto along_2 = glm::dot(intersection, glm::normalize(axis2));
    if (glm::abs(along_1) > 0.5f * glm::length(axis1) ||
        glm::abs(along_2) > 0.5f * glm::length(axis2)) {
        does_intersect = false;
    }

    return std::make_tuple(does_intersect, distance, intersection);
}

// class FpsCounter

FpsCounter::FpsCounter() : v_prev_(glfwGetTime()), s_prev_(v_prev_), threshold_(1.) {}

FpsCounter::~FpsCounter() = default;

void FpsCounter::countVolume() {
    ++v_frames_;
    double curr = glfwGetTime();
    if (curr - v_prev_ > threshold_) {
        v_fps_ = v_frames_ / (curr - v_prev_);
        v_prev_ = curr;
        v_frames_ = 0;
    }
}

void FpsCounter::countSlice() {
    ++s_frames_;
    double curr = glfwGetTime();
    if (curr - s_prev_ > threshold_) {
        s_fps_ = s_frames_ / (curr - s_prev_);
        s_prev_ = curr;
        s_frames_ = 0;
    }
}

[[nodiscard]] double FpsCounter::volumeFrameRate() {
    if (glfwGetTime() - v_prev_ > FpsCounter::reset_interval_) v_fps_ = 0.;
    return v_fps_;
}

[[nodiscard]] double FpsCounter::sliceFrameRate() {
    if (glfwGetTime() - s_prev_ > FpsCounter::reset_interval_) s_fps_ = 0.;
    return s_fps_;
}

} // namespace recastx::gui
