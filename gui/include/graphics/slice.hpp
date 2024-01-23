/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_SLICE_H
#define GUI_SLICE_H

#include <array>
#include <cstddef>
#include <memory>
#include <optional>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include "textures.hpp"
#include "common/config.hpp"

namespace recastx::gui {

class Light;
class ShaderProgram;

class Slice {

  public:

    using DataType = std::vector<float>;
    using Orient4Type = glm::mat4;

    enum class Plane { XY, YZ, XZ };

  private:

    int id_ = -1;
    uint32_t x_;
    uint32_t y_;
    DataType data_;

    std::optional<std::array<float, 2>> min_max_vals_;

    bool update_texture_ = false;
    SliceTexture texture_;

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    bool hovered_ = false;
    bool highlighted_ = false;
    static constexpr glm::vec4 frame_color_ {0.6, 0.6, 0.0, 1.};

    Plane plane_;
    Orient4Type orient_;
    glm::vec3 normal_;

    void updateMinMaxVal();

  public:

    Slice(int slice_id, Plane plane);
    ~Slice();

    [[nodiscard]] int id() const;

    bool setData(const std::string& data, uint32_t x, uint32_t y);

    void preRender();

    void render(const glm::mat4& view,
                const glm::mat4& projection,
                float min_v,
                float max_v,
                bool fallback_to_preview,
                const glm::vec3& view_dir,
                const glm::vec3& view_pos,
                const Light& light);

    [[nodiscard]] bool hasTexture() const { return texture_.isReady(); }

    void clear();

    [[nodiscard]] bool hovered() const { return hovered_; }

    [[nodiscard]] bool transparent() const  { return hovered_ || highlighted_ || data_.empty(); }

    void setHovered(bool state) { hovered_ = state; }

    void setHighlighted(bool state) { highlighted_ = state; }

    void setOrientation(const glm::vec3& base, const glm::vec3& x, const glm::vec3& y);
    void setOrientation(const Slice::Orient4Type& orient);

    void reset();

    void setPlane(Plane plane);

    void translate(const glm::vec3& v);

    Plane plane() const { return plane_; }

    [[nodiscard]] Orientation orientation3() const;

    [[nodiscard]] const Orient4Type& orientation4() const { return orient_; }

    [[nodiscard]] const glm::vec3& normal() const { return normal_; }

    [[nodiscard]] const DataType& data() const { return data_; }

    [[nodiscard]] const std::optional<std::array<float, 2>>& minMaxVals() const { return min_max_vals_; }
};

} // namespace recastx::gui

#endif // GUI_SLICE_H