/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_PROJECTION_H
#define GUI_PROJECTION_H

#include <array>
#include <vector>

#include "common/config.hpp"
#include "textures.hpp"

namespace recastx::gui {

class Projection {

  public:

    using DataType = std::vector<RawDtype>;
    using SizeType = std::array<size_t, 2>;

  private:

    int id_ = -1;
    SizeType size_;
    DataType data_;

    ImageTexture<RawDtype> texture_;

  public:

    explicit Projection(int proj_id);
    ~Projection();

    [[nodiscard]] int id() const;

    void setData(DataType&& data, const SizeType& size);

    void bind() const;
    void unbind() const;

    [[nodiscard]] GLuint texture() const { return texture_.texture(); }
};

} // namespace recastx::gui

#endif // GUI_PROJECTION_H
