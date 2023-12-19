/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
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

class ShaderProgram;

class Slice {

  public:

    using DataType = std::vector<float>;
    using ShapeType = std::array<size_t, 2>;
    using Orient4Type = glm::mat4;

    enum class Plane { XY, YZ, XZ };

  private:

    int id_ = -1;
    ShapeType shape_;
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

    void updateMinMaxVal();

  public:

    Slice(int slice_id, Plane plane);
    ~Slice();

    [[nodiscard]] int id() const;

    void setData(DataType&& data, const ShapeType& size);

    void preRender();

    void render(const glm::mat4& view,
                const glm::mat4& projection,
                float min_v,
                float max_v,
                bool fallback_to_preview);

    [[nodiscard]] bool empty() const { return data_.empty(); }

    [[nodiscard]] bool hovered() const { return hovered_; }

    [[nodiscard]] bool transparent() const  { return hovered_ || highlighted_ || data_.empty(); }

    void setEmpty() {
        data_.clear();
        shape_.fill(0);
        update_texture_ = true;
    }

    void setHovered(bool state) { hovered_ = state; }

    void setHighlighted(bool state) { highlighted_ = state; }

    void setOrientation(const glm::vec3& base, const glm::vec3& x, const glm::vec3& y);
    void setOrientation(const Slice::Orient4Type& orient);

    void reset();

    void setPlane(Plane plane) {
        plane_ = plane;
        reset();
    }

    Plane plane() const { return plane_; }

    [[nodiscard]] Orientation orientation3() const;

    Orient4Type& orientation4() { return orient_; }

    [[nodiscard]] const DataType& data() const { return data_; }

    [[nodiscard]] const std::optional<std::array<float, 2>>& minMaxVals() const { return min_max_vals_; }
};

} // namespace recastx::gui

#endif // GUI_SLICE_H