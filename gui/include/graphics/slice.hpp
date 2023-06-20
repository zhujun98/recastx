#ifndef GUI_SLICE_H
#define GUI_SLICE_H

#include <array>
#include <cstddef>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include "textures.hpp"
#include "common/config.hpp"

namespace recastx::gui {

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

    bool hovered_ = false;
    bool inactive_ = false;

    std::array<float, 2> min_max_vals_ {0.f, 0.f};

    Orient4Type orient_;

    void updateMinMaxVal();

  public:

    explicit Slice(int slice_id);
    ~Slice();

    [[nodiscard]] int id() const;

    void setData(DataType&& data, const SizeType& size);

    void bind() const;

    void unbind() const;

    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool hovered() const;
    [[nodiscard]] bool inactive() const;
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