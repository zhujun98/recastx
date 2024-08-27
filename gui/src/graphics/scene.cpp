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
    if (ImGui::CollapsingHeader("LIGHTING", ImGuiTreeNodeFlags_DefaultOpen)) {
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