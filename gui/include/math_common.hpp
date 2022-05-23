#pragma once

#include <glm/glm.hpp>

namespace gui {

glm::mat4 create_orientation_matrix(glm::vec3 base, glm::vec3 x, glm::vec3 y);

} // namespace gui
