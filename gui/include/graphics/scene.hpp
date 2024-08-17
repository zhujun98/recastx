/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_SCENE_H
#define GUI_SCENE_H

#include <any>
#include <array>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "event.hpp"
#include "viewport.hpp"

namespace recastx::gui {

class Camera;
class Renderer;
class Renderable;
class Interactable;
class MeshObject;
class VoxelObject;
class SliceObject;
class ImageObject;
class SimpleObject;
class GlyphObject;
class LightManager;
class Light;

class Scene {

    Viewport viewport_;
    glm::mat4 prev_inv_vp_ {1.f};

    std::shared_ptr<Camera> camera_;
    bool camera_fixed_ = false;

    std::unique_ptr<LightManager> light_manager_;

    std::vector<std::shared_ptr<MeshObject>> mesh_objects_;
    std::vector<std::shared_ptr<VoxelObject>> voxel_objects_;
    std::vector<std::shared_ptr<SliceObject>> slice_objects_;
    std::vector<std::shared_ptr<ImageObject>> image_objects_;
    std::vector<std::shared_ptr<SimpleObject>> simple_objects_;
    std::vector<std::shared_ptr<GlyphObject>> glyph_objects_;

    Interactable* hovered_object_ { nullptr };
    Interactable* dragged_object_ { nullptr };

    [[nodiscard]] Interactable* findClosestSlice(float x, float y) const;

    [[nodiscard]] std::optional<std::array<double, 2>> mapToSceneCoordinate(double x, double y) const;

    template<typename T>
    [[nodiscard]] bool mapToSceneEvent(T& ev) const {
        auto pos = mapToSceneCoordinate(ev.pos.x, ev.pos.y);
        if (!pos) return false;
        auto [x_s, y_s] = pos.value();
        ev.pos = { x_s, y_s };
        return true;
    }

  public:

    Scene();

    ~Scene();

    void setViewport(float x, float y, float width, float height, Viewport::Type type) {
        viewport_ = {x, y, width, height, type};
    }

    void setCamera(std::shared_ptr<Camera> camera);

    template<typename T, typename ...Args,
            typename = std::enable_if_t<std::is_base_of_v<Camera, T>>>
    T* setCamera(Args&&... args);

    [[nodiscard]] std::shared_ptr<Camera> camera() { return camera_; }

    void setCameraFixed(bool state) { camera_fixed_ = state; }

    template<typename T, typename... Args,
            typename = std::enable_if_t<std::is_base_of_v<Renderable, T>>>
    T* addObject(Args&&... args);

    Light* addLight();

    void draw(Renderer* renderer);

    bool consumeEvent(InputEvent& event);

    void drawLightControlGUI();
    void drawCameraControlGUI();
};

}  // namespace recastx::gui

#endif // GUI_SCENES_SCENE_H