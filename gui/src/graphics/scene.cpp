/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/scene.hpp"
#include "graphics/renderer.hpp"
#include "graphics/camera.hpp"
#include "graphics/style.hpp"
#include "graphics/light_manager.hpp"
#include "graphics/mesh_object.hpp"
#include "graphics/voxel_object.hpp"
#include "graphics/slice_object.hpp"
#include "graphics/image_object.hpp"
#include "graphics/simple_object.hpp"
#include "graphics/glyph_object.hpp"
#include "graphics/light_object.hpp"

namespace recastx::gui {

Scene::Scene()
    : camera_(new Camera()),
      light_manager_(new LightManager) {
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
    auto light = light_manager_->addLight();
    addObject<LightObject>(light);
    return light.get();
}

void Scene::draw(Renderer* renderer) {
    renderer->update(camera_.get(), light_manager_.get());
    renderer->useViewport(viewport_id_);

    renderer->render(voxel_objects_);
    renderer->render(mesh_objects_);
    renderer->render(slice_objects_);
    renderer->render(image_objects_);
    renderer->render(simple_objects_);
    renderer->render(glyph_objects_);
}

bool Scene::handleMouseButton(int button, int action) {
    if (!camera_fixed_ && camera_->handleMouseButton(button, action)) return true;
    return false;
}

bool Scene::handleScroll(float offset) {
    if (!camera_fixed_ && camera_->handleScroll(offset)) return true;
    return false;
}

bool Scene::handleMouseMoved(float x, float y) {
//    y = -y;
//
//    if (prev_y_ < -1.0f) {
//        prev_x_ = x;
//        prev_y_ = y;
//    }
//
//    delta_ = {x - prev_x_, y - prev_y_};
//    prev_x_ = x;
//    prev_y_ = y;

    if (!camera_fixed_ && camera_->handleMouseMoved(x, y)) return true;
    return false;
}

bool Scene::handleKey(int key, int action, int mods) {
    if (!camera_fixed_ && camera_->handleKey(key, action, mods)) return true;
    return false;
}

void Scene::drawLightControlGUI() {
    light_manager_->renderGUI();
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

}  // namespace recastx::gui
