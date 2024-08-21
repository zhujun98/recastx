/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include  <limits>

#include "graphics/scene.hpp"
#include "graphics/renderer.hpp"
#include "graphics/camera.hpp"
#include "graphics/style.hpp"
#include "graphics/mesh_object.hpp"
#include "graphics/voxel_object.hpp"
#include "graphics/slice_object.hpp"
#include "graphics/image_object.hpp"
#include "graphics/simple_object.hpp"
#include "graphics/glyph_object.hpp"
#include "graphics/light_object.hpp"
#include "graphics/ray_caster.hpp"
#include "graphics/widgets/light_widget.hpp"

namespace recastx::gui {

Scene::Scene() : camera_(new Camera()) {
}

Scene::~Scene() = default;

void Scene::setCamera(std::shared_ptr<Camera> camera) {
    camera_ = std::move(camera);
}

template<typename T, typename... Args, typename>
T* Scene::setCamera(Args&&... args) {
    auto camera = std::make_shared<T>(std::forward<Args>(args)...);
    addCamera(camera);
    return camera.get();
}

template<typename T, typename... Args, typename>
T* Scene::addObject(Args&&... args) {
    auto obj = std::make_shared<T>(std::forward<Args>(args)...);
    if constexpr (std::is_base_of_v<VoxelObject, T>) {
        voxel_objects_.push_back(obj);
    } else if constexpr (std::is_base_of_v<MeshObject, T>) {
        mesh_objects_.push_back(obj);
    } else if constexpr (std::is_base_of_v<SliceObject, T>) {
        slice_objects_.push_back(obj);
    } else if constexpr (std::is_base_of_v<ImageObject, T>) {
        image_objects_.push_back(obj);
    } else if constexpr (std::is_base_of_v<SimpleObject, T>) {
        simple_objects_.push_back(obj);
    } else if constexpr (std::is_base_of_v<GlyphObject, T>) {
        glyph_objects_.push_back(obj);
    } else {
        throw std::invalid_argument("");
    }

    return obj.get();
}

template
VoxelObject* Scene::addObject<VoxelObject>();
template
MeshObject* Scene::addObject<MeshObject>(ShapeType&&);
template
SliceObject* Scene::addObject<SliceObject>();
template
ImageObject* Scene::addObject<ImageObject>();
template
LightObject* Scene::addObject<LightObject>(std::shared_ptr<Light>&&);
template
SimpleObject* Scene::addObject<SimpleObject>(SimpleObject::Type&&);
template
GlyphObject* Scene::addObject<GlyphObject>(std::string&&);

Light* Scene::addLight() {
    light_ = std::make_shared<Light>();
    addObject<LightObject>(light_);
    light_widget_ = std::make_unique<LightWidget>(light_);
    return light_.get();
}

void Scene::draw(Renderer* renderer) {
    renderer->update(camera_.get(), light_.get());
    renderer->useViewport(viewport_);
    prev_inv_vp_ = glm::inverse(renderer->vpMatrix());

    renderer->render(voxel_objects_);
    renderer->render(mesh_objects_);
    renderer->render(slice_objects_);
    renderer->render(image_objects_);
    renderer->render(simple_objects_);
    renderer->render(glyph_objects_);
}

bool Scene::consumeEvent(InputEvent& event) {
    if (std::holds_alternative<MouseHoverEvent>(event)) {
        auto& ev = std::get<MouseHoverEvent>(event);

        if (!mapToSceneEvent(ev)) return false;

        auto object = findClosestSlice(ev.pos.x, ev.pos.y);
        if (object != hovered_object_) {
            if (hovered_object_ != nullptr) {
                ev.entering = false;
                ev.exiting = true;
                hovered_object_->mouseHoverEvent(ev);
            }
            if (object != nullptr) {
                ev.entering = true;
                ev.exiting = false;
                object->mouseHoverEvent(ev);
            }

            hovered_object_ = object;
        }

        return true;

    } else if (std::holds_alternative<MouseDragEvent>(event)) {
        auto& ev = std::get<MouseDragEvent>(event);

        if (ev.exiting && dragged_object_ != nullptr) {
            dragged_object_->mouseDragEvent(ev);
            dragged_object_ = nullptr;
            return true;
        }

        if (!mapToSceneEvent(ev)) return false;

        if (ev.button == MouseButton::LEFT) {
            if (ev.entering) {
                auto object = findClosestSlice(ev.pos.x, ev.pos.y);
                if (object != nullptr) dragged_object_ = object;
            }
            if (dragged_object_ != nullptr) {
                dragged_object_->mouseDragEvent(ev);
                return true;
            }
        }

        if (!camera_fixed_ && camera_->mouseDragEvent(ev)) return true;

    } else if (std::holds_alternative<MouseScrollEvent>(event)) {
        auto& ev = std::get<MouseScrollEvent>(event);
        if (!mapToSceneEvent(ev)) return false;
        if (!camera_fixed_ && camera_->mouseScrollEvent(ev)) return true;

    } else if (std::holds_alternative<KeyEvent>(event)) {
        auto& ev = std::get<KeyEvent>(event);
        if (!camera_fixed_ && camera_->keyEvent(ev)) return true;
    }
    return false;
}

void Scene::drawLightControlGUI() {
    ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
    if (ImGui::CollapsingHeader("LIGHTS", ImGuiTreeNodeFlags_DefaultOpen)) {
        light_widget_->draw();
    }
    ImGui::PopStyleColor();
}

void Scene::drawCameraControlGUI() {
    ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
    if (ImGui::CollapsingHeader("CAMERA", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Fix camera", &camera_fixed_);

        if (ImGui::Button("Y-Z##CAMERA")) {
            camera_->setFrontView();
        }
        ImGui::SameLine();
        if (ImGui::Button("X-Z##CAMERA")) {
            camera_->setSideView();
        }
        ImGui::SameLine();
        if (ImGui::Button("X-Y##CAMERA")) {
            camera_->setTopView();
        }
        ImGui::SameLine();
        if (ImGui::Button("Perspective##CAMERA")) {
            camera_->setPerspectiveView();
        }
    }
    ImGui::PopStyleColor();
}

Interactable* Scene::findClosestSlice(float x, float y) const {
    auto point1 = prev_inv_vp_ * glm::vec4(x, y, -1.f, 1.f);
    point1 /= point1[3];
    auto point2 = prev_inv_vp_ * glm::vec4(x, y,  1.f, 1.f);
    point2 /= point2[3];

    float min_dist = std::numeric_limits<float>::max();
    SliceObject* found = nullptr;
    for (const auto& object : slice_objects_) {
        if (!object->visible()) continue;
        auto q = castRayRectangle(glm::vec3(point1), glm::vec3(point2 - point1), object->geometry());
        if (q) {
            float dist = glm::distance(q.value(), glm::vec3(point1));
            if (dist < min_dist) {
                min_dist = dist;
                found = object.get();
            }
        }
    }

    return found;
}

std::optional<std::array<double, 2>> Scene::mapToSceneCoordinate(double x, double y) const {
    double x_vp = (x + 1. - 2. * viewport_.x) / viewport_.width - 1.;
    double y_vp = (y + 1. - 2. * viewport_.y) / viewport_.height - 1.;
    if (x_vp >= -1. && x_vp <= 1. && y_vp >= -1. && y_vp <= 1.) return std::array<double, 2>{x_vp, y_vp};
    return std::nullopt;
}

}  // namespace recastx::gui


//std::vector<Slice*> ReconComponent::sortedSlices() const {
//    std::vector<Slice*> sorted;
//    for (auto& [_, policy, slice] : slices_) {
//        if (policy == DISABLE_SLI) continue;
//        sorted.push_back(slice.get());
//    }
//    std::sort(sorted.begin(), sorted.end(), [](auto& lhs, auto& rhs) -> bool {
//        if (rhs->transparent() == lhs->transparent()) {
//            return rhs->id() < lhs->id();
//        }
//        return rhs->transparent();
//    });
//    return sorted;
//}
//
//bool SliceComponent::handleMouseButton(int button, int action) {
//
//    if (action == GLFW_PRESS) {
//        if (button == GLFW_MOUSE_BUTTON_LEFT) {
//            if (hovered_slice_ >= 0) {
//                maybeSwitchDragMachine(DragType::translator);
//                dragged_slice_ = hovered_slice_;
//
//                log::debug("Set dragged slice: {}", dragged_slice_);
//
//                return true;
//            }
//        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
//            if (hovered_slice_ >= 0) {
//                maybeSwitchDragMachine(DragType::rotator);
//                dragged_slice_ = hovered_slice_;
//
//                log::debug("Set dragged slice: {}", dragged_slice_);
//
//                return true;
//            }
//        }
//    } else if (action == GLFW_RELEASE) {
//        if (dragged_slice_ != nullptr) {
//            std::get<0>(slices_[dragged_slice_->id()]) += MAX_NUM_SLICES;
//            scene_.client()->setSlice(std::get<0>(slices_[dragged_slice_->id()]),
//                                      dragged_slice_->orientation3());
//
//            log::debug("Sent slice {} ({}) orientation update request",
//                       dragged_slice_->id(), std::get<0>(slices_[dragged_slice_->id()]));
//
//            dragged_slice_ = -1;
//            drag_machine_ = nullptr;
//            return true;
//        }
//        drag_machine_ = nullptr;
//    }
//
//    return false;
//}

//bool SliceComponent::handleMouseMoved(glm::vec2 delta) {
// TODO: fix for screen ratio
//    if (dragged_slice_ >= 0) {
//        dragged_slice_->clear();
//        update_min_max_val_ = true;
//
//        drag_machine_->onDrag(delta);
//        return true;
//    }
//
//    updateHoveringSlice(x, y);
//}

//
//void SliceTranslator::onDrag(const glm::vec2& delta) {
//    Slice* slice = comp_.draggedSlice();
//    const auto& o = slice->orientation4();
//
//    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
//    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
//    const auto& normal = slice->normal();
//
//    // project the normal vector to screen coordinates
//    // FIXME maybe need window matrix here too which would be kind of
//    // painful maybe
//    auto base_point_normal = glm::vec3(o[2][0], o[2][1], o[2][2]) + 0.5f * (axis1 + axis2);
//    auto end_point_normal = base_point_normal + normal;
//
//    auto a = comp_.matrix_ * glm::vec4(base_point_normal, 1.0f);
//    auto b = comp_.matrix_ * glm::vec4(end_point_normal, 1.0f);
//    auto normal_delta = b - a;
//    float difference = glm::dot(glm::vec2(normal_delta.x, normal_delta.y), delta);
//
//    // take the inner product of delta x and this normal vector
//
//    auto dx = difference * normal;
//    // FIXME check if it is still inside the bounding box of the volume
//    // probably by checking all four corners are inside bounding box, should
//    // define this box somewhere
//    slice->translate(dx);
//}

// ReconItem::SliceRotator
//
//SliceRotator::SliceRotator(ReconComponent& comp, const glm::vec2& initial)
//        : DragMachine(comp, initial, DragType::rotator) {
//    // 1. need to identify the opposite axis
//    // a) get the position within the slice
//    auto tf = comp.matrix_;
//    auto inv_matrix = glm::inverse(tf);
//
//    Slice* slice = comp.hoveredSlice();
//    assert(slice != nullptr);
//    const auto& o = slice->orientation4();
//
//    auto maybe_point = intersectionPoint(inv_matrix, o, initial_);
//    assert(std::get<0>(maybe_point));
//
//    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
//    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
//    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);
//
//    auto in_world = std::get<2>(maybe_point);
//    auto rel = in_world - base;
//
//    auto x = 0.5f * glm::dot(rel, axis1) - 1.0f;
//    auto y = 0.5f * glm::dot(rel, axis2) - 1.0f;
//
//    // 2. need to rotate around that at on drag
//    auto other = glm::vec3();
//    if (glm::abs(x) > glm::abs(y)) {
//        if (x > 0.0f) {
//            rot_base = base;
//            rot_end = rot_base + axis2;
//            other = axis1;
//        } else {
//            rot_base = base + axis1;
//            rot_end = rot_base + axis2;
//            other = -axis1;
//        }
//    } else {
//        if (y > 0.0f) {
//            rot_base = base;
//            rot_end = rot_base + axis1;
//            other = axis2;
//        } else {
//            rot_base = base + axis2;
//            rot_end = rot_base + axis1;
//            other = -axis2;
//        }
//    }
//
//    auto center = 0.5f * (rot_end + rot_base);
//    auto opposite_center = 0.5f * (rot_end + rot_base) + other;
//    auto from = tf * glm::vec4(glm::rotate(rot_base - center,
//                                           glm::half_pi<float>(), other) +
//                               opposite_center,
//                               1.0f);
//    auto to = tf * glm::vec4(glm::rotate(rot_end - center,
//                                         glm::half_pi<float>(), other) +
//                             opposite_center,
//                             1.0f);
//
//    screen_direction = glm::normalize(from - to);
//}
//
//SliceRotator::~SliceRotator() = default;
//
//void SliceRotator::onDrag(const glm::vec2& delta) {
//    Slice* slice = comp_.draggedSlice();
//    const auto& o = slice->orientation4();
//
//    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
//    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
//    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);
//
//    auto a = base - rot_base;
//    auto b = base + axis1 - rot_base;
//    auto c = base + axis2 - rot_base;
//
//    auto weight = glm::dot(delta, screen_direction);
//    a = glm::rotate(a, weight, rot_end - rot_base) + rot_base;
//    b = glm::rotate(b, weight, rot_end - rot_base) + rot_base;
//    c = glm::rotate(c, weight, rot_end - rot_base) + rot_base;
//
//    slice->setOrientation(a, b - a, c - a);
//}