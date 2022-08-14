#ifndef GUI_GRAPHICS_VOLUME_H
#define GUI_GRAPHICS_VOLUME_H

#include <array>
#include <limits>

#include "textures.hpp"

namespace tomcat::gui {

class Volume {

  public:

    using DataType = std::vector<float>;
    using SizeType = std::array<int32_t, 3>;

  private:

    SizeType size_;
    DataType data_;

    Texture3d<float> texture_;

    std::array<float, 2> minMaxVals_ {std::numeric_limits<float>::min(), std::numeric_limits<float>::max()};

    void updateMinMaxVal();

public:

    Volume();
    ~Volume();

    void setData(DataType&& data, const SizeType& size);

    void bind();
    void unbind();

    [[nodiscard]] const std::array<float, 2>& minMaxVals() const;
};


} // tomcat::gui

#endif // GUI_GRAPHICS_VOLUME_H
