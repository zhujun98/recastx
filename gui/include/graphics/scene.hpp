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
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "viewport.hpp"

namespace recastx::gui {

class Camera;
class Renderer;
class Renderable;
class MeshObject;
class VoxelObject;
class SliceObject;
class ImageObject;
class SimpleObject;
class GlyphObject;
class LightManager;
class Light;

class Scene {

    ViewportID viewport_id_;

  protected:

    std::shared_ptr<Camera> camera_;
    bool camera_fixed_ = false;

    std::unique_ptr<LightManager> light_manager_;

    std::vector<std::shared_ptr<MeshObject>> mesh_objects_;
    std::vector<std::shared_ptr<VoxelObject>> voxel_objects_;
    std::vector<std::shared_ptr<SliceObject>> slice_objects_;
    std::vector<std::shared_ptr<ImageObject>> image_objects_;
    std::vector<std::shared_ptr<SimpleObject>> simple_objects_;
    std::vector<std::shared_ptr<GlyphObject>> glyph_objects_;

  public:

    Scene();

    ~Scene();

    void setViewport(ViewportID id) { viewport_id_ = id; }

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

    bool handleMouseButton(int button, int action);
    bool handleScroll(float offset);
    bool handleMouseMoved(float x, float y);
    bool handleKey(int key, int action, int mods);

    void drawLightControlGUI();
    void drawCameraControlGUI();
};

}  // namespace recastx::gui

#endif // GUI_SCENES_SCENE_H