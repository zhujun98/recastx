#include <iostream>

#include <spdlog/spdlog.h>

#include "glm/gtc/constants.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "GL/gl3w.h"
#include "GLFW/glfw3.h"
#include "imgui.h"

#include "tomcat/tomcat.hpp"

#include "graphics/scenes/recon_component.hpp"
#include "graphics/scenes/scene_camera3d.hpp"
#include "graphics/primitives.hpp"
#include "util.hpp"

namespace tomcat::gui {

ReconComponent::ReconComponent(Scene& scene) : scene_(scene) {
    glGenVertexArrays(1, &vao_handle_);
    glBindVertexArray(vao_handle_);
    glGenBuffers(1, &vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), square(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &line_vao_handle_);
    glBindVertexArray(line_vao_handle_);
    glGenBuffers(1, &line_vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GLfloat), line(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &cube_vao_handle_);
    glBindVertexArray(cube_vao_handle_);

    cube_index_count_ = 24;
    glGenBuffers(1, &cube_index_handle_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_index_handle_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube_index_count_ * sizeof(GLuint),
                 cube_wireframe_idxs(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glGenBuffers(1, &cube_vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), cube_wireframe(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    auto simple_vert =
#include "../shaders/simple_3d.vert"
    ;
    auto simple_frag =
#include "../shaders/simple_3d.frag"
    ;

    program_ = std::make_unique<ShaderProgram>(simple_vert, simple_frag);

    auto cube_vert =
#include "../shaders/wireframe_cube.vert"
    ;
    auto cube_frag =
#include "../shaders/wireframe_cube.frag"
    ;
    cube_program_ = std::make_unique<ShaderProgram>(cube_vert, cube_frag);

    initSlices();

    initVolume();

    cm_texture_id_ = scene_.camera().colormapTextureId();
}

ReconComponent::~ReconComponent() {
    glDeleteVertexArrays(1, &cube_vao_handle_);
    glDeleteBuffers(1, &cube_vbo_handle_);
    glDeleteVertexArrays(1, &vao_handle_);
    glDeleteBuffers(1, &vbo_handle_);
    // FIXME FULLY DELETE CUBE AND LINE
    glDeleteVertexArrays(1, &cube_vao_handle_);
    glDeleteBuffers(1, &cube_vbo_handle_);
    glDeleteBuffers(1, &cube_index_handle_);
    glDeleteVertexArrays(1, &line_vao_handle_);
    glDeleteBuffers(1, &line_vbo_handle_);
}

void ReconComponent::requestSlices() {
    for (auto& slice : slices_) {
        auto packet = SetSlicePacket(slice.first, slice.second->orientation3());
        scene_.send(packet);
    }
}

void ReconComponent::setSliceData(std::vector<float>&& data,
                                  const std::array<int32_t, 2>& size,
                                  int slice_idx) {
    Slice* slice;
    if (slices_.find(slice_idx) != slices_.end()) {
        slice = slices_[slice_idx].get();
    } else {
        std::cout << "Updating inactive slice: " << slice_idx << "\n";
        return;
    }

    if (slice == dragged_slice_) return;

    slice->setData(std::move(data), size);
}

void ReconComponent::setVolumeData(std::vector<float>&& data, const std::array<int32_t, 3>& size) {
    volume_->setData(std::move(data), size);
}

void ReconComponent::describe() {}

void ReconComponent::render(const glm::mat4& world_to_screen) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    program_->use();

    program_->setInt("texture_sampler", 0);
    program_->setInt("colormap_sampler", 1);
    program_->setInt("volume_data_sampler", 3);

    auto [slices_min, slices_max] = minMaxValsSlices();
    program_->setFloat("min_value", slices_min);
    program_->setFloat("max_value", slices_max);
    auto [volume_min, volume_max] = volume_->minMaxVals();
    program_->setFloat("volume_min_value", volume_min);
    program_->setFloat("volume_max_value", volume_max);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, cm_texture_id_);

    glm::mat4 full_transform = world_to_screen * volume_transform_;

    std::vector<Slice*> slices;
    for (auto& [slice_id, slice] : slices_) {
        if (slice->inactive()) continue;
        slices.push_back(slice.get());
    }
    std::sort(slices.begin(), slices.end(), [](auto& lhs, auto& rhs) -> bool {
        if (rhs->transparent() == lhs->transparent()) {
            return rhs->id() < lhs->id();
        }
        return rhs->transparent();
    });

    volume_->bind();
    for (auto slice : slices) drawSlice(slice, full_transform);
    volume_->unbind();

    cube_program_->use();
    cube_program_->setMat4("transform_matrix", full_transform);
    cube_program_->setVec4("line_color", glm::vec4(0.5f, 0.5f, 0.5f, 0.3f));

    glBindVertexArray(cube_vao_handle_);
    glLineWidth(3.0f);
    glDrawElements(GL_LINES, cube_index_count_, GL_UNSIGNED_INT, nullptr);

    glDisable(GL_DEPTH_TEST);

    if (drag_machine_ != nullptr && drag_machine_->type() == DragType::rotator) {
        auto& rotator = *(SliceRotator*)drag_machine_.get();
        cube_program_->setMat4("transform_matrix",
                               full_transform * glm::translate(rotator.rot_base) * glm::scale(rotator.rot_end - rotator.rot_base));
        cube_program_->setVec4("line_color", glm::vec4(1.0f, 1.0f, 0.5f, 0.5f));
        glBindVertexArray(line_vao_handle_);
        glLineWidth(5.0f);
        glDrawArrays(GL_LINES, 0, 2);
    }

    glDisable(GL_BLEND);
}

bool ReconComponent::handleMouseButton(int button, int action) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (hovered_slice_ != nullptr) {
                maybeSwitchDragMachine(DragType::translator);
                dragged_slice_ = hovered_slice_;

#if (VERBOSITY >= 4)
                spdlog::info("Set dragged slice: {}", dragged_slice_->id());
#endif

                return true;
            }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (hovered_slice_ != nullptr) {
                maybeSwitchDragMachine(DragType::rotator);
                dragged_slice_ = hovered_slice_;

#if (VERBOSITY >= 4)
                spdlog::info("Set dragged slice: {}", dragged_slice_->id());
#endif

                return true;
            }
        }
    } else if (action == GLFW_RELEASE) {
        if (dragged_slice_ != nullptr) {
            auto packet = SetSlicePacket(dragged_slice_->id(), dragged_slice_->orientation3());
            scene_.send(packet);

            dragged_slice_ = nullptr;
            drag_machine_ = nullptr;
            return true;
        }
        drag_machine_ = nullptr;
    }

    return false;
}

bool ReconComponent::handleMouseMoved(double x, double y) {
    // update slice that is being hovered over
    y = -y;

    if (prev_y_ < -1.0) {
        prev_x_ = x;
        prev_y_ = y;
    }

    glm::vec2 delta(x - prev_x_, y - prev_y_);
    prev_x_ = x;
    prev_y_ = y;

    // TODO: fix for screen ratio
    if (dragged_slice_ != nullptr) {
        drag_machine_->onDrag(delta);
        return true;
    }

    updateHoveringSlice(x, y);

    return false;
}

void ReconComponent::initSlices() {
    for (int i = 0; i < 3; ++i) slices_[i] = std::make_unique<Slice>(i);
    resetSlices();
}

void ReconComponent::resetSlices() {
    // slice along axis 0 = x
    slices_[0]->setOrientation(glm::vec3(0.0f, -1.0f, -1.0f),
                               glm::vec3(0.0f, 2.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 2.0f));

    // slice along axis 1 = y
    slices_[1]->setOrientation(glm::vec3(-1.0f, 0.0f, -1.0f),
                               glm::vec3(2.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 2.0f));

    // slice along axis 2 = z
    slices_[2]->setOrientation(glm::vec3(-1.0f, -1.0f, 0.0f),
                               glm::vec3(2.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 2.0f, 0.0f));
}

void ReconComponent::initVolume() {
    volume_ = std::make_unique<Volume>();

    glm::vec3 min_pt(-1.0f), max_pt(1.0f);
    auto center = 0.5f * (min_pt + max_pt);
    volume_transform_ = glm::translate(center) *
                        glm::scale(glm::vec3(max_pt - min_pt)) *
                        glm::scale(glm::vec3(0.5f));
    scene_.camera().lookAt(center);
}

void ReconComponent::updateHoveringSlice(float x, float y) {
    auto inv_matrix = glm::inverse(scene_.camera().matrix() * volume_transform_);
    int slice_id = -1;
    float best_z = std::numeric_limits<float>::max();
    for (auto& [sid, slice] : slices_) {
        if (slice->inactive()) continue;
        slice->setHovered(false);
        auto maybe_point = intersectionPoint(inv_matrix, slice->orientation4(), glm::vec2(x, y));
        if (std::get<0>(maybe_point)) {
            auto z = std::get<1>(maybe_point);
            if (z < best_z) {
                best_z = z;
                slice_id = sid;
            }
        }
    }

    if (slice_id >= 0) {
        slices_[slice_id]->setHovered(true);
        hovered_slice_ = slices_[slice_id].get();
    } else {
        hovered_slice_ = nullptr;
    }
}

void ReconComponent::maybeSwitchDragMachine(ReconComponent::DragType type) {
    if (drag_machine_ == nullptr || drag_machine_->type() != type) {
        switch (type) {
            case DragType::translator:
                drag_machine_ = std::make_unique<SliceTranslator>(
                        *this, glm::vec2{prev_x_, prev_y_});
                break;
            case DragType::rotator:
                drag_machine_ = std::make_unique<SliceRotator>(
                        *this, glm::vec2{prev_x_, prev_y_});
                break;
            default:
                break;
        }
    }
}

void ReconComponent::drawSlice(Slice* slice, const glm::mat4& world_to_screen) {
    slice->bind();

    program_->setMat4("world_to_screen_matrix", world_to_screen);
    program_->setMat4("orientation_matrix",
                      slice->orientation4() * glm::translate(glm::vec3(0.0, 0.0, 1.0)));
    program_->setBool("hovered", slice->hovered());
    program_->setBool("has_data", !slice->empty());

    glBindVertexArray(vao_handle_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    slice->unbind();
}

std::array<float, 2> ReconComponent::minMaxValsSlices() {
    auto overall_min = std::numeric_limits<float>::max();
    auto overall_max = std::numeric_limits<float>::min();
    for (auto&& [slice_idx, slice] : slices_) {
        (void)slice_idx;

        auto [min_v, max_v] = slice->minMaxVals();
        overall_min = min_v < overall_min ? min_v : overall_min;
        overall_max = max_v > overall_max ? max_v : overall_max;
    }

    return {overall_min - (0.2f * (overall_max - overall_min)),
            overall_max + (0.2f * (overall_max - overall_min))};
}

// ReconComponent::DragMachine

ReconComponent::DragMachine::DragMachine(ReconComponent& comp, const glm::vec2& initial, DragType type)
    : comp_(comp), initial_(initial), type_(type) {}

ReconComponent::DragMachine::~DragMachine() = default;

// ReconComponent::SliceTranslator

ReconComponent::SliceTranslator::SliceTranslator(ReconComponent &comp, const glm::vec2& initial)
    : DragMachine(comp, initial, DragType::translator) {}

ReconComponent::SliceTranslator::~SliceTranslator() = default;

void ReconComponent::SliceTranslator::onDrag(glm::vec2 delta) {
    // 1) what are we dragging, and does it have data?
    // if it does then we need to make a new slice
    // else we drag the current slice along the normal
    if (!comp_.dragged_slice()) {
        std::unique_ptr<Slice> new_slice;
        int id = comp_.generate_slice_idx();
        int to_remove = -1;
        for (auto& id_the_slice : comp_.slices()) {
            auto& the_slice = id_the_slice.second;
            if (the_slice->hovered()) {
                if (!the_slice->empty()) {
                    new_slice = std::make_unique<Slice>(id);
                    new_slice->setOrientation(the_slice->orientation4());
                    to_remove = the_slice->id();
                    // FIXME need to generate a new id and upon 'popping'
                    // send a UpdateSlice packet
                    comp_.dragged_slice() = new_slice.get();
                } else {
                    comp_.dragged_slice() = the_slice.get();
                }
                break;
            }
        }
        if (new_slice) {
            comp_.slices()[new_slice->id()] = std::move(new_slice);
        }
        if (to_remove >= 0) {
            comp_.slices().erase(to_remove);
            // send slice packet
            auto packet = RemoveSlicePacket(to_remove);
            comp_.scene().send(packet);
        }
        if (!comp_.dragged_slice()) {
            std::cout << "WARNING: No dragged slice found." << std::endl;
            return;
        }
    }

    auto slice = comp_.dragged_slice();
    auto& o = slice->orientation4();

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto normal = glm::normalize(glm::cross(axis1, axis2));

    // project the normal vector to screen coordinates
    // FIXME maybe need window matrix here too which would be kind of
    // painful maybe
    auto base_point_normal =
        glm::vec3(o[2][0], o[2][1], o[2][2]) + 0.5f * (axis1 + axis2);
    auto end_point_normal = base_point_normal + normal;

    auto a = comp_.scene().camera().matrix() * comp_.volume_transform() *
             glm::vec4(base_point_normal, 1.0f);
    auto b = comp_.scene().camera().matrix() * comp_.volume_transform() *
             glm::vec4(end_point_normal, 1.0f);
    auto normal_delta = b - a;
    float difference =
        glm::dot(glm::vec2(normal_delta.x, normal_delta.y), delta);

    // take the inner product of delta x and this normal vector

    auto dx = difference * normal;
    // FIXME check if it is still inside the bounding box of the volume
    // probably by checking all four corners are inside bounding box, should
    // define this box somewhere
    o[2][0] += dx[0];
    o[2][1] += dx[1];
    o[2][2] += dx[2];
}

// ReconComponent::SliceRotator

ReconComponent::SliceRotator::SliceRotator(ReconComponent& comp, const glm::vec2& initial)
    : DragMachine(comp, initial, DragType::rotator) {
    // 1. need to identify the opposite axis
    // a) get the position within the slice
    auto tf = comp.scene().camera().matrix() * comp.volume_transform();
    auto inv_matrix = glm::inverse(tf);

    auto slice = comp.hovered_slice();
    assert(slice);
    auto o = slice->orientation4();

    auto maybe_point = intersectionPoint(inv_matrix, o, initial_);
    assert(std::get<0>(maybe_point));

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);

    auto in_world = std::get<2>(maybe_point);
    auto rel = in_world - base;

    auto x = 0.5f * glm::dot(rel, axis1) - 1.0f;
    auto y = 0.5f * glm::dot(rel, axis2) - 1.0f;

    // 2. need to rotate around that at on drag
    auto other = glm::vec3();
    if (glm::abs(x) > glm::abs(y)) {
        if (x > 0.0f) {
            rot_base = base;
            rot_end = rot_base + axis2;
            other = axis1;
        } else {
            rot_base = base + axis1;
            rot_end = rot_base + axis2;
            other = -axis1;
        }
    } else {
        if (y > 0.0f) {
            rot_base = base;
            rot_end = rot_base + axis1;
            other = axis2;
        } else {
            rot_base = base + axis2;
            rot_end = rot_base + axis1;
            other = -axis2;
        }
    }

    auto center = 0.5f * (rot_end + rot_base);
    auto opposite_center = 0.5f * (rot_end + rot_base) + other;
    auto from = tf * glm::vec4(glm::rotate(rot_base - center,
                                           glm::half_pi<float>(), other) +
                                   opposite_center,
                               1.0f);
    auto to = tf * glm::vec4(glm::rotate(rot_end - center,
                                         glm::half_pi<float>(), other) +
                                 opposite_center,
                             1.0f);

    screen_direction = glm::normalize(from - to);
}

ReconComponent::SliceRotator::~SliceRotator() = default;

void ReconComponent::SliceRotator::onDrag(glm::vec2 delta) {
    // 1) what are we dragging, and does it have data?
    // if it does then we need to make a new slice
    // else we drag the current slice along the normal
    if (!comp_.dragged_slice()) {
        std::unique_ptr<Slice> new_slice;
        int id = comp_.generate_slice_idx();
        int to_remove = -1;
        for (auto& id_the_slice : comp_.slices()) {
            auto& the_slice = id_the_slice.second;
            if (the_slice->hovered()) {
                if (!the_slice->empty()) {
                    new_slice = std::make_unique<Slice>(id);
                    new_slice->setOrientation(the_slice->orientation4());
                    to_remove = the_slice->id();
                    // FIXME need to generate a new id and upon 'popping'
                    // send a UpdateSlice packet
                    comp_.dragged_slice() = new_slice.get();
                } else {
                    comp_.dragged_slice() = the_slice.get();
                }
                break;
            }
        }
        if (new_slice) {
            comp_.slices()[new_slice->id()] = std::move(new_slice);
        }
        if (to_remove >= 0) {
            comp_.slices().erase(to_remove);
            // send slice packet
            auto packet = RemoveSlicePacket(to_remove);
            comp_.scene().send(packet);
        }
        assert(comp_.dragged_slice());
    }

    auto slice = comp_.dragged_slice();
    auto& o = slice->orientation4();

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);

    auto a = base - rot_base;
    auto b = base + axis1 - rot_base;
    auto c = base + axis2 - rot_base;

    auto weight = glm::dot(delta, screen_direction);
    a = glm::rotate(a, weight, rot_end - rot_base) + rot_base;
    b = glm::rotate(b, weight, rot_end - rot_base) + rot_base;
    c = glm::rotate(c, weight, rot_end - rot_base) + rot_base;

    slice->setOrientation(a, b - a, c - a);
}

} // tomcat::gui
