#ifndef GUI_GRAPHICS_SLICE_H
#define GUI_GRAPHICS_SLICE_H

#include <array>
#include <cstddef>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include "textures.hpp"

namespace tomcat::gui {

class Slice {

  public:

    using DataType = std::vector<float>;
    using SizeType = std::array<int32_t, 2>;
    using Orient3Type = std::array<float, 9>; // for SlicePacket
    using Orient4Type = glm::mat4;

  private:

    int id_ = -1;
    DataType data_;
    SizeType size_;
  
    Texture<float> texture_;

    bool hovered_ = false;
    bool inactive_ = false;

    std::array<float, 2> minMaxVals_ {-1.f, 1.f};

    Orient4Type orient_;

    void updateMinMaxVal();

  public:

    explicit Slice(int slice_id);
    ~Slice();

    [[nodiscard]] int id() const;

    void setData(DataType&& data, const SizeType& size);

    void updateTexture();

    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool hovered() const;
    [[nodiscard]] bool inactive() const;
    [[nodiscard]] bool transparent() const;

    void setHovered(bool state);

    auto& texture() { return texture_; }

    void setOrientation(glm::vec3 base, glm::vec3 x, glm::vec3 y);
    void setOrientation(const Slice::Orient4Type& orient);

    [[nodiscard]] Orient3Type orientation3() const;
    Orient4Type& orientation4();

    [[nodiscard]] const std::array<float, 2>& minMaxVals() const;
};

} // tomcat::gui

#endif // GUI_GRAPHICS_SLICE_H