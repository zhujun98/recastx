/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_RENDERER_H
#define GUI_RENDERER_H

#include <memory>
#include <unordered_map>
#include <vector>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <glm/glm.hpp>

#include "light.hpp"
#include "viewport.hpp"

namespace recastx::gui {

class Camera;
class MeshObject;
class VoxelObject;
class SliceObject;
class ImageObject;
class SimpleObject;
class GlyphObject;
class GlyphRenderer;

class Renderer {

    int width_;
    int height_;

    Camera* camera_;
    Light* light_;

    glm::mat4 view_matrix_;
    glm::mat4 proj_matrix_;
    glm::mat4 vp_matrix_;
    glm::vec3 view_pos_;
    glm::vec3 view_dir_;
    float aspect_ratio_;

    std::unique_ptr<GlyphRenderer> glyph_renderer_;

    std::unique_ptr<Viewport> prev_vp_;

  public:

    Renderer();

    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init(GLFWwindow* window);

    void begin();
    void end();

    void render(const std::vector<std::shared_ptr<MeshObject>>& objects);

    void render(const std::vector<std::shared_ptr<VoxelObject>>& objects);

    void render(const std::vector<std::shared_ptr<SliceObject>>& objects);

    void render(const std::vector<std::shared_ptr<ImageObject>>& objects);

    void render(const std::vector<std::shared_ptr<SimpleObject>>& objects);

    void render(const std::vector<std::shared_ptr<GlyphObject>>& objects);

    void useViewport(const Viewport& vp);
    void useViewport();

    void onFramebufferSizeChanged(int width, int height);

    void update(Camera* camera, Light* light);

    [[nodiscard]] const glm::mat4& vpMatrix() const { return vp_matrix_; }
    [[nodiscard]] const glm::mat4& view() const { return view_matrix_; }
    [[nodiscard]] const glm::mat4& projection() const { return proj_matrix_; }
    [[nodiscard]] const glm::vec3& viewPos() const { return view_pos_; }
    [[nodiscard]] const glm::vec3& viewDir() const { return view_dir_; }
    [[nodiscard]] float aspectRatio() const { return aspect_ratio_; }

    [[nodiscard]] Light* light() { return light_; }

    [[nodiscard]] GlyphRenderer* glyph() const { return glyph_renderer_.get(); }
};

}  // namespace recastx::gui

#endif // GUI_RENDERER_H
