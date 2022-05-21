#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace gui {

struct Keyframe {
    float time_step;
};

struct PositionKeyframe : public Keyframe {
    glm::vec3 position;
};

struct RotationKeyframe : public Keyframe {
    glm::quat quaternion;
};

} // namespace gui
