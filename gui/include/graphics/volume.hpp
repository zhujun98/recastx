#ifndef GUI_VOLUME_H
#define GUI_VOLUME_H

#include <array>

#include "textures.hpp"

namespace recastx::gui {

class Volume {

  public:

    using DataType = std::vector<float>;
    using SizeType = std::array<size_t, 3>;

  private:

    SizeType size_;
    DataType data_;

    VolumeTexture<float> texture_;

    std::array<float, 2> min_max_vals_ {0.f, 0.f};

    void updateMinMaxVal();

public:

    Volume();
    ~Volume();

    void setData(DataType&& data, const SizeType& size);

    void bind() const;

    void unbind() const;

    [[nodiscard]] const std::array<float, 2>& minMaxVals() const;
};


} // namespace recastx::gui

#endif // GUI_VOLUME_H
