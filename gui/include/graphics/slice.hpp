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

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include "textures.hpp"
#include "common/config.hpp"

namespace recastx::gui {

class ShaderProgram;

class Slice {

  public:

    using DataType = std::vector<float>;
    using SizeType = std::array<size_t, 2>;
    using Orient4Type = glm::mat4;

  private:

    int id_ = -1;
    SizeType size_;
    DataType data_;

    SliceTexture<float> texture_;

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    bool hovered_ = false;
    bool visible_ = true;

    std::array<float, 2> min_max_vals_ {0.f, 0.f};

    Orient4Type orient_;

    void updateMinMaxVal();

  public:

    explicit Slice(int slice_id);
    ~Slice();

    [[nodiscard]] int id() const;

    void setData(DataType&& data, const SizeType& size);

    void render(const glm::mat4& view,
                const glm::mat4& projection,
                float min_v,
                float max_v);

    [[nodiscard]] bool empty() const;

    [[nodiscard]] bool hovered() const;

    [[nodiscard]] bool visible() const { return visible_; }

    void setVisible(bool visible) { visible_ = visible; }

    [[nodiscard]] bool transparent() const;

    void setHovered(bool state);

    void setEmpty();

    void setOrientation(const glm::vec3& base, const glm::vec3& x, const glm::vec3& y);
    void setOrientation(const Slice::Orient4Type& orient);

    [[nodiscard]] Orientation orientation3() const;
    Orient4Type& orientation4();

    [[nodiscard]] const DataType& data() const;

    [[nodiscard]] const std::array<float, 2>& minMaxVals() const;
};

} // namespace recastx::gui

#endif // GUI_SLICE_H